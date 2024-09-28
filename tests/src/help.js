const { Asset, Name, TimePointSec, Checksum256 } = require('@greymass/eosio')
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

const decodeReturn_verify = returnValue => {
    const statusLen = returnValue[0]
    const status = returnValue.subarray(1, statusLen + 1).toString()

    const reasonLen = returnValue[statusLen + 1]
    const reason = returnValue.subarray(2 + statusLen, statusLen + reasonLen + 1).toString()

    const hash = returnValue.subarray(2 + statusLen + reasonLen, returnValue.length)

    const result = {
        status,
        reason: reason,
        hash: Checksum256.from(hash).toString(),
    }
    if (status === 'verify_fail') {
        console.log(result)
    }

    return result
}

const max_chunk_size = 512 * 1024

module.exports = {
    getTokenBalance,
    addTime,
    decodeReturn_verify: decodeReturn_verify,
    max_chunk_size,
}
