const { Name, Asset, TimePointSec } = require('@greymass/eosio')
const { Blockchain, log, expectToThrow } = require('@proton/vert')
const { EOS, BTC, BTC_CONTRACT } = require('./src/constants')
const { getTokenBalance, addTime } = require('./src/help')

// Vert EOS VM
const blockchain = new Blockchain()
// log.setLevel('debug');
// contracts
const contracts = {
    staking: blockchain.createContract('staking.xsat', 'tests/wasm/staking.xsat', true),
    endrmng: blockchain.createContract('endrmng.xsat', 'tests/wasm/endrmng.xsat', true),
    eos: blockchain.createContract('eosio.token', 'tests/wasm/btc.xsat', true),
    btc: blockchain.createContract('btc.xsat', 'tests/wasm/btc.xsat', true),
}
const UNLOCKING_DURATION = 28 * 86400

blockchain.createAccounts('alice', 'bob', 'anna', 'tony')

const get_token = () => {
    return contracts.staking.tables.tokens().getTableRows()
}

const get_staking = (staker, id) => {
    const scope = Name.from(staker).value.value
    return contracts.staking.tables.staking(scope).getTableRow(BigInt(id))
}

const get_release = staker => {
    const scope = Name.from(staker).value.value
    return contracts.staking.tables.releases(scope).getTableRows()
}

// one-time setup
beforeAll(async () => {
    blockchain.setTime(TimePointSec.from(new Date()))

    // create BTC token
    await contracts.btc.actions.create(['btc.xsat', '10000000.00000000 BTC']).send('btc.xsat@active')
    await contracts.btc.actions.issue(['btc.xsat', '10000000.00000000 BTC', 'init']).send('btc.xsat@active')

    // transfer BTC to account
    await contracts.btc.actions.transfer(['btc.xsat', 'bob', '1000000.00000000 BTC', '']).send('btc.xsat@active')
    await contracts.btc.actions.transfer(['btc.xsat', 'alice', '1000000.00000000 BTC', '']).send('btc.xsat@active')
    await contracts.btc.actions.transfer(['btc.xsat', 'anna', '1000000.00000000 BTC', '']).send('btc.xsat@active')

    // create EOS token
    await contracts.eos.actions.create(['eosio.token', '10000000000.0000 EOS']).send('eosio.token@active')
    await contracts.eos.actions.issue(['eosio.token', '10000000000.0000 EOS', 'init']).send('eosio.token@active')

    // transfer EOS to account
    await contracts.eos.actions.transfer(['eosio.token', 'alice', '100000.0000 EOS', 'init']).send('eosio.token@active')
    await contracts.eos.actions.transfer(['eosio.token', 'bob', '100000.0000 EOS', 'init']).send('eosio.token@active')

    await contracts.endrmng.actions.regvalidator(['alice', 'alice', 2000]).send('alice@active')
})

