const { Asset, Name, TimePointSec } = require('@greymass/eosio')
const getTokenBalance = (blockchain, account, contract, symcode) => {
    let scope = Name.from(account).value.value
    const primaryKey = Asset.SymbolCode.from(symcode).value.value
    const result = blockchain.getAccount(Name.from(contract)).tables.accounts(scope).getTableRow(primaryKey)
    if (result) {
        return Asset.from(result.balance).units.toNumber()
    }
    return 0
}

const addTime = (from, time) => {
    return TimePointSec.fromMilliseconds(from.toMilliseconds() + time.toMilliseconds())
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

const max_chunk_size = 512 * 1024

module.exports = {
    getTokenBalance,
    addTime,
    decodeReturn_addblock,
    max_chunk_size,
}
