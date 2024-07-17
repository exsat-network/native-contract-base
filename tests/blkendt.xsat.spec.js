const { Asset, TimePointSec } = require('@greymass/eosio')
const { Blockchain, log, expectToThrow } = require('@proton/vert')
const { BTC, BTC_CONTRACT } = require('./src/constants')
const { getTokenBalance } = require('./src/help')

// Vert EOS VM
const blockchain = new Blockchain()
//log.setLevel('debug');
// contracts
const contracts = {
    blkendt: blockchain.createContract('blkendt.xsat', 'tests/wasm/blkendt.xsat', true),
    endrmng: blockchain.createContract('endrmng.xsat', 'tests/wasm/endrmng.xsat', true),
    utxomng: blockchain.createContract('utxomng.xsat', 'tests/wasm/utxomng.xsat', true),
    eos: blockchain.createContract('eosio.token', 'tests/wasm/btc.xsat', true),
    btc: blockchain.createContract('btc.xsat', 'tests/wasm/btc.xsat', true),
    staking: blockchain.createContract('staking.xsat', 'tests/wasm/staking.xsat', true),
    rescmng: blockchain.createContract('rescmng.xsat', 'tests/wasm/rescmng.xsat', true),
}

blockchain.createAccounts('fees.xsat', 'alice', 'amy', 'anna', 'bob', 'brian')

const get_endorsements = height => {
    return contracts.blkendt.tables.endorsements(BigInt(height)).getTableRows()
}

// one-time setup
beforeAll(async () => {
    blockchain.setTime(TimePointSec.from(new Date()))
    await contracts.utxomng.actions
        .init([
            839999,
            '0000000000000000000172014ba58d66455762add0512355ad651207918494ab',
            '0000000000000000000000000000000000000000753b8c1eaae701e1f0146360',
        ])
        .send('utxomng.xsat@active')

    // create BTC token
    await contracts.btc.actions.create(['btc.xsat', '10000000.00000000 BTC']).send('btc.xsat@active')
    await contracts.btc.actions.issue(['btc.xsat', '10000000.00000000 BTC', 'init']).send('btc.xsat@active')

    // transfer BTC to account
    await contracts.btc.actions.transfer(['btc.xsat', 'bob', '1000000.00000000 BTC', '']).send('btc.xsat@active')
    await contracts.btc.actions.transfer(['btc.xsat', 'alice', '1000000.00000000 BTC', '']).send('btc.xsat@active')
    await contracts.btc.actions.transfer(['btc.xsat', 'anna', '1000000.00000000 BTC', '']).send('btc.xsat@active')
    await contracts.btc.actions.transfer(['btc.xsat', 'brian', '1000000.00000000 BTC', '']).send('btc.xsat@active')

    // create EOS token
    await contracts.eos.actions.create(['eosio.token', '10000000000.0000 EOS']).send('eosio.token@active')
    await contracts.eos.actions.issue(['eosio.token', '10000000000.0000 EOS', 'init']).send('eosio.token@active')

    // transfer EOS to account
    await contracts.eos.actions.transfer(['eosio.token', 'alice', '100000.0000 EOS', 'init']).send('eosio.token@active')
    await contracts.eos.actions.transfer(['eosio.token', 'bob', '100000.0000 EOS', 'init']).send('eosio.token@active')

    // add staking token
    await contracts.staking.actions.addtoken([{ sym: BTC, contract: BTC_CONTRACT }]).send('staking.xsat@active')

    // deposit fees
    await contracts.btc.actions.transfer(['bob', 'rescmng.xsat', '100.00000000 BTC', 'bob']).send('bob@active')
    await contracts.btc.actions.transfer(['alice', 'rescmng.xsat', '100.00000000 BTC', 'alice']).send('alice@active')
    await contracts.btc.actions.transfer(['anna', 'rescmng.xsat', '100.00000000 BTC', 'anna']).send('anna@active')
    await contracts.btc.actions.transfer(['brian', 'rescmng.xsat', '100.00000000 BTC', 'brian']).send('brian@active')

    // register validator
    await contracts.endrmng.actions.regvalidator(['alice', 'alice', 2000]).send('alice@active')
    await contracts.endrmng.actions.regvalidator(['bob', 'bob', 2000]).send('bob@active')
    await contracts.endrmng.actions.regvalidator(['anna', 'anna', 2000]).send('anna@active')
    await contracts.endrmng.actions.regvalidator(['brian', 'brian', 2000]).send('brian@active')

    // init
    await contracts.rescmng.actions
        .init({
            cost_per_slot: '0.00000001 BTC',
            cost_per_endorsement: '0.00000004 BTC',
            cost_per_parse: '0.00000005 BTC',
            cost_per_upload: '0.00000002 BTC',
            cost_per_verification: '0.00000003 BTC',
            fee_account: 'fees.xsat',
        })
        .send('rescmng.xsat@active')
})

