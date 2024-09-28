const { Asset } = require('@greymass/eosio')


const BTC_CONTRACT = 'btc.xsat'
const XSAT_CONTRACT = 'exsat.xsat'
const RESCMNG_CONTRACT = 'rescmng.xsat'
const RWDDIST_CONTRACT = 'rwddist.xsat'
const POOLREG_CONTRACT = 'poolreg.xsat'
const BLKSYNC_CONTRACT = 'blksync.xsat'
const BLKENDT_CONTRACT = 'blkendt.xsat'
const ENDRMNG_CONTRACT = 'endrmng.xsat'
const STAKING_CONTRACT = 'staking.xsat'
const UTXO_CONTRACT = 'utxomng.xsat'
const BRDGMNG_CONTRACT = 'brdgmng.xsat'

const DECIMAL = 1000000000000
const EOS = Asset.Symbol.fromParts('EOS', 4)
const BTC = Asset.Symbol.fromParts('BTC', 8)
const XSAT = Asset.Symbol.fromParts('XSAT', 8)
const ZERO_BTC = Asset.from(0, BTC).toString()
const ZERO_XSAT = Asset.from(0, XSAT).toString()

module.exports = {
    BTC_CONTRACT,
    XSAT_CONTRACT,
    RESCMNG_CONTRACT,
    RWDDIST_CONTRACT,
    POOLREG_CONTRACT,
    BLKENDT_CONTRACT,
    BLKSYNC_CONTRACT,
    ENDRMNG_CONTRACT,
    STAKING_CONTRACT,
    UTXO_CONTRACT,
    BRDGMNG_CONTRACT,
    DECIMAL,
    EOS,
    BTC,
    XSAT: XSAT,
    ZERO_BTC,
    ZERO_XSAT: ZERO_XSAT
}
