const { UInt128, Name, Asset, TimePointSec } = require('@greymass/eosio')
const { Blockchain, log, expectToThrow } = require('@proton/vert')
const { getTokenBalance } = require('./src/help')
const { RESCMNG_CONTRACT, BTC_CONTRACT, BTC } = require('./src/constants')

// Vert EOS VM
const blockchain = new Blockchain()
//log.setLevel('debug');
// contracts
const contracts = {
    rescmng: blockchain.createContract('rescmng.xsat', 'tests/wasm/rescmng.xsat', true),
    eos: blockchain.createContract('eosio.token', 'tests/wasm/btc.xsat', true),
    btc: blockchain.createContract('btc.xsat', 'tests/wasm/btc.xsat', true),
}

blockchain.createAccounts('alice', 'bob', 'fees.xsat', 'poolreg.xsat', 'blksync.xsat', 'blkendt.xsat', 'utxomng.xsat')

const get_account = account => {
    let key = Name.from(account).value.value
    return contracts.rescmng.tables.accounts().getTableRow(key)
}

const get_config = () => {
    return contracts.rescmng.tables.config().getTableRows()[0]
}

// one-time setup
beforeAll(async () => {
    blockchain.setTime(TimePointSec.from(new Date()))

    // create BTC token
    await contracts.btc.actions.create(['btc.xsat', '10000000.00000000 BTC']).send('btc.xsat@active')
    await contracts.btc.actions.issue(['btc.xsat', '10000000.00000000 BTC', 'init']).send('btc.xsat@active')

    // transfer BTC to account
    await contracts.btc.actions.transfer(['btc.xsat', 'bob', '1000000.00000000 BTC', '']).send('btc.xsat@active')
    await contracts.btc.actions.transfer(['btc.xsat', 'alice', '1000000.00000000 BTC', '']).send('btc.xsat@active')

    // create EOS token
    await contracts.eos.actions.create(['eosio.token', '10000000000.0000 EOS']).send('eosio.token@active')
    await contracts.eos.actions.issue(['eosio.token', '10000000000.0000 EOS', 'init']).send('eosio.token@active')

    // transfer EOS to account
    await contracts.eos.actions.transfer(['eosio.token', 'alice', '100000.0000 EOS', 'init']).send('eosio.token@active')
    await contracts.eos.actions.transfer(['eosio.token', 'bob', '100000.0000 EOS', 'init']).send('eosio.token@active')
})

