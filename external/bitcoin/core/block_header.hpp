#pragma once

#include <cstdint>
#include <array>
#include <bitcoin/core/transaction.hpp>
#include <bitcoin/utility/crypto.hpp>
#include <bitcoin/utility/types.hpp>
#include <eosio/datastream.hpp>
#include <eosio/crypto.hpp>
#include <eosio/serialize.hpp>

namespace bitcoin::core {

    bitcoin::uint256_t generate_header_merkle(const std::vector<bitcoin::core::transaction>& transactions) {
        auto transaction_hashes = std::vector<bitcoin::uint256_t>();
        transaction_hashes.reserve(transactions.size());
        for (const auto& transaction : transactions) {
            auto hash = transaction.merkle_hash();
            transaction_hashes.emplace_back(std::move(hash));
        }

        return bitcoin::generate_merkle_root(transaction_hashes);
    }

    bitcoin::uint256_t generate_witness_merkle(std::vector<eosio::checksum256>& hashes,
                                               const eosio::checksum256& witness_reserved_value) {
        bitcoin::uint256_t witness_merkle = generate_merkle_root(hashes);
        auto concatenated_hashes = std::array<uint8_t, 64>();
        auto ds = eosio::datastream<uint8_t*>(concatenated_hashes.data(), concatenated_hashes.size());
        ds << witness_merkle << witness_reserved_value;

        return bitcoin::dhash(concatenated_hashes);
    }

    bitcoin::uint256_t generate_witness_merkle(const std::vector<bitcoin::core::transaction>& transactions) {
        auto transaction_hashes = std::vector<bitcoin::uint256_t>();
        transaction_hashes.reserve(transactions.size());

        const bool include_coinbase = transactions.front().is_coinbase();
        if (include_coinbase) {
            // coinbase transaction has a zero hash
            transaction_hashes.emplace_back(0);
        }
        for (uint64_t i = include_coinbase; i < transactions.size(); i++) {
            auto hash = transactions[i].hash();
            transaction_hashes.emplace_back(std::move(hash));
        }

        auto cur_hashes = std::vector<bitcoin::uint256_t>(transaction_hashes);
        return bitcoin::generate_merkle_root(cur_hashes);
    }
    struct block_header {
        uint32_t version;
        bitcoin::uint256_t previous_block_hash;
        bitcoin::uint256_t merkle;
        uint32_t timestamp;
        uint32_t bits;
        uint32_t nonce;

        bitcoin::uint256_t hash() const {
            auto header_data = std::array<uint8_t, 80>();
            auto ds = eosio::datastream<uint8_t*>(header_data.data(), header_data.size());
            ds << *this;

            return bitcoin::dhash(header_data);
        }

        bitcoin::uint256_t target() const { return bitcoin::compact::expand(bits); }

        bitcoin::uint256_t work() const {
            // refer to
            // @link
            // https://github.com/libbitcoin/libbitcoin-system/blob/93af70d36316f39be4f5e28375bb007590beda5e/src/chain/header.cpp#L255-L276
            // for the original implementation
            const auto target_hash = target();
            if (target_hash == 0) {
                return 0;
            }

            return ((~target_hash) / (target_hash + 1)) + 1;
        }

        bool target_is_valid() const { return hash() <= target(); }

        bool transactions_are_valid(const std::vector<bitcoin::core::transaction>& transactions) const {
            return generate_header_merkle(transactions) == merkle;
        }

        bool transactions_are_valid(const std::vector<bitcoin::core::transaction>& transactions,
                                    const uint256_t& witness_reserved_value, const uint256_t& witness_commitment) {
            bitcoin::uint256_t witness_merkle = generate_witness_merkle(transactions);
            auto concatenated_hashes = std::array<uint8_t, 64>();
            auto ds = eosio::datastream<uint8_t*>(concatenated_hashes.data(), concatenated_hashes.size());
            ds << witness_merkle << witness_reserved_value;

            auto expected_witness_commitment = bitcoin::dhash(concatenated_hashes);

            return witness_commitment == expected_witness_commitment;
        }

        EOSLIB_SERIALIZE(block_header, (version)(previous_block_hash)(merkle)(timestamp)(bits)(nonce))
    };

};  // namespace bitcoin::core