const { TimePointSec, Name, PermissionLevel, Authority, Checksum160 } = require('@greymass/eosio');
const { Blockchain, log, expectToThrow, AccountPermission } = require('@proton/vert');
const { getTokenBalance } = require('./src/help');

// Vert EOS VM
const blockchain = new Blockchain();
//log.setLevel('debug');
// contracts
const contracts = {
  brdgmng: blockchain.createContract('brdgmng.xsat', 'tests/wasm/brdgmng.xsat', true),
  eos: blockchain.createContract('eosio.token', 'tests/wasm/exsat.xsat', true),
  btc: blockchain.createContract('btc.xsat', 'tests/wasm/btc.xsat', true),
};

blockchain.createAccounts('alice', 'bob', 'boot.xsat', 'actor1.xsat', 'actor2.xsat', 'evm.xsat');

const permissionScope = BigInt(0);

// one-time setup
beforeAll(async () => {
  blockchain.setTime(TimePointSec.from(new Date()));

  // create BTC token
  await contracts.btc.actions.create(['btc.xsat', '21000000.00000000 BTC']).send('btc.xsat@active');

  // create EOS token
  await contracts.eos.actions.create(['eosio.token', '10000000000.0000 EOS']).send('eosio.token@active');
  await contracts.eos.actions.issue(['eosio.token', '10000000000.0000 EOS', 'init']).send('eosio.token@active');

  // transfer EOS to account
  await contracts.eos.actions.transfer(['eosio.token', 'alice', '100000.0000 EOS', 'init']).send('eosio.token@active');
  await contracts.eos.actions.transfer(['eosio.token', 'bob', '100000.0000 EOS', 'init']).send('eosio.token@active');

  await contracts.brdgmng.actions
    .updateconfig([true, true, false, 10000, 1000, 2000, 1000, 2, 30, 10])
    .send('brdgmng.xsat');

  contracts.btc.setPermissions([
    AccountPermission.from({
      parent: 'owner',
      perm_name: 'active',
      required_auth: Authority.from({
        threshold: 1,
        accounts: [
          {
            weight: 1,
            permission: PermissionLevel.from('brdgmng.xsat@eosio.code'),
          },
          {
            weight: 1,
            permission: PermissionLevel.from('btc.xsat@eosio.code'),
          },
        ],
      }),
    }),
  ]);
});

