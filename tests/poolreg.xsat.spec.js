const { Asset, Name, TimePointSec } = require('@greymass/eosio')
const { Blockchain, log, expectToThrow } = require('@proton/vert')
const { getTokenBalance } = require('./src/help')
const { BTC, XSAT } = require('./src/constants')

// Vert EOS VM
const blockchain = new Blockchain()
//log.setLevel('debug');
// contracts
const contracts = {
    poolreg: blockchain.createContract('poolreg.xsat', 'tests/wasm/poolreg.xsat', true),
    exsat: blockchain.createContract('exsat.xsat', 'tests/wasm/exsat.xsat', true),
    rescmng: blockchain.createContract('rescmng.xsat', 'tests/wasm/rescmng.xsat', true),
    btc: blockchain.createContract('btc.xsat', 'tests/wasm/btc.xsat', true),
    eos: blockchain.createContract('eosio.token', 'tests/wasm/exsat.xsat', true),
}

// accounts
blockchain.createAccounts('blksync.xsat', 'rwddist.xsat', 'alice', 'bob', 'amy', 'fees.xsat', 'donate.xsat')

const get_config = () => {
    return contracts.poolreg.tables.config().getTableRows()[0]
}

const get_synchronizer = synchronizer => {
    let key = Name.from(synchronizer).value.value
    return contracts.poolreg.tables.synchronizer().getTableRow(key)
}

const get_miners = () => {
    return contracts.poolreg.tables.miners().getTableRows()
}

// one-time setup
beforeAll(async () => {
    blockchain.setTime(TimePointSec.from(new Date()))
    // create XSAT token
    await contracts.exsat.actions.create(['exsat.xsat', '10000000.00000000 XSAT']).send('exsat.xsat@active')
    await contracts.exsat.actions.issue(['exsat.xsat', '10000000.00000000 XSAT', 'init']).send('exsat.xsat@active')

    // transfer XSAT to account
    await contracts.exsat.actions.transfer(['exsat.xsat', 'bob', '1000000.00000000 XSAT', '']).send('exsat.xsat@active')
    await contracts.exsat.actions
        .transfer(['exsat.xsat', 'rwddist.xsat', '1000000.00000000 XSAT', ''])
        .send('exsat.xsat@active')

    // create EOS token
    await contracts.eos.actions.create(['eosio.token', '10000000000.0000 EOS']).send('eosio.token@active')
    await contracts.eos.actions.issue(['eosio.token', '10000000000.0000 EOS', 'init']).send('eosio.token@active')

    // transfer EOS to account
    await contracts.eos.actions.transfer(['eosio.token', 'bob', '100000.0000 EOS', 'init']).send('eosio.token@active')
    await contracts.eos.actions
        .transfer(['eosio.token', 'rwddist.xsat', '100000.0000 EOS', 'init'])
        .send('eosio.token@active')

    // create BTC token
    await contracts.btc.actions.create(['btc.xsat', '10000000.00000000 BTC']).send('btc.xsat@active')
    await contracts.btc.actions.issue(['btc.xsat', '10000000.00000000 BTC', 'init']).send('btc.xsat@active')

    // transfer BTC to account
    await contracts.btc.actions.transfer(['btc.xsat', 'bob', '1000000.00000000 BTC', '']).send('btc.xsat@active')
    await contracts.btc.actions.transfer(['btc.xsat', 'alice', '1000000.00000000 BTC', '']).send('btc.xsat@active')
    await contracts.btc.actions.transfer(['btc.xsat', 'amy', '1000000.00000000 BTC', '']).send('btc.xsat@active')

    // rescmng.xsat init
    await contracts.rescmng.actions
        .init([
            'fees.xsat',
            Asset.fromUnits(5, BTC),
            Asset.fromUnits(2, BTC),
            Asset.fromUnits(1, BTC),
            Asset.fromUnits(3, BTC),
            Asset.fromUnits(3, BTC),
        ])
        .send('rescmng.xsat@active')
})

