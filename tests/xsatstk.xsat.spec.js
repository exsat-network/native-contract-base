const { Name, Asset, TimePointSec } = require('@greymass/eosio')
const { Blockchain, log, expectToThrow } = require('@proton/vert')
const { EOS, XSAT, XSAT_CONTRACT } = require('./src/constants')
const { getTokenBalance, addTime } = require('./src/help')

// Vert EOS VM
const blockchain = new Blockchain()
// log.setLevel('debug');
// contracts
const contracts = {
    xsatstk: blockchain.createContract('xsatstk.xsat', 'tests/wasm/xsatstk.xsat', true),
    endrmng: blockchain.createContract('endrmng.xsat', 'tests/wasm/endrmng.xsat', true),
    eos: blockchain.createContract('eosio.token', 'tests/wasm/exsat.xsat', true),
    xsat: blockchain.createContract('exsat.xsat', 'tests/wasm/exsat.xsat', true),
}
const UNLOCKING_DURATION = 28 * 86400

blockchain.createAccounts('alice', 'bob', 'anna', 'tony')

const get_staking = (staker) => {
    const key = Name.from(staker).value.value
    return contracts.xsatstk.tables.staking().getTableRow(key)
}

const get_release = staker => {
    const scope = Name.from(staker).value.value
    return contracts.xsatstk.tables.releases(scope).getTableRows()
}

// one-time setup
beforeAll(async () => {
    blockchain.setTime(TimePointSec.from(new Date()))

    // create XSAT token
    await contracts.xsat.actions.create(['exsat.xsat', '10000000.00000000 XSAT']).send('exsat.xsat@active')
    await contracts.xsat.actions.issue(['exsat.xsat', '10000000.00000000 XSAT', 'init']).send('exsat.xsat@active')

    // transfer XSAT to account
    await contracts.xsat.actions.transfer(['exsat.xsat', 'bob', '1000000.00000000 XSAT', '']).send('exsat.xsat@active')
    await contracts.xsat.actions.transfer(['exsat.xsat', 'alice', '1000000.00000000 XSAT', '']).send('exsat.xsat@active')
    await contracts.xsat.actions.transfer(['exsat.xsat', 'anna', '1000000.00000000 XSAT', '']).send('exsat.xsat@active')

    // create EOS token
    await contracts.eos.actions.create(['eosio.token', '10000000000.0000 EOS']).send('eosio.token@active')
    await contracts.eos.actions.issue(['eosio.token', '10000000000.0000 EOS', 'init']).send('eosio.token@active')

    // transfer EOS to account
    await contracts.eos.actions.transfer(['eosio.token', 'alice', '100000.0000 EOS', 'init']).send('eosio.token@active')
    await contracts.eos.actions.transfer(['eosio.token', 'bob', '100000.0000 EOS', 'init']).send('eosio.token@active')

    await contracts.endrmng.actions.regvalidator(['alice', 'alice', 2000]).send('alice@active')

    await contracts.xsatstk.actions.setstatus([false]).send('xsatstk.xsat@active')
})

