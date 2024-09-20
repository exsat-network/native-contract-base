const { Name, TimePointSec } = require('@greymass/eosio')
const { Blockchain, log, expectToThrow } = require('@proton/vert')
const { BTC, BTC_CONTRACT } = require('./src/constants')
const fs = require('fs')
const path = require('path')

const { decodeReturn_addblock, max_chunk_size } = require('./src/help')

// Vert EOS VM
const blockchain = new Blockchain()
//log.setLevel('debug')

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

const get_pass_index = height => {
    return contracts.blksync.tables.passedindexs(BigInt(height)).getTableRows()
}

const get_consensus_block = bucket_id => {
    if (bucket_id) {
        return contracts.utxomng.tables.consensusblk().getTableRow(BigInt(bucket_id))
    } else {
        return contracts.utxomng.tables.consensusblk().getTableRows()
    }
}

const get_block_chunks = chunk_id => {
    return contracts.blksync.tables['block.chunk'](BigInt(chunk_id)).getTableRows()
}

const get_block_bucket = synchronizer => {
    const scope = Name.from(synchronizer).value.value
    return contracts.blksync.tables.blockbuckets(scope).getTableRows()
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

const getChunk = (block, index) => {
    const block_buffer = Uint8Array.from(Buffer.from(block, 'hex'))
    // todo check
    let begin = (index - 1) * max_chunk_size
    const end = Math.min(begin + max_chunk_size, block_buffer.byteLength)
    return block_buffer.slice(begin, end)
}

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
    await contracts.blkendt.actions.config([0, 0, 2]).send('blkendt.xsat@active')

    // staking
    await contracts.btc.actions.transfer(['alice', 'staking.xsat', '100.00000000 BTC', 'alice']).send('alice@active')
    await contracts.btc.actions.transfer(['bob', 'staking.xsat', '200.00000000 BTC', 'bob']).send('bob@active')
    await contracts.btc.actions.transfer(['amy', 'staking.xsat', '300.00000000 BTC', 'amy']).send('amy@active')
    await contracts.btc.actions.transfer(['anna', 'staking.xsat', '700.00000000 BTC', 'anna']).send('anna@active')
    await contracts.btc.actions.transfer(['brian', 'staking.xsat', '900.00000000 BTC', 'brian']).send('brian@active')

    // init
    await contracts.utxomng.actions
        .init([
            839999,
            '0000000000000000000172014ba58d66455762add0512355ad651207918494ab',
            '0000000000000000000000000000000000000000753b8c1eaae701e1f0146360',
        ])
        .send('utxomng.xsat@active')
    await contracts.utxomng.actions.config([600, 100, 5000, 100, 11, 10]).send('utxomng.xsat')

    await contracts.utxomng.actions
        .addblock({
            height: 839999,
            hash: '0000000000000000000172014ba58d66455762add0512355ad651207918494ab',
            cumulative_work: '0000000000000000000000000000000000000000753b8c1eaae701e1f0146360',
            version: 671088644,
            previous_block_hash: '00000000000000000001dcce6ce7c8a45872cafd1fb04732b447a14a91832591',
            merkle: '5cdb277afa34ea35aa620e5cad205f18acda80b80dec9dacf4b84636a5ad0448',
            timestamp: 1713571533,
            bits: 17034219,
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

describe('blksync.xsat', () => {
    it('consensus: missing required authority', async () => {
        await expectToThrow(
            contracts.blksync.actions.consensus([840000, 'alice', 1]).send('alice@active'),
            'missing required authority utxomng.xsat'
        )
    })

    it('pushchunk: not initialized', async () => {
        const height = 840000
        const hash = '000000000000000000029730547464f056f8b6e2e0a02eaf69c24389983a04f5'
        await expectToThrow(
            pushChunk('alice', height, hash, 0, getChunk(read_block(height), 1)),
            'eosio_assert: 2012:blksync.xsat::pushchunk: [blockbuckets] does not exists'
        )
    })

    it('height must be greater than 840000', async () => {
        const height = 839999
        const hash = '000000000000000000029730547464f056f8b6e2e0a02eaf69c24389983a04f5'
        const block = read_block(height)
        const block_size = block.length / 2
        const num_chunks = Math.ceil(block.length / max_chunk_size)
        await expectToThrow(
            contracts.blksync.actions
                .initbucket(['alice', height, hash, block_size, num_chunks, max_chunk_size])
                .send('alice@active'),
            'eosio_assert: 2001:blksync.xsat::initbucket: height must be greater than 840000'
        )
    })

    it('block_size must be greater than 80 and less than or equal to 4194304', async () => {
        const height = 840000
        const hash = '0000000000000000000320283a032748cef8227873ff4872689bf23f1cda83a5'
        await expectToThrow(
            contracts.blksync.actions.initbucket(['alice', height, hash, 1, 1, max_chunk_size]).send('alice@active'),
            'eosio_assert: 2002:blksync.xsat::initbucket: block_size must be greater than 80 and less than or equal to 4194304'
        )

        await expectToThrow(
            contracts.blksync.actions
                .initbucket(['alice', height, hash, 4194305, 1, max_chunk_size])
                .send('alice@active'),
            'eosio_assert: 2002:blksync.xsat::initbucket: block_size must be greater than 80 and less than or equal to 4194304'
        )
    })

    it('block_size must be greater than 80 and less than or equal to 4194304', async () => {
        const height = 840000
        const hash = '0000000000000000000320283a032748cef8227873ff4872689bf23f1cda83a5'
        await expectToThrow(
            contracts.blksync.actions.initbucket(['alice', height, hash, 2097152, 0, max_chunk_size]).send('alice@active'),
            'eosio_assert: 2003:blksync.xsat::initbucket: num_chunks must be greater than 0 and less than or equal to 64'
        )

        await expectToThrow(
            contracts.blksync.actions
                .initbucket(['alice', height, hash, 2097152, 65, max_chunk_size])
                .send('alice@active'),
            'eosio_assert: 2003:blksync.xsat::initbucket: num_chunks must be greater than 0 and less than or equal to 64'
        )
    })

    it('init bucket', async () => {
        const height = 840000
        const hash = '0000000000000000000320283a032748cef8227873ff4872689bf23f1cda83a5'
        const block = read_block(height)
        const block_size = block.length / 2
        const num_chunks = Math.ceil(block.length / max_chunk_size)
        await contracts.blksync.actions
            .initbucket(['alice', height, hash, block_size, num_chunks, max_chunk_size])
            .send('alice@active')

        expect(get_block_bucket('alice')).toEqual([
            {
                bucket_id: 1,
                height,
                hash,
                num_chunks: num_chunks,
                size: block_size,
                uploaded_size: 0,
                chunk_size: max_chunk_size,
                chunk_ids: [],
                status: 1,
                uploaded_num_chunks: 0,
                updated_at: TimePointSec.from(blockchain.timestamp).toString(),
                reason: '',
                verify_info: null,
            },
        ])
    })

    it('pushchunk', async () => {
        const height = 840000
        const hash = '0000000000000000000320283a032748cef8227873ff4872689bf23f1cda83a5'
        await pushChunk('alice', height, hash, 0, getChunk(read_block(height), 1))
        const chunks = get_block_chunks(1)
        expect(chunks.length).toEqual(1)
    })

    it('delchunk', async () => {
        const height = 840000
        const hash = '0000000000000000000320283a032748cef8227873ff4872689bf23f1cda83a5'
        await contracts.blksync.actions.delchunk(['alice', height, hash, 0]).send('alice@active')
        const chunks = get_block_chunks(1)
        expect(chunks.length).toEqual(0)
    })

    it('delbucket', async () => {
        const height = 840000
        const hash = '0000000000000000000320283a032748cef8227873ff4872689bf23f1cda83a5'
        await pushUpload('alice', height, hash, read_block(height))

        await contracts.blksync.actions.delbucket(['alice', height, hash]).send('alice@active')
        const rows = await get_block_chunks(1)
        expect(rows.length).toEqual(0)
    })

    it('verify: merkle_invalid', async () => {
        const height = 840000
        const hash = '0000000000000000000320283a032748cef8227873ff4872689bf23f1cda83a5'
        let block = read_block(height)
        block = block.slice(0, block.length - 2) + 'ab'
        const block_size = block.length / 2
        const num_chunks = Math.ceil(block.length / max_chunk_size)
        await contracts.blksync.actions
            .initbucket(['alice', height, hash, block_size, num_chunks, max_chunk_size])
            .send('alice@active')
        await pushUpload('alice', height, hash, block)

        await contracts.blksync.actions.verify(['alice', height, hash]).send('alice@active')
        await contracts.blksync.actions.verify(['alice', height, hash]).send('alice@active')

        expect(get_block_bucket('alice')).toEqual([
            {
                bucket_id: 2,
                hash: '0000000000000000000320283a032748cef8227873ff4872689bf23f1cda83a5',
                height: 840000,
                num_chunks: 9,
                size: 2325617,
                uploaded_size: 2325617,
                chunk_size: max_chunk_size,
                chunk_ids: [0, 1, 2, 3, 4, 5, 6, 7, 8],
                status: 6,
                uploaded_num_chunks: 9,
                updated_at: TimePointSec.from(blockchain.timestamp).toString(),
                reason: 'merkle_invalid',
                verify_info: null,
            },
        ])
        await expectToThrow(
            contracts.blksync.actions.verify(['alice', height, hash]).send('alice@active'),
            'eosio_assert_message: 2019:blksync.xsat::verify: cannot validate block in the current state [verify_fail]'
        )
    })

    it('initbucket 840000', async () => {
        const height = 840000
        const hash = '0000000000000000000320283a032748cef8227873ff4872689bf23f1cda83a5'

        await contracts.blksync.actions.delbucket(['alice', height, hash]).send('alice@active')

        const block = read_block(height)
        const block_size = block.length / 2
        const num_chunks = Math.ceil(block.length / max_chunk_size)
        await contracts.blksync.actions
            .initbucket(['bob', height, hash, block_size, num_chunks, max_chunk_size])
            .send('bob@active')
        expect(get_block_bucket('bob')).toEqual([
            {
                bucket_id: 3,
                hash: '0000000000000000000320283a032748cef8227873ff4872689bf23f1cda83a5',
                height: 840000,
                num_chunks: 9,
                size: 2325617,
                uploaded_size: 0,
                chunk_size: max_chunk_size,
                chunk_ids: [],
                updated_at: TimePointSec.from(blockchain.timestamp).toString(),
                status: 1,
                uploaded_num_chunks: 0,
                reason: '',
                verify_info: null,
            },
        ])
    })

    it('push chunks', async () => {
        const height = 840000
        const hash = '0000000000000000000320283a032748cef8227873ff4872689bf23f1cda83a5'
        await pushUpload('bob', height, hash, read_block(height))

        expect(get_block_bucket('bob')).toEqual([
            {
                bucket_id: 3,
                hash: '0000000000000000000320283a032748cef8227873ff4872689bf23f1cda83a5',
                height: 840000,
                num_chunks: 9,
                size: 2325617,
                uploaded_size: 2325617,
                chunk_size: max_chunk_size,
                chunk_ids: [0, 1, 2, 3, 4, 5, 6, 7, 8],
                status: 2,
                uploaded_num_chunks: 9,
                updated_at: TimePointSec.from(blockchain.timestamp).toString(),
                reason: '',
                verify_info: null,
            },
        ])
    })

    it('accepts and verify block 840000', async () => {
        blockchain.addBlocks(10)
        const height = 840000
        const hash = '0000000000000000000320283a032748cef8227873ff4872689bf23f1cda83a5'
        await contracts.blksync.actions.verify(['bob', height, hash]).send('bob@active')
        retval = decodeReturn_addblock(blockchain.actionTraces[0].returnValue)
        expect(retval.status).toBe('verify_merkle')
        await contracts.blksync.actions.verify(['bob', height, hash]).send('bob@active')
        await contracts.blksync.actions.verify(['bob', height, hash]).send('bob@active')
        retval = decodeReturn_addblock(blockchain.actionTraces[0].returnValue)
        expect(retval.status).toBe('verify_pass')
        expect(get_pass_index(height)).toEqual([
            {
                id: 1,
                bucket_id: 3,
                hash: '0000000000000000000320283a032748cef8227873ff4872689bf23f1cda83a5',
                cumulative_work: '0000000000000000000000000000000000000000753bdab0e0d745453677442b',
                synchronizer: 'bob',
                miner: 'bob',
                created_at: TimePointSec.from(blockchain.timestamp).toString(),
            },
        ])

        expect(get_block_bucket('bob')).toEqual([
            {
                bucket_id: 3,
                hash: '0000000000000000000320283a032748cef8227873ff4872689bf23f1cda83a5',
                height: 840000,
                num_chunks: 9,
                size: 2325617,
                uploaded_size: 2325617,
                uploaded_num_chunks: 9,
                chunk_size: max_chunk_size,
                chunk_ids: [0, 1, 2, 3, 4, 5, 6, 7, 8],
                reason: '',
                status: 7,
                updated_at: TimePointSec.from(blockchain.timestamp).toString(),
                verify_info: null,
            },
        ])
    })

    it('invalid state', async () => {
        const height = 840000
        const hash = '0000000000000000000320283a032748cef8227873ff4872689bf23f1cda83a5'

        await expectToThrow(
            pushChunk('bob', height, hash, 1, getChunk(read_block(height), 1)),
            'eosio_assert_message: 2013:blksync.xsat::pushchunk: cannot push chunk in the current state [verify_pass]'
        )

        await expectToThrow(
            contracts.blksync.actions.delchunk(['bob', height, hash, 1]).send('bob@active'),
            'eosio_assert_message: 2015:blksync.xsat::delchunk: cannot delete chunk in the current state [verify_pass]'
        )
    })

    it('endorse pass 840000', async () => {
        const height = 840000
        const hash = '0000000000000000000320283a032748cef8227873ff4872689bf23f1cda83a5'
        await contracts.blkendt.actions.endorse(['amy', height, hash]).send('amy@active')
        await contracts.blkendt.actions.endorse(['bob', height, hash]).send('bob@active')
        await contracts.blkendt.actions.endorse(['alice', height, hash]).send('alice@active')
        await contracts.blkendt.actions.endorse(['anna', height, hash]).send('anna@active')
        await contracts.blkendt.actions.endorse(['brian', height, hash]).send('brian@active')
        expect(get_consensus_block(3)).toEqual({
            bits: 386089497,
            bucket_id: 3,
            cumulative_work: '0000000000000000000000000000000000000000753bdab0e0d745453677442b',
            hash: '0000000000000000000320283a032748cef8227873ff4872689bf23f1cda83a5',
            height: 840000,
            merkle: '031b417c3a1828ddf3d6527fc210daafcc9218e81f98257f88d4d43bd7a5894f',
            nonce: 3932395645,
            previous_block_hash: '0000000000000000000172014ba58d66455762add0512355ad651207918494ab',
            timestamp: 1713571767,
            version: 710926336,
            miner: 'bob',
            parser: '',
            synchronizer: 'bob',
            num_utxos: 0,
            parse: false,
            irreversible: false,
            created_at: TimePointSec.from(blockchain.timestamp).toString(),
        })
        expect(get_block_bucket('bob')).toEqual([])
        expect(get_pass_index(height)).toEqual([])
    })

    it('initbucket: the block has reached consensus', async () => {
        const height = 840000
        const block = read_block(height)
        const block_size = block.length / 2
        const num_chunks = Math.ceil(block.length / max_chunk_size)
        const hash = '0000000000000000000320283a032748cef8227873ff4872689bf23f1cda83a5'
        await expectToThrow(
            contracts.blksync.actions
                .initbucket(['alice', height, hash, block_size, num_chunks, max_chunk_size])
                .send('alice@active'),
            'eosio_assert: 2005:blksync.xsat::initbucket: the block has reached consensus'
        )
    })

    it('initbucket: not enough slots, please buy more slots', async () => {
        // init bucket and pushchunk
        const mainnet_blocks = [
            { height: 840001, hash: '00000000000000000001b48a75d5a3077913f3f441eb7e08c13c43f768db2463' },
            { height: 840002, hash: '00000000000000000002c0cc73626b56fb3ee1ce605b0ce125cc4fb58775a0a9' },
        ]
        for (const block_info of mainnet_blocks) {
            const height = block_info.height
            const hash = block_info.hash

            const block = read_block(height)
            const block_size = block.length / 2
            const num_chunks = Math.ceil(block.length / max_chunk_size)
            await contracts.blksync.actions
                .initbucket(['alice', height, hash, block_size, num_chunks, max_chunk_size])
                .send('alice@active')
        }

        const height = 840003
        const hash = '00000000000000000001cfe8671cb9269dfeded2c4e900e365fffae09b34b119'

        const block = read_block(height)
        const block_size = block.length / 2
        const num_chunks = Math.ceil(block.length / max_chunk_size)
        await expectToThrow(
            contracts.blksync.actions
                .initbucket(['alice', height, hash, block_size, num_chunks, max_chunk_size])
                .send('alice@active'),
            'eosio_assert: 2007:blksync.xsat::initbucket: not enough slots, please buy more slots'
        )
    })

    it('verify: cannot validate block in the current state [verify_fail]', async () => {
        const height = 840001
        const hash = '00000000000000000001b48a75d5a3077913f3f441eb7e08c13c43f768db2463'
        const block = read_block(height) + 'ab'
        const block_size = block.length / 2
        const num_chunks = Math.ceil(block.length / max_chunk_size)
        await contracts.blksync.actions
            .initbucket(['alice', height, hash, block_size, num_chunks, max_chunk_size])
            .send('alice@active')
        await pushUpload('alice', height, hash, block)

        await contracts.blksync.actions.verify(['alice', height, hash]).send('alice@active')
        await contracts.blksync.actions.verify(['alice', height, hash]).send('alice@active')

        expect(get_block_bucket('alice')[0].reason).toEqual('data_exceeds')
        await expectToThrow(
            contracts.blksync.actions.verify(['alice', height, hash]).send('alice@active'),
            'eosio_assert_message: 2019:blksync.xsat::verify: cannot validate block in the current state [verify_fail]'
        )
    })
})