describe('brdgmng.xsat', () => {
  it('missing required authority', async () => {
    await expectToThrow(
      contracts.brdgmng.actions.initialize([]).send('alice@active'),
      'missing required authority boot.xsat'
    );
  });

  it('initialize issue 1 BTC', async () => {
    await contracts.brdgmng.actions.initialize([]).send('boot.xsat@active');
    const btcBalance = getTokenBalance(blockchain, 'boot.xsat', 'btc.xsat', 'BTC');
    expect(btcBalance).toEqual(100000000);
  });

  it('add permission', async () => {
    await contracts.brdgmng.actions.addperm([0, ['actor1.xsat', 'actor2.xsat']]).send('brdgmng.xsat@active');
    const result = contracts.brdgmng.tables.permissions().getTableRows()[0];
    expect(result).toEqual({
      id: 0,
      actors: ['actor1.xsat', 'actor2.xsat'],
    });
  });

  it('add addresses', async () => {
    await contracts.brdgmng.actions
      .addaddresses({
        actor: 'actor1.xsat',
        permission_id: 0,
        b_id: 'bid',
        wallet_code: 'wallet_code',
        btc_addresses: ['3LB8ocwXtqgq7sDfiwv3EbDZNEPwKLQcsN', 'tb1qgg49qv5r6keqzal7h8m5ts85pypjgwdrexpc0a'],
      })
      .send('actor1.xsat@active');
    await contracts.brdgmng.actions
      .valaddress({
        actor: 'actor2.xsat',
        permission_id: 0,
        address_id: 1,
        status: 1,
      })
      .send('actor2.xsat@active');
    await contracts.brdgmng.actions
      .valaddress({
        actor: 'actor2.xsat',
        permission_id: 0,
        address_id: 2,
        status: 2,
      })
      .send('actor2.xsat@active');
    const result = contracts.brdgmng.tables.addresses(permissionScope).getTableRows();
    expect(result.length).toEqual(2);
    expect(result[0]).toEqual({
      id: 1,
      permission_id: 0,
      provider_actors: ['actor1.xsat', 'actor2.xsat'],
      statuses: '0001',
      confirmed_count: 2,
      global_status: 1,
      b_id: 'bid',
      wallet_code: 'wallet_code',
      btc_address: '3LB8ocwXtqgq7sDfiwv3EbDZNEPwKLQcsN',
    });
    expect(result[1]).toEqual({
      id: 2,
      permission_id: 0,
      provider_actors: ['actor1.xsat', 'actor2.xsat'],
      statuses: '0002',
      confirmed_count: 1,
      global_status: 2,
      b_id: 'bid',
      wallet_code: 'wallet_code',
      btc_address: 'tb1qgg49qv5r6keqzal7h8m5ts85pypjgwdrexpc0a',
    });
  });

  it('mapping address', async () => {
    await contracts.brdgmng.actions
      .mappingaddr(['actor1.xsat', 0, '2614e5588275b02B23CdbeFed8E5dA6D2f59d1c6'])
      .send('actor1.xsat@active');
    const result = contracts.brdgmng.tables.addrmappings(permissionScope).getTableRows();
    expect(result[0].btc_address).toEqual('3LB8ocwXtqgq7sDfiwv3EbDZNEPwKLQcsN');
  });

  it('deposit failed', async () => {
    const depositInfo = {
      actor: 'actor1.xsat',
      permission_id: 0,
      b_id: 'bid',
      wallet_code: 'wallet_code',
      btc_address: '3LB8ocwXtqgq7sDfiwv3EbDZNEPwKLQcsN',
      order_id: 'order_id',
      block_height: 840000,
      tx_id: '6f69d6bc9509e62b3eccefcc4eae205dd12da589731549417b505f4009e1e4eb',
      index: 0,
      amount: 100000000,
      tx_status: 0,
      remark_detail: null,
      tx_time_stamp: 1781234567890,
      create_time_stamp: 1781234567890,
    };
    await expectToThrow(
      contracts.brdgmng.actions.deposit(depositInfo).send('actor2.xsat@active'),
      'missing required authority actor1.xsat'
    );

    await contracts.brdgmng.actions.deposit(depositInfo).send('actor1.xsat@active');
    let depositingTable = contracts.brdgmng.tables.depositings(permissionScope).getTableRows();
    let depositTable = contracts.brdgmng.tables.deposits(permissionScope).getTableRows();
    expect(depositingTable.length).toEqual(1);
    expect(depositTable.length).toEqual(0);
    expect(depositingTable[0].tx_status).toEqual(0);
    await contracts.brdgmng.actions.upddeposit({ actor: 'actor1.xsat', permission_id: 0, deposit_id: 1, tx_status: 1 }).send('actor1.xsat@active');
    depositingTable = contracts.brdgmng.tables.depositings(permissionScope).getTableRows();
    expect(depositingTable[0].tx_status).toEqual(1);
    await contracts.brdgmng.actions.valdeposit({ actor: 'actor1.xsat', permission_id: 0, deposit_id: 1, tx_status: 1 }).send('actor1.xsat@active');
    depositingTable = contracts.brdgmng.tables.depositings(permissionScope).getTableRows();
    expect(depositingTable[0].tx_status).toEqual(1);
    await contracts.brdgmng.actions.valdeposit({ actor: 'actor2.xsat', permission_id: 0, deposit_id: 1, tx_status: 2 }).send('actor2.xsat@active');
    depositingTable = contracts.brdgmng.tables.depositings(permissionScope).getTableRows();
    depositTable = contracts.brdgmng.tables.deposits(permissionScope).getTableRows();
    expect(depositingTable.length).toEqual(0);
    expect(depositTable.length).toEqual(1);
    const btcBalance = getTokenBalance(blockchain, 'evm.xsat', 'btc.xsat', 'BTC');
    const configs = contracts.brdgmng.tables.configs().getTableRows();
    expect(btcBalance).toEqual(0);
  });

  it('deposit success', async () => {
    const depositInfo = {
      actor: 'actor1.xsat',
      permission_id: 0,
      b_id: 'bid',
      wallet_code: 'wallet_code',
      btc_address: '3LB8ocwXtqgq7sDfiwv3EbDZNEPwKLQcsN',
      order_id: 'order_id_success',
      block_height: 840000,
      tx_id: '6f69d6bc9509e62b3eccefcc4eae205dd12da589731549417b505f4009e1e4eb',
      index: 0,
      amount: 100000000,
      tx_status: 0,
      remark_detail: null,
      tx_time_stamp: 1781234567890,
      create_time_stamp: 1781234567890,
    };
    await expectToThrow(
      contracts.brdgmng.actions.deposit(depositInfo).send('actor2.xsat@active'),
      'missing required authority actor1.xsat'
    );

    await contracts.brdgmng.actions.deposit(depositInfo).send('actor1.xsat@active');
    let depositingTable = contracts.brdgmng.tables.depositings(permissionScope).getTableRows();
    let depositTable = contracts.brdgmng.tables.deposits(permissionScope).getTableRows();
    expect(depositingTable.length).toEqual(1);
    expect(depositTable.length).toEqual(1);
    expect(depositingTable[0].tx_status).toEqual(0);

    await expectToThrow(
      contracts.brdgmng.actions.valdeposit({ actor: 'actor1.xsat', permission_id: 0, deposit_id: 2, tx_status: 1 }).send('actor1.xsat@active'),
      'eosio_assert: brdgmng.xsat::valdeposit: only final tx status can be confirmed'
    );

    await contracts.brdgmng.actions.upddeposit({ actor: 'actor1.xsat', permission_id: 0, deposit_id: 2, tx_status: 1 }).send('actor1.xsat@active');
    depositingTable = contracts.brdgmng.tables.depositings(permissionScope).getTableRows();
    expect(depositingTable[0].tx_status).toEqual(1);
    await contracts.brdgmng.actions.valdeposit({ actor: 'actor1.xsat', permission_id: 0, deposit_id: 2, tx_status: 1 }).send('actor1.xsat@active');
    depositingTable = contracts.brdgmng.tables.depositings(permissionScope).getTableRows();
    expect(depositingTable[0].tx_status).toEqual(1);
    await contracts.brdgmng.actions.valdeposit({ actor: 'actor2.xsat', permission_id: 0, deposit_id: 2, tx_status: 1 }).send('actor2.xsat@active');
    depositingTable = contracts.brdgmng.tables.depositings(permissionScope).getTableRows();
    depositTable = contracts.brdgmng.tables.deposits(permissionScope).getTableRows();
    expect(depositingTable.length).toEqual(0);
    expect(depositTable.length).toEqual(2);
    const btcBalance = getTokenBalance(blockchain, 'evm.xsat', 'btc.xsat', 'BTC');
    const configs = contracts.brdgmng.tables.configs().getTableRows();
    expect(btcBalance).toEqual(100000000 - configs[0].deposit_fee);
  });

  it('withdraw amount too small', async () => {
    await expectToThrow(
      contracts.btc.actions.transfer(['evm.xsat', 'brdgmng.xsat', '0.00001000 BTC', '0,0x2614e5588275b02B23CdbeFed8E5dA6D2f59d1c6,tb1qgg49qv5r6keqzal7h8m5ts85pypjgwdrexpc0a,fast']).send('evm.xsat@active'),
      'eosio_assert_message: brdgmng.xsat: withdraw amount must be more than 0.00001000 BTC'
    );
  });

  it('withdraw failed', async () => {
    await contracts.btc.actions.transfer(['evm.xsat', 'brdgmng.xsat', '0.10000000 BTC', '0,0x2614e5588275b02B23CdbeFed8E5dA6D2f59d1c6,tb1qgg49qv5r6keqzal7h8m5ts85pypjgwdrexpc0a,fast']).send('evm.xsat@active');
    let withdrawingTable = contracts.brdgmng.tables.withdrawings(permissionScope).getTableRows();
    let withdrawTable = contracts.brdgmng.tables.withdraws(permissionScope).getTableRows();
    expect(withdrawingTable.length).toEqual(1);
    expect(withdrawTable.length).toEqual(0);
    expect(withdrawingTable[0].order_no).toBeTruthy();
    await contracts.brdgmng.actions.updwithdraw({ actor: 'actor1.xsat', permission_id: 0, withdraw_id: 1, withdraw_status: 2, order_status: 0 }).send('actor1.xsat@active');
    withdrawingTable = contracts.brdgmng.tables.withdrawings(permissionScope).getTableRows();
    expect(withdrawingTable[0].withdraw_status).toEqual(2);

    const withdrawInfo = {
      actor: 'actor1.xsat',
      permission_id: 0,
      withdraw_id: 1,
      b_id: 'bid',
      wallet_code: 'wallet_code',
      order_id: '',
      withdraw_status: 3,
      order_status: 0,
      block_height: 840000,
      tx_id: '6f69d6bc9509e62b3eccefcc4eae205dd12da589731549417b505f4009e1e4eb',
      remark_detail: null,
      tx_time_stamp: 1781234567890,
      create_time_stamp: 1781234567890,
    };
    await expectToThrow(
      contracts.brdgmng.actions.withdrawinfo(withdrawInfo).send('actor2.xsat@active'),
      'missing required authority actor1.xsat'
    );

    await contracts.brdgmng.actions.withdrawinfo(withdrawInfo).send('actor1.xsat@active');
    withdrawingTable = contracts.brdgmng.tables.withdrawings(permissionScope).getTableRows();
    expect(withdrawingTable[0].withdraw_status).toEqual(3);
    expect(withdrawingTable[0].order_status).toEqual(0);
    await contracts.brdgmng.actions.updwithdraw({ actor: 'actor1.xsat', permission_id: 0, withdraw_id: 1, withdraw_status: 3, order_status: 5 }).send('actor1.xsat@active');
    withdrawingTable = contracts.brdgmng.tables.withdrawings(permissionScope).getTableRows();
    expect(withdrawingTable[0].order_status).toEqual(5);

    await contracts.brdgmng.actions.valwithdraw({ actor: 'actor1.xsat', permission_id: 0, withdraw_id: 1, order_status: 5 }).send('actor1.xsat@active');
    withdrawingTable = contracts.brdgmng.tables.withdrawings(permissionScope).getTableRows();
    expect(withdrawingTable[0].order_status).toEqual(5);
    await contracts.brdgmng.actions.valwithdraw({ actor: 'actor2.xsat', permission_id: 0, withdraw_id: 1, order_status: 3 }).send('actor2.xsat@active');
    withdrawingTable = contracts.brdgmng.tables.withdrawings(permissionScope).getTableRows();
    withdrawTable = contracts.brdgmng.tables.withdraws(permissionScope).getTableRows();
    expect(withdrawingTable.length).toEqual(0);
    expect(withdrawTable.length).toEqual(1);
    const btcBalance = getTokenBalance(blockchain, 'brdgmng.xsat', 'btc.xsat', 'BTC');
    console.log('btcBalance', btcBalance);
    expect(btcBalance).toEqual(10000000);
  });

  it('withdraw success', async () => {
    await contracts.btc.actions.transfer(['evm.xsat', 'brdgmng.xsat', '0.10000000 BTC', '0,0x2614e5588275b02B23CdbeFed8E5dA6D2f59d1c6,tb1qgg49qv5r6keqzal7h8m5ts85pypjgwdrexpc0a,fast']).send('evm.xsat@active');
    let withdrawingTable = contracts.brdgmng.tables.withdrawings(permissionScope).getTableRows();
    let withdrawTable = contracts.brdgmng.tables.withdraws(permissionScope).getTableRows();
    expect(withdrawingTable.length).toEqual(1);
    expect(withdrawTable.length).toEqual(1);
    await contracts.brdgmng.actions.updwithdraw({ actor: 'actor1.xsat', permission_id: 0, withdraw_id: 2, withdraw_status: 2, order_status: 0 }).send('actor1.xsat@active');
    withdrawingTable = contracts.brdgmng.tables.withdrawings(permissionScope).getTableRows();
    expect(withdrawingTable[0].withdraw_status).toEqual(2);

    const withdrawInfo = {
      actor: 'actor1.xsat',
      permission_id: 0,
      withdraw_id: 2,
      b_id: 'bid',
      wallet_code: 'wallet_code',
      order_id: '',
      withdraw_status: 3,
      order_status: 0,
      block_height: 840000,
      tx_id: '6f69d6bc9509e62b3eccefcc4eae205dd12da589731549417b505f4009e1e4eb',
      remark_detail: null,
      tx_time_stamp: 1781234567890,
      create_time_stamp: 1781234567890,
    };
    await expectToThrow(
      contracts.brdgmng.actions.withdrawinfo(withdrawInfo).send('actor2.xsat@active'),
      'missing required authority actor1.xsat'
    );

    await contracts.brdgmng.actions.withdrawinfo(withdrawInfo).send('actor1.xsat@active');
    withdrawingTable = contracts.brdgmng.tables.withdrawings(permissionScope).getTableRows();
    expect(withdrawingTable[0].withdraw_status).toEqual(3);
    expect(withdrawingTable[0].order_status).toEqual(0);
    await contracts.brdgmng.actions.updwithdraw({ actor: 'actor1.xsat', permission_id: 0, withdraw_id: 2, withdraw_status: 3, order_status: 5 }).send('actor1.xsat@active');
    withdrawingTable = contracts.brdgmng.tables.withdrawings(permissionScope).getTableRows();
    expect(withdrawingTable[0].order_status).toEqual(5);

    await contracts.brdgmng.actions.valwithdraw({ actor: 'actor1.xsat', permission_id: 0, withdraw_id: 2, order_status: 5 }).send('actor1.xsat@active');
    withdrawingTable = contracts.brdgmng.tables.withdrawings(permissionScope).getTableRows();
    expect(withdrawingTable[0].order_status).toEqual(5);
    await contracts.brdgmng.actions.valwithdraw({ actor: 'actor2.xsat', permission_id: 0, withdraw_id: 2, order_status: 5 }).send('actor2.xsat@active');
    withdrawingTable = contracts.brdgmng.tables.withdrawings(permissionScope).getTableRows();
    withdrawTable = contracts.brdgmng.tables.withdraws(permissionScope).getTableRows();
    expect(withdrawingTable.length).toEqual(0);
    expect(withdrawTable.length).toEqual(2);
    const btcBalance = getTokenBalance(blockchain, 'brdgmng.xsat', 'btc.xsat', 'BTC');
    console.log('btcBalance', btcBalance);
    expect(btcBalance).toEqual(10000000);
  });

  it('withdraw with slow gas level', async () => {
    await contracts.btc.actions.transfer(['evm.xsat', 'brdgmng.xsat', '0.10000000 BTC', '0,0x2614e5588275b02B23CdbeFed8E5dA6D2f59d1c6,tb1qgg49qv5r6keqzal7h8m5ts85pypjgwdrexpc0a,slow']).send('evm.xsat@active');
    await contracts.btc.actions.transfer(['evm.xsat', 'brdgmng.xsat', '0.20000000 BTC', '0,0x2614e5588275b02B23CdbeFed8E5dA6D2f59d1c6,tb1qgg49qv5r6keqzal7h8m5ts85pypjgwdrexpc0a,slow']).send('evm.xsat@active');
    await contracts.btc.actions.transfer(['evm.xsat', 'brdgmng.xsat', '0.30000000 BTC', '0,0x2614e5588275b02B23CdbeFed8E5dA6D2f59d1c6,tb1qgg49qv5r6keqzal7h8m5ts85pypjgwdrexpc0a,slow']).send('evm.xsat@active');
    let withdrawingTable = contracts.brdgmng.tables.withdrawings(permissionScope).getTableRows();
    expect(withdrawingTable.length).toEqual(3);
    for (const item of withdrawingTable) {
      expect(item.gas_level).toEqual('slow');
      expect(item.order_no).toEqual('');
    }
  });

  it('generate order no', async () => {
    await contracts.brdgmng.actions.genorderno([0]).send('alice@active');
    let withdrawingTable = contracts.brdgmng.tables.withdrawings(permissionScope).getTableRows();
    expect(withdrawingTable.length).toEqual(3);
    expect(withdrawingTable[0].order_no).toBeTruthy();
    expect(withdrawingTable[1].order_no).toBeTruthy();
    expect(withdrawingTable[2].order_no).toEqual('');
  });

  it('del permission', async () => {
    await contracts.brdgmng.actions.delperm([0]).send('brdgmng.xsat@active');
    const result = contracts.brdgmng.tables.permissions().getTableRows();
    expect(result.length).toEqual(0);
  });
});
