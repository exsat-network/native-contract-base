#include <vector>
#include <string>
#include <bitcoin/script/script.hpp>
#include <bitcoin/utility/base58.hpp>
#include <bitcoin/utility/bech32.hpp>

typedef std::vector<unsigned char> valtype;

namespace bitcoin {

/** Signature hash sizes */
static constexpr size_t WITNESS_V0_SCRIPTHASH_SIZE = 32;
static constexpr size_t WITNESS_V0_KEYHASH_SIZE = 20;
static constexpr size_t WITNESS_V1_TAPROOT_SIZE = 32;

namespace CPubKey {

/**
 * secp256k1:
 */
static constexpr unsigned int SIZE = 65;
static constexpr unsigned int COMPRESSED_SIZE = 33;
static constexpr unsigned int SIGNATURE_SIZE = 72;
static constexpr unsigned int COMPACT_SIGNATURE_SIZE = 65;

//! Compute the length of a pubkey with a given first byte.
unsigned int static GetLen(unsigned char chHeader) {
    if (chHeader == 2 || chHeader == 3) return COMPRESSED_SIZE;
    if (chHeader == 4 || chHeader == 6 || chHeader == 7) return SIZE;
    return 0;
}

bool static ValidSize(const std::vector<unsigned char>& vch) { return vch.size() > 0 && GetLen(vch[0]) == vch.size(); }

}  // namespace CPubKey

enum class TxoutType {
    NONSTANDARD,
    // 'standard' transaction types:
    PUBKEY,
    PUBKEYHASH,
    SCRIPTHASH,
    MULTISIG,
    NULL_DATA,  //!< unspendable OP_RETURN script that carries data
    WITNESS_V0_SCRIPTHASH,
    WITNESS_V0_KEYHASH,
    WITNESS_V1_TAPROOT,
    WITNESS_UNKNOWN,  //!< Only for Witness versions not already defined above
};

static bool MatchPayToPubkey(const std::vector<unsigned char>& script, valtype& pubkey) {
    if (script.size() == CPubKey::SIZE + 2 && script[0] == CPubKey::SIZE && script.back() == OP_CHECKSIG) {
        pubkey = valtype(script.begin() + 1, script.begin() + CPubKey::SIZE + 1);
        return CPubKey::ValidSize(pubkey);
    }
    if (script.size() == CPubKey::COMPRESSED_SIZE + 2 && script[0] == CPubKey::COMPRESSED_SIZE
        && script.back() == OP_CHECKSIG) {
        pubkey = valtype(script.begin() + 1, script.begin() + CPubKey::COMPRESSED_SIZE + 1);
        return CPubKey::ValidSize(pubkey);
    }
    return false;
}

static bool MatchPayToPubkeyHash(const std::vector<unsigned char>& script, valtype& pubkeyhash) {
    if (script.size() == 25 && script[0] == OP_DUP && script[1] == OP_HASH160 && script[2] == 20
        && script[23] == OP_EQUALVERIFY && script[24] == OP_CHECKSIG) {
        pubkeyhash = valtype(script.begin() + 3, script.begin() + 23);
        return true;
    }
    return false;
}

/** Test for "small positive integer" script opcodes - OP_1 through OP_16. */
static constexpr bool IsSmallInteger(opcodetype opcode) { return opcode >= OP_1 && opcode <= OP_16; }

static bool MatchMultisig(const std::vector<unsigned char>& script, unsigned int& required,
                          std::vector<valtype>& pubkeys) {
    opcodetype opcode;
    valtype data;
    std::vector<unsigned char>::const_iterator it = script.begin();
    if (script.size() < 1 || script.back() != OP_CHECKMULTISIG) return false;

    if (!GetScriptOp(it, script.end(), opcode, &data) || !IsSmallInteger(opcode)) return false;
    required = DecodeOP_N(opcode);
    while (GetScriptOp(it, script.end(), opcode, &data) && CPubKey::ValidSize(data)) {
        pubkeys.emplace_back(std::move(data));
    }
    if (!IsSmallInteger(opcode)) return false;
    unsigned int keys = DecodeOP_N(opcode);
    if (pubkeys.size() != keys || keys < required) return false;
    return (it + 1 == script.end());
}

bool IsPayToScriptHash(const std::vector<unsigned char>& scriptHash) {
    // Extra-fast test for pay-to-script-hash CScripts:
    return (scriptHash.size() == 23 && scriptHash[0] == OP_HASH160 && scriptHash[1] == 0x14
            && scriptHash[22] == OP_EQUAL);
}

bool IsPayToWitnessScriptHash(const std::vector<unsigned char>& scriptHash) {
    // Extra-fast test for pay-to-witness-script-hash CScripts:
    return (scriptHash.size() == 34 && scriptHash[0] == OP_0 && scriptHash[1] == 0x20);
}

// A witness program is any valid CScript that consists of a 1-byte push opcode
// followed by a data push between 2 and 40 bytes.
bool IsWitnessProgram(const std::vector<unsigned char>& scriptHash, int& version, std::vector<unsigned char>& program) {
    if (scriptHash.size() < 4 || scriptHash.size() > 42) {
        return false;
    }
    if (scriptHash[0] != OP_0 && (scriptHash[0] < OP_1 || scriptHash[0] > OP_16)) {
        return false;
    }
    if ((size_t)(scriptHash[1] + 2) == scriptHash.size()) {
        version = DecodeOP_N((opcodetype)scriptHash[0]);
        program = std::vector<unsigned char>(scriptHash.begin() + 2, scriptHash.end());
        return true;
    }
    return false;
}

bool IsPushOnly(std::vector<unsigned char>::const_iterator pc, const std::vector<unsigned char>::const_iterator end) {
    while (pc < end) {
        opcodetype opcode;
        if (!GetScriptOp(pc, end, opcode, nullptr)) return false;
        // Note that IsPushOnly() *does* consider OP_RESERVED to be a
        // push-type opcode, however execution of OP_RESERVED fails, so
        // it's not relevant to P2SH/BIP62 as the scriptSig would fail prior to
        // the P2SH special validation code being executed.
        if (opcode > OP_16) return false;
    }
    return true;
}

TxoutType Solver(const std::vector<unsigned char>& scriptPubKey,
                 std::vector<std::vector<unsigned char>>& vSolutionsRet) {
    vSolutionsRet.clear();

    // Shortcut for pay-to-script-hash, which are more constrained than the other types:
    // it is always OP_HASH160 20 [20 byte hash] OP_EQUAL
    if (IsPayToScriptHash(scriptPubKey)) {
        std::vector<unsigned char> hashBytes(scriptPubKey.begin() + 2, scriptPubKey.begin() + 22);
        vSolutionsRet.push_back(hashBytes);
        return TxoutType::SCRIPTHASH;
    }

    int witnessversion;
    std::vector<unsigned char> witnessprogram;
    if (IsWitnessProgram(scriptPubKey, witnessversion, witnessprogram)) {
        if (witnessversion == 0 && witnessprogram.size() == WITNESS_V0_KEYHASH_SIZE) {
            vSolutionsRet.push_back(witnessprogram);
            return TxoutType::WITNESS_V0_KEYHASH;
        }
        if (witnessversion == 0 && witnessprogram.size() == WITNESS_V0_SCRIPTHASH_SIZE) {
            vSolutionsRet.push_back(witnessprogram);
            return TxoutType::WITNESS_V0_SCRIPTHASH;
        }
        if (witnessversion == 1 && witnessprogram.size() == WITNESS_V1_TAPROOT_SIZE) {
            vSolutionsRet.push_back(std::vector<unsigned char>{(unsigned char)witnessversion});
            vSolutionsRet.push_back(std::move(witnessprogram));
            return TxoutType::WITNESS_V1_TAPROOT;
        }
        if (witnessversion != 0) {
            vSolutionsRet.push_back(std::vector<unsigned char>{(unsigned char)witnessversion});
            vSolutionsRet.push_back(std::move(witnessprogram));
            return TxoutType::WITNESS_UNKNOWN;
        }
        return TxoutType::NONSTANDARD;
    }

    // Provably prunable, data-carrying output
    //
    // So long as script passes the IsUnspendable() test and all but the first
    // byte passes the IsPushOnly() test we don't care what exactly is in the
    // script.
    if (scriptPubKey.size() >= 1 && scriptPubKey[0] == OP_RETURN
        && IsPushOnly(scriptPubKey.begin() + 1, scriptPubKey.end())) {
        return TxoutType::NULL_DATA;
    }

    std::vector<unsigned char> data;
    if (MatchPayToPubkey(scriptPubKey, data)) {
        vSolutionsRet.push_back(std::move(data));
        return TxoutType::PUBKEY;
    }

    if (MatchPayToPubkeyHash(scriptPubKey, data)) {
        vSolutionsRet.push_back(std::move(data));
        return TxoutType::PUBKEYHASH;
    }

    unsigned int required;
    std::vector<std::vector<unsigned char>> keys;
    if (MatchMultisig(scriptPubKey, required, keys)) {
        vSolutionsRet.push_back({static_cast<unsigned char>(required)});  // safe as required is in range 1..16
        vSolutionsRet.insert(vSolutionsRet.end(), keys.begin(), keys.end());
        vSolutionsRet.push_back({static_cast<unsigned char>(keys.size())});  // safe as size is in range 1..16
        return TxoutType::MULTISIG;
    }

    vSolutionsRet.clear();
    return TxoutType::NONSTANDARD;
}

/** Convert from one power-of-2 number base to another. */
template <int frombits, int tobits, bool pad, typename O, typename I>
bool ConvertBits(const O& outfn, I it, I end) {
    size_t acc = 0;
    size_t bits = 0;
    constexpr size_t maxv = (1 << tobits) - 1;
    constexpr size_t max_acc = (1 << (frombits + tobits - 1)) - 1;
    while (it != end) {
        acc = ((acc << frombits) | *it) & max_acc;
        bits += frombits;
        while (bits >= tobits) {
            bits -= tobits;
            outfn((acc >> bits) & maxv);
        }
        ++it;
    }
    if (pad) {
        if (bits) outfn((acc << (tobits - bits)) & maxv);
    } else if (bits >= frombits || ((acc << (tobits - bits)) & maxv)) {
        return false;
    }
    return true;
}

#if defined(TESTNET)
const unsigned char PKHashPrefix = 0x6f;      // 0x00 for mainnet,  0x6f for testnet
const unsigned char ScriptHashPrefix = 0xcf;  // 0x05 for mainnet, 0xc4 for testnet
const std::string Bech32HRP = "tb";           // "bc" for mainnet, "tb" for testnet
#elif defined(MAINNET) || !defined(TESTNET)
const unsigned char PKHashPrefix = 0x00;      // 0x00 for mainnet,  0x6f for testnet
const unsigned char ScriptHashPrefix = 0x05;  // 0x05 for mainnet, 0xc4 for testnet
const std::string Bech32HRP = "bc";           // "bc" for mainnet, "tb" for testnet
#endif

bool ExtractDestination(const std::vector<unsigned char>& scriptPubKey, std::vector<std::string>& addressRet) {
    std::vector<valtype> vSolutions;
    TxoutType whichType = Solver(scriptPubKey, vSolutions);

    if (whichType == TxoutType::PUBKEY) {
        if (vSolutions[0].size() == 0) {
            return false;
        }
        std::vector<unsigned char> data = {PKHashPrefix};
        data.insert(data.end(), vSolutions[0].begin(), vSolutions[0].end());
        addressRet.push_back(bitcoin::EncodeBase58Check(data));

        return true;
    } else if (whichType == TxoutType::PUBKEYHASH) {
        std::vector<unsigned char> data = {PKHashPrefix};
        data.insert(data.end(), vSolutions[0].begin(), vSolutions[0].begin() + 20);
        addressRet.push_back(bitcoin::EncodeBase58Check(data));

        return true;
    } else if (whichType == TxoutType::SCRIPTHASH) {
        std::vector<unsigned char> data = {ScriptHashPrefix};
        data.insert(data.end(), vSolutions[0].begin(), vSolutions[0].begin() + 20);
        addressRet.push_back(bitcoin::EncodeBase58Check(data));
        return true;
    } else if (whichType == TxoutType::WITNESS_V0_KEYHASH) {
        std::vector<unsigned char> data = {0};
        data.reserve(33);
        ConvertBits<8, 5, true>(
            [&](unsigned char c) {
                data.push_back(c);
            },
            vSolutions[0].begin(), vSolutions[0].end());
        addressRet.push_back(bitcoin::bech32::Encode(bech32::Encoding::BECH32, Bech32HRP, data));
        return true;
    } else if (whichType == TxoutType::WITNESS_V0_SCRIPTHASH) {
        std::vector<unsigned char> data = {0};
        data.reserve(53);
        ConvertBits<8, 5, true>(
            [&](unsigned char c) {
                data.push_back(c);
            },
            vSolutions[0].begin(), vSolutions[0].end());

        addressRet.push_back(bitcoin::bech32::Encode(bech32::Encoding::BECH32, Bech32HRP, data));
        return true;
    } else if (whichType == TxoutType::WITNESS_UNKNOWN || whichType == TxoutType::WITNESS_V1_TAPROOT) {
        unsigned int version = vSolutions[0][0];
        unsigned int length = vSolutions[1].size();

        if (version < 1 || version > 16 || length < 2 || length > 40) {
            return false;
        }
        std::vector<unsigned char> data = {(unsigned char)version};
        data.reserve(1 + (length * 8 + 4) / 5);
        ConvertBits<8, 5, true>(
            [&](unsigned char c) {
                data.push_back(c);
            },
            vSolutions[1].begin(), vSolutions[1].begin() + length);
        addressRet.push_back(bitcoin::bech32::Encode(bech32::Encoding::BECH32M, Bech32HRP, data));
        return true;
    } else if (whichType == TxoutType::MULTISIG) {
        for (unsigned int i = 1; i < vSolutions.size() - 1; i++) {
            if (vSolutions[i].size() == 0) {
                continue;
            }
            std::vector<unsigned char> data = {PKHashPrefix};
            data.insert(data.end(), vSolutions[i].begin(), vSolutions[i].end());
            addressRet.push_back(bitcoin::EncodeBase58Check(data));
        }

        if (addressRet.empty()) return false;
    }
    // Multisig txns have more than one address...
    return false;
}
}  // namespace bitcoin