describe('xsatstk.xsat', () => {
    it('stake: only transfer [exsat.xsat/XSAT]', async () => {
        await expectToThrow(
            contracts.eos.actions.transfer(['bob', 'xsatstk.xsat', Asset.from(100, EOS), 'bob']).send('bob@active'),
            'eosio_assert: xsatstk.xsat: only transfer [exsat.xsat/XSAT]'
        )
    })

    it('stake: invalid memo', async () => {
        await expectToThrow(
            contracts.xsat.actions.transfer(['bob', 'xsatstk.xsat', Asset.from(100, XSAT), '']).send('bob@active'),
            'eosio_assert: xsatstk.xsat: invalid memo, ex: "<validator>"'
        )
    })

    it('stake: [validators] does not exists', async () => {
        await expectToThrow(
            contracts.xsat.actions.transfer(['bob', 'xsatstk.xsat', Asset.from(100, XSAT), 'tony']).send('bob@active'),
            'eosio_assert: endrmng.xsat::stakexsat: [validators] does not exists'
        )
    })

    it('stake', async () => {
        await contracts.xsat.actions.transfer(['bob', 'xsatstk.xsat', Asset.from(1, XSAT), 'alice']).send('bob@active')
        expect(get_staking('bob')).toEqual({
            staker: 'bob',
            quantity: '1.00000000 XSAT',
        })
    })

    it('release: missing required authority', async () => {
        await expectToThrow(
            contracts.xsatstk.actions.release(['bob', 'bob', Asset.from(1, XSAT)]).send('alice@active'),
            'missing required authority bob'
        )
    })

    it('release: [staking] does not exists', async () => {
        await expectToThrow(
            contracts.xsatstk.actions.release(['alice', 'alice', Asset.from(1, XSAT)]).send('alice@active'),
            'eosio_assert: xsatstk.xsat::release: [staking] does not exists'
        )
    })

    it('release: the amount released exceeds the amount pledged', async () => {
        await expectToThrow(
            contracts.xsatstk.actions.release(['bob', 'alice', Asset.from(100, XSAT)]).send('bob@active'),
            'eosio_assert: xsatstk.xsat::release: the amount released exceeds the amount pledged'
        )
    })

    it('release', async () => {
        await contracts.xsatstk.actions.release(['bob', 'alice', Asset.from(1, XSAT)]).send('bob@active')
        expect(get_release('bob')).toEqual([
            {
                expiration_time: addTime(blockchain.timestamp, TimePointSec.from(UNLOCKING_DURATION)).toString(),
                id: 1,
                quantity: '1.00000000 XSAT'
            },
        ])
    })

    it('withdraw: missing required authority', async () => {
        await expectToThrow(
            contracts.xsatstk.actions.withdraw(['bob']).send('alice@active'),
            'missing required authority bob'
        )
    })

    it('withdraw: there is no expired token that can be withdrawn', async () => {
        await expectToThrow(
            contracts.xsatstk.actions.withdraw(['bob']).send('bob@active'),
            'eosio_assert: xsatstk.xsat::withdraw: there is no expired token that can be withdrawn'
        )
    })

    it('withdraw', async () => {
        blockchain.addTime(TimePointSec.fromInteger(UNLOCKING_DURATION))
        await contracts.xsat.actions.transfer(['bob', 'xsatstk.xsat', Asset.from(1, XSAT), 'alice']).send('bob@active')
        await contracts.xsatstk.actions.release(['bob', 'alice', Asset.from(1, XSAT)]).send('bob@active')

        const before_balance = getTokenBalance(blockchain, 'bob', 'exsat.xsat', XSAT.code)
        await contracts.xsatstk.actions.withdraw(['bob']).send('bob@active')
        const after_balance = getTokenBalance(blockchain, 'bob', 'exsat.xsat', XSAT.code)
        expect(after_balance - before_balance).toEqual(Asset.from(1, XSAT).units.toNumber())

        expect(get_staking('bob')).toEqual({ staker: 'bob', quantity: '0.00000000 XSAT' })
        expect(get_release('bob')).toEqual([
            {
                expiration_time: addTime(blockchain.timestamp, TimePointSec.from(UNLOCKING_DURATION)).toString(),
                id: 2,
                quantity: '1.00000000 XSAT'
            },
        ])
    })

    it('setstatus: missing required authority', async () => {
        await expectToThrow(
            contracts.xsatstk.actions.setstatus([true]).send('bob@active'),
            'missing required authority xsatstk.xsat'
        )
    })

    it('disabled_staking: the current xsat staking status is disabled', async () => {
        await contracts.xsatstk.actions.setstatus([true]).send('xsatstk.xsat@active')
        await expectToThrow(
            contracts.xsat.actions.transfer(['bob', 'xsatstk.xsat', Asset.from(1, XSAT), 'alice']).send('bob@active'),
            'eosio_assert: xsatstk.xsat: the current xsat staking status is disabled'
        )
    })
})