describe('blkendt.xsat', () => {
    it('endorse: missing required authority', async () => {
        await expectToThrow(
            contracts.blkendt.actions
                .endorse(['bob', 840000, '0000000000000000000320283a032748cef8227873ff4872689bf23f1cda83a5'])
                .send('alice@active'),
            'missing required authority bob'
        )
    })

    it('endorse: height must be greater than 840000', async () => {
        await expectToThrow(
            contracts.blkendt.actions
                .endorse(['bob', 839999, '0000000000000000000320283a032748cef8227873ff4872689bf23f1cda83a5'])
                .send('bob@active'),
            'eosio_assert: block_endorse::endorse: the block has been parsed and does not need to be endorsed'
        )
    })

    it('endorse: no validator with more than 100 BTC pledged was found', async () => {
        await expectToThrow(
            contracts.blkendt.actions
                .endorse(['amy', 840000, '0000000000000000000320283a032748cef8227873ff4872689bf23f1cda83a5'])
                .send('amy@active'),
            'eosio_assert: block_endorse::endorse: no validators found with staking amounts exceeding 100 BTC'
        )
    })

    it('endorse: the validator has less than 100 BTC staked.', async () => {
        // staking
        await contracts.btc.actions
            .transfer(['alice', 'staking.xsat', Asset.from(100, BTC), 'alice'])
            .send('alice@active')
        await contracts.btc.actions.transfer(['bob', 'staking.xsat', Asset.from(200, BTC), 'bob']).send('bob@active')
        await contracts.btc.actions
            .transfer(['brian', 'staking.xsat', Asset.from(300, BTC), 'brian'])
            .send('brian@active')

        const height = 840000
        const hash = '0000000000000000000320283a032748cef8227873ff4872689bf23f1cda83a5'
        expect(get_endorsements(height)).toEqual([])

        await expectToThrow(
            contracts.blkendt.actions.endorse(['amy', height, hash]).send('amy@active'),
            'eosio_assert: block_endorse::endorse: the validator has less than 100 BTC staked.'
        )
    })

    it('endorse', async () => {
        const height = 840000
        const hash = '0000000000000000000320283a032748cef8227873ff4872689bf23f1cda83a5'
        await contracts.blkendt.actions.endorse(['alice', height, hash]).send('alice@active')
        expect(get_endorsements(height)).toEqual([
            {
                id: 0,
                hash: '0000000000000000000320283a032748cef8227873ff4872689bf23f1cda83a5',
                requested_validators: [
                    {
                        account: 'bob',
                        staking: '20000000000',
                    },
                    {
                        account: 'brian',
                        staking: '30000000000',
                    },
                ],
                provider_validators: [
                    {
                        account: 'alice',
                        staking: '10000000000',
                    },
                ],
            },
        ])
        await contracts.blkendt.actions.endorse(['bob', height, hash]).send('bob@active')
        expect(get_endorsements(height)).toEqual([
            {
                id: 0,
                hash: '0000000000000000000320283a032748cef8227873ff4872689bf23f1cda83a5',
                requested_validators: [
                    {
                        account: 'brian',
                        staking: '30000000000',
                    },
                ],
                provider_validators: [
                    {
                        account: 'alice',
                        staking: '10000000000',
                    },
                    {
                        account: 'bob',
                        staking: '20000000000',
                    },
                ],
            },
        ])
        await contracts.blkendt.actions.endorse(['brian', height, hash]).send('brian@active')
        expect(get_endorsements(height)).toEqual([
            {
                id: 0,
                hash: '0000000000000000000320283a032748cef8227873ff4872689bf23f1cda83a5',
                requested_validators: [],
                provider_validators: [
                    {
                        account: 'alice',
                        staking: '10000000000',
                    },
                    {
                        account: 'bob',
                        staking: '20000000000',
                    },
                    {
                        account: 'brian',
                        staking: '30000000000',
                    },
                ],
            },
        ])
    })

    it('endorse: validator is on the list of provider validators', async () => {
        const height = 840000
        const hash = '0000000000000000000320283a032748cef8227873ff4872689bf23f1cda83a5'
        await expectToThrow(
            contracts.blkendt.actions.endorse(['brian', height, hash]).send('brian@active'),
            'eosio_assert: block_endorse::endorse: validator is on the list of provider validators'
        )
    })
})
