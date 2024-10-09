const { Asset, TimePointSec } = require('@greymass/eosio')
const { Blockchain, log, expectToThrow } = require('@proton/vert')
const { BTC, BTC_CONTRACT, XSAT } = require('./src/constants')
const { addTime } = require('./src/help')

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
    xsat: blockchain.createContract('exsat.xsat', 'tests/wasm/exsat.xsat', true),
    xsatstk: blockchain.createContract('xsatstk.xsat', 'tests/wasm/xsatstk.xsat', true),
}

blockchain.createAccounts('fees.xsat', 'alice', 'amy', 'anna', 'bob', 'brian')

const get_endorsements = height => {
    return contracts.blkendt.tables.endorsements(BigInt(height)).getTableRows()
}

const get_config = () => {
    return contracts.blkendt.tables.config().getTableRows()[0]
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

    // create XSAT token
    await contracts.xsat.actions.create(['exsat.xsat', '10000000.00000000 XSAT']).send('exsat.xsat@active')
    await contracts.xsat.actions.issue(['exsat.xsat', '10000000.00000000 XSAT', 'init']).send('exsat.xsat@active')

    // transfer XSAT to account
    await contracts.xsat.actions.transfer(['exsat.xsat', 'bob', '1000000.00000000 XSAT', '']).send('exsat.xsat@active')
    await contracts.xsat.actions
        .transfer(['exsat.xsat', 'alice', '1000000.00000000 XSAT', ''])
        .send('exsat.xsat@active')
    await contracts.xsat.actions.transfer(['exsat.xsat', 'anna', '1000000.00000000 XSAT', '']).send('exsat.xsat@active')
    await contracts.xsat.actions
        .transfer(['exsat.xsat', 'brian', '1000000.00000000 XSAT', ''])
        .send('exsat.xsat@active')

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

    // init xsatstk.xsat
    await contracts.xsatstk.actions.setstatus([false]).send('xsatstk.xsat@active')

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
    it('config: missing required authority', async () => {
        await expectToThrow(
            contracts.blkendt.actions.config([0, 0, 15, 860000, 0, '21000.00000000 XSAT']).send('alice@active'),
            'missing required authority blkendt.xsat'
        )
    })

    it('config: min_validators must be greater than 0', async () => {
        await expectToThrow(
            contracts.blkendt.actions.config([0, 0, 0, 860000, 0, '21000.00000000 XSAT']).send('blkendt.xsat@active'),
            'eosio_assert: blkendt.xsat::config: min_validators must be greater than 0'
        )
    })

    it('config: min_xsat_qualification symbol must be XSAT', async () => {
        await expectToThrow(
            contracts.blkendt.actions.config([0, 0, 2, 860000, 0, '0.00000000 BTC']).send('blkendt.xsat@active'),
            'eosio_assert: blkendt.xsat::config: min_xsat_qualification symbol must be XSAT'
        )
    })

    it('config: min_xsat_qualification must be greater than 0BTC and less than 21000000XSAT', async () => {
        await expectToThrow(
            contracts.blkendt.actions.config([0, 0, 2, 860000, 0, '0.00000000 XSAT']).send('blkendt.xsat@active'),
            'eosio_assert: blkendt.xsat::config: min_xsat_qualification must be greater than 0BTC and less than 21000000XSAT'
        )
        await expectToThrow(
            contracts.blkendt.actions
                .config([0, 0, 2, 860000, 0, '21000000.00000001 XSAT'])
                .send('blkendt.xsat@active'),
            'eosio_assert: blkendt.xsat::config: min_xsat_qualification must be greater than 0BTC and less than 21000000XSAT'
        )
    })

    it('config', async () => {
        await contracts.blkendt.actions.config([1, 1, 5, 850000, 1, '2.00000000 XSAT']).send('blkendt.xsat@active')
        expect(get_config()).toEqual({
            limit_endorse_height: 1,
            limit_num_endorsed_blocks: 1,
            min_validators: 5,
            xsat_stake_activation_height: 850000,
            consensus_interval_seconds: 1,
            min_xsat_qualification: '2.00000000 XSAT',
        })
        await contracts.blkendt.actions.config([0, 0, 10, 860000, 0, '21000.00000000 XSAT']).send('blkendt.xsat@active')
        expect(get_config()).toEqual({
            limit_endorse_height: 0,
            limit_num_endorsed_blocks: 0,
            min_validators: 10,
            xsat_stake_activation_height: 860000,
            consensus_interval_seconds: 0,
            min_xsat_qualification: '21000.00000000 XSAT',
        })
    })

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
            'eosio_assert: 1002:blkendt.xsat::endorse: the current block is irreversible and does not need to be endorsed'
        )
    })

    it('endorse: the number of valid validators must be greater than or equal to 10', async () => {
        await expectToThrow(
            contracts.blkendt.actions
                .endorse(['amy', 840000, '0000000000000000000320283a032748cef8227873ff4872689bf23f1cda83a5'])
                .send('amy@active'),
            'eosio_assert_message: 1004:blkendt.xsat::endorse: the number of valid validators must be greater than or equal to 10'
        )
    })

    it('config', async () => {
        await contracts.blkendt.actions.config([0, 0, 2, 860000, 0, '21000.00000000 XSAT']).send('blkendt.xsat@active')
    })

    it('endorse: the validator has less than 100 BTC staked', async () => {
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
            'eosio_assert_message: 1005:blkendt.xsat::endorse: the validator has less than 100 BTC staked'
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
                        created_at: TimePointSec.from(blockchain.timestamp).toString(),
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
                        created_at: TimePointSec.from(blockchain.timestamp).toString(),
                    },
                    {
                        account: 'bob',
                        staking: '20000000000',
                        created_at: TimePointSec.from(blockchain.timestamp).toString(),
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
                        created_at: TimePointSec.from(blockchain.timestamp).toString(),
                    },
                    {
                        account: 'bob',
                        staking: '20000000000',
                        created_at: TimePointSec.from(blockchain.timestamp).toString(),
                    },
                    {
                        account: 'brian',
                        staking: '30000000000',
                        created_at: TimePointSec.from(blockchain.timestamp).toString(),
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
            'eosio_assert: 1006:blkendt.xsat::endorse: validator is on the list of provider validators'
        )
    })

    it('config', async () => {
        await contracts.blkendt.actions.config([1, 0, 2, 860000, 0, '21000.00000000 XSAT']).send('blkendt.xsat@active')
        expect(get_config()).toEqual({
            limit_endorse_height: 1,
            limit_num_endorsed_blocks: 0,
            min_validators: 2,
            xsat_stake_activation_height: 860000,
            consensus_interval_seconds: 0,
            min_xsat_qualification: '21000.00000000 XSAT',
        })
    })

    it('endorse: the current endorsement status is disabled', async () => {
        const height = 840000
        const hash = '0000000000000000000320283a032748cef8227873ff4872689bf23f1cda83a5'
        await expectToThrow(
            contracts.blkendt.actions.endorse(['brian', height, hash]).send('brian@active'),
            'eosio_assert: 1001:blkendt.xsat::endorse: the current endorsement status is disabled'
        )
    })

    it('config', async () => {
        await contracts.blkendt.actions.config([0, 1, 2, 860000, 0, '21000.00000000 XSAT']).send('blkendt.xsat@active')
        expect(get_config()).toEqual({
            limit_endorse_height: 0,
            limit_num_endorsed_blocks: 1,
            min_validators: 2,
            xsat_stake_activation_height: 860000,
            consensus_interval_seconds: 0,
            min_xsat_qualification: '21000.00000000 XSAT',
        })
    })

    it('endorse: the endorsement height cannot exceed', async () => {
        const height = 840001
        const hash = '00000000000000000001b48a75d5a3077913f3f441eb7e08c13c43f768db2463'
        await expectToThrow(
            contracts.blkendt.actions.endorse(['brian', height, hash]).send('brian@active'),
            'eosio_assert_message: 1003:blkendt.xsat::endorse: the endorsement height cannot exceed height 840000'
        )
    })

    it('endorse: the next endorsement time has not yet been reached', async () => {
        await contracts.blkendt.actions
            .config([0, 50000, 2, 860000, 10, '21000.00000000 XSAT'])
            .send('blkendt.xsat@active')
        const height = 860000
        const hash = '00000000000000000001b48a75d5a3077913f3f441eb7e08c13c43f768db2463'
        await expectToThrow(
            contracts.blkendt.actions.endorse(['brian', height, hash]).send('brian@active'),
            'eosio_assert: 1008:blkendt.xsat::endorse: the next endorsement time has not yet been reached'
        )
    })
    it('add consensus block 859999', async () => {
        await contracts.utxomng.actions
            .addconsesblk({
                bucket_id: 859999,
                height: 859999,
                hash: '0000000000000000000196a7111c7aa3368af5a803a81c7bfb32860100dfd9c7',
                cumulative_work: '00000000000000000000000000000000000000008cd4a7e44d9bf664c7f73f60',
                version: 539074560,
                timestamp: 1725538632,
                merkle: '5badee0f7f69d8ff278574e82c8bdc651f3524e1dd753c92997a009f1bbaeaf0',
                previous_block_hash: '00000000000000000000de531aab72aa763d8f1dea92efdb47c96ea5d209b8ef',
                nonce: 1941400253,
                bits: 386082139,
                synchronizer: "",
                miner: "",
                created_at: blockchain.timestamp.toString(),
            })
            .send('utxomng.xsat@active')
    })

    it('endorse: the next endorsement time has not yet been reached', async () => {
        const height = 860000
        const hash = '00000000000000000001b48a75d5a3077913f3f441eb7e08c13c43f768db2463'
        await expectToThrow(
            contracts.blkendt.actions.endorse(['brian', height, hash]).send('brian@active'),
            'eosio_assert_message: 1008:blkendt.xsat::endorse: the next endorsement time has not yet been reached ' + addTime(blockchain.timestamp, TimePointSec.from(10))
        )
    })

    it('endorse: the number of valid validators must be greater than or equal to 2', async () => {
        blockchain.addTime(TimePointSec.from(10))
        const height = 860000
        const hash = '00000000000000000001b48a75d5a3077913f3f441eb7e08c13c43f768db2463'
        await expectToThrow(
            contracts.blkendt.actions.endorse(['brian', height, hash]).send('brian@active'),
            'eosio_assert_message: 1004:blkendt.xsat::endorse: the number of valid validators must be greater than or equal to 2'
        )
    })

    it('endorse', async () => {
        // staking
        await contracts.btc.actions.transfer(['anna', 'staking.xsat', Asset.from(10, BTC), 'anna']).send('anna@active')
        await contracts.xsat.actions
            .transfer(['anna', 'xsatstk.xsat', Asset.from(21000, XSAT), 'anna'])
            .send('anna@active')
        await contracts.xsat.actions
            .transfer(['alice', 'xsatstk.xsat', Asset.from(21000, XSAT), 'alice'])
            .send('alice@active')

        const height = 860000
        const hash = '00000000000000000001b48a75d5a3077913f3f441eb7e08c13c43f768db2463'
        await contracts.blkendt.actions.endorse(['anna', height, hash]).send('anna@active')

        expect(get_endorsements(height)).toEqual([
            {
                id: 0,
                hash: '00000000000000000001b48a75d5a3077913f3f441eb7e08c13c43f768db2463',
                requested_validators: [
                    {
                        account: 'alice',
                        staking: Asset.from(100, BTC).units.toString(),
                    },
                ],
                provider_validators: [
                    {
                        account: 'anna',
                        staking: Asset.from(10, BTC).units.toNumber(),
                        created_at: TimePointSec.from(blockchain.timestamp).toString(),
                    },
                ],
            },
        ])
    })

    it('add consensus block 860000', async () => {
        await contracts.utxomng.actions
            .addconsesblk({
                bucket_id: 860000,
                height: 860000,
                hash: '0000000000000000000095dd5c0c8e176a6498eb335c491b96df1a1ae178bfbd',
                cumulative_work: '00000000000000000000000000000000000000008cd4f9445dc7ed9b84b84022',
                version: 618168320,
                timestamp: 1725539528,
                merkle: '97f0f4c20752b9dc338dd671137041513a3053b0116db66385d8bee8e597571a',
                previous_block_hash: '0000000000000000000196a7111c7aa3368af5a803a81c7bfb32860100dfd9c7',
                nonce: 3812343282,
                bits: 386082139,
                synchronizer: "",
                miner: "",
                created_at: blockchain.timestamp.toString(),
            })
            .send('utxomng.xsat@active')
    })


    it('endorse: the number of valid validators must be greater than or equal to 2', async () => {
        await contracts.blkendt.actions
            .config([0, 50000, 2, 860000, 0, '21001.00000000 XSAT'])
            .send('blkendt.xsat@active')
        const height = 860001
        const hash = '00000000000000000001b48a75d5a3077913f3f441eb7e08c13c43f768db2463'
        await expectToThrow(
            contracts.blkendt.actions.endorse(['brian', height, hash]).send('brian@active'),
            'eosio_assert_message: 1004:blkendt.xsat::endorse: the number of valid validators must be greater than or equal to 2'
        )
    })

    it('endorse: the validator has less than 21000.00000000 XSAT staked', async () => {
        await contracts.blkendt.actions
            .config([0, 50000, 2, 860000, 0, '21000.00000000 XSAT'])
            .send('blkendt.xsat@active')
        const height = 860001
        const hash = '00000000000000000001b48a75d5a3077913f3f441eb7e08c13c43f768db2463'
        await expectToThrow(
            contracts.blkendt.actions.endorse(['brian', height, hash]).send('brian@active'),
            'eosio_assert_message: 1005:blkendt.xsat::endorse: the validator has less than 21000.00000000 XSAT staked'
        )
    })
})
