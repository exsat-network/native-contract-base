const { Name, Asset, TimePointSec } = require('@greymass/eosio')
const { Blockchain, expectToThrow } = require('@proton/vert')
const { BTC, EOS, XSAT } = require('./src/constants')
const { getTokenBalance, subTime } = require('./src/help')

// Vert EOS VM
const blockchain = new Blockchain()
//log.setLevel('debug');
// contracts
const contracts = {
    endrmng: blockchain.createContract('endrmng.xsat', 'tests/wasm/endrmng.xsat', true),
    eos: blockchain.createContract('eosio.token', 'tests/wasm/exsat.xsat', true),
    exsat: blockchain.createContract('exsat.xsat', 'tests/wasm/exsat.xsat', true),
}

blockchain.createAccounts('staking.xsat', 'xsatstk.xsat', 'rwddist.xsat', 'custody.xsat', 'alice', 'bob', 'amy', 'anna', 'tony', 'tom')

const get_whitelist = (type, account) => {
    const scope = Name.from(type).value.value
    const key = Name.from(account).value.value
    return contracts.endrmng.tables.whitelist(scope).getTableRow(key)
}

const get_evm_proxy = type => {
    const scope = Name.from(type).value.value
    return contracts.endrmng.tables.evmproxys(scope).getTableRows()
}

const get_validator = validatory => {
    const key = Name.from(validatory).value.value
    return contracts.endrmng.tables.validators().getTableRow(key)
}

const get_native_staker = staker_id => {
    return contracts.endrmng.tables.stakers().getTableRow(BigInt(staker_id))
}

const get_evm_staker = staker => {
    const key = Name.from(staker).value.value
    return contracts.endrmng.tables.evmstakers().getTableRow(key)
}

// one-time setup
beforeAll(async () => {
    blockchain.setTime(TimePointSec.from(new Date()))

    // create XSAT token
    await contracts.exsat.actions.create(['exsat.xsat', '10000000.00000000 XSAT']).send('exsat.xsat@active')
    await contracts.exsat.actions.issue(['exsat.xsat', '10000000.00000000 XSAT', 'init']).send('exsat.xsat@active')

    // transfer XSAT to account
    await contracts.exsat.actions
        .transfer(['exsat.xsat', 'rwddist.xsat', '1000000.00000000 XSAT', ''])
        .send('exsat.xsat@active')

    // create EOS token
    await contracts.eos.actions.create(['eosio.token', '10000000000.0000 EOS']).send('eosio.token@active')
    await contracts.eos.actions.issue(['eosio.token', '10000000000.0000 EOS', 'init']).send('eosio.token@active')

    // transfer EOS to account
    await contracts.eos.actions
        .transfer(['eosio.token', 'rwddist.xsat', '1000000.0000 EOS', ''])
        .send('eosio.token@active')
})