describe('poolreg.xsat', () => {
    it('initpool: missing required authority', async () => {
        await expectToThrow(
            contracts.poolreg.actions
                .initpool(['bob', 839999, 'bob', ['12KKDt4Mj7N5UAkQMN7LtPZMayenXHa8KL']])
                .send('alice@active'),
            'missing required authority poolreg.xsat'
        )
    })

    it('initpool: invalid miner', async () => {
        await expectToThrow(
            contracts.poolreg.actions
                .initpool([
                    'bob',
                    839999,
                    'bob',
                    ['12KKDt4Mj7N5UAkQMN7LtPZMayenXHa8KA', '1CGB4JC7iaThXywv1j6PNFx7jUgRhFuPTmX'],
                ])
                .send('poolreg.xsat@active'),
            'eosio_assert_message: poolreg.xsat::initpool: invalid miner ["12KKDt4Mj7N5UAkQMN7LtPZMayenXHa8KA"]'
        )
    })

    it('initpool', async () => {
        await contracts.poolreg.actions
            .initpool([
                'bob',
                839999,
                'bob',
                ['12KKDt4Mj7N5UAkQMN7LtPZMayenXHa8KL', '1CGB4JC7iaThXyv1j6PNFx7jUgRhFuPTmx'],
            ])
            .send('poolreg.xsat@active')
        expect(get_synchronizer('bob')).toEqual({
            synchronizer: 'bob',
            total_donated: '0.00000000 XSAT',
            donate_rate: 0,
            claimed: '0.00000000 XSAT',
            latest_produced_block_height: 839999,
            memo: '',
            num_slots: 2,
            produced_block_limit: 432,
            reward_recipient: 'bob',
            unclaimed: '0.00000000 XSAT',
            latest_reward_block: 0,
            latest_reward_time: '1970-01-01T00:00:00',
        })
        expect(get_miners()).toEqual([
            {
                id: 0,
                synchronizer: 'bob',
                miner: '12KKDt4Mj7N5UAkQMN7LtPZMayenXHa8KL',
            },
            {
                id: 1,
                synchronizer: 'bob',
                miner: '1CGB4JC7iaThXyv1j6PNFx7jUgRhFuPTmx',
            },
        ])
    })

    it('unbundle: missing required authority', async () => {
        await expectToThrow(
            contracts.poolreg.actions.unbundle([1]).send('alice@active'),
            'missing required authority poolreg.xsat'
        )
    })

    it('unbundle', async () => {
        await contracts.poolreg.actions.unbundle([1]).send('poolreg.xsat@active')

        expect(get_miners()).toEqual([
            {
                id: 0,
                synchronizer: 'bob',
                miner: '12KKDt4Mj7N5UAkQMN7LtPZMayenXHa8KL',
            },
        ])
    })

    it('delpool: missing required authority', async () => {
        await expectToThrow(
            contracts.poolreg.actions.delpool(['bob']).send('alice@active'),
            'missing required authority poolreg.xsat'
        )
    })

    it('delpool', async () => {
        await contracts.poolreg.actions.delpool(['bob']).send('poolreg.xsat@active')
        expect(get_synchronizer('bob')).toEqual(undefined)
        expect(get_miners()).toEqual([])
    })

    it('no balance to claim', async () => {
        await contracts.poolreg.actions
            .initpool([
                'bob',
                839999,
                'bob',
                ['12KKDt4Mj7N5UAkQMN7LtPZMayenXHa8KL', '1CGB4JC7iaThXyv1j6PNFx7jUgRhFuPTmx'],
            ])
            .send('poolreg.xsat@active')
        await expectToThrow(
            contracts.poolreg.actions.claim(['bob']).send('poolreg.xsat@active'),
            'eosio_assert: poolreg.xsat::claim: no balance to claim'
        )
    })

    it('only transfer from [rwddist.xsat]', async () => {
        await expectToThrow(
            contracts.exsat.actions.transfer(['bob', 'poolreg.xsat', '1.00000000 XSAT', '']).send('bob@active'),
            'eosio_assert: poolreg.xsat: only transfer from [rwddist.xsat]'
        )
    })

    it('only transfer [exsat.xsat/XSAT]', async () => {
        await expectToThrow(
            contracts.eos.actions
                .transfer(['rwddist.xsat', 'poolreg.xsat', '1.0000 EOS', ''])
                .send('rwddist.xsat@active'),
            'eosio_assert: poolreg.xsat: only transfer [exsat.xsat/XSAT]'
        )
    })

    it('invalid memo', async () => {
        await expectToThrow(
            contracts.exsat.actions
                .transfer(['rwddist.xsat', 'poolreg.xsat', '1.00000000 XSAT', ''])
                .send('rwddist.xsat@active'),
            'eosio_assert: poolreg.xsat: invalid memo ex: "<synchronizer>,<height>"'
        )
    })

    it('reward received', async () => {
        await contracts.exsat.actions
            .transfer(['rwddist.xsat', 'poolreg.xsat', '100.00000000 XSAT', 'bob,840000'])
            .send('rwddist.xsat@active')

        expect(get_synchronizer('bob')).toEqual({
            synchronizer: 'bob',
            total_donated: '0.00000000 XSAT',
            donate_rate: 0,
            claimed: '0.00000000 XSAT',
            latest_produced_block_height: 839999,
            memo: '',
            num_slots: 2,
            produced_block_limit: 432,
            reward_recipient: 'bob',
            unclaimed: '100.00000000 XSAT',
            latest_reward_block: 840000,
            latest_reward_time: TimePointSec.from(blockchain.timestamp).toString(),
        })
    })

    it('setfinacct: missing required authority', async () => {
        await expectToThrow(
            contracts.poolreg.actions.setfinacct(['bob', 'bob']).send('alice@active'),
            'missing required authority bob'
        )
    })

    it('setfinacct', async () => {
        await contracts.poolreg.actions
            .setfinacct(['bob', '0x36825bf3Fbdf5a29E2d5148bfe7Dcf7B5639e320'])
            .send('bob@active')
        expect(get_synchronizer('bob')).toEqual({
            synchronizer: 'bob',
            total_donated: '0.00000000 XSAT',
            donate_rate: 0,
            claimed: '0.00000000 XSAT',
            latest_produced_block_height: 839999,
            memo: '0x36825bf3Fbdf5a29E2d5148bfe7Dcf7B5639e320',
            num_slots: 2,
            produced_block_limit: 432,
            reward_recipient: 'erc2o.xsat',
            unclaimed: '100.00000000 XSAT',
            latest_reward_block: 840000,
            latest_reward_time: TimePointSec.from(blockchain.timestamp).toString(),
        })

        await contracts.poolreg.actions.setfinacct(['bob', 'alice']).send('bob@active')
        expect(get_synchronizer('bob')).toEqual({
            synchronizer: 'bob',
            total_donated: '0.00000000 XSAT',
            donate_rate: 0,
            claimed: '0.00000000 XSAT',
            latest_produced_block_height: 839999,
            memo: '',
            num_slots: 2,
            produced_block_limit: 432,
            reward_recipient: 'alice',
            unclaimed: '100.00000000 XSAT',
            latest_reward_block: 840000,
            latest_reward_time: TimePointSec.from(blockchain.timestamp).toString(),
        })
    })

    it('claim: missing authority financial account', async () => {
        await expectToThrow(
            contracts.poolreg.actions.claim(['bob']).send('bob@active'),
            'missing required authority alice'
        )
    })

    it('claim', async () => {
        const before_balance = getTokenBalance(blockchain, 'alice', 'exsat.xsat', 'XSAT')
        await contracts.poolreg.actions.claim(['bob']).send('alice@active')
        const after_balance = getTokenBalance(blockchain, 'alice', 'exsat.xsat', 'XSAT')
        expect(after_balance - before_balance).toEqual(10000000000)

        expect(get_synchronizer('bob')).toEqual({
            synchronizer: 'bob',
            total_donated: '0.00000000 XSAT',
            donate_rate: 0,
            claimed: '100.00000000 XSAT',
            latest_produced_block_height: 839999,
            memo: '',
            num_slots: 2,
            produced_block_limit: 432,
            reward_recipient: 'alice',
            unclaimed: '0.00000000 XSAT',
            latest_reward_block: 840000,
            latest_reward_time: TimePointSec.from(blockchain.timestamp).toString(),
        })
    })

    it('buyslot: missing required authority', async () => {
        await expectToThrow(
            contracts.poolreg.actions.buyslot(['bob', 'bob', 1]).send('alice@active'),
            'missing required authority bob'
        )
    })

    it('buyslot: insufficient balance', async () => {
        await expectToThrow(
            contracts.poolreg.actions.buyslot(['bob', 'bob', 1]).send('bob@active'),
            'eosio_assert: 3003:rescmng.xsat::pay: insufficient balance'
        )
    })

    it('buyslot', async () => {
        // recharge
        await contracts.btc.actions.transfer(['bob', 'rescmng.xsat', '1.00000000 BTC', 'bob']).send('bob@active')
        await contracts.btc.actions.transfer(['amy', 'rescmng.xsat', '1.00000000 BTC', 'amy']).send('amy@active')

        // buyslot for self
        const before_bob_slots = get_synchronizer('bob').num_slots
        await contracts.poolreg.actions.buyslot(['bob', 'bob', 1]).send('bob@active')
        const after_bob_slots = get_synchronizer('bob').num_slots
        expect(after_bob_slots - before_bob_slots).toEqual(1)

        // buyslot for others
        const before_alice_slots = get_synchronizer('bob').num_slots
        await contracts.poolreg.actions.buyslot(['amy', 'bob', 1]).send('amy@active')
        const after_alice_slots = get_synchronizer('bob').num_slots
        expect(after_alice_slots - before_alice_slots).toEqual(1)
    })

    it('buyslot: the total number of slots purchased cannot exceed [1000]', async () => {
        await expectToThrow(
            contracts.poolreg.actions.buyslot(['bob', 'bob', 1000]).send('bob@active'),
            'eosio_assert: recsmng.xsat::buyslot: the total number of slots purchased cannot exceed [1000]'
        )
    })

    it('config: missing required authority', async () => {
        await expectToThrow(
            contracts.poolreg.actions.config(['bob', 1]).send('alice@active'),
            'missing required authority poolreg.xsat'
        )
    })

    it('config', async () => {
        await contracts.poolreg.actions.config(['bob', 0]).send('poolreg.xsat@active')
        expect(get_synchronizer('bob').produced_block_limit).toEqual(0)
    })
    
    it('config', async () => {
        await contracts.poolreg.actions.config(['bob', 0]).send('poolreg.xsat@active')
        expect(get_synchronizer('bob').produced_block_limit).toEqual(0)
    })

    it('config', async () => {
        await contracts.poolreg.actions.config(['bob', 0]).send('poolreg.xsat@active')
        expect(get_synchronizer('bob').produced_block_limit).toEqual(0)
    })

    it('setdonateacc: missing required authority', async () => {
        await expectToThrow(
            contracts.poolreg.actions.setdonateacc(['alice']).send('alice@active'),
            'missing required authority poolreg.xsat'
        )
    })

    it('setdonateacc: donation account does not exists', async () => {
        await expectToThrow(
            contracts.poolreg.actions.setdonateacc(['xsat']).send('poolreg.xsat@active'),
            'eosio_assert: poolreg.xsat::setdonateacc: donation account does not exists'
        )
    })

    it('setdonateacc', async () => {
        await contracts.poolreg.actions.setdonateacc(['donate.xsat']).send('poolreg.xsat@active'),
            expect(get_config()).toEqual({
                donation_account: 'donate.xsat',
            })
    })

    it('setdonate: missing required authority', async () => {
        await expectToThrow(
            contracts.poolreg.actions.setdonate(['bob', 100]).send('alice@active'),
            'missing required authority bob'
        )
    })

    it('setdonate: donate_rate must be less than or equal to 10000', async () => {
        await expectToThrow(
            contracts.poolreg.actions.setdonate(['bob', 20000]).send('bob@active'),
            'eosio_assert_message: poolreg.xsat::setdonate: donate_rate must be less than or equal to 10000'
        )
    })

    it('setdonate: 10%', async () => {
        await contracts.poolreg.actions.setdonate(['bob', 100]).send('bob@active')
        const synchronizer = get_synchronizer('bob')
        expect(synchronizer.donate_rate).toEqual(100)
    })

    it('transfer reward', async () => {
        await contracts.exsat.actions
            .transfer(['rwddist.xsat', 'poolreg.xsat', '100.00000000 XSAT', 'bob,840001'])
            .send('rwddist.xsat@active')
    })

    it('claim: should check if donate_rate > 0', async () => {
        const alice_before_balance = getTokenBalance(blockchain, 'alice', 'exsat.xsat', XSAT.code)
        const donate_before_balance = getTokenBalance(blockchain, 'donate.xsat', 'exsat.xsat', XSAT.code)
        await contracts.poolreg.actions.claim(['bob']).send('alice@active')
        const alice_after_balance = getTokenBalance(blockchain, 'alice', 'exsat.xsat', XSAT.code)
        const donate_after_balance = getTokenBalance(blockchain, 'donate.xsat', 'exsat.xsat', XSAT.code)
        expect(alice_after_balance - alice_before_balance).toEqual(9900000000)
        expect(donate_after_balance - donate_before_balance).toEqual(100000000)
    })

    it('setdonate: 100%', async () => {
        await contracts.poolreg.actions.setdonate(['bob', 10000]).send('bob@active')
        const synchronizer = get_synchronizer('bob')
        expect(synchronizer.donate_rate).toEqual(10000)
    })

    it('transfer reward', async () => {
        await contracts.exsat.actions
            .transfer(['rwddist.xsat', 'poolreg.xsat', '100.00000000 XSAT', 'bob,840003'])
            .send('rwddist.xsat@active')
    })

    it('claim: donate_rate = 100%', async () => {
        const alice_before_balance = getTokenBalance(blockchain, 'alice', 'exsat.xsat', XSAT.code)
        const donate_before_balance = getTokenBalance(blockchain, 'donate.xsat', 'exsat.xsat', XSAT.code)
        await contracts.poolreg.actions.claim(['bob']).send('alice@active')
        const alice_after_balance = getTokenBalance(blockchain, 'alice', 'exsat.xsat', XSAT.code)
        const donate_after_balance = getTokenBalance(blockchain, 'donate.xsat', 'exsat.xsat', XSAT.code)
        expect(alice_after_balance - alice_before_balance).toEqual(0)
        expect(donate_after_balance - donate_before_balance).toEqual(10000000000)
    })
})
