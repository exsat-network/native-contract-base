#pragma once

#include <cstdint>
#include <vector>
#include <optional>
#include <bitcoin/utility/crypto.hpp>
#include <bitcoin/utility/types.hpp>
#include <eosio/datastream.hpp>
#include <eosio/crypto.hpp>
#include <eosio/serialize.hpp>

namespace bitcoin::core {
struct script {
    std::vector<uint8_t> data;

    // see below for serialization
};

struct transaction_input {
    bitcoin::uint256_t previous_output_hash;
    uint32_t previous_output_index;
    script script_sig;
    uint32_t sequence;

    EOSLIB_SERIALIZE(transaction_input, (previous_output_hash)(previous_output_index)(script_sig)(sequence))
};

struct transaction_output {
    uint64_t value;
    script script;

    EOSLIB_SERIALIZE(transaction_output, (value)(script))
};

struct transaction_witness {
    std::vector<std::vector<uint8_t>> data;

    // see below for serialization
};

struct transaction {
    uint32_t version;
    std::vector<transaction_input> inputs;
    std::vector<transaction_output> outputs;
    std::vector<transaction_witness> witness;
    uint32_t locktime;

    template <typename Stream>
    void serialize_for_merkle(eosio::datastream<Stream>& ds) const {
        ds << version;
        bitcoin::varint::encode(ds, inputs.size());
        for (const auto& input : inputs) {
            ds << input;
        }

        bitcoin::varint::encode(ds, outputs.size());
        for (const auto& output : outputs) {
            ds << output;
        }
        ds << locktime;
    }

    uint256_t merkle_hash() const {
        eosio::datastream<size_t> ps;
        serialize_for_merkle(ps);
        auto serialized_size = ps.tellp();
        std::vector<char> hash_data;
        hash_data.resize(serialized_size);
        eosio::datastream<char*> ds(hash_data.data(), hash_data.size());
        serialize_for_merkle(ds);
        return bitcoin::dhash(hash_data);
    }

    uint256_t hash() const {
        auto hash_data = eosio::pack(*this);
        return bitcoin::dhash(hash_data);
    }
    bool is_coinbase() const { return (inputs.size() == 1 && inputs[0].previous_output_hash == uint256_t(0)); }

    std::optional<eosio::checksum256> get_witness_reserve_value() const {
        if (witness.size() < 1) {
            // coinbase has no witness data
            return std::nullopt;
        }

        // extract first transaction's witness
        const auto& _witness = witness.front();
        if (_witness.data.size() != 1) {
            // coinbase first witness does not have exactly 1 stack item
            return std::nullopt;
        }

        if (_witness.data.front().size() != 32) {
            // coinbase first witness only stack item is not 32 bytes
            return std::nullopt;
        }

        eosio::checksum256 result;
        eosio::datastream<const char*> ds((const char*)_witness.data.front().data(), _witness.data.front().size());
        ds >> result;
        return result;
    }

    std::optional<eosio::checksum256> get_witness_commitment() const {
        for (int idx = outputs.size() - 1; idx >= 0; idx--) {
            const auto& output = outputs[idx];
            if (output.script.data.size() < 34) {
                continue;
            }

            constexpr auto witness_commitment_header = std::array<uint8_t, 6>{0x6a, 0x24, 0xaa, 0x21, 0xa9, 0xed};

            if (0
                != std::memcmp(output.script.data.data(), witness_commitment_header.data(),
                               witness_commitment_header.size())) {
                continue;
            }

            eosio::checksum256 result;
            eosio::datastream<const char*> ds((const char*)output.script.data.data() + 6, 32);
            ds >> result;
            return result;
        }

        return std::nullopt;
    };

    // see below for serialization
};
};  // namespace bitcoin::core