describe('rescmng.xsat', () => {
    it('init: missing required authority', async () => {
        await expectToThrow(
            contracts.rescmng.actions
                .init([
                    'fees.xsat',
                    Asset.fromUnits(5, BTC),
                    Asset.fromUnits(2, BTC),
                    Asset.fromUnits(1, BTC),
                    Asset.fromUnits(3, BTC),
                    Asset.fromUnits(3, BTC),
                ])
                .send('bob@active'),
            'missing required authority rescmng.xsat'
        )
    })

    it('init: fee_account does not exists', async () => {
        await expectToThrow(
            contracts.rescmng.actions
                .init([
                    'anna',
                    Asset.fromUnits(1, BTC),
                    Asset.fromUnits(2, BTC),
                    Asset.fromUnits(3, BTC),
                    Asset.fromUnits(4, BTC),
                    Asset.fromUnits(5, BTC),
                ])
                .send('rescmng.xsat@active'),
            'eosio_assert: rescmng.xsat::init: fee_account does not exists'
        )
    })

    it('init', async () => {
        const config = {
            cost_per_slot: '0.00000001 BTC',
            cost_per_endorsement: '0.00000004 BTC',
            cost_per_parse: '0.00000005 BTC',
            cost_per_upload: '0.00000002 BTC',
            cost_per_verification: '0.00000003 BTC',
            fee_account: 'fees.xsat',
            disabled_withdraw: false,
        }
        await contracts.rescmng.actions.init(config).send('rescmng.xsat@active')
        expect(get_config()).toEqual(config)
    })

    it('only transfer [btc.xsat/BTC]', async () => {
        await expectToThrow(
            contracts.eos.actions.transfer(['bob', 'rescmng.xsat', '1.0000 EOS', '']).send('bob@active'),
            'eosio_assert: rescmng.xsat: only transfer [btc.xsat/BTC]'
        )
    })

    it('no balance to withdraw', async () => {
        await expectToThrow(
            contracts.rescmng.actions.withdraw(['bob', Asset.from(1, BTC)]).send('bob@active'),
            'eosio_assert: rescmng.xsat::withdraw: no balance to withdraw'
        )
    })

    it('invalid memo', async () => {
        await expectToThrow(
            contracts.btc.actions.transfer(['bob', 'rescmng.xsat', '1000.00000000 BTC', '']).send('bob@active'),
            'eosio_assert: rescmng.xsat: invalid memo, ex: "<receiver>"'
        )
    })

    it('deposit BTC', async () => {
        await contracts.btc.actions.transfer(['bob', 'rescmng.xsat', '1000.00000000 BTC', 'bob']).send('bob@active')
        expect(get_account('bob')).toEqual({
            balance: '1000.00000000 BTC',
            owner: 'bob',
        })
    })

    it('setstatus: missing required authority', async () => {
        await expectToThrow(
            contracts.rescmng.actions.setstatus([true]).send('bob@active'),
            'missing required authority rescmng.xsat'
        )
    })

    it('setstatus', async () => {
        await contracts.rescmng.actions.setstatus([true]).send('rescmng.xsat@active')
        expect(get_config().disabled_withdraw).toEqual(true)
    })

    it('withdraw: missing required authority', async () => {
        await expectToThrow(
            contracts.rescmng.actions.withdraw(['bob', Asset.from(1, BTC)]).send('alice@active'),
            'missing required authority bob'
        )
    })

    it('withdraw: withdraw balance status is disabled', async () => {
        await expectToThrow(
            contracts.rescmng.actions.withdraw(['bob', Asset.from(1, BTC)]).send('bob@active'),
            'eosio_assert: rescmng.xsat::withdraw: withdraw balance status is disabled'
        )
    })

    it('withdraw', async () => {
        await contracts.rescmng.actions.setstatus({disabled_withdraw: false}).send('rescmng.xsat@active')

        const before_balance = getTokenBalance(blockchain, 'bob', 'btc.xsat', 'BTC')
        await contracts.rescmng.actions.withdraw(['bob', Asset.from(1000, BTC)]).send('bob@active')
        const after_balance = getTokenBalance(blockchain, 'bob', 'btc.xsat', 'BTC')
        expect(after_balance - before_balance).toEqual(Asset.from(1000, BTC).units.toNumber())
    })

    it('pay: missing required authority', async () => {
        await expectToThrow(
            contracts.rescmng.actions.pay([840000, '0000000000000000000320283a032748cef8227873ff4872689bf23f1cda83a5', 'bob', 1, 1]).send('alice@active'),
            'eosio_assert: 3001:rescmng.xsat::pay: missing auth [blksync.xsat/blkendt.xsat/utxmng.xsat/poolreg.xsat]'
        )
    })

    it('pay: insufficient balance', async () => {
        await expectToThrow(
            contracts.rescmng.actions.pay([840000, '0000000000000000000320283a032748cef8227873ff4872689bf23f1cda83a5','bob', 1, 1]).send('poolreg.xsat@active'),
            'eosio_assert: 3003:rescmng.xsat::pay: insufficient balance'
        )
    })

    it('pay', async () => {
        // recharge
        await contracts.btc.actions.transfer(['bob', 'rescmng.xsat', '1.00000000 BTC', 'bob']).send('bob@active')
        // pay
        const config = get_config()
        const payers = [
            { type: 1, caller: 'poolreg.xsat', base_fees: Asset.from(config.cost_per_slot).units },
            { type: 2, caller: 'blksync.xsat', base_fees: Asset.from(config.cost_per_upload).units },
            { type: 3, caller: 'blksync.xsat', base_fees: Asset.from(config.cost_per_verification).units },
            { type: 4, caller: 'blkendt.xsat', base_fees: Asset.from(config.cost_per_endorsement).units },
            { type: 5, caller: 'utxomng.xsat', base_fees: Asset.from(config.cost_per_parse).units },
        ]
        for (const payer of payers) {
            const nums = Math.floor(Math.random() * 100) + 1
            const type = payer.type
            let fees = payer.base_fees.multiplying(nums).toNumber()
            const before_rescmng_balance = getTokenBalance(blockchain, RESCMNG_CONTRACT, BTC_CONTRACT, BTC.code)
            const before_fees_balance = getTokenBalance(blockchain, 'fees.xsat', BTC_CONTRACT, BTC.code)
            await contracts.rescmng.actions.pay([840000, '0000000000000000000320283a032748cef8227873ff4872689bf23f1cda83a5','bob', type, nums]).send(`${payer.caller}@active`)
            const after_rescmng_balance = getTokenBalance(blockchain, RESCMNG_CONTRACT, BTC_CONTRACT, BTC.code)
            const after_fees_balance = getTokenBalance(blockchain, 'fees.xsat', BTC_CONTRACT, BTC.code)
            expect(before_rescmng_balance - after_rescmng_balance).toEqual(fees)
            expect(after_fees_balance - before_fees_balance).toEqual(fees)
        }
    })
})
