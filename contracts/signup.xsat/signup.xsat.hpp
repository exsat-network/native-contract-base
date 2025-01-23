#pragma once

#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include <eosio/singleton.hpp>
#include <string>

#include "../internal/utils.hpp"
#include "constants.hpp"

using namespace eosio;
using std::string;

class [[eosio::contract("signup.xsat")]] signup : public contract {
   public:
    using contract::contract;
    static constexpr name EOSIO = "eosio"_n;
    static constexpr name SUFFIX = "sao"_n;
    static constexpr symbol EOS_SYMBOL = symbol("EOS", 4);

    [[eosio::action]]
    void config(const name& recharge_account, const asset& stake_net, const asset& stake_cpu, const uint64_t buy_ram_bytes);

    [[eosio::action]]
    void addtoken(const name& contract, const asset& cost);

    [[eosio::action]]
    void deltoken(const name& contract);

    /**
     * ## ACTION `on_transfer`
     *
     * - **authority**: `*`
     *
     * > Handle incoming transfer actions
     *
     * ### params
     *
     * - `{name} from` - Sender's account name
     * - `{name} to` - Receiver's account name
     * - `{asset} quantity` - Amount transferred
     * - `{string} memo` - Transfer memo
     */
    [[eosio::on_notify("*::transfer")]]
    void on_transfer(const name& from, const name& to, const asset& quantity, const string& memo);

   private:
    struct [[eosio::table]] config_row {
        name recharge_account = "xsat"_n;
        asset stake_net = asset(1, EOS_SYMBOL);
        asset stake_cpu = asset(1, EOS_SYMBOL);
        uint64_t buy_ram_bytes = 10;
    };
    typedef singleton<"config"_n, config_row> config_index;
    config_index _config = config_index(_self, _self.value);

    struct [[eosio::table]] token_row {
        name contract;
        asset cost;
        uint64_t primary_key() const { return contract.value; }
    };
    typedef eosio::multi_index<"token"_n, token_row> token_index;
    token_index _token = token_index(_self, _self.value);

    struct signup_public_key {
        uint8_t type;
        std::array<unsigned char, 33> data;
    };

    struct permission_level_weight {
        permission_level permission;
        uint16_t weight;
    };

    struct key_weight {
        signup_public_key key;
        uint16_t weight;
    };

    struct wait_weight {
        uint32_t wait_sec;
        uint16_t weight;
    };

    struct authority {
        uint32_t threshold;
        std::vector<key_weight> keys;
        std::vector<permission_level_weight> accounts;
        std::vector<wait_weight> waits;
    };

    struct newaccount {
        name creator;
        name name;
        authority owner;
        authority active;
    };

    std::array<unsigned char, 33> validate_key(std::string key_str) {
        std::string pubkey_prefix;
        std::string base58substr;
        std::vector<unsigned char> vch;

        print_f("\n Key Length: %", key_str.length());

        if (key_str.length() == 53) {
            pubkey_prefix = "EOS";
            auto result = mismatch(pubkey_prefix.begin(), pubkey_prefix.end(), key_str.begin());
            check(result.first == pubkey_prefix.end(), "NON_EOS_PBK");
            base58substr = key_str.substr(pubkey_prefix.length());
        } else if (key_str.length() == 57) {
            pubkey_prefix = "PUB_";
            auto result = mismatch(pubkey_prefix.begin(), pubkey_prefix.end(), key_str.begin());
            check(result.first == pubkey_prefix.end(), "NON_PUB_PBK");
            base58substr = key_str.substr(pubkey_prefix.length() + 3);
        }

        check(key_str.length() == 53 || key_str.length() == 57, "PBK_SIZE_ERR");

        check(decode_base58(base58substr, vch), "DEC_FAIL");
        check(vch.size() == 37, "INV_KEY");

        std::array<unsigned char, 33> pubkey_data;

        copy_n(vch.begin(), 33, pubkey_data.begin());

        // checksum160 check_pubkey;
        // ripemd160(reinterpret_cast<char *>(pubkey_data.data()), 33, &check_pubkey);
        // check(memcmp(&check_pubkey.hash, &vch.end()[-4], 4) == 0, "INV_KEY");

        return pubkey_data;
    }

    bool DecodeBase58(const char* psz, std::vector<unsigned char>& vch) {
        // Skip leading spaces.
        while (*psz && isspace(*psz)) psz++;
        // Skip and count leading '1's.
        int zeroes = 0;
        int length = 0;
        while (*psz == '1') {
            zeroes++;
            psz++;
        }
        // Allocate enough space in big-endian base256 representation.
        int size = strlen(psz) * 733 / 1000 + 1;  // log(58) / log(256), rounded up.
        std::vector<unsigned char> b256(size);
        // Process the characters.
        static_assert(sizeof(mapBase58) / sizeof(mapBase58[0]) == 256, "mapBase58.size() should be 256");  // guarantee not out of range
        while (*psz && !isspace(*psz)) {
            // Decode base58 character
            int carry = mapBase58[(uint8_t)*psz];
            if (carry == -1)  // Invalid b58 character
                return false;
            int i = 0;
            for (std::vector<unsigned char>::reverse_iterator it = b256.rbegin(); (carry != 0 || i < length) && (it != b256.rend()); ++it, ++i) {
                carry += 58 * (*it);
                *it = carry % 256;
                carry /= 256;
            }
            check(carry == 0, "Non-zero carry");
            length = i;
            psz++;
        }
        // Skip trailing spaces.
        while (isspace(*psz)) psz++;
        if (*psz != 0) return false;
        // Skip leading zeroes in b256.
        std::vector<unsigned char>::iterator it = b256.begin() + (size - length);
        while (it != b256.end() && *it == 0) it++;
        // Copy result into output vector.
        vch.reserve(zeroes + (b256.end() - it));
        vch.assign(zeroes, 0x00);
        while (it != b256.end()) vch.push_back(*(it++));
        return true;
    }

    bool decode_base58(const std::string& str, std::vector<unsigned char>& vch) { return DecodeBase58(str.c_str(), vch); }
};