describe('staking.xsat', () => {
    it('addtoken: missing required authority', async () => {
        await expectToThrow(
            contracts.staking.actions.addtoken([{ sym: BTC, contract: BTC_CONTRACT }]).send('bob@active'),
            'missing required authority staking.xsat'
        )
    })

    it('addtoken', async () => {
        await contracts.staking.actions.addtoken([{ sym: BTC, contract: BTC_CONTRACT }]).send('staking.xsat@active')
        expect(get_token()).toEqual([
            {
                id: 1,
                disabled_staking: false,
                token: {
                    contract: 'btc.xsat',
                    sym: '8,BTC',
                },
            },
        ])
    })

    it('deltoken: missing required authority', async () => {
        await expectToThrow(
            contracts.staking.actions.deltoken([1]).send('bob@active'),
            'missing required authority staking.xsat'
        )
    })

    it('deltoken: [tokens] does not exists', async () => {
        await expectToThrow(
            contracts.staking.actions.deltoken([2]).send('staking.xsat@active'),
            'eosio_assert: staking.xsat::deltoken: [tokens] does not exists'
        )
    })

    it('deltoken', async () => {
        await contracts.staking.actions.deltoken([1]).send('staking.xsat@active')
        expect(get_token()).toEqual([])
    })

    it('stake: invalid memo', async () => {
        await expectToThrow(
            contracts.btc.actions.transfer(['bob', 'staking.xsat', Asset.from(100, BTC), '']).send('bob@active'),
            'eosio_assert: staking.xsat: invalid memo, ex: "<validator>"'
        )
    })

    it('stake: token does not support staking', async () => {
        await expectToThrow(
            contracts.eos.actions.transfer(['bob', 'staking.xsat', Asset.from(100, EOS), 'bob']).send('bob@active'),
            'eosio_assert: staking.xsat: token does not support staking'
        )
    })

    it('stake: [validators] does not exists', async () => {
        await contracts.staking.actions.addtoken([{ sym: BTC, contract: BTC_CONTRACT }]).send('staking.xsat@active')
        await expectToThrow(
            contracts.btc.actions.transfer(['bob', 'staking.xsat', Asset.from(100, BTC), 'tony']).send('bob@active'),
            'eosio_assert: endrmng.xsat::stake: [validators] does not exists'
        )
    })

    it('stake', async () => {
        await contracts.btc.actions.transfer(['bob', 'staking.xsat', Asset.from(1, BTC), 'alice']).send('bob@active')
        expect(get_staking('bob', 1)).toEqual({
            id: 1,
            quantity: {
                contract: 'btc.xsat',
                quantity: '1.00000000 BTC',
            },
        })
    })

    it('release: missing required authority', async () => {
        await expectToThrow(
            contracts.staking.actions.release([1, 'bob', 'bob', '1.00000000 BTC']).send('alice@active'),
            'missing required authority bob'
        )
    })

    it('release: staking_id does not exists', async () => {
        await expectToThrow(
            contracts.staking.actions.release([2, 'bob', 'alice', Asset.from(1, BTC)]).send('bob@active'),
            'eosio_assert: staking.xsat::release: [staking] does not exists'
        )
    })

    it('release: the amount released exceeds the amount pledged', async () => {
        await expectToThrow(
            contracts.staking.actions.release([1, 'bob', 'alice', Asset.from(100, BTC)]).send('bob@active'),
            'eosio_assert: staking.xsat::release: the amount released exceeds the amount pledged'
        )
    })

    it('release', async () => {
        await contracts.staking.actions.release([1, 'bob', 'alice', Asset.from(1, BTC)]).send('bob@active')
        expect(get_release('bob')).toEqual([
            {
                expiration_time: addTime(blockchain.timestamp, TimePointSec.from(UNLOCKING_DURATION)).toString(),
                id: 1,
                quantity: { contract: 'btc.xsat', quantity: '1.00000000 BTC' },
            },
        ])
    })

    it('withdraw: missing required authority', async () => {
        await expectToThrow(
            contracts.staking.actions.withdraw(['bob']).send('alice@active'),
            'missing required authority bob'
        )
    })

    it('withdraw: there is no expired token that can be withdrawn', async () => {
        await expectToThrow(
            contracts.staking.actions.withdraw(['bob']).send('bob@active'),
            'eosio_assert: staking.xsat::withdraw: there is no expired token that can be withdrawn'
        )
    })

    it('withdraw', async () => {
        blockchain.addTime(TimePointSec.fromInteger(UNLOCKING_DURATION))
        await contracts.btc.actions.transfer(['bob', 'staking.xsat', Asset.from(1, BTC), 'alice']).send('bob@active')
        await contracts.staking.actions.release([1, 'bob', 'alice', Asset.from(1, BTC)]).send('bob@active')

        const before_balance = getTokenBalance(blockchain, 'bob', 'btc.xsat', BTC.code)
        await contracts.staking.actions.withdraw(['bob']).send('bob@active')
        const after_balance = getTokenBalance(blockchain, 'bob', 'btc.xsat', BTC.code)
        expect(after_balance - before_balance).toEqual(Asset.from(1, BTC).units.toNumber())

        expect(get_staking('bob', 1)).toEqual({ id: 1, quantity: { contract: 'btc.xsat', quantity: '0.00000000 BTC' } })
        expect(get_release('bob')).toEqual([
            {
                expiration_time: addTime(blockchain.timestamp, TimePointSec.from(UNLOCKING_DURATION)).toString(),
                id: 2,
                quantity: {
                    contract: 'btc.xsat',
                    quantity: '1.00000000 BTC',
                },
            },
        ])
    })

    it('setstatus: missing required authority', async () => {
        await expectToThrow(
            contracts.staking.actions.setstatus([1, true]).send('bob@active'),
            'missing required authority staking.xsat'
        )
    })

    it('disabled_staking: this token is prohibited from staking', async () => {
        await contracts.staking.actions.setstatus([1, true]).send('staking.xsat@active')
        await expectToThrow(
            contracts.btc.actions.transfer(['bob', 'staking.xsat', Asset.from(1, BTC), 'alice']).send('bob@active'),
            'eosio_assert: staking.xsat: this token is prohibited from staking'
        )
    })

    it('deltoken: there is currently a balance that has not been withdrawn', async () => {
        await contracts.staking.actions.setstatus([1, false]).send('staking.xsat@active')
        await contracts.btc.actions.transfer(['bob', 'staking.xsat', Asset.from(1, BTC), 'alice']).send('bob@active')
        await expectToThrow(
            contracts.staking.actions.deltoken([1]).send('staking.xsat@active'),
            'eosio_assert: staking.xsat::deltoken: there is currently a balance that has not been withdrawn'
        )
    })
})
