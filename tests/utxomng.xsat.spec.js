const { Blockchain, expectToThrow } = require('@proton/vert')

// Vert EOS VM
const blockchain = new Blockchain()
//log.setLevel('debug');
// contracts
const contracts = {
    utxomng: blockchain.createContract('utxomng.xsat', 'tests/wasm/utxomng.xsat', true),
}

blockchain.createAccounts('alice')

const get_utxo = id => {
    return contracts.utxomng.tables.utxos().getTableRow(BigInt(id))
}

const get_chain_state = () => {
    return contracts.utxomng.tables.chainstate().getTableRows()[0]
}

const get_block = height => {
    return contracts.utxomng.tables.blocks().getTableRow(BigInt(height))
}

describe('utxomng.xsat', () => {
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
            num_transactions: 0,
            num_utxos: 0,
            parsed_expiration_time: '1970-01-01T00:00:00',
            parser: '',
            parsing_bucket_id: 0,
            parsing_height: 0,
            parsing_hash: '0000000000000000000000000000000000000000000000000000000000000000',
            processed_position: 0,
            processed_transactions: 0,
            processed_vin: 0,
            processed_vout: 0,
            status: 0,
            synchronizer: '',
            miner: '',
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
            bits: 386089497,
            cumulative_work: '0000000000000000000172014ba58d66455762add0512355ad651207918494ab',
            hash: '0000000000000000000172014ba58d66455762add0512355ad651207918494ab',
            height: 839999,
            merkle: '5cdb277afa34ea35aa620e5cad205f18acda80b80dec9dacf4b84636a5ad0448',
            nonce: 3205594798,
            previous_block_hash: '00000000000000000001dcce6ce7c8a45872cafd1fb04732b447a14a91832591',
            timestamp: 1713571533,
            version: 671088644,
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

    it('init: missing required authority utxomng.xsat', async () => {
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
            num_transactions: 0,
            num_utxos: 0,
            parsed_expiration_time: '1970-01-01T00:00:00',
            parser: '',
            parsing_bucket_id: 0,
            parsing_height: 0,
            parsing_hash: '0000000000000000000000000000000000000000000000000000000000000000',
            processed_position: 0,
            processed_transactions: 0,
            processed_vin: 0,
            processed_vout: 0,
            status: 1,
            synchronizer: '',
            miner: '',
            num_validators_assigned: 0,
            num_provider_validators: 0,
        })
    })
})