describe('endrmng.xsat', () => {
    it('addwhitelist: missing required authority', async () => {
        await expectToThrow(
            contracts.endrmng.actions.addwhitelist(['proxyreg', 'alice']).send('alice@active'),
            'missing required authority endrmng.xsat'
        )
    })

    it('addwhitelist: type invalid', async () => {
        await expectToThrow(
            contracts.endrmng.actions.addwhitelist(['proxy', 'alice']).send('endrmng.xsat@active'),
            'eosio_assert: endrmng.xsat::addwhitelist: type invalid'
        )
    })

    it('addwhitelist: account does not exists', async () => {
        await expectToThrow(
            contracts.endrmng.actions.addwhitelist(['proxyreg', 'brian']).send('endrmng.xsat@active'),
            'eosio_assert: endrmng.xsat::addwhitelist: account does not exist'
        )
    })

    it('addwhitelist', async () => {
        await contracts.endrmng.actions.addwhitelist(['proxyreg', 'alice']).send('endrmng.xsat@active')
        expect(get_whitelist('proxyreg', 'alice')).toEqual({ account: 'alice' })
    })

    it('addwhitelist: whitelist account already exists', async () => {
        await expectToThrow(
            contracts.endrmng.actions.addwhitelist(['proxyreg', 'alice']).send('endrmng.xsat@active'),
            'eosio_assert: endrmng.xsat::addwhitelist: whitelist account already exists'
        )
    })

    it('delwhitelist: missing required authority', async () => {
        await expectToThrow(
            contracts.endrmng.actions.delwhitelist(['proxyreg', 'alice']).send('alice@active'),
            'missing required authority endrmng.xsat'
        )
    })

    it('delwhitelist: whitelist account does not exist', async () => {
        await expectToThrow(
            contracts.endrmng.actions.delwhitelist(['proxyreg', 'brian']).send('endrmng.xsat@active'),
            'eosio_assert: endrmng.xsat::delwhitelist: whitelist account does not exist'
        )
    })

    it('delwhitelist', async () => {
        await contracts.endrmng.actions.delwhitelist(['proxyreg', 'alice']).send('endrmng.xsat@active')
        expect(get_whitelist('proxyreg', 'alice')).toEqual(undefined)
    })

    //it('addevmproxy: missing required authority', async () => {
    //    await expectToThrow(
    //        contracts.endrmng.actions.addevmproxy(['78357316239040e19fC823372cC179ca75e64b81']).send('alice@active'),
    //        'missing required authority endrmng.xsat'
    //    )
    //})

    //it('addevmproxy', async () => {
    //    await contracts.endrmng.actions
    //        .addevmproxy(['78357316239040e19fC823372cC179ca75e64b81'])
    //        .send('endrmng.xsat@active')
    //    expect(get_evm_proxy()).toEqual([{}])
    //})

    //it('addevmproxy: proxy already exists', async () => {
    //    await expectToThrow(
    //        contracts.endrmng.actions
    //            .addevmproxy(['78357316239040e19fC823372cC179ca75e64b81'])
    //            .send('endrmng.xsat@active'),
    //        'eosio_assert: endrmng.xsat::addevmproxy: proxy already exists'
    //    )
    //})

    //it('delevmproxy: missing required authority', async () => {
    //    await expectToThrow(
    //        contracts.endrmng.actions.delevmproxy(['78357316239040e19fC823372cC179ca75e64b81']).send('alice@active'),
    //        'missing required authority endrmng.xsat'
    //    )
    //})

    //it('delevmproxy: [evmproxys] does not exist', async () => {
    //    await expectToThrow(
    //        contracts.endrmng.actions
    //            .delevmproxy(['78357316239040e19fC823372cC179ca75e64b81'])
    //            .send('endrmng.xsat@active'),
    //        'endrmng.xsat::delevmproxy: [evmproxys] does not exist'
    //    )
    //})

    //it('delevmproxy', async () => {
    //    await contracts.endrmng.actions
    //        .delevmproxy(['78357316239040e19fC823372cC179ca75e64b81'])
    //        .send('endrmng.xsat@active')
    //    expect(get_evm_proxy()).toEqual(undefined)
    //})

    it('regvalidator: missing required authority', async () => {
        await expectToThrow(
            contracts.endrmng.actions.regvalidator(['alice', 'alice', 2000]).send('bob@active'),
            'missing required authority alice'
        )
    })

    it('regvalidator', async () => {
        await contracts.endrmng.actions.regvalidator(['alice', 'alice', 3000]).send('alice@active')
        expect(get_validator('alice')).toEqual({
            commission_rate: 3000,
            consensus_acc_per_share: 0,
            consensus_reward_balance: '0.00000000 XSAT',
            consensus_reward_claimed: '0.00000000 XSAT',
            consensus_reward_unclaimed: '0.00000000 XSAT',
            latest_reward_block: 0,
            latest_reward_time: '1970-01-01T00:00:00',
            latest_staking_time: '1970-01-01T00:00:00',
            memo: '',
            owner: 'alice',
            quantity: '0.00000000 BTC',
            qualification: '0.00000000 BTC',
            xsat_quantity: '0.00000000 XSAT',
            reward_recipient: 'alice',
            stake_acc_per_share: 0,
            staking_reward_balance: '0.00000000 XSAT',
            staking_reward_claimed: '0.00000000 XSAT',
            staking_reward_unclaimed: '0.00000000 XSAT',
            disabled_staking: false,
            total_consensus_reward: '0.00000000 XSAT',
            total_staking_reward: '0.00000000 XSAT',
        })
    })

    it('regvalidator: [validators] already exists', async () => {
        await expectToThrow(
            contracts.endrmng.actions.regvalidator(['alice', 'alice', 3000]).send('alice@active'),
            'eosio_assert: endrmng.xsat::regvalidator: [validators] already exists'
        )
    })

    it('proxyreg: missing required authority', async () => {
        await expectToThrow(
            contracts.endrmng.actions.proxyreg(['bob', 'alice', 'alice', 2000]).send('alice@active'),
            'missing required authority bob'
        )
    })

    it('proxyreg: caller is not in the `proxyreg` whitelist', async () => {
        await expectToThrow(
            contracts.endrmng.actions.proxyreg(['bob', 'alice', 'alice', 2000]).send('bob@active'),
            'eosio_assert: endrmng.xsat::proxyreg: caller is not in the `proxyreg` whitelist'
        )
    })

    it('proxyreg: validator account does not exists', async () => {
        await contracts.endrmng.actions.addwhitelist(['proxyreg', 'bob']).send('endrmng.xsat@active')
        await expectToThrow(
            contracts.endrmng.actions.proxyreg(['bob', 'brian', 'brian', 2000]).send('bob@active'),
            'eosio_assert: endrmng.xsat::proxyreg: validator account does not exists'
        )
    })

    it('proxyreg: [validators] already exists', async () => {
        await expectToThrow(
            contracts.endrmng.actions.proxyreg(['bob', 'alice', 'alice', 2000]).send('bob@active'),
            'eosio_assert: endrmng.xsat::regvalidator: [validators] already exists'
        )
    })

    it('proxyreg: financial account does not exists', async () => {
        await expectToThrow(
            contracts.endrmng.actions.proxyreg(['bob', 'amy', 'brian', 2000]).send('bob@active'),
            'eosio_assert: endrmng.xsat::regvalidator: financial account does not exists'
        )
    })

    it('proxyreg: invalid financial account', async () => {
        await expectToThrow(
            contracts.endrmng.actions
                .proxyreg(['bob', 'amy', '78357316239040e19fC823372cC179ca75e64b1', 2000])
                .send('bob@active'),
            'eosio_assert: endrmng.xsat::regvalidator: invalid financial account'
        )
    })

    it('proxyreg', async () => {
        await contracts.endrmng.actions.proxyreg(['bob', 'amy', 'anna', 2000]).send('bob@active')
        expect(get_validator('amy')).toEqual({
            disabled_staking: false,
            commission_rate: 2000,
            consensus_acc_per_share: 0,
            consensus_reward_balance: '0.00000000 XSAT',
            consensus_reward_claimed: '0.00000000 XSAT',
            consensus_reward_unclaimed: '0.00000000 XSAT',
            latest_reward_block: 0,
            latest_reward_time: '1970-01-01T00:00:00',
            latest_staking_time: '1970-01-01T00:00:00',
            memo: '',
            owner: 'amy',
            quantity: '0.00000000 BTC',
            qualification: '0.00000000 BTC',
            xsat_quantity: '0.00000000 XSAT',
            reward_recipient: 'anna',
            stake_acc_per_share: 0,
            staking_reward_balance: '0.00000000 XSAT',
            staking_reward_claimed: '0.00000000 XSAT',
            staking_reward_unclaimed: '0.00000000 XSAT',
            total_consensus_reward: '0.00000000 XSAT',
            total_staking_reward: '0.00000000 XSAT',
        })

        // evm financial account
        await contracts.endrmng.actions
            .proxyreg(['bob', 'tony', '78357316239040e19fC823372cC179ca75e64b81', 1000])
            .send('bob@active')
        expect(get_validator('tony')).toEqual({
            disabled_staking: false,
            commission_rate: 1000,
            consensus_acc_per_share: 0,
            consensus_reward_balance: '0.00000000 XSAT',
            consensus_reward_claimed: '0.00000000 XSAT',
            consensus_reward_unclaimed: '0.00000000 XSAT',
            latest_reward_block: 0,
            latest_reward_time: '1970-01-01T00:00:00',
            latest_staking_time: '1970-01-01T00:00:00',
            memo: '78357316239040e19fC823372cC179ca75e64b81',
            owner: 'tony',
            quantity: '0.00000000 BTC',
            qualification: '0.00000000 BTC',
            xsat_quantity: '0.00000000 XSAT',
            reward_recipient: 'erc2o.xsat',
            stake_acc_per_share: 0,
            staking_reward_balance: '0.00000000 XSAT',
            staking_reward_claimed: '0.00000000 XSAT',
            staking_reward_unclaimed: '0.00000000 XSAT',
            total_consensus_reward: '0.00000000 XSAT',
            total_staking_reward: '0.00000000 XSAT',
        })
    })

    it('config: missing required authority', async () => {
        await expectToThrow(
            contracts.endrmng.actions.config(['amy', 2000, 'alice']).send('tony@active'),
            'missing required authority amy'
        )
    })

    it('config: [validators] does not exist', async () => {
        await expectToThrow(
            contracts.endrmng.actions.config(['tom', 2000, 'alice']).send('tom@active'),
            'eosio_assert: endrmng.xsat::config: [validators] does not exists'
        )
    })

    it('config: commission_rate must be less than or equal to 10000', async () => {
        await expectToThrow(
            contracts.endrmng.actions
                .config(['amy', 100000, '78357316239040e19fC823372cC179ca75e64b81'])
                .send('amy@active'),
            'eosio_assert_message: endrmng.xsat::config: commission_rate must be less than or equal to 10000'
        )
    })

    it('config: financial account does not exists', async () => {
        await expectToThrow(
            contracts.endrmng.actions.config(['amy', 2000, 'brian']).send('amy@active'),
            'eosio_assert: endrmng.xsat::config: financial account does not exists'
        )
    })

    it('config: invalid financial account', async () => {
        await expectToThrow(
            contracts.endrmng.actions
                .config(['amy', 2000, '78357316239040e19fC823372cC179ca75e64b1'])
                .send('amy@active'),
            'eosio_assert: endrmng.xsat::config: invalid financial account'
        )
    })

    it('config', async () => {
        await contracts.endrmng.actions.config(['amy', 1000, 'tony']).send('amy@active')

        expect(get_validator('amy')).toEqual({
            disabled_staking: false,
            commission_rate: 1000,
            consensus_acc_per_share: 0,
            consensus_reward_balance: '0.00000000 XSAT',
            consensus_reward_claimed: '0.00000000 XSAT',
            consensus_reward_unclaimed: '0.00000000 XSAT',
            latest_reward_block: 0,
            latest_reward_time: '1970-01-01T00:00:00',
            latest_staking_time: '1970-01-01T00:00:00',
            memo: '',
            owner: 'amy',
            quantity: '0.00000000 BTC',
            qualification: '0.00000000 BTC',
            xsat_quantity: '0.00000000 XSAT',
            reward_recipient: 'tony',
            stake_acc_per_share: 0,
            staking_reward_balance: '0.00000000 XSAT',
            staking_reward_claimed: '0.00000000 XSAT',
            staking_reward_unclaimed: '0.00000000 XSAT',
            total_consensus_reward: '0.00000000 XSAT',
            total_staking_reward: '0.00000000 XSAT',
        })

        await contracts.endrmng.actions
            .config(['amy', 5000, '78357316239040e19fC823372cC179ca75e64b81'])
            .send('amy@active')

        expect(get_validator('amy')).toEqual({
            disabled_staking: false,
            commission_rate: 5000,
            consensus_acc_per_share: 0,
            consensus_reward_balance: '0.00000000 XSAT',
            consensus_reward_claimed: '0.00000000 XSAT',
            consensus_reward_unclaimed: '0.00000000 XSAT',
            latest_reward_block: 0,
            latest_reward_time: '1970-01-01T00:00:00',
            latest_staking_time: '1970-01-01T00:00:00',
            memo: '78357316239040e19fC823372cC179ca75e64b81',
            owner: 'amy',
            quantity: '0.00000000 BTC',
            qualification: '0.00000000 BTC',
            xsat_quantity: '0.00000000 XSAT',
            reward_recipient: 'erc2o.xsat',
            stake_acc_per_share: 0,
            staking_reward_balance: '0.00000000 XSAT',
            staking_reward_claimed: '0.00000000 XSAT',
            staking_reward_unclaimed: '0.00000000 XSAT',
            total_consensus_reward: '0.00000000 XSAT',
            total_staking_reward: '0.00000000 XSAT',
        })
    })

    it('setstatus: missing required authority', async () => {
        await expectToThrow(
            contracts.endrmng.actions.setstatus(['alice', true]).send('alice@active'),
            'missing required authority endrmng.xsat'
        )
    })

    it('setstatus: [validators] does not exists', async () => {
        await expectToThrow(
            contracts.endrmng.actions.setstatus(['tom', true]).send('endrmng.xsat@active'),
            'eosio_assert: endrmng.xsat::setstatus: [validators] does not exists'
        )
    })

    it('setstatus', async () => {
        await contracts.endrmng.actions.setstatus(['amy', true]).send('endrmng.xsat@active')
        expect(get_validator('amy').disabled_staking).toEqual(true)

        await contracts.endrmng.actions.setstatus(['amy', false]).send('endrmng.xsat@active')
        expect(get_validator('amy').disabled_staking).toEqual(false)
    })

    it('stake: missing required authority', async () => {
        await expectToThrow(
            contracts.endrmng.actions.stake(['tony', 'amy', Asset.from(100, BTC)]).send('alice@active'),
            'missing required authority staking.xsat'
        )
    })

    it('stake: quantity symbol must be BTC', async () => {
        await expectToThrow(
            contracts.endrmng.actions.stake(['tony', 'amy', Asset.from(100, EOS)]).send('staking.xsat@active'),
            'eosio_assert: endrmng.xsat::stake: quantity symbol must be BTC'
        )
    })

    it('stake: quantity must be greater than 0', async () => {
        await expectToThrow(
            contracts.endrmng.actions.stake(['tony', 'amy', Asset.from(0, BTC)]).send('staking.xsat@active'),
            'eosio_assert: endrmng.xsat::stake: quantity must be greater than 0'
        )
    })

    it('stake: [validators] does not exists', async () => {
        await expectToThrow(
            contracts.endrmng.actions.stake(['tom', 'tom', Asset.from(100, BTC)]).send('staking.xsat@active'),
            'eosio_assert: endrmng.xsat::stake: [validators] does not exists'
        )
    })

    it("stake: the current validator's staking status is disabled", async () => {
        await contracts.endrmng.actions.setstatus(['amy', true]).send('endrmng.xsat@active')
        await expectToThrow(
            contracts.endrmng.actions.stake(['tony', 'amy', Asset.from(100, BTC)]).send('staking.xsat@active'),
            "eosio_assert: endrmng.xsat::stake: the current validator's staking status is disabled"
        )
        await contracts.endrmng.actions.setstatus(['amy', false]).send('endrmng.xsat@active')
    })

    it('stake', async () => {
        await contracts.endrmng.actions.stake(['tony', 'alice', Asset.from(100, BTC)]).send('staking.xsat@active')
        expect(get_validator('alice')).toEqual({
            commission_rate: 3000,
            consensus_acc_per_share: 0,
            consensus_reward_balance: '0.00000000 XSAT',
            consensus_reward_claimed: '0.00000000 XSAT',
            consensus_reward_unclaimed: '0.00000000 XSAT',
            latest_reward_block: 0,
            latest_reward_time: '1970-01-01T00:00:00',
            latest_staking_time: TimePointSec.from(blockchain.timestamp).toString(),
            memo: '',
            owner: 'alice',
            quantity: '100.00000000 BTC',
            qualification: '100.00000000 BTC',
            xsat_quantity: '0.00000000 XSAT',
            reward_recipient: 'alice',
            stake_acc_per_share: 0,
            staking_reward_balance: '0.00000000 XSAT',
            staking_reward_claimed: '0.00000000 XSAT',
            staking_reward_unclaimed: '0.00000000 XSAT',
            disabled_staking: false,
            total_consensus_reward: '0.00000000 XSAT',
            total_staking_reward: '0.00000000 XSAT',
        })
        expect(get_native_staker(1)).toEqual({
            id: 1,
            quantity: '100.00000000 BTC',
            xsat_quantity: '0.00000000 XSAT',
            staker: 'tony',
            stake_debt: 0,
            staking_reward_claimed: '0.00000000 XSAT',
            staking_reward_unclaimed: '0.00000000 XSAT',
            consensus_debt: 0,
            consensus_reward_claimed: '0.00000000 XSAT',
            consensus_reward_unclaimed: '0.00000000 XSAT',
            validator: 'alice',
        })
    })

    it('unstake: missing required authority', async () => {
        await expectToThrow(
            contracts.endrmng.actions.unstake(['tony', 'amy', Asset.from(100, BTC)]).send('alice@active'),
            'missing required authority staking.xsat'
        )
    })

    it('unstake: quantity symbol must be BTC', async () => {
        await expectToThrow(
            contracts.endrmng.actions.unstake(['tony', 'amy', Asset.from(100, EOS)]).send('staking.xsat@active'),
            'eosio_assert: endrmng.xsat::unstake: quantity symbol must be BTC'
        )
    })

    it('unstake: quantity must be greater than 0', async () => {
        await expectToThrow(
            contracts.endrmng.actions.unstake(['tony', 'amy', Asset.from(0, BTC)]).send('staking.xsat@active'),
            'eosio_assert: endrmng.xsat::unstake: quantity must be greater than 0'
        )
    })

    it('unstake: [stakers] does not exists', async () => {
        await expectToThrow(
            contracts.endrmng.actions.unstake(['bob', 'alice', Asset.from(100, BTC)]).send('staking.xsat@active'),
            'eosio_assert: endrmng.xsat::unstake: [stakers] does not exists'
        )
    })

    it('unstake', async () => {
        blockchain.addTime(TimePointSec.from(1000))
        await contracts.endrmng.actions.unstake(['tony', 'alice', Asset.from(2, BTC)]).send('staking.xsat@active')
        expect(get_validator('alice')).toEqual({
            commission_rate: 3000,
            consensus_acc_per_share: 0,
            consensus_reward_balance: '0.00000000 XSAT',
            consensus_reward_claimed: '0.00000000 XSAT',
            consensus_reward_unclaimed: '0.00000000 XSAT',
            latest_reward_block: 0,
            latest_reward_time: '1970-01-01T00:00:00',
            latest_staking_time: TimePointSec.from(blockchain.timestamp).toString(),
            memo: '',
            owner: 'alice',
            quantity: '98.00000000 BTC',
            qualification: '98.00000000 BTC',
            xsat_quantity: '0.00000000 XSAT',
            reward_recipient: 'alice',
            stake_acc_per_share: 0,
            staking_reward_balance: '0.00000000 XSAT',
            staking_reward_claimed: '0.00000000 XSAT',
            staking_reward_unclaimed: '0.00000000 XSAT',
            disabled_staking: false,
            total_consensus_reward: '0.00000000 XSAT',
            total_staking_reward: '0.00000000 XSAT',
        })
        expect(get_native_staker(1)).toEqual({
            id: 1,
            quantity: '98.00000000 BTC',
            xsat_quantity: '0.00000000 XSAT',
            staker: 'tony',
            stake_debt: 0,
            staking_reward_claimed: '0.00000000 XSAT',
            staking_reward_unclaimed: '0.00000000 XSAT',
            consensus_debt: 0,
            consensus_reward_claimed: '0.00000000 XSAT',
            consensus_reward_unclaimed: '0.00000000 XSAT',
            validator: 'alice',
        })
    })

    it('newstake: missing required authority', async () => {
        await expectToThrow(
            contracts.endrmng.actions.newstake(['tony', 'alice', 'tony', Asset.from(100, BTC)]).send('alice@active'),
            'missing required authority tony'
        )
    })

    it('newstake: quantity symbol must be BTC', async () => {
        await expectToThrow(
            contracts.endrmng.actions.newstake(['tony', 'alice', 'tony', Asset.from(100, EOS)]).send('tony@active'),
            'eosio_assert: endrmng.xsat::unstake: quantity symbol must be BTC'
        )
    })

    it('newstake: quantity must be greater than 0', async () => {
        await expectToThrow(
            contracts.endrmng.actions.newstake(['tony', 'alice', 'tony', Asset.from(0, BTC)]).send('tony@active'),
            'eosio_assert: endrmng.xsat::unstake: quantity must be greater than 0'
        )
    })

    it('newstake: [stakers] does not exists', async () => {
        await expectToThrow(
            contracts.endrmng.actions.newstake(['tony', 'bob', 'tony', Asset.from(100, BTC)]).send('tony@active'),
            'eosio_assert: endrmng.xsat::unstake: [stakers] does not exists'
        )
    })

    it('newstake: the number of unstakes exceeds the pledge amount', async () => {
        await expectToThrow(
            contracts.endrmng.actions.newstake(['tony', 'alice', 'brian', Asset.from(100, BTC)]).send('tony@active'),
            'eosio_assert: endrmng.xsat::unstake: the number of unstakes exceeds the staking amount'
        )
    })

    it('newstake: [validators] does not exists', async () => {
        await expectToThrow(
            contracts.endrmng.actions.newstake(['tony', 'alice', 'brian', Asset.from(2, BTC)]).send('tony@active'),
            'eosio_assert: endrmng.xsat::stake: [validators] does not exists'
        )
    })

    it('newstake', async () => {
        await contracts.endrmng.actions.newstake(['tony', 'alice', 'tony', Asset.from(2, BTC)]).send('tony@active')
        expect(get_native_staker(1)).toEqual({
            id: 1,
            quantity: '96.00000000 BTC',
            xsat_quantity: '0.00000000 XSAT',
            staker: 'tony',
            stake_debt: 0,
            staking_reward_claimed: '0.00000000 XSAT',
            staking_reward_unclaimed: '0.00000000 XSAT',
            consensus_debt: 0,
            consensus_reward_claimed: '0.00000000 XSAT',
            consensus_reward_unclaimed: '0.00000000 XSAT',
            validator: 'alice',
        })
        expect(get_native_staker(2)).toEqual({
            id: 2,
            quantity: '2.00000000 BTC',
            xsat_quantity: '0.00000000 XSAT',
            staker: 'tony',
            stake_debt: 0,
            staking_reward_claimed: '0.00000000 XSAT',
            staking_reward_unclaimed: '0.00000000 XSAT',
            consensus_debt: 0,
            consensus_reward_claimed: '0.00000000 XSAT',
            consensus_reward_unclaimed: '0.00000000 XSAT',
            validator: 'tony',
        })

        expect(get_validator('alice')).toEqual({
            commission_rate: 3000,
            consensus_acc_per_share: 0,
            consensus_reward_balance: '0.00000000 XSAT',
            consensus_reward_claimed: '0.00000000 XSAT',
            consensus_reward_unclaimed: '0.00000000 XSAT',
            latest_reward_block: 0,
            latest_reward_time: '1970-01-01T00:00:00',
            latest_staking_time: TimePointSec.from(blockchain.timestamp).toString(),
            memo: '',
            owner: 'alice',
            quantity: '96.00000000 BTC',
            qualification: '96.00000000 BTC',
            xsat_quantity: '0.00000000 XSAT',
            reward_recipient: 'alice',
            stake_acc_per_share: 0,
            staking_reward_balance: '0.00000000 XSAT',
            staking_reward_claimed: '0.00000000 XSAT',
            staking_reward_unclaimed: '0.00000000 XSAT',
            disabled_staking: false,
            total_consensus_reward: '0.00000000 XSAT',
            total_staking_reward: '0.00000000 XSAT',
        })

        expect(get_validator('tony')).toEqual({
            commission_rate: 1000,
            consensus_acc_per_share: 0,
            consensus_reward_balance: '0.00000000 XSAT',
            consensus_reward_claimed: '0.00000000 XSAT',
            consensus_reward_unclaimed: '0.00000000 XSAT',
            latest_reward_block: 0,
            latest_reward_time: '1970-01-01T00:00:00',
            latest_staking_time: TimePointSec.from(blockchain.timestamp).toString(),
            memo: '78357316239040e19fC823372cC179ca75e64b81',
            owner: 'tony',
            quantity: '2.00000000 BTC',
            qualification: '2.00000000 BTC',
            xsat_quantity: '0.00000000 XSAT',
            reward_recipient: 'erc2o.xsat',
            stake_acc_per_share: 0,
            staking_reward_balance: '0.00000000 XSAT',
            staking_reward_claimed: '0.00000000 XSAT',
            staking_reward_unclaimed: '0.00000000 XSAT',
            disabled_staking: false,
            total_consensus_reward: '0.00000000 XSAT',
            total_staking_reward: '0.00000000 XSAT',
        })
    })

    it('transfer reward: only transfer [exsat.xsat/XSAT]', async () => {
        await expectToThrow(
            contracts.eos.actions
                .transfer(['rwddist.xsat', 'endrmng.xsat', '100.0000 EOS', ''])
                .send('rwddist.xsat@active'),
            'eosio_assert: endrmng.xsat: only transfer [exsat.xsat/XSAT]'
        )
    })

    it('claim: no balance to claim', async () => {
        await expectToThrow(
            contracts.endrmng.actions.claim(['tony', 'alice']).send('tony@active'),
            'eosio_assert: endrmng.xsat::claim: no balance to claim'
        )
    })

    it('distribute: missing required authority rwddist.xsat', async () => {
        await expectToThrow(
            contracts.endrmng.actions
                .distribute([
                    840000,
                    [{ validator: 'alice', staking_rewards: '22.50000000 XSAT', consensus_rewards: '2.50000000 XSAT' }],
                ])
                .send('alice@active'),
            'missing required authority rwddist.xsat'
        )
    })

    it('transfer reward', async () => {
        await contracts.exsat.actions
            .transfer([
                'rwddist.xsat',
                'endrmng.xsat',
                '25.00000000 XSAT',
                'alice,840000,22.50000000 XSAT,2.50000000 XSAT',
            ])
            .send('rwddist.xsat@active')

        await contracts.endrmng.actions
            .distribute([
                840000,
                [{ validator: 'alice', staking_rewards: '22.50000000 XSAT', consensus_rewards: '2.50000000 XSAT' }],
            ])
            .send('rwddist.xsat@active')

        expect(get_validator('alice')).toEqual({
            commission_rate: 3000,
            consensus_acc_per_share: Asset.from('1.75000000 XSAT')
                .units.multiplying(100000000)
                .dividing(Asset.from('96.00000000 BTC').units)
                .toNumber(),
            consensus_reward_balance: '2.50000000 XSAT',
            consensus_reward_claimed: '0.00000000 XSAT',
            consensus_reward_unclaimed: '0.75000000 XSAT',
            latest_reward_block: 840000,
            latest_reward_time: TimePointSec.from(blockchain.timestamp).toString(),
            latest_staking_time: TimePointSec.from(blockchain.timestamp).toString(),
            memo: '',
            owner: 'alice',
            quantity: '96.00000000 BTC',
            qualification: '96.00000000 BTC',
            xsat_quantity: '0.00000000 XSAT',
            reward_recipient: 'alice',
            stake_acc_per_share: Asset.from('15.75000000 XSAT')
                .units.multiplying(100000000)
                .dividing(Asset.from('96.00000000 BTC').units)
                .toNumber(),
            staking_reward_balance: '22.50000000 XSAT',
            staking_reward_claimed: '0.00000000 XSAT',
            staking_reward_unclaimed: '6.75000000 XSAT',
            disabled_staking: false,
            total_consensus_reward: '2.50000000 XSAT',
            total_staking_reward: '22.50000000 XSAT',
        })
    })

    it('claim: missing required authority', async () => {
        await expectToThrow(
            contracts.endrmng.actions.claim(['tony', 'tony']).send('alice@active'),
            'missing required authority tony'
        )
    })

    it('claim: [stakers] does not exists', async () => {
        await expectToThrow(
            contracts.endrmng.actions.claim(['tony', 'tom']).send('tony@active'),
            'eosio_assert: endrmng.xsat::claim: [stakers] does not exists'
        )
    })

    it('claim', async () => {
        const before_balance = getTokenBalance(blockchain, 'tony', 'exsat.xsat', XSAT.code)
        await contracts.endrmng.actions.claim(['tony', 'alice']).send('tony@active')
        const after_balance = getTokenBalance(blockchain, 'tony', 'exsat.xsat', XSAT.code)

        expect(after_balance - before_balance).toEqual(1575000000 + 174999936)

        expect(get_native_staker(1)).toEqual({
            id: 1,
            quantity: '96.00000000 BTC',
            xsat_quantity: '0.00000000 XSAT',
            staker: 'tony',
            stake_debt: 1575000000,
            staking_reward_claimed: '15.75000000 XSAT',
            staking_reward_unclaimed: '0.00000000 XSAT',
            consensus_debt: 174999936,
            consensus_reward_claimed: '1.74999936 XSAT',
            consensus_reward_unclaimed: '0.00000000 XSAT',
            validator: 'alice',
        })
        expect(get_validator('alice')).toEqual({
            commission_rate: 3000,
            consensus_acc_per_share: 1822916,
            consensus_reward_balance: '0.75000064 XSAT',
            consensus_reward_claimed: '0.00000000 XSAT',
            consensus_reward_unclaimed: '0.75000000 XSAT',
            latest_reward_block: 840000,
            latest_reward_time: TimePointSec.from(blockchain.timestamp).toString(),
            latest_staking_time: TimePointSec.from(blockchain.timestamp).toString(),
            memo: '',
            owner: 'alice',
            quantity: '96.00000000 BTC',
            qualification: '96.00000000 BTC',
            xsat_quantity: '0.00000000 XSAT',
            reward_recipient: 'alice',
            stake_acc_per_share: 16406250,
            staking_reward_balance: '6.75000000 XSAT',
            staking_reward_claimed: '0.00000000 XSAT',
            staking_reward_unclaimed: '6.75000000 XSAT',
            disabled_staking: false,
            total_consensus_reward: '2.50000000 XSAT',
            total_staking_reward: '22.50000000 XSAT',
        })
    })

    it('stake', async () => {
        await contracts.endrmng.actions.stake(['tony', 'alice', Asset.from(2, BTC)]).send('staking.xsat@active')
        expect(get_native_staker(1)).toEqual({
            id: 1,
            quantity: '98.00000000 BTC',
            xsat_quantity: '0.00000000 XSAT',
            staker: 'tony',
            stake_debt: 1607812500,
            staking_reward_claimed: '15.75000000 XSAT',
            staking_reward_unclaimed: '0.00000000 XSAT',
            consensus_debt: 178645768,
            consensus_reward_claimed: '1.74999936 XSAT',
            consensus_reward_unclaimed: '0.00000000 XSAT',
            validator: 'alice',
        })
        expect(get_validator('alice')).toEqual({
            commission_rate: 3000,
            consensus_acc_per_share: 1822916,
            consensus_reward_balance: '0.75000064 XSAT',
            consensus_reward_claimed: '0.00000000 XSAT',
            consensus_reward_unclaimed: '0.75000000 XSAT',
            latest_reward_block: 840000,
            latest_reward_time: TimePointSec.from(blockchain.timestamp).toString(),
            latest_staking_time: TimePointSec.from(blockchain.timestamp).toString(),
            memo: '',
            owner: 'alice',
            quantity: '98.00000000 BTC',
            qualification: '98.00000000 BTC',
            xsat_quantity: '0.00000000 XSAT',
            reward_recipient: 'alice',
            stake_acc_per_share: 16406250,
            staking_reward_balance: '6.75000000 XSAT',
            staking_reward_claimed: '0.00000000 XSAT',
            staking_reward_unclaimed: '6.75000000 XSAT',
            disabled_staking: false,
            total_consensus_reward: '2.50000000 XSAT',
            total_staking_reward: '22.50000000 XSAT',
        })
    })

    it('vdrclaim: missing required authority', async () => {
        await expectToThrow(
            contracts.endrmng.actions.vdrclaim(['alice']).send('bob@active'),
            'missing required authority alice'
        )
    })

    it('vdrclaim', async () => {
        const before_balance = getTokenBalance(blockchain, 'alice', 'exsat.xsat', XSAT.code)
        await contracts.endrmng.actions.vdrclaim(['alice']).send('alice@active')
        const after_balance = getTokenBalance(blockchain, 'alice', 'exsat.xsat', XSAT.code)
        expect(after_balance - before_balance).toEqual(75000000 + 675000000)

        expect(get_validator('alice')).toEqual({
            commission_rate: 3000,
            consensus_acc_per_share: 1822916,
            consensus_reward_balance: '0.00000064 XSAT',
            consensus_reward_claimed: '0.75000000 XSAT',
            consensus_reward_unclaimed: '0.00000000 XSAT',
            latest_reward_block: 840000,
            latest_reward_time: TimePointSec.from(blockchain.timestamp).toString(),
            latest_staking_time: TimePointSec.from(blockchain.timestamp).toString(),
            memo: '',
            owner: 'alice',
            quantity: '98.00000000 BTC',
            qualification: '98.00000000 BTC',
            xsat_quantity: '0.00000000 XSAT',
            reward_recipient: 'alice',
            stake_acc_per_share: 16406250,
            staking_reward_balance: '0.00000000 XSAT',
            staking_reward_claimed: '6.75000000 XSAT',
            staking_reward_unclaimed: '0.00000000 XSAT',
            disabled_staking: false,
            total_consensus_reward: '2.50000000 XSAT',
            total_staking_reward: '22.50000000 XSAT',
        })
    })

    it('stakexsat: missing required authority', async () => {
        await expectToThrow(
            contracts.endrmng.actions.stakexsat(['tony', 'amy', Asset.from(100, XSAT)]).send('alice@active'),
            'missing required authority xsatstk.xsat'
        )
    })

    it('stakexsat: quantity symbol must be XSAT', async () => {
        await expectToThrow(
            contracts.endrmng.actions.stakexsat(['tony', 'amy', Asset.from(100, EOS)]).send('xsatstk.xsat@active'),
            'eosio_assert: endrmng.xsat::stakexsat: quantity symbol must be XSAT'
        )
    })

    it('stakexsat: quantity must be greater than 0', async () => {
        await expectToThrow(
            contracts.endrmng.actions.stakexsat(['tony', 'amy', Asset.from(0, XSAT)]).send('xsatstk.xsat@active'),
            'eosio_assert: endrmng.xsat::stakexsat: quantity must be greater than 0'
        )
    })

    it('stakexsat: [validators] does not exists', async () => {
        await expectToThrow(
            contracts.endrmng.actions.stakexsat(['tom', 'tom', Asset.from(100, XSAT)]).send('xsatstk.xsat@active'),
            'eosio_assert: endrmng.xsat::stakexsat: [validators] does not exists'
        )
    })

    it("stakexsat: the current validator's staking status is disabled", async () => {
        await contracts.endrmng.actions.setstatus(['amy', true]).send('endrmng.xsat@active')
        await expectToThrow(
            contracts.endrmng.actions.stakexsat(['tony', 'amy', Asset.from(100, XSAT)]).send('xsatstk.xsat@active'),
            "eosio_assert: endrmng.xsat::stakexsat: the current validator's staking status is disabled"
        )
        await contracts.endrmng.actions.setstatus(['amy', false]).send('endrmng.xsat@active')
    })

    // XSAT alice 0 => 100
    it('stakexsat', async () => {
        await contracts.endrmng.actions.stakexsat(['tony', 'alice', Asset.from(100, XSAT)]).send('xsatstk.xsat@active')
        expect(get_validator('alice')).toEqual({
            commission_rate: 3000,
            consensus_acc_per_share: 1822916,
            consensus_reward_balance: '0.00000064 XSAT',
            consensus_reward_claimed: '0.75000000 XSAT',
            consensus_reward_unclaimed: '0.00000000 XSAT',
            latest_reward_block: 840000,
            latest_reward_time: TimePointSec.from(blockchain.timestamp).toString(),
            latest_staking_time: TimePointSec.from(blockchain.timestamp).toString(),
            memo: '',
            owner: 'alice',
            quantity: '98.00000000 BTC',
            qualification: '98.00000000 BTC',
            xsat_quantity: '100.00000000 XSAT',
            reward_recipient: 'alice',
            stake_acc_per_share: 16406250,
            staking_reward_balance: '0.00000000 XSAT',
            staking_reward_claimed: '6.75000000 XSAT',
            staking_reward_unclaimed: '0.00000000 XSAT',
            disabled_staking: false,
            total_consensus_reward: '2.50000000 XSAT',
            total_staking_reward: '22.50000000 XSAT',
        })
        expect(get_native_staker(1)).toEqual({
            id: 1,
            quantity: '98.00000000 BTC',
            xsat_quantity: '100.00000000 XSAT',
            staker: 'tony',
            stake_debt: 1607812500,
            staking_reward_claimed: '15.75000000 XSAT',
            staking_reward_unclaimed: '0.00000000 XSAT',
            consensus_debt: 178645768,
            consensus_reward_claimed: '1.74999936 XSAT',
            consensus_reward_unclaimed: '0.00000000 XSAT',
            validator: 'alice',
        })
    })

    it('unstakexsat: missing required authority', async () => {
        await expectToThrow(
            contracts.endrmng.actions.unstakexsat(['tony', 'amy', Asset.from(100, XSAT)]).send('alice@active'),
            'missing required authority xsatstk.xsat'
        )
    })

    it('unstakexsat: quantity symbol must be XSAT', async () => {
        await expectToThrow(
            contracts.endrmng.actions.unstakexsat(['tony', 'amy', Asset.from(100, EOS)]).send('xsatstk.xsat@active'),
            'eosio_assert: endrmng.xsat::unstakexsat: quantity symbol must be XSAT'
        )
    })

    it('unstakexsat: quantity must be greater than 0', async () => {
        await expectToThrow(
            contracts.endrmng.actions.unstakexsat(['tony', 'amy', Asset.from(0, XSAT)]).send('xsatstk.xsat@active'),
            'eosio_assert: endrmng.xsat::unstakexsat: quantity must be greater than 0'
        )
    })

    it('unstakexsat: [stakers] does not exists', async () => {
        await expectToThrow(
            contracts.endrmng.actions.unstakexsat(['bob', 'alice', Asset.from(100, XSAT)]).send('xsatstk.xsat@active'),
            'eosio_assert: endrmng.xsat::unstakexsat: [stakers] does not exists'
        )
    })

    // XSAT alice 100 => 98
    it('unstakexsat', async () => {
        blockchain.addTime(TimePointSec.from(1000))
        await contracts.endrmng.actions.unstakexsat(['tony', 'alice', Asset.from(2, XSAT)]).send('xsatstk.xsat@active')
        expect(get_validator('alice')).toEqual({
            commission_rate: 3000,
            consensus_acc_per_share: 1822916,
            consensus_reward_balance: '0.00000064 XSAT',
            consensus_reward_claimed: '0.75000000 XSAT',
            consensus_reward_unclaimed: '0.00000000 XSAT',
            latest_reward_block: 840000,
            latest_reward_time: subTime(blockchain.timestamp, TimePointSec.from(1000)).toString(),
            latest_staking_time: TimePointSec.from(blockchain.timestamp).toString(),
            memo: '',
            owner: 'alice',
            quantity: '98.00000000 BTC',
            qualification: '98.00000000 BTC',
            xsat_quantity: '98.00000000 XSAT',
            reward_recipient: 'alice',
            stake_acc_per_share: 16406250,
            staking_reward_balance: '0.00000000 XSAT',
            staking_reward_claimed: '6.75000000 XSAT',
            staking_reward_unclaimed: '0.00000000 XSAT',
            disabled_staking: false,
            total_consensus_reward: '2.50000000 XSAT',
            total_staking_reward: '22.50000000 XSAT',
        })
        expect(get_native_staker(1)).toEqual({
            id: 1,
            quantity: '98.00000000 BTC',
            xsat_quantity: '98.00000000 XSAT',
            staker: 'tony',
            stake_debt: 1607812500,
            staking_reward_claimed: '15.75000000 XSAT',
            staking_reward_unclaimed: '0.00000000 XSAT',
            consensus_debt: 178645768,
            consensus_reward_claimed: '1.74999936 XSAT',
            consensus_reward_unclaimed: '0.00000000 XSAT',
            validator: 'alice',
        })
    })

    it('restakexsat: missing required authority', async () => {
        await expectToThrow(
            contracts.endrmng.actions
                .restakexsat(['tony', 'alice', 'tony', Asset.from(100, XSAT)])
                .send('alice@active'),
            'missing required authority tony'
        )
    })

    it('restakexsat: quantity symbol must be XSAT', async () => {
        await expectToThrow(
            contracts.endrmng.actions.restakexsat(['tony', 'alice', 'tony', Asset.from(100, EOS)]).send('tony@active'),
            'eosio_assert: endrmng.xsat::unstakexsat: quantity symbol must be XSAT'
        )
    })

    it('restakexsat: quantity must be greater than 0', async () => {
        await expectToThrow(
            contracts.endrmng.actions.restakexsat(['tony', 'alice', 'tony', Asset.from(0, XSAT)]).send('tony@active'),
            'eosio_assert: endrmng.xsat::unstakexsat: quantity must be greater than 0'
        )
    })

    it('restakexsat: [stakers] does not exists', async () => {
        await expectToThrow(
            contracts.endrmng.actions.restakexsat(['tony', 'bob', 'tony', Asset.from(100, XSAT)]).send('tony@active'),
            'eosio_assert: endrmng.xsat::unstakexsat: [stakers] does not exists'
        )
    })

    it('restakexsat: the number of unstakes exceeds the pledge amount', async () => {
        await expectToThrow(
            contracts.endrmng.actions
                .restakexsat(['tony', 'alice', 'brian', Asset.from(100, XSAT)])
                .send('tony@active'),
            'eosio_assert: endrmng.xsat::unstakexsat: the number of unstakes exceeds the staking amount'
        )
    })

    it('restakexsat: [validators] does not exists', async () => {
        await expectToThrow(
            contracts.endrmng.actions.restakexsat(['tony', 'alice', 'brian', Asset.from(2, XSAT)]).send('tony@active'),
            'eosio_assert: endrmng.xsat::stakexsat: [validators] does not exists'
        )
    })

    // XSAT alice 98 => 96 tony 0 => 2
    it('restakexsat', async () => {
        await contracts.endrmng.actions.restakexsat(['tony', 'alice', 'tony', Asset.from(2, XSAT)]).send('tony@active')
        expect(get_native_staker(1)).toEqual({
            id: 1,
            quantity: '98.00000000 BTC',
            xsat_quantity: '96.00000000 XSAT',
            staker: 'tony',
            stake_debt: 1607812500,
            staking_reward_claimed: '15.75000000 XSAT',
            staking_reward_unclaimed: '0.00000000 XSAT',
            consensus_debt: 178645768,
            consensus_reward_claimed: '1.74999936 XSAT',
            consensus_reward_unclaimed: '0.00000000 XSAT',
            validator: 'alice',
        })
        expect(get_native_staker(2)).toEqual({
            id: 2,
            quantity: '2.00000000 BTC',
            xsat_quantity: '2.00000000 XSAT',
            staker: 'tony',
            stake_debt: 0,
            staking_reward_claimed: '0.00000000 XSAT',
            staking_reward_unclaimed: '0.00000000 XSAT',
            consensus_debt: 0,
            consensus_reward_claimed: '0.00000000 XSAT',
            consensus_reward_unclaimed: '0.00000000 XSAT',
            validator: 'tony',
        })

        expect(get_validator('alice')).toEqual({
            commission_rate: 3000,
            consensus_acc_per_share: 1822916,
            consensus_reward_balance: '0.00000064 XSAT',
            consensus_reward_claimed: '0.75000000 XSAT',
            consensus_reward_unclaimed: '0.00000000 XSAT',
            latest_reward_block: 840000,
            latest_reward_time: subTime(blockchain.timestamp, TimePointSec.from(1000)).toString(),
            latest_staking_time: TimePointSec.from(blockchain.timestamp).toString(),
            memo: '',
            owner: 'alice',
            quantity: '98.00000000 BTC',
            qualification: '98.00000000 BTC',
            xsat_quantity: '96.00000000 XSAT',
            reward_recipient: 'alice',
            stake_acc_per_share: 16406250,
            staking_reward_balance: '0.00000000 XSAT',
            staking_reward_claimed: '6.75000000 XSAT',
            staking_reward_unclaimed: '0.00000000 XSAT',
            disabled_staking: false,
            total_consensus_reward: '2.50000000 XSAT',
            total_staking_reward: '22.50000000 XSAT',
        })

        expect(get_validator('tony')).toEqual({
            commission_rate: 1000,
            consensus_acc_per_share: 0,
            consensus_reward_balance: '0.00000000 XSAT',
            consensus_reward_claimed: '0.00000000 XSAT',
            consensus_reward_unclaimed: '0.00000000 XSAT',
            latest_reward_block: 0,
            latest_reward_time: '1970-01-01T00:00:00',
            latest_staking_time: TimePointSec.from(blockchain.timestamp).toString(),
            memo: '78357316239040e19fC823372cC179ca75e64b81',
            owner: 'tony',
            quantity: '2.00000000 BTC',
            qualification: '2.00000000 BTC',
            xsat_quantity: '2.00000000 XSAT',
            reward_recipient: 'erc2o.xsat',
            stake_acc_per_share: 0,
            staking_reward_balance: '0.00000000 XSAT',
            staking_reward_claimed: '0.00000000 XSAT',
            staking_reward_unclaimed: '0.00000000 XSAT',
            disabled_staking: false,
            total_consensus_reward: '0.00000000 XSAT',
            total_staking_reward: '0.00000000 XSAT',
        })
    })

    it('transfer reward: only transfer [exsat.xsat/XSAT]', async () => {
        await expectToThrow(
            contracts.eos.actions
                .transfer(['rwddist.xsat', 'endrmng.xsat', '100.0000 EOS', ''])
                .send('rwddist.xsat@active'),
            'eosio_assert: endrmng.xsat: only transfer [exsat.xsat/XSAT]'
        )
    })

    // XSAT alice 96 => 98
    it('stakexsat', async () => {
        await contracts.endrmng.actions.stakexsat(['tony', 'alice', Asset.from(2, XSAT)]).send('xsatstk.xsat@active')
        expect(get_native_staker(1)).toEqual({
            id: 1,
            quantity: '98.00000000 BTC',
            xsat_quantity: '98.00000000 XSAT',
            staker: 'tony',
            stake_debt: 1607812500,
            staking_reward_claimed: '15.75000000 XSAT',
            staking_reward_unclaimed: '0.00000000 XSAT',
            consensus_debt: 178645768,
            consensus_reward_claimed: '1.74999936 XSAT',
            consensus_reward_unclaimed: '0.00000000 XSAT',
            validator: 'alice',
        })
        expect(get_validator('alice')).toEqual({
            commission_rate: 3000,
            consensus_acc_per_share: 1822916,
            consensus_reward_balance: '0.00000064 XSAT',
            consensus_reward_claimed: '0.75000000 XSAT',
            consensus_reward_unclaimed: '0.00000000 XSAT',
            latest_reward_block: 840000,
            latest_reward_time: subTime(blockchain.timestamp, TimePointSec.from(1000)).toString(),
            latest_staking_time: TimePointSec.from(blockchain.timestamp).toString(),
            memo: '',
            owner: 'alice',
            quantity: '98.00000000 BTC',
            qualification: '98.00000000 BTC',
            xsat_quantity: '98.00000000 XSAT',
            reward_recipient: 'alice',
            stake_acc_per_share: 16406250,
            staking_reward_balance: '0.00000000 XSAT',
            staking_reward_claimed: '6.75000000 XSAT',
            staking_reward_unclaimed: '0.00000000 XSAT',
            disabled_staking: false,
            total_consensus_reward: '2.50000000 XSAT',
            total_staking_reward: '22.50000000 XSAT',
        })
    })

    /// Not implemented _ashlti3
    //it('creditstake: quantity symbol must be BTC', async () => {
    //    await expectToThrow(
    //        contracts.endrmng.actions
    //            .creditstake([
    //                '78357316239040e19fC823372cC179ca75e64b81',
    //                '78357316239040e19fC823372cC179ca75e64b81',
    //                'alice',
    //                Asset.from(2, BTC),
    //            ])
    //            .send('custody.xsat@active'),
    //        'eosio_assert: endrmng.xsat::creditstake: quantity symbol must be BTC'
    //    )
    //})

    //it('creditstake: quantity must be less than or equal 100 BTC', async () => {
    //    await expectToThrow(
    //     contracts.endrmng.actions.creditstake([Asset.from(101, BTC)]).send('custody.xsat@active'),
    //        'eosio_assert: endrmng.xsat::creditstake: quantity must be less than or equal 100 BTC'
    //    )
    //})
})
