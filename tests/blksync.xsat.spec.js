const { Name, TimePointSec } = require('@greymass/eosio')
const { Blockchain, log, expectToThrow } = require('@proton/vert')
const { BTC, BTC_CONTRACT } = require('./src/constants')
const { addTime } = require('./src/help')
const fs = require('fs')
const path = require('path')

// Vert EOS VM
const blockchain = new Blockchain()
// log.setLevel('info')

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

const get_chain_state = () => {
    return contracts.utxomng.tables.chainstate().getTableRows()[0]
}

const get_pass_index = height => {
    return contracts.blksync.tables.passedindexs(BigInt(height)).getTableRows()
}

const get_block_miner = height => {
    return contracts.blksync.tables.blockminer(BigInt(height)).getTableRows()
}


const get_tx_output = () => {
    return contracts.parse.tables['tx.output']().getTableRows()
}

const get_block = height => {
    return contracts.utxomng.tables.blocks().getTableRow(BigInt(height))
}

const get_block_extra = height => {
    return contracts.utxomng.tables['block.extra'](BigInt(height)).getTableRows()
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

const decodeReturn_addblock = returnValue => {
    const statusLen = returnValue[0]
    const status = returnValue.subarray(1, statusLen + 1).toString()

    return {
        status,
        hash: returnValue
            .subarray(statusLen + 1, statusLen + 1 + 32)
            .reverse()
            .toString(),
        work: returnValue
            .subarray(statusLen + 1 + 32, statusLen + 1 + 32 + 32)
            .reverse()
            .toString(),
    }
}

const pushUpload = async (sender, height, hash, block) => {
    const max_chunk_size = 512 * 1024
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

const max_chunk_size = 512 * 1024
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
    await contracts.utxomng.actions.config([600, 100, 100, 11, 10]).send('utxomng.xsat')

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
            'eosio_assert: blksync.xsat::pushchunk: [blockbuckets] does not exists'
        )
    })

    it('height must be greater than 840000', async () => {
        const height = 839999
        const block_size = read_block(height).length / 2
        const num_chunks = Math.ceil(block_size / max_chunk_size)
        const hash = '000000000000000000029730547464f056f8b6e2e0a02eaf69c24389983a04f5'
        await expectToThrow(
            contracts.blksync.actions.initbucket(['alice', height, hash, block_size, num_chunks]).send('alice@active'),
            'eosio_assert: blksync.xsat::initbucket: height must be greater than 840000'
        )
    })

    it('init bucket', async () => {
        const height = 840000
        const block_size = read_block(height).length / 2
        const num_chunks = Math.ceil(block_size / max_chunk_size)
        const hash = '0000000000000000000320283a032748cef8227873ff4872689bf23f1cda83a5'
        await contracts.blksync.actions.initbucket(['alice', height, hash, block_size, num_chunks]).send('alice@active')
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

    it('processblock: merkle_invalid', async () => {
        const height = 840000
        const hash = '0000000000000000000320283a032748cef8227873ff4872689bf23f1cda83a5'
        let block = read_block(height)
        block = block.slice(0, block.length - 2) + 'ab'
        const block_size = block.length / 2
        const num_chunks = Math.ceil(block.length / max_chunk_size)
        await contracts.blksync.actions.initbucket(['alice', height, hash, block_size, num_chunks]).send('alice@active')
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
                status: 6,
                uploaded_num_chunks: 9,
                reason: 'merkle_invalid',
                verify_info: null,
            },
        ])
        await expectToThrow(
            contracts.blksync.actions.verify(['alice', height, hash]).send('alice@active'),
            'eosio_assert_message: blksync.xsat::verify: cannot validate block in the current state [verify_fail]'
        )
    })

    it('initbucket 840000', async () => {
        const height = 840000
        const hash = '0000000000000000000320283a032748cef8227873ff4872689bf23f1cda83a5'

        await contracts.blksync.actions.delbucket(['alice', height, hash]).send('alice@active')

        const block_size = read_block(height).length / 2
        const num_chunks = Math.ceil(read_block(height).length / max_chunk_size)
        await contracts.blksync.actions.initbucket(['bob', height, hash, block_size, num_chunks]).send('bob@active')
        expect(get_block_bucket('bob')).toEqual([
            {
                bucket_id: 3,
                hash: '0000000000000000000320283a032748cef8227873ff4872689bf23f1cda83a5',
                height: 840000,
                num_chunks: 9,
                size: 2325617,
                uploaded_size: 0,
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
                status: 2,
                uploaded_num_chunks: 9,
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
                bucket_id: 3,
                hash: '0000000000000000000320283a032748cef8227873ff4872689bf23f1cda83a5',
                cumulative_work: '0000000000000000000000000000000000000000753bdab0e0d745453677442b',
                id: 0,
                synchronizer: 'bob',
                miner: 'bob',
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
                status: 7,
                uploaded_num_chunks: 9,
                reason: '',
                verify_info: null,
            },
        ])
    })

    it('invalid state', async () => {
        const height = 840000
        const hash = '0000000000000000000320283a032748cef8227873ff4872689bf23f1cda83a5'
        let action = pushChunk('bob', height, hash, 1, getChunk(read_block(height), 1))
        await expectToThrow(
            action,
            'eosio_assert_message: blksync.xsat::pushchunk: cannot push chunk in the current state [verify_pass]'
        )

        action = contracts.blksync.actions.delchunk(['bob', height, hash, 1]).send('bob@active')
        await expectToThrow(
            action,
            'eosio_assert_message: blksync.xsat::delchunk: cannot delete chunk in the current state [verify_pass]'
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
            miner: 'bob',
            nonce: 3932395645,
            previous_block_hash: '0000000000000000000172014ba58d66455762add0512355ad651207918494ab',
            synchronizer: 'bob',
            timestamp: 1713571767,
            version: 710926336,
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
            contracts.blksync.actions.initbucket(['alice', height, hash, block_size, num_chunks]).send('alice@active'),
            'eosio_assert: blksync.xsat::initbucket: the block has reached consensus'
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
                .initbucket(['alice', height, hash, block_size, num_chunks])
                .send('alice@active')
        }

        const height = 840003
        const hash = '00000000000000000001cfe8671cb9269dfeded2c4e900e365fffae09b34b119'

        const block = read_block(height)
        const block_size = block.length / 2
        const num_chunks = Math.ceil(block.length / max_chunk_size)
        await expectToThrow(
            contracts.blksync.actions.initbucket(['alice', height, hash, block_size, num_chunks]).send('alice@active'),
            'eosio_assert: blksync.xsat::initbucket: not enough slots, please buy more slots'
        )
    })

    it('processblock: must be more than 6 blocks to process', async () => {
        await expectToThrow(
            contracts.utxomng.actions.processblock(['alice', 1]).send('alice@active'),
            'eosio_assert_message: utxomng.xsat::processblock: must be more than 6 blocks to process'
        )
    })

    it('processblock: cannot validate block in the current state [verify_fail]', async () => {
        const height = 840001
        const hash = '00000000000000000001b48a75d5a3077913f3f441eb7e08c13c43f768db2463'
        const block = read_block(height) + 'ab'
        const block_size = block.length / 2
        const num_chunks = Math.ceil(block.length / max_chunk_size)
        await contracts.blksync.actions.initbucket(['alice', height, hash, block_size, num_chunks]).send('alice@active')
        await pushUpload('alice', height, hash, block)

        await contracts.blksync.actions.verify(['alice', height, hash]).send('alice@active')
        await contracts.blksync.actions.verify(['alice', height, hash]).send('alice@active')

        expect(get_block_bucket('alice')[0].reason).toEqual('data_exceeds')
        await expectToThrow(
            contracts.blksync.actions.verify(['alice', height, hash]).send('alice@active'),
            'eosio_assert_message: blksync.xsat::verify: cannot validate block in the current state [verify_fail]'
        )
    })

    it('push_chunk: 840001 - 8400005', async () => {
        // buy slots
        await contracts.poolreg.actions.buyslot(['alice', 'alice', 10]).send('alice@active')

        // init bucket and pushchunk
        const mainnet_blocks = [
            { height: 840001, hash: '00000000000000000001b48a75d5a3077913f3f441eb7e08c13c43f768db2463' },
            { height: 840002, hash: '00000000000000000002c0cc73626b56fb3ee1ce605b0ce125cc4fb58775a0a9' },
            { height: 840003, hash: '00000000000000000001cfe8671cb9269dfeded2c4e900e365fffae09b34b119' },
            { height: 840004, hash: '000000000000000000028458274b1f458d57d817fdce349e31dd5cb51b277d36' },
            { height: 840005, hash: '000000000000000000027b0ec0e3acadd018cd19e7dd976602f216a1bc12d079' },
            { height: 840006, hash: '0000000000000000000098dab8c28e5f20ab1663b8dd6c81bb54bbbcd0ead5ac' },
            { height: 840007, hash: '000000000000000000030d1455700ec234e4214e75e8e1112632b74febe80c78' },
        ]
        for (const block_info of mainnet_blocks) {
            const height = block_info.height
            const hash = block_info.hash

            const block = read_block(height)
            const block_size = block.length / 2
            const num_chunks = Math.ceil(block.length / max_chunk_size)
            await contracts.blksync.actions
                .initbucket(['alice', height, hash, block_size, num_chunks])
                .send('alice@active')
            await pushUpload('alice', height, hash, read_block(height))
            let max_times = 10
            while (max_times--) {
                await contracts.blksync.actions.verify(['alice', height, hash]).send('alice@active')
                const retval = decodeReturn_addblock(blockchain.actionTraces[0].returnValue)
                if (retval.status == 'verify_pass') break
            }

            await contracts.blkendt.actions.endorse(['amy', height, hash]).send('amy@active')
            await contracts.blkendt.actions.endorse(['anna', height, hash]).send('anna@active')
            await contracts.blkendt.actions.endorse(['brian', height, hash]).send('brian@active')
            await contracts.blkendt.actions.endorse(['bob', height, hash]).send('bob@active')
        }
    }, 15000)

    it('parse 840000', async () => {
        //coinbase 1 vin 2 vout
        await contracts.utxomng.actions.processblock(['bob', 1]).send('bob@active')
        expect(get_chain_state()).toEqual({
            num_transactions: 3050,
            processed_position: 0,
            processed_transactions: 0,
            processed_vin: 0,
            processed_vout: 1,
            head_height: 840007,
            irreversible_hash: '0000000000000000000172014ba58d66455762add0512355ad651207918494ab',
            irreversible_height: 839999,
            parsing_height: 840000,
            parsing_hash: '0000000000000000000320283a032748cef8227873ff4872689bf23f1cda83a5',
            num_transactions: 3050,
            num_utxos: 1,
            parsed_expiration_time: addTime(blockchain.timestamp, TimePointSec.from(10 * 60)).toString(),
            parser: 'bob',
            parsing_bucket_id: 3,
            status: 2,
            synchronizer: 'bob',
            miner: 'bob',
            num_validators_assigned: 0,
            num_provider_validators: 5,
        })

        await contracts.utxomng.actions.processblock(['bob', 1]).send('bob@active')
        expect(get_chain_state()).toEqual({
            num_transactions: 3050,
            processed_position: 0,
            processed_transactions: 0,
            processed_vin: 0,
            processed_vout: 2,
            head_height: 840007,
            irreversible_hash: '0000000000000000000172014ba58d66455762add0512355ad651207918494ab',
            irreversible_height: 839999,
            parsing_height: 840000,
            parsing_hash: '0000000000000000000320283a032748cef8227873ff4872689bf23f1cda83a5',
            num_transactions: 3050,
            num_utxos: 1,
            parsed_expiration_time: addTime(blockchain.timestamp, TimePointSec.from(10 * 60)).toString(),
            parser: 'bob',
            parsing_bucket_id: 3,
            status: 2,
            synchronizer: 'bob',
            miner: 'bob',
            num_validators_assigned: 0,
            num_provider_validators: 5,
        })
        // parse utxo
        await contracts.utxomng.actions.processblock(['bob', 0]).send('bob@active')
        expect(get_chain_state()).toEqual({
            num_transactions: 3050,
            processed_position: 2325537,
            processed_transactions: 3050,
            processed_vin: 0,
            processed_vout: 0,
            head_height: 840007,
            irreversible_hash: '0000000000000000000172014ba58d66455762add0512355ad651207918494ab',
            irreversible_height: 839999,
            parsing_height: 840000,
            parsing_hash: '0000000000000000000320283a032748cef8227873ff4872689bf23f1cda83a5',
            num_transactions: 3050,
            num_utxos: 6683,
            parsed_expiration_time: addTime(blockchain.timestamp, TimePointSec.from(10 * 60)).toString(),
            parser: 'bob',
            parsing_bucket_id: 3,
            status: 3,
            synchronizer: 'bob',
            miner: 'bob',
            num_validators_assigned: 0,
            num_provider_validators: 5,
        })
        // erase data
        await contracts.utxomng.actions.processblock(['bob', 1]).send('bob@active')
        expect(get_chain_state()).toEqual({
            num_transactions: 3050,
            processed_position: 2325537,
            processed_transactions: 3050,
            processed_vin: 0,
            processed_vout: 0,
            head_height: 840007,
            irreversible_hash: '0000000000000000000172014ba58d66455762add0512355ad651207918494ab',
            irreversible_height: 839999,
            num_transactions: 3050,
            num_utxos: 6683,
            parsed_expiration_time: addTime(blockchain.timestamp, TimePointSec.from(10 * 60)).toString(),
            parsing_height: 840000,
            parsing_hash: '0000000000000000000320283a032748cef8227873ff4872689bf23f1cda83a5',
            parser: 'bob',
            parsing_bucket_id: 3,
            status: 4,
            synchronizer: 'bob',
            miner: 'bob',
            num_validators_assigned: 0,
            num_provider_validators: 5,
        })
        // distribute reward
        await contracts.utxomng.actions.processblock(['bob', 1]).send('bob@active')
        expect(get_chain_state()).toEqual({
            num_transactions: 0,
            processed_position: 0,
            processed_transactions: 0,
            processed_vin: 0,
            processed_vout: 0,
            head_height: 840007,
            irreversible_hash: '0000000000000000000320283a032748cef8227873ff4872689bf23f1cda83a5',
            irreversible_height: 840000,
            num_transactions: 0,
            num_utxos: 6683,
            parsed_expiration_time: addTime(blockchain.timestamp, TimePointSec.from(10 * 60)).toString(),
            parser: 'alice',
            parsing_bucket_id: 4,
            parsing_hash: '00000000000000000001b48a75d5a3077913f3f441eb7e08c13c43f768db2463',
            parsing_height: 840001,
            status: 1,
            synchronizer: 'alice',
            miner: '',
            num_validators_assigned: 0,
            num_provider_validators: 4,
        })
        const height = 840000
        expect(get_consensus_block(3)).toEqual(undefined)
        expect(get_block_extra(height)).toEqual([
            {
                bucket_id: 3,
            },
        ])
        expect(get_block(height)).toEqual({
            height: 840000,
            hash: '0000000000000000000320283a032748cef8227873ff4872689bf23f1cda83a5',
            bits: 386089497,
            cumulative_work: '0000000000000000000000000000000000000000753bdab0e0d745453677442b',
            merkle: '031b417c3a1828ddf3d6527fc210daafcc9218e81f98257f88d4d43bd7a5894f',
            nonce: 3932395645,
            previous_block_hash: '0000000000000000000172014ba58d66455762add0512355ad651207918494ab',
            timestamp: 1713571767,
            version: 710926336,
        })
    }, 15000)

    it('parse 840001', async () => {
        blockchain.addTime(TimePointSec.from(600))
        await contracts.utxomng.actions.processblock(['alice', 0]).send('alice@active')
        await contracts.utxomng.actions.processblock(['alice', 0]).send('alice@active')
        await contracts.utxomng.actions.processblock(['alice', 0]).send('alice@active')
        const height = 840001
        expect(get_block(height)).toEqual({
            bits: 386089497,
            cumulative_work: '0000000000000000000000000000000000000000753c294316c788a87cda24f6',
            hash: '00000000000000000001b48a75d5a3077913f3f441eb7e08c13c43f768db2463',
            height: 840001,
            merkle: '38de638a0541345dbebf8780a7dfe96e0db4e224071033f863b0b718867fc0bc',
            nonce: 3425079405,
            previous_block_hash: '0000000000000000000320283a032748cef8227873ff4872689bf23f1cda83a5',
            timestamp: 1713571854,
            version: 538968068,
        })
        expect(get_consensus_block(4)).toEqual(undefined)
        expect(get_block_extra(height)).toEqual([
            {
                bucket_id: 4,
            },
        ])
    }, 10000)
})
