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

module.exports = {
    getTokenBalance,
    addTime
}