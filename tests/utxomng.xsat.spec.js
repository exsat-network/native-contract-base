const { TimePointSec } = require('@greymass/eosio')
const { Blockchain, expectToThrow } = require('@proton/vert')
const { BTC, BTC_CONTRACT } = require('./src/constants')
const fs = require('fs')
const path = require('path')
const { addTime, decodeReturn_verify, max_chunk_size } = require('./src/help')

// Vert EOS VM
const blockchain = new Blockchain()
//log.setLevel('debug');
// contracts
const contracts = {
    blksync: blockchain.createContract('blksync.xsat', 'tests/wasm/blksync.xsat', true),
    poolreg: blockchain.createContract('poolreg.xsat', 'tests/wasm/poolreg.xsat', true),
    rescmng: blockchain.createContract('rescmng.xsat', 'tests/wasm/rescmng.xsat', true),
    utxomng: blockchain.createContract('utxomng.xsat', 'tests/wasm/utxomng.xsat', true),
    blkendt: blockchain.createContract('blkendt.xsat', 'tests/wasm/blkendt.xsat', true),
    staking: blockchain.createContract('staking.xsat', 'tests/wasm/staking.xsat', true),
    endrmng: blockchain.createContract('endrmng.xsat', 'tests/wasm/endrmng.xsat', true),
    btc: blockchain.createContract('btc.xsat', 'tests/wasm/btc.xsat', true),
    exsat: blockchain.createContract('exsat.xsat', 'tests/wasm/exsat.xsat', true),
    rwddist: blockchain.createContract('rwddist.xsat', 'tests/wasm/rwddist.xsat', true),
}

// accounts
blockchain.createAccounts('fees.xsat', 'alice', 'amy', 'anna', 'bob', 'brian')

const read_block = height => fs.readFileSync(path.join(__dirname, `data/mainnet-${height}.json`)).toString('utf8')

const get_utxo = id => {
    return contracts.utxomng.tables.utxos().getTableRow(BigInt(id))
}

const get_chain_state = () => {
    return contracts.utxomng.tables.chainstate().getTableRows()[0]
}

const get_consensus_block = bucket_id => {
    if (bucket_id) {
        return contracts.utxomng.tables.consensusblk().getTableRow(BigInt(bucket_id))
    } else {
        return contracts.utxomng.tables.consensusblk().getTableRows()
    }
}

const get_block = height => {
    return contracts.utxomng.tables.blocks().getTableRow(BigInt(height))
}

const get_config = () => {
    return contracts.utxomng.tables.config().getTableRows()[0]
}

const pushUpload = async (sender, height, hash, block) => {
    const chunks = []
    let next_offset = 0
    while (next_offset < block.length) {
        const end = Math.min(next_offset + max_chunk_size, block.length)
        const chunk = block.substring(next_offset, end)
        chunks.push(chunk)
        next_offset += max_chunk_size
    }

    for (let i = 0; i < chunks.length; i++) {
        await pushChunk(sender, height, hash, i, chunks[i])
    }
}

const pushChunk = (sender, height, hash, chunk_id, chunk) => {
    return contracts.blksync.actions.pushchunk([sender, height, hash, chunk_id, chunk]).send(`${sender}@active`)
}

const get_nonce = () => new Date().getTime()

