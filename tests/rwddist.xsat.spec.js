const { TimePointSec } = require('@greymass/eosio')
const { Blockchain, log, expectToThrow } = require('@proton/vert')

// Vert EOS VM
const blockchain = new Blockchain()
//log.setLevel('debug');
// contracts
const contracts = {
    rwddist: blockchain.createContract('rwddist.xsat', 'tests/wasm/rwddist.xsat', true),
    staking: blockchain.createContract('staking.xsat', 'tests/wasm/staking.xsat', true),
    utxomng: blockchain.createContract('utxomng.xsat', 'tests/wasm/utxomng.xsat', true),
    eos: blockchain.createContract('eosio.token', 'tests/wasm/exsat.xsat', true),
    exsat: blockchain.createContract('exsat.xsat', 'tests/wasm/exsat.xsat', true),
}

blockchain.createAccounts('alice', 'bob')

// one-time setup
beforeAll(async () => {
    blockchain.setTime(TimePointSec.from(new Date()))

    // create BTC token
    await contracts.exsat.actions.create(['rwddist.xsat', '10000000.00000000 XSAT']).send('exsat.xsat@active')

    // create EOS token
    await contracts.eos.actions.create(['eosio.token', '10000000000.0000 EOS']).send('eosio.token@active')
    await contracts.eos.actions.issue(['eosio.token', '10000000000.0000 EOS', 'init']).send('eosio.token@active')

    // transfer EOS to account
    await contracts.eos.actions.transfer(['eosio.token', 'alice', '100000.0000 EOS', 'init']).send('eosio.token@active')
    await contracts.eos.actions.transfer(['eosio.token', 'bob', '100000.0000 EOS', 'init']).send('eosio.token@active')

    await contracts.utxomng.actions.init([839999, '0000000000000000000172014ba58d66455762add0512355ad651207918494ab', '0000000000000000000000000000000000000000753b8c1eaae701e1f0146360']).send('utxomng.xsat')
    await contracts.utxomng.actions.config([600, 100, 5000, 100, 11, 10]).send('utxomng.xsat')
})

describe('rwddist.xsat', () => {
    it('missing required authority', async () => {
        await expectToThrow(
            contracts.rwddist.actions.distribute([840000]).send('alice@active'),
            'missing required authority utxomng.xsat'
        )
    })

    it('distribute', async () => {
        await expectToThrow(
            contracts.rwddist.actions.distribute([840000]).send('utxomng.xsat@active'),
            'eosio_assert: rwddist.xsat::distribute: no rewards are being distributed'
        )
    })
})