namespace eosio {
/**
 *  Serialize a witness
 *
 *  @param ds - The stream to write
 *  @param v - The value to serialize
 *  @tparam Stream - Type of datastream buffer
 *  @return datastream<Stream>& - Reference to the datastream
 */
template <typename Stream>
datastream<Stream>& operator<<(datastream<Stream>& ds, const bitcoin::core::transaction& v) {
    ds << v.version;
    if (v.witness.size() > 0) {
        ds << (uint16_t)0x0100;
    }

    bitcoin::varint::encode(ds, v.inputs.size());
    for (const auto& input : v.inputs) {
        ds << input;
    }

    bitcoin::varint::encode(ds, v.outputs.size());
    for (const auto& output : v.outputs) {
        ds << output;
    }

    if (v.witness.size() > 0) {
        for (const auto& w : v.witness) {
            ds << w;
        }
    }

    ds << v.locktime;
    return ds;
}

/**
 *  Deserialize a witness
 *
 *  @param ds - The stream to read
 *  @param v - The destination for deserialized value
 *  @tparam Stream - Type of datastream buffer
 *  @return datastream<Stream>& - Reference to the datastream
 */
template <typename Stream>
datastream<Stream>& operator>>(datastream<Stream>& ds, bitcoin::core::transaction& v) {
    ds >> v.version;
    auto rewind = ds.tellp();
    uint16_t has_witness;
    ds >> has_witness;
    if (has_witness != 0x0100) {
        ds.seekp(rewind);
    }

    auto input_count = bitcoin::varint::decode(ds);
    v.inputs.reserve(input_count);
    for (auto i = 0; i < input_count; i++) {
        bitcoin::core::transaction_input input;
        ds >> input;
        v.inputs.push_back(input);
    }

    auto output_count = bitcoin::varint::decode(ds);
    v.outputs.reserve(output_count);
    for (auto i = 0; i < output_count; i++) {
        bitcoin::core::transaction_output output;
        ds >> output;
        v.outputs.push_back(output);
    }

    if (has_witness == 0x0100) {
        v.witness.reserve(input_count);
        for (auto i = 0; i < input_count; i++) {
            bitcoin::core::transaction_witness witness;
            ds >> witness;
            v.witness.push_back(witness);
        }
    }

    ds >> v.locktime;
    return ds;
}

/**
 *  Serialize a witness
 *
 *  @param ds - The stream to write
 *  @param v - The value to serialize
 *  @tparam Stream - Type of datastream buffer
 *  @return datastream<Stream>& - Reference to the datastream
 */
template <typename Stream>
datastream<Stream>& operator<<(datastream<Stream>& ds, const bitcoin::core::transaction_witness& v) {
    bitcoin::varint::encode(ds, v.data.size());
    for (const auto& element : v.data) {
        bitcoin::varint::encode(ds, element.size());
        ds.write((char*)element.data(), element.size());
    }
    return ds;
}

/**
 *  Deserialize a witness
 *
 *  @param ds - The stream to read
 *  @param v - The destination for deserialized value
 *  @tparam Stream - Type of datastream buffer
 *  @return datastream<Stream>& - Reference to the datastream
 */
template <typename Stream>
datastream<Stream>& operator>>(datastream<Stream>& ds, bitcoin::core::transaction_witness& v) {
    auto stack_length = bitcoin::varint::decode(ds);
    v.data.resize(stack_length);
    for (uint64_t i = 0; i < stack_length; i++) {
        auto element_length = bitcoin::varint::decode(ds);
        v.data[i].resize(element_length);
        ds.read((char*)v.data[i].data(), element_length);
    }
    return ds;
}

/**
 *  Serialize a script
 *
 *  @param ds - The stream to write
 *  @param v - The value to serialize
 *  @tparam Stream - Type of datastream buffer
 *  @return datastream<Stream>& - Reference to the datastream
 */
template <typename Stream>
datastream<Stream>& operator<<(datastream<Stream>& ds, const bitcoin::core::script& v) {
    bitcoin::varint::encode(ds, v.data.size());
    ds.write((char*)v.data.data(), v.data.size());
    return ds;
}

/**
 *  Deserialize a script
 *
 *  @param ds - The stream to read
 *  @param v - The destination for deserialized value
 *  @tparam Stream - Type of datastream buffer
 *  @return datastream<Stream>& - Reference to the datastream
 */
template <typename Stream>
datastream<Stream>& operator>>(datastream<Stream>& ds, bitcoin::core::script& v) {
    auto length = bitcoin::varint::decode(ds);
    v.data.resize(length);
    ds.read((char*)v.data.data(), length);
    return ds;
}
}  // namespace eosio