// one-time setup
beforeAll(async () => {
    blockchain.setTime(TimePointSec.from(new Date()))

    // create XSAT token
    await contracts.exsat.actions.create(['rwddist.xsat', '10000000.00000000 XSAT']).send('exsat.xsat@active')
    //await contracts.exsat.actions.issue(['rwddist.xsat', '10000000.00000000 XSAT', 'init']).send('rwddist.xsat@active')

    // create BTC token
    await contracts.btc.actions.create(['btc.xsat', '10000000.00000000 BTC']).send('btc.xsat@active')
    await contracts.btc.actions.issue(['btc.xsat', '10000000.00000000 BTC', 'init']).send('btc.xsat@active')

    // transfer BTC to account
    await contracts.btc.actions.transfer(['btc.xsat', 'bob', '1000000.00000000 BTC', '']).send('btc.xsat@active')
    await contracts.btc.actions.transfer(['btc.xsat', 'alice', '1000000.00000000 BTC', '']).send('btc.xsat@active')
    await contracts.btc.actions.transfer(['btc.xsat', 'amy', '1000000.00000000 BTC', '']).send('btc.xsat@active')
    await contracts.btc.actions.transfer(['btc.xsat', 'anna', '1000000.00000000 BTC', '']).send('btc.xsat@active')
    await contracts.btc.actions.transfer(['btc.xsat', 'brian', '1000000.00000000 BTC', '']).send('btc.xsat@active')

    // deposit fees
    await contracts.btc.actions.transfer(['alice', 'rescmng.xsat', '1000.00000000 BTC', 'alice']).send('alice@active')
    await contracts.btc.actions.transfer(['bob', 'rescmng.xsat', '1000.00000000 BTC', 'bob']).send('bob@active')
    await contracts.btc.actions.transfer(['amy', 'rescmng.xsat', '1000.00000000 BTC', 'amy']).send('amy@active')
    await contracts.btc.actions.transfer(['brian', 'rescmng.xsat', '1000.00000000 BTC', 'brian']).send('brian@active')
    await contracts.btc.actions.transfer(['anna', 'rescmng.xsat', '1000.00000000 BTC', 'anna']).send('anna@active')

    // add staking token
    await contracts.staking.actions.addtoken([{ sym: BTC, contract: BTC_CONTRACT }]).send('staking.xsat@active')

    // register synchronizer
    await contracts.poolreg.actions
        .initpool([
            'alice',
            839997,
            'alice',
            ['37jKPSmbEGwgfacCr2nayn1wTaqMAbA94Z', '39C7fxSzEACPjM78Z7xdPxhf7mKxJwvfMJ'],
        ])
        .send('poolreg.xsat@active')

    await contracts.poolreg.actions
        .initpool([
            'bob',
            839999,
            'bob',
            ['bc1qte0s6pz7gsdlqq2cf6hv5mxcfksykyyyjkdfd5', '18cBEMRxXHqzWWCxZNtU91F5sbUNKhL5PX'],
        ])
        .send('poolreg.xsat@active')

    // register validator
    await contracts.endrmng.actions.regvalidator(['alice', 'alice', 2000]).send('alice@active')
    await contracts.endrmng.actions.regvalidator(['bob', 'bob', 2000]).send('bob@active')
    await contracts.endrmng.actions.regvalidator(['amy', 'amy', 2000]).send('amy@active')
    await contracts.endrmng.actions.regvalidator(['anna', 'anna', 2000]).send('anna@active')
    await contracts.endrmng.actions.regvalidator(['brian', 'brian', 2000]).send('brian@active')

    // init
    await contracts.blkendt.actions.config([0, 0, 2, 860000, 0, "21000.00000000 XSAT"]).send('blkendt.xsat@active')

    // staking
    await contracts.btc.actions.transfer(['alice', 'staking.xsat', '100.00000000 BTC', 'alice']).send('alice@active')
    await contracts.btc.actions.transfer(['bob', 'staking.xsat', '200.00000000 BTC', 'bob']).send('bob@active')
    await contracts.btc.actions.transfer(['amy', 'staking.xsat', '300.00000000 BTC', 'amy']).send('amy@active')
    await contracts.btc.actions.transfer(['anna', 'staking.xsat', '700.00000000 BTC', 'anna']).send('anna@active')
    await contracts.btc.actions.transfer(['brian', 'staking.xsat', '900.00000000 BTC', 'brian']).send('brian@active')

    await contracts.utxomng.actions
        .addblock({
            height: 839999,
            hash: '0000000000000000000172014ba58d66455762add0512355ad651207918494ab',
            cumulative_work: '0000000000000000000000000000000000000000753b8c1eaae701e1f0146360',
            version: 671088644,
            previous_block_hash: '00000000000000000001dcce6ce7c8a45872cafd1fb04732b447a14a91832591',
            merkle: '5cdb277afa34ea35aa620e5cad205f18acda80b80dec9dacf4b84636a5ad0448',
            timestamp: 1713571533,
            bits: 386089497,
            nonce: 3205594798,
        })
        .send('utxomng.xsat@active')

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

describe('utxomng.xsat', () => {
    it('unspendable', async () => {
        await contracts.utxomng.actions
            .unspendable([840327, '6a206d1988a6cdd64bf8103095f4fea8bbcb45efb2143196c4fac4a32e85df10cc43'])
            .send('utxomng.xsat@active')
        expect(blockchain.actionTraces[0].returnValue[0]).toEqual(1)

        await contracts.utxomng.actions
            .unspendable([
                840673,
                'fabe6d6d553d6eb4ada3a4795b3da6bab58e55b7bfa362e40fb9350d4fa781057f9512d40100000000000000',
            ])
            .send('utxomng.xsat@active')
        expect(blockchain.actionTraces[0].returnValue[0]).toEqual(0)
    })

    it('addutxo: missing required authority', async () => {
        await expectToThrow(
            contracts.utxomng.actions
                .addutxo([
                    1,
                    'a0db149ace545beabbd87a8d6b20ffd6aa3b5a50e58add49a3d435f898c272cf',
                    1,
                    '76a914536ffa992491508dca0354e52f32a3a7a679a53a88ac',
                    4075061499,
                ])
                .send('alice@active'),
            'missing required authority utxomng.xsat'
        )
    })

    it('addutxo', async () => {
        const utxo = {
            id: 1,
            index: 1,
            scriptpubkey: '76a914536ffa992491508dca0354e52f32a3a7a679a53a88ac',
            txid: 'a0db149ace545beabbd87a8d6b20ffd6aa3b5a50e58add49a3d435f898c272cf',
            value: 4075061499,
        }
        await contracts.utxomng.actions.addutxo(utxo).send('utxomng.xsat@active')
        expect(get_utxo(1)).toEqual(utxo)
        expect(get_chain_state().num_utxos).toEqual(1)
        // update
        await contracts.utxomng.actions.addutxo(utxo).send('utxomng.xsat@active')
        expect(get_utxo(1)).toEqual(utxo)
        expect(get_chain_state().num_utxos).toEqual(1)
    })

    it('delutxo: missing required authority utxomng.xsat', async () => {
        await expectToThrow(
            contracts.utxomng.actions.delutxo([1]).send('alice@active'),
            'missing required authority utxomng.xsat'
        )
    })

    it('delutxo: [utxos] does not exist', async () => {
        await expectToThrow(
            contracts.utxomng.actions.delutxo([840000]).send('utxomng.xsat@active'),
            'eosio_assert: utxomng.xsat::delutxo: [utxos] does not exist'
        )
    })

    it('delutxo', async () => {
        await contracts.utxomng.actions.delutxo([1]).send('utxomng.xsat@active')
        expect(get_utxo(1)).toEqual(undefined)
        expect(get_chain_state()).toEqual({
            head_height: 0,
            irreversible_height: 0,
            irreversible_hash: '0000000000000000000000000000000000000000000000000000000000000000',
            num_utxos: 0,
            migrating_height: 0,
            migrating_hash: '0000000000000000000000000000000000000000000000000000000000000000',
            migrating_num_utxos: 0,
            migrated_num_utxos: 0,
            parsed_height: 0,
            parsing_height: 0,
            parsing_progress_of: [],
            status: 0,
            synchronizer: '',
            miner: '',
            parser: '',
            num_validators_assigned: 0,
            num_provider_validators: 0,
        })
    })

    it('addblock: missing required authority utxomng.xsat', async () => {
        await expectToThrow(
            contracts.utxomng.actions
                .addblock([
                    839999,
                    '0000000000000000000172014ba58d66455762add0512355ad651207918494ab',
                    '0000000000000000000172014ba58d66455762add0512355ad651207918494ab',
                    671088644,
                    '00000000000000000001dcce6ce7c8a45872cafd1fb04732b447a14a91832591',
                    '5cdb277afa34ea35aa620e5cad205f18acda80b80dec9dacf4b84636a5ad0448',
                    1713571533,
                    386089497,
                    3205594798,
                ])
                .send('alice@active'),
            'missing required authority utxomng.xsat'
        )
    })

    it('addblock: height must be less than or equal to 839999', async () => {
        await expectToThrow(
            contracts.utxomng.actions
                .addblock([
                    840000,
                    '0000000000000000000172014ba58d66455762add0512355ad651207918494ab',
                    '0000000000000000000172014ba58d66455762add0512355ad651207918494ab',
                    671088644,
                    '00000000000000000001dcce6ce7c8a45872cafd1fb04732b447a14a91832591',
                    '5cdb277afa34ea35aa620e5cad205f18acda80b80dec9dacf4b84636a5ad0448',
                    1713571533,
                    386089497,
                    3205594798,
                ])
                .send('utxomng.xsat@active'),
            'eosio_assert: utxomng.xsat::addblock: height must be less than or equal to 839999'
        )

        await expectToThrow(
            contracts.utxomng.actions.delblock([840000]).send('utxomng.xsat@active'),
            'eosio_assert: utxomng.xsat::delblock: height must be less than or equal to 839999'
        )
    })

    it('addblock', async () => {
        const block = {
            height: 839999,
            hash: '0000000000000000000172014ba58d66455762add0512355ad651207918494ab',
            cumulative_work: '0000000000000000000172014ba58d66455762add0512355ad651207918494ab',
            merkle: '5cdb277afa34ea35aa620e5cad205f18acda80b80dec9dacf4b84636a5ad0448',
            previous_block_hash: '00000000000000000001dcce6ce7c8a45872cafd1fb04732b447a14a91832591',
            timestamp: 1713571533,
            version: 671088644,
            nonce: 3205594798,
            bits: 386089497,
        }
        await contracts.utxomng.actions.addblock(block).send('utxomng.xsat@active')
        expect(get_block(839999)).toEqual(block)
    })

    it('delblock: missing required authority utxomng.xsat', async () => {
        await expectToThrow(
            contracts.utxomng.actions.delblock([1]).send('alice@active'),
            'missing required authority utxomng.xsat'
        )
    })

    it('delblock:  [blocks] does not exist', async () => {
        await expectToThrow(
            contracts.utxomng.actions.delblock([1]).send('utxomng.xsat@active'),
            'eosio_assert: utxomng.xsat::delblock: [blocks] does not exist'
        )
    })

    it('delblock', async () => {
        await contracts.utxomng.actions.delblock([839999]).send('utxomng.xsat@active')
        expect(get_block(839999)).toEqual(undefined)
    })

    it('init: missing required authority', async () => {
        await expectToThrow(
            contracts.utxomng.actions
                .init([
                    839999,
                    '0000000000000000000172014ba58d66455762add0512355ad651207918494ab',
                    '0000000000000000000000000000000000000000753b8c1eaae701e1f0146360',
                ])
                .send('alice@active'),
            'missing required authority utxomng.xsat'
        )
    })

    it('init: block height must be 839999', async () => {
        await expectToThrow(
            contracts.utxomng.actions
                .init([
                    840000,
                    '0000000000000000000172014ba58d66455762add0512355ad651207918494ab',
                    '0000000000000000000000000000000000000000753b8c1eaae701e1f0146360',
                ])
                .send('utxomng.xsat@active'),
            'eosio_assert: utxomng.xsat::init: block height must be 839999'
        )
    })

    it('init: invalid hash', async () => {
        await expectToThrow(
            contracts.utxomng.actions
                .init([
                    839999,
                    '0000000000000000000000000000000000000000000000000000000000000000',
                    '0000000000000000000000000000000000000000000000000000000000000000',
                ])
                .send('utxomng.xsat@active'),
            'eosio_assert: utxomng.xsat::init: invalid hash'
        )
    })

    it('init: invalid cumulative_work', async () => {
        await expectToThrow(
            contracts.utxomng.actions
                .init([
                    839999,
                    '0000000000000000000172014ba58d66455762add0512355ad651207918494ab',
                    '0000000000000000000000000000000000000000000000000000000000000000',
                ])
                .send('utxomng.xsat@active'),
            'eosio_assert: utxomng.xsat::init: invalid cumulative_work'
        )
    })

    it('init', async () => {
        await contracts.utxomng.actions
            .init([
                839999,
                '0000000000000000000172014ba58d66455762add0512355ad651207918494ab',
                '0000000000000000000000000000000000000000753b8c1eaae701e1f0146360',
            ])
            .send('utxomng.xsat@active')
        expect(get_chain_state()).toEqual({
            head_height: 839999,
            irreversible_height: 839999,
            irreversible_hash: '0000000000000000000172014ba58d66455762add0512355ad651207918494ab',
            num_utxos: 0,
            migrating_height: 0,
            migrating_hash: '0000000000000000000000000000000000000000000000000000000000000000',
            migrating_num_utxos: 0,
            migrated_num_utxos: 0,
            parsed_height: 839999,
            parsing_height: 0,
            parsing_progress_of: [],
            status: 1,
            synchronizer: '',
            miner: '',
            parser: '',
            num_validators_assigned: 0,
            num_provider_validators: 0,
        })
    })

    it('config: missing required authority', async () => {
        await expectToThrow(
            contracts.utxomng.actions.config([600, 100, 5000, 100, 11, 10]).send('alice'),
            'missing required authority utxomng.xsat'
        )
    })

    it('config', async () => {
        await contracts.utxomng.actions.config([600, 100, 5000, 100, 11, 0]).send('utxomng.xsat')
        expect(get_config()).toEqual({
            num_merkle_layer: 11,
            num_miner_priority_blocks: 0,
            num_retain_data_blocks: 100,
            num_txs_per_verification: 2048,
            num_validators_per_distribution: 100,
            parse_timeout_seconds: 600,
            retained_spent_utxo_blocks: 5000,
        })
    })

    it('consensus: 839999', async () => {
        await contracts.utxomng.actions
            .addblock({
                height: 839999,
                hash: '0000000000000000000172014ba58d66455762add0512355ad651207918494ab',
                cumulative_work: '0000000000000000000000000000000000000000753b8c1eaae701e1f0146360',
                version: 671088644,
                previous_block_hash: '00000000000000000001dcce6ce7c8a45872cafd1fb04732b447a14a91832591',
                merkle: '5cdb277afa34ea35aa620e5cad205f18acda80b80dec9dacf4b84636a5ad0448',
                timestamp: 1713571533,
                bits: 386089497,
                nonce: 3205594798,
            })
            .send('utxomng.xsat@active')
    })

    it('consensus: 840000', async () => {
        const height = 840000
        const hash = '0000000000000000000320283a032748cef8227873ff4872689bf23f1cda83a5'
        const block = read_block(height)
        const block_size = block.length / 2
        const num_chunks = Math.ceil(block.length / max_chunk_size)
        await contracts.blksync.actions
            .initbucket(['bob', height, hash, block_size, num_chunks, max_chunk_size])
            .send('bob@active')
        await pushUpload('bob', height, hash, block)
        let max_times = 10
        while (max_times--) {
            await contracts.blksync.actions.verify(['bob', height, hash, get_nonce()]).send('bob@active')
            const retval = decodeReturn_verify(blockchain.actionTraces[0].returnValue)
            //if(retval.status == 'verify_fail') console.log(get_block_bucket('bob'))
            if (retval.status == 'verify_pass') break
        }

        blockchain.addTime(TimePointSec.from(1000))
        await contracts.blkendt.actions.endorse(['amy', height, hash]).send('amy@active')
        await contracts.blkendt.actions.endorse(['anna', height, hash]).send('anna@active')
        await contracts.blkendt.actions.endorse(['brian', height, hash]).send('brian@active')
        await contracts.blkendt.actions.endorse(['bob', height, hash]).send('bob@active')

        expect(get_chain_state()).toEqual({
            head_height: 840000,
            irreversible_hash: '0000000000000000000172014ba58d66455762add0512355ad651207918494ab',
            irreversible_height: 839999,
            num_utxos: 0,
            migrating_height: 0,
            migrating_hash: '0000000000000000000000000000000000000000000000000000000000000000',
            migrated_num_utxos: 0,
            migrating_num_utxos: 0,
            parsed_height: 839999,
            parsing_height: 840000,
            parsing_progress_of: [
                {
                    first: hash,
                    second: {
                        bucket_id: 1,
                        num_utxos: 0,
                        num_transactions: 0,
                        parsed_position: 0,
                        parsed_transactions: 0,
                        parsed_vin: 0,
                        parsed_vout: 0,
                        parser: 'bob',
                        parse_expiration_time: addTime(blockchain.timestamp, TimePointSec.from(10 * 60)).toString(),
                    },
                },
            ],
            synchronizer: '',
            miner: '',
            parser: '',
            num_validators_assigned: 0,
            num_provider_validators: 0,
            status: 1,
        })
    })

    it('parse 840000', async () => {
        //coinbase 1 vin 2 vout
        await contracts.utxomng.actions.processblock(['bob', 1, get_nonce()]).send('bob@active')
        expect(get_chain_state()).toEqual({
            head_height: 840000,
            irreversible_hash: '0000000000000000000172014ba58d66455762add0512355ad651207918494ab',
            irreversible_height: 839999,
            num_utxos: 0,
            migrating_height: 0,
            migrating_hash: '0000000000000000000000000000000000000000000000000000000000000000',
            migrated_num_utxos: 0,
            migrating_num_utxos: 0,
            parsed_height: 839999,
            parsing_height: 840000,
            parsing_progress_of: [
                {
                    first: '0000000000000000000320283a032748cef8227873ff4872689bf23f1cda83a5',
                    second: {
                        bucket_id: 1,
                        num_transactions: 3050,
                        parse_expiration_time: addTime(blockchain.timestamp, TimePointSec.from(10 * 60)).toString(),
                        parsed_position: 0,
                        parsed_transactions: 0,
                        parsed_vin: 1,
                        parsed_vout: 0,
                        parser: 'bob',
                        num_utxos: 0,
                    },
                },
            ],
            synchronizer: '',
            miner: '',
            parser: '',
            num_validators_assigned: 0,
            num_provider_validators: 0,
            status: 5,
        })

        await contracts.utxomng.actions.processblock(['bob', 1, get_nonce()]).send('bob@active')
        expect(get_chain_state()).toEqual({
            head_height: 840000,
            irreversible_hash: '0000000000000000000172014ba58d66455762add0512355ad651207918494ab',
            irreversible_height: 839999,
            num_utxos: 0,
            migrating_height: 0,
            migrating_hash: '0000000000000000000000000000000000000000000000000000000000000000',
            migrated_num_utxos: 0,
            migrating_num_utxos: 0,
            parsed_height: 839999,
            parsing_height: 840000,
            parsing_progress_of: [
                {
                    first: '0000000000000000000320283a032748cef8227873ff4872689bf23f1cda83a5',
                    second: {
                        bucket_id: 1,
                        num_utxos: 1,
                        num_transactions: 3050,
                        parse_expiration_time: addTime(blockchain.timestamp, TimePointSec.from(10 * 60)).toString(),
                        parsed_position: 0,
                        parsed_transactions: 0,
                        parsed_vin: 1,
                        parsed_vout: 1,
                        parser: 'bob',
                    },
                },
            ],
            synchronizer: '',
            miner: '',
            parser: '',
            num_validators_assigned: 0,
            num_provider_validators: 0,
            status: 5,
        })

        await contracts.utxomng.actions.processblock(['bob', 0, get_nonce()]).send('bob@active')
        expect(get_chain_state()).toEqual({
            head_height: 840000,
            irreversible_hash: '0000000000000000000172014ba58d66455762add0512355ad651207918494ab',
            irreversible_height: 839999,
            num_utxos: 0,
            migrating_height: 0,
            migrating_hash: '0000000000000000000000000000000000000000000000000000000000000000',
            migrated_num_utxos: 0,
            migrating_num_utxos: 0,
            parsed_height: 840000,
            parsing_height: 0,
            parsing_progress_of: [],
            synchronizer: '',
            miner: '',
            parser: '',
            num_validators_assigned: 0,
            num_provider_validators: 0,
            status: 1,
        })

        expect(get_consensus_block(1).parser).toEqual('bob')
    })

    it('buy slot', async () => {
        await contracts.poolreg.actions.buyslot(['alice', 'alice', 10]).send('alice@active')
    })

    it('consensus: 840001', async () => {
        const height = 840001
        const hash = '00000000000000000001b48a75d5a3077913f3f441eb7e08c13c43f768db2463'
        const block = read_block(height)
        const block_size = block.length / 2
        const num_chunks = Math.ceil(block.length / max_chunk_size)
        await contracts.blksync.actions
            .initbucket(['alice', height, hash, block_size, num_chunks, max_chunk_size])
            .send('alice@active')
        await pushUpload('alice', height, hash, block)
        let max_times = 10
        while (max_times--) {
            await contracts.blksync.actions.verify(['alice', height, hash, get_nonce()]).send('alice@active')
            const retval = decodeReturn_verify(blockchain.actionTraces[0].returnValue)
            if (retval.status == 'verify_pass') break
        }
        
        blockchain.addTime(TimePointSec.from(1000))
        await contracts.blkendt.actions.endorse(['amy', height, hash]).send('amy@active')
        await contracts.blkendt.actions.endorse(['anna', height, hash]).send('anna@active')
        await contracts.blkendt.actions.endorse(['brian', height, hash]).send('brian@active')
        await contracts.blkendt.actions.endorse(['bob', height, hash]).send('bob@active')

        expect(get_chain_state()).toEqual({
            head_height: 840001,
            irreversible_hash: '0000000000000000000172014ba58d66455762add0512355ad651207918494ab',
            irreversible_height: 839999,
            num_utxos: 0,
            migrating_height: 0,
            migrating_hash: '0000000000000000000000000000000000000000000000000000000000000000',
            migrated_num_utxos: 0,
            migrating_num_utxos: 0,
            parsed_height: 840000,
            parsing_height: 840001,
            parsing_progress_of: [
                {
                    first: hash,
                    second: {
                        bucket_id: 2,
                        num_utxos: 0,
                        num_transactions: 0,
                        parsed_position: 0,
                        parsed_transactions: 0,
                        parsed_vin: 0,
                        parsed_vout: 0,
                        parser: 'alice',
                        parse_expiration_time: addTime(blockchain.timestamp, TimePointSec.from(10 * 60)).toString(),
                    },
                },
            ],
            synchronizer: '',
            miner: '',
            parser: '',
            num_validators_assigned: 0,
            num_provider_validators: 0,
            status: 1,
        })
    })

    it('parse 840001: you are not a parser of the current block', async () => {
        await expectToThrow(
            contracts.utxomng.actions.processblock(['bob', 100, get_nonce()]).send('bob@active'),
            'eosio_assert: 4003:utxomng.xsat::processblock: you are not a parser of the current block'
        )
    })

    it('parse 840001', async () => {
        blockchain.addTime(TimePointSec.from(600))
        await contracts.utxomng.actions.processblock(['bob', 0, get_nonce()]).send('bob@active')

        expect(get_consensus_block(2).parser).toEqual('bob')

        expect(get_chain_state()).toEqual({
            head_height: 840001,
            irreversible_hash: '0000000000000000000172014ba58d66455762add0512355ad651207918494ab',
            irreversible_height: 839999,
            num_utxos: 0,
            migrating_height: 0,
            migrating_hash: '0000000000000000000000000000000000000000000000000000000000000000',
            migrated_num_utxos: 0,
            migrating_num_utxos: 0,
            parsed_height: 840001,
            parsing_height: 0,
            parsing_progress_of: [],
            synchronizer: '',
            miner: '',
            parser: '',
            num_validators_assigned: 0,
            num_provider_validators: 0,
            status: 1,
        })
    })

    it('there are currently no block to parse', async () => {
        await expectToThrow(
            contracts.utxomng.actions.processblock(['bob', 0, get_nonce()]).send('bob@active'),
            'eosio_assert: 4001:utxomng.xsat::processblock: there are currently no block to parse'
        )
    })

    it('consensus 840002', async () => {
        const height = 840002
        const hash = '00000000000000000002c0cc73626b56fb3ee1ce605b0ce125cc4fb58775a0a9'
        const block = read_block(height)
        const block_size = block.length / 2
        const num_chunks = Math.ceil(block.length / max_chunk_size)
        await contracts.blksync.actions
            .initbucket(['alice', height, hash, block_size, num_chunks, max_chunk_size])
            .send('alice@active')
        await pushUpload('alice', height, hash, block)
        let max_times = 10
        while (max_times--) {
            await contracts.blksync.actions.verify(['alice', height, hash, get_nonce()]).send('alice@active')
            const retval = decodeReturn_verify(blockchain.actionTraces[0].returnValue)
            if (retval.status == 'verify_pass') break
        }

        blockchain.addTime(TimePointSec.from(1000))
        await contracts.blkendt.actions.endorse(['amy', height, hash]).send('amy@active')
        await contracts.blkendt.actions.endorse(['anna', height, hash]).send('anna@active')
        await contracts.blkendt.actions.endorse(['brian', height, hash]).send('brian@active')
        await contracts.blkendt.actions.endorse(['bob', height, hash]).send('bob@active')
    })

    it('consensus 840003', async () => {
        const height = 840003
        const hash = '00000000000000000001cfe8671cb9269dfeded2c4e900e365fffae09b34b119'
        const block = read_block(height)
        const block_size = block.length / 2
        const num_chunks = Math.ceil(block.length / max_chunk_size)
        await contracts.blksync.actions
            .initbucket(['alice', height, hash, block_size, num_chunks, max_chunk_size])
            .send('alice@active')
        await pushUpload('alice', height, hash, block)
        let max_times = 10
        while (max_times--) {
            await contracts.blksync.actions.verify(['alice', height, hash, get_nonce()]).send('alice@active')
            const retval = decodeReturn_verify(blockchain.actionTraces[0].returnValue)
            if (retval.status == 'verify_pass') break
        }

        blockchain.addTime(TimePointSec.from(1000))
        await contracts.blkendt.actions.endorse(['amy', height, hash]).send('amy@active')
        await contracts.blkendt.actions.endorse(['anna', height, hash]).send('anna@active')
        await contracts.blkendt.actions.endorse(['brian', height, hash]).send('brian@active')
        await contracts.blkendt.actions.endorse(['bob', height, hash]).send('bob@active')
    })

    it('consensus 840004', async () => {
        const height = 840004
        const hash = '000000000000000000028458274b1f458d57d817fdce349e31dd5cb51b277d36'
        const block = read_block(height)
        const block_size = block.length / 2
        const num_chunks = Math.ceil(block.length / max_chunk_size)
        await contracts.blksync.actions
            .initbucket(['alice', height, hash, block_size, num_chunks, max_chunk_size])
            .send('alice@active')
        await pushUpload('alice', height, hash, block)
        let max_times = 10
        while (max_times--) {
            await contracts.blksync.actions.verify(['alice', height, hash, get_nonce()]).send('alice@active')
            const retval = decodeReturn_verify(blockchain.actionTraces[0].returnValue)
            if (retval.status == 'verify_pass') break
        }

        blockchain.addTime(TimePointSec.from(1000))
        await contracts.blkendt.actions.endorse(['amy', height, hash]).send('amy@active')
        await contracts.blkendt.actions.endorse(['anna', height, hash]).send('anna@active')
        await contracts.blkendt.actions.endorse(['brian', height, hash]).send('brian@active')
        await contracts.blkendt.actions.endorse(['bob', height, hash]).send('bob@active')
    })

    it('consensus 840005', async () => {
        const height = 840005
        const hash = '000000000000000000027b0ec0e3acadd018cd19e7dd976602f216a1bc12d079'
        const block = read_block(height)
        const block_size = block.length / 2
        const num_chunks = Math.ceil(block.length / max_chunk_size)
        await contracts.blksync.actions
            .initbucket(['alice', height, hash, block_size, num_chunks, max_chunk_size])
            .send('alice@active')
        await pushUpload('alice', height, hash, block)
        let max_times = 10
        while (max_times--) {
            await contracts.blksync.actions.verify(['alice', height, hash, get_nonce()]).send('alice@active')
            const retval = decodeReturn_verify(blockchain.actionTraces[0].returnValue)
            if (retval.status == 'verify_pass') break
        }

        blockchain.addTime(TimePointSec.from(1000))
        await contracts.blkendt.actions.endorse(['amy', height, hash]).send('amy@active')
        await contracts.blkendt.actions.endorse(['anna', height, hash]).send('anna@active')
        await contracts.blkendt.actions.endorse(['brian', height, hash]).send('brian@active')
        await contracts.blkendt.actions.endorse(['bob', height, hash]).send('bob@active')
    })

    it('consensus 840006', async () => {
        const height = 840006
        const hash = '0000000000000000000098dab8c28e5f20ab1663b8dd6c81bb54bbbcd0ead5ac'
        const block = read_block(height)
        const block_size = block.length / 2
        const num_chunks = Math.ceil(block.length / max_chunk_size)
        await contracts.blksync.actions
            .initbucket(['alice', height, hash, block_size, num_chunks, max_chunk_size])
            .send('alice@active')
        await pushUpload('alice', height, hash, block)
        let max_times = 10
        while (max_times--) {
            await contracts.blksync.actions.verify(['alice', height, hash, get_nonce()]).send('alice@active')
            const retval = decodeReturn_verify(blockchain.actionTraces[0].returnValue)
            if (retval.status == 'verify_pass') break
        }

        blockchain.addTime(TimePointSec.from(1000))
        await contracts.blkendt.actions.endorse(['amy', height, hash]).send('amy@active')
        await contracts.blkendt.actions.endorse(['anna', height, hash]).send('anna@active')
        await contracts.blkendt.actions.endorse(['brian', height, hash]).send('brian@active')
        await contracts.blkendt.actions.endorse(['bob', height, hash]).send('bob@active')
    })

    it('consensus 840007', async () => {
        const height = 840007
        const hash = '000000000000000000030d1455700ec234e4214e75e8e1112632b74febe80c78'
        const block = read_block(height)
        const block_size = block.length / 2
        const num_chunks = Math.ceil(block.length / max_chunk_size)
        await contracts.blksync.actions
            .initbucket(['alice', height, hash, block_size, num_chunks, max_chunk_size])
            .send('alice@active')
        await pushUpload('alice', height, hash, block)
        let max_times = 10
        while (max_times--) {
            await contracts.blksync.actions.verify(['alice', height, hash, get_nonce()]).send('alice@active')
            const retval = decodeReturn_verify(blockchain.actionTraces[0].returnValue)
            if (retval.status == 'verify_pass') break
        }

        blockchain.addTime(TimePointSec.from(1000))
        await contracts.blkendt.actions.endorse(['amy', height, hash]).send('amy@active')
        await contracts.blkendt.actions.endorse(['anna', height, hash]).send('anna@active')
        await contracts.blkendt.actions.endorse(['brian', height, hash]).send('brian@active')
        await contracts.blkendt.actions.endorse(['bob', height, hash]).send('bob@active')
    })

    it('parse 840002', async () => {
        blockchain.addTime(TimePointSec.from(600))
        await contracts.utxomng.actions.processblock(['alice', 0, get_nonce()]).send('alice@active')
        expect(get_chain_state()).toEqual({
            head_height: 840007,
            irreversible_hash: '0000000000000000000172014ba58d66455762add0512355ad651207918494ab',
            irreversible_height: 839999,
            num_utxos: 0,
            migrating_height: 0,
            migrating_hash: '0000000000000000000000000000000000000000000000000000000000000000',
            migrated_num_utxos: 0,
            migrating_num_utxos: 0,
            parsed_height: 840002,
            parsing_height: 840003,
            parsing_progress_of: [
                {
                    first: '00000000000000000001cfe8671cb9269dfeded2c4e900e365fffae09b34b119',
                    second: {
                        bucket_id: 4,
                        num_utxos: 0,
                        num_transactions: 0,
                        parse_expiration_time: addTime(blockchain.timestamp, TimePointSec.from(10 * 60)).toString(),
                        parsed_position: 0,
                        parsed_transactions: 0,
                        parsed_vin: 0,
                        parsed_vout: 0,
                        parser: 'alice',
                    },
                },
            ],
            synchronizer: '',
            miner: '',
            parser: '',
            num_validators_assigned: 0,
            num_provider_validators: 0,
            status: 1,
        })
    })

    it('parse 840003', async () => {
        blockchain.addTime(TimePointSec.from(600))
        await contracts.utxomng.actions.processblock(['alice', 0, get_nonce()]).send('alice@active')
        expect(get_chain_state()).toEqual({
            head_height: 840007,
            irreversible_hash: '0000000000000000000172014ba58d66455762add0512355ad651207918494ab',
            irreversible_height: 839999,
            num_utxos: 0,
            migrating_height: 0,
            migrating_hash: '0000000000000000000000000000000000000000000000000000000000000000',
            migrated_num_utxos: 0,
            migrating_num_utxos: 0,
            parsed_height: 840003,
            parsing_height: 840004,
            parsing_progress_of: [
                {
                    first: '000000000000000000028458274b1f458d57d817fdce349e31dd5cb51b277d36',
                    second: {
                        bucket_id: 5,
                        num_utxos: 0,
                        num_transactions: 0,
                        parse_expiration_time: addTime(blockchain.timestamp, TimePointSec.from(10 * 60)).toString(),
                        parsed_position: 0,
                        parsed_transactions: 0,
                        parsed_vin: 0,
                        parsed_vout: 0,
                        parser: 'alice',
                    },
                },
            ],
            synchronizer: '',
            miner: '',
            parser: '',
            num_validators_assigned: 0,
            num_provider_validators: 0,
            status: 1,
        })
    })

    it('parse 840004', async () => {
        blockchain.addTime(TimePointSec.from(600))
        await contracts.utxomng.actions.processblock(['alice', 0, get_nonce()]).send('alice@active')
        expect(get_chain_state()).toEqual({
            head_height: 840007,
            irreversible_hash: '0000000000000000000172014ba58d66455762add0512355ad651207918494ab',
            irreversible_height: 839999,
            num_utxos: 0,
            migrating_height: 0,
            migrating_hash: '0000000000000000000000000000000000000000000000000000000000000000',
            migrated_num_utxos: 0,
            migrating_num_utxos: 0,
            parsed_height: 840004,
            parsing_height: 840005,
            parsing_progress_of: [
                {
                    first: '000000000000000000027b0ec0e3acadd018cd19e7dd976602f216a1bc12d079',
                    second: {
                        bucket_id: 6,
                        num_utxos: 0,
                        num_transactions: 0,
                        parse_expiration_time: addTime(blockchain.timestamp, TimePointSec.from(10 * 60)).toString(),
                        parsed_position: 0,
                        parsed_transactions: 0,
                        parsed_vin: 0,
                        parsed_vout: 0,
                        parser: 'alice',
                    },
                },
            ],
            synchronizer: '',
            miner: '',
            parser: '',
            num_validators_assigned: 0,
            num_provider_validators: 0,
            status: 1,
        })
    })

    it('parse 840005', async () => {
        blockchain.addTime(TimePointSec.from(600))
        await contracts.utxomng.actions.processblock(['alice', 0, get_nonce()]).send('alice@active')
        expect(get_chain_state()).toEqual({
            head_height: 840007,
            irreversible_hash: '0000000000000000000172014ba58d66455762add0512355ad651207918494ab',
            irreversible_height: 839999,
            num_utxos: 0,
            migrating_height: 840000,
            migrating_hash: '0000000000000000000320283a032748cef8227873ff4872689bf23f1cda83a5',
            migrating_num_utxos: 11447,
            migrated_num_utxos: 0,
            parsed_height: 840005,
            parsing_height: 840006,
            parsing_progress_of: [
                {
                    first: '0000000000000000000098dab8c28e5f20ab1663b8dd6c81bb54bbbcd0ead5ac',
                    second: {
                        bucket_id: 7,
                        num_utxos: 0,
                        num_transactions: 0,
                        parse_expiration_time: addTime(blockchain.timestamp, TimePointSec.from(10 * 60)).toString(),
                        parsed_position: 0,
                        parsed_transactions: 0,
                        parsed_vin: 0,
                        parsed_vout: 0,
                        parser: 'alice',
                    },
                },
            ],
            synchronizer: 'bob',
            miner: 'bob',
            parser: 'bob',
            num_validators_assigned: 0,
            num_provider_validators: 4,
            status: 1,
        })
    })

    it('parse 840006: migrate utxo', async () => {
        blockchain.addTime(TimePointSec.from(600))
        while (true) {
            await contracts.utxomng.actions.processblock(['alice', 5000, get_nonce()]).send('alice@active')
            if (get_chain_state().status == 3) {
                break
            }
        }
        expect(get_chain_state()).toEqual({
            head_height: 840007,
            irreversible_hash: '0000000000000000000172014ba58d66455762add0512355ad651207918494ab',
            irreversible_height: 839999,
            num_utxos: 6683,
            migrating_height: 840000,
            migrating_hash: '0000000000000000000320283a032748cef8227873ff4872689bf23f1cda83a5',
            migrated_num_utxos: 11447,
            migrating_num_utxos: 11447,
            parsed_height: 840005,
            parsing_height: 840006,
            parsing_progress_of: [
                {
                    first: '0000000000000000000098dab8c28e5f20ab1663b8dd6c81bb54bbbcd0ead5ac',
                    second: {
                        bucket_id: 7,
                        num_utxos: 0,
                        num_transactions: 0,
                        parse_expiration_time: addTime(blockchain.timestamp, TimePointSec.from(10 * 60)).toString(),
                        parsed_position: 0,
                        parsed_transactions: 0,
                        parsed_vin: 0,
                        parsed_vout: 0,
                        parser: 'alice',
                    },
                },
            ],
            synchronizer: 'bob',
            miner: 'bob',
            parser: 'bob',
            num_validators_assigned: 0,
            num_provider_validators: 4,
            status: 3,
        })
    })

    it('parse 840006: delete data', async () => {
        blockchain.addTime(TimePointSec.from(600))
        await contracts.utxomng.actions.processblock(['alice', 0, get_nonce()]).send('alice@active')
        expect(get_chain_state()).toEqual({
            head_height: 840007,
            irreversible_hash: '0000000000000000000172014ba58d66455762add0512355ad651207918494ab',
            irreversible_height: 839999,
            num_utxos: 6683,
            migrating_height: 840000,
            migrating_hash: '0000000000000000000320283a032748cef8227873ff4872689bf23f1cda83a5',
            migrated_num_utxos: 11447,
            migrating_num_utxos: 11447,
            parsed_height: 840005,
            parsing_height: 840006,
            parsing_progress_of: [
                {
                    first: '0000000000000000000098dab8c28e5f20ab1663b8dd6c81bb54bbbcd0ead5ac',
                    second: {
                        bucket_id: 7,
                        num_utxos: 0,
                        num_transactions: 0,
                        parse_expiration_time: addTime(blockchain.timestamp, TimePointSec.from(10 * 60)).toString(),
                        parsed_position: 0,
                        parsed_transactions: 0,
                        parsed_vin: 0,
                        parsed_vout: 0,
                        parser: 'alice',
                    },
                },
            ],
            synchronizer: 'bob',
            miner: 'bob',
            parser: 'bob',
            num_validators_assigned: 0,
            num_provider_validators: 4,
            status: 4,
        })
    })

    it('parse 840006: distribute rewards', async () => {
        blockchain.addTime(TimePointSec.from(600))
        await contracts.utxomng.actions.processblock(['alice', 0, get_nonce()]).send('alice@active'),
            expect(get_chain_state()).toEqual({
                head_height: 840007,
                irreversible_hash: '0000000000000000000320283a032748cef8227873ff4872689bf23f1cda83a5',
                irreversible_height: 840000,
                migrated_num_utxos: 0,
                migrating_hash: '0000000000000000000000000000000000000000000000000000000000000000',
                migrating_height: 0,
                migrating_num_utxos: 0,
                num_utxos: 6683,
                parsed_height: 840005,
                parsing_height: 840006,
                parsing_progress_of: [
                    {
                        first: '0000000000000000000098dab8c28e5f20ab1663b8dd6c81bb54bbbcd0ead5ac',
                        second: {
                            bucket_id: 7,
                            num_utxos: 0,
                            num_transactions: 0,
                            parse_expiration_time: addTime(blockchain.timestamp, TimePointSec.from(10 * 60)).toString(),
                            parsed_position: 0,
                            parsed_transactions: 0,
                            parsed_vin: 0,
                            parsed_vout: 0,
                            parser: 'alice',
                        },
                    },
                ],
                synchronizer: '',
                miner: '',
                parser: '',
                num_validators_assigned: 0,
                num_provider_validators: 0,
                status: 5,
            })
    })

    it('parse 840006: parse', async () => {
        blockchain.addTime(TimePointSec.from(600))
        await contracts.utxomng.actions.processblock(['alice', 0, get_nonce()]).send('alice@active'),
            expect(get_chain_state()).toEqual({
                head_height: 840007,
                irreversible_hash: '0000000000000000000320283a032748cef8227873ff4872689bf23f1cda83a5',
                irreversible_height: 840000,
                migrated_num_utxos: 0,
                migrating_hash: '00000000000000000001b48a75d5a3077913f3f441eb7e08c13c43f768db2463',
                migrating_height: 840001,
                migrating_num_utxos: 11888,
                parsed_height: 840006,
                parsing_height: 840007,
                parsing_progress_of: [
                    {
                        first: '000000000000000000030d1455700ec234e4214e75e8e1112632b74febe80c78',
                        second: {
                            bucket_id: 8,
                            num_transactions: 0,
                            num_utxos: 0,
                            parse_expiration_time: addTime(blockchain.timestamp, TimePointSec.from(10 * 60)).toString(),
                            parsed_position: 0,
                            parsed_transactions: 0,
                            parsed_vin: 0,
                            parsed_vout: 0,
                            parser: 'alice',
                        },
                    },
                ],
                synchronizer: 'alice',
                miner: '',
                parser: 'bob',
                num_utxos: 6683,
                num_validators_assigned: 0,
                num_provider_validators: 4,
                status: 1,
            })
    })
})
