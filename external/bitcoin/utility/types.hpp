#pragma once

#include <intx/intx.hpp>
#include <array>

namespace bitcoin {
    using uint256_t = intx::uint256;
    uint256_t be_uint_from_checksum256(const eosio::checksum256& checksum) {
        auto buffer = checksum.extract_as_byte_array();
        return intx::be::unsafe::load<uint256_t>((uint8_t*)buffer.data());
    }

    uint256_t le_uint_from_checksum256(const eosio::checksum256& checksum) {
        auto buffer = checksum.extract_as_byte_array();
        return intx::le::unsafe::load<uint256_t>((uint8_t*)buffer.data());
    }

    uint256_t be_uint_from_string(const std::string& value) {
        return intx::be::unsafe::load<uint256_t>((uint8_t*)value.data());
    }

    uint256_t le_uint_from_string(const std::string& value) {
        return intx::le::unsafe::load<uint256_t>((uint8_t*)value.data());
    }

    eosio::checksum256 be_checksum256_from_uint(const uint256_t& value) {
        auto buffer = std::array<uint8_t, 32>();
        intx::be::unsafe::store<uint256_t>(buffer.data(), value);
        return eosio::checksum256(buffer);
    }

    eosio::checksum256 le_checksum256_from_uint(const uint256_t& value) {
        auto buffer = std::array<uint8_t, 32>();
        intx::le::unsafe::store<uint256_t>(buffer.data(), value);
        return eosio::checksum256(buffer);
    }

    eosio::checksum256 checksum256_from_vector(const std::vector<uint8_t>& vec) {
        std::array<uint8_t, 32> buffer;
        std::copy(vec.begin(), vec.end(), buffer.data());
        return eosio::checksum256(buffer);
    }

    namespace varint {
        template <typename Stream>
        uint64_t decode(eosio::datastream<Stream>& ds) {
            uint8_t length_code = 0;
            ds.get(length_code);
            if (length_code < 0xFD) {
                return length_code;
            } else if (length_code == 0xFD) {
                uint16_t result;
                ds >> result;
                return result;
            } else if (length_code == 0xFE) {
                uint32_t result;
                ds >> result;
                return result;
            } else {
                uint64_t result;
                ds >> result;
                return result;
            }
        }

        template <typename Stream>
        void encode(eosio::datastream<Stream>& ds, uint64_t value) {
            if (value < 0xFD) {
                ds << (uint8_t)value;
            } else if (value <= 0xFFFF) {
                ds << (uint8_t)0xFD;
                ds << (uint16_t)value;
            } else if (value <= 0xFFFFFFFF) {
                ds << (uint8_t)0xFE;
                ds << (uint32_t)value;
            } else {
                ds << (uint8_t)0xFF;
                ds << (uint64_t)value;
            }
        }
    }  // namespace varint

    namespace compact {
        inline uint256_t decode(const uint32_t& compact) {
            constexpr uint32_t sign_mask = 0x00800000U;
            constexpr uint32_t exp_shift = 24U;
            constexpr uint32_t mnt_mask = 0x007FFFFFU;

            if ((compact & sign_mask) != 0) {
                // this is a consensus oddity in bitcoin
                return 0;
            }

            bitcoin::uint256_t result;
            auto exponent = (compact >> exp_shift) & 0xFFU;
            result = compact & mnt_mask;

            if (exponent <= 3) {
                return result >> (8 * (3 - exponent));
            } else {
                return result << (8 * (exponent - 3));
            }
        }

        inline uint32_t encode(const bitcoin::uint256_t& value, const bool fNegative = false) {
            eosio::check(value > 0, "invalid bits");
            int bits = sizeof(value) * 8 - intx::clz(value);
            int nSize = (bits + 7) / 8;
            uint32_t nCompact = 0;
            constexpr uint64_t mask = 0xFFFFFFFFFFFFFFFFU;
            if (nSize <= 3) {
                nCompact = uint64_t(value & mask) << 8 * (3 - nSize);
            } else {
                uint256_t bn = value >> 8 * (nSize - 3);
                nCompact = uint64_t(bn & mask);
            }
            // The 0x00800000 bit denotes the sign.
            // Thus, if it is already set, divide the mantissa by 256 and increase the exponent.
            if (nCompact & 0x00800000) {
                nCompact >>= 8;
                nSize++;
            }
            eosio::check((nCompact & ~0x007fffffU) == 0, "invalid bits");
            eosio::check(nSize < 256, "invalid bits");
            nCompact |= nSize << 24;
            nCompact |= (fNegative && (nCompact & 0x007fffff) ? 0x00800000 : 0);
            return nCompact;
        }
    }  // namespace compact
}  // namespace bitcoin

namespace eosio {
    /**
     *  Serialize a bitcoin::uint256_t
     *
     *  @param ds - The stream to write
     *  @param v - The value to serialize
     *  @tparam Stream - Type of datastream buffer
     *  @return datastream<Stream>& - Reference to the datastream
     */
    template <typename Stream>
    datastream<Stream>& operator<<(datastream<Stream>& ds, const bitcoin::uint256_t& v) {
        std::array<uint8_t, 32> buffer;
        intx::le::unsafe::store<bitcoin::uint256_t>(buffer.data(), v);
        ds.write((char*)buffer.data(), buffer.size());
        return ds;
    }

    /**
     *  Deserialize a bitcoin::uint256_t
     *
     *  @param ds - The stream to read
     *  @param v - The destination for deserialized value
     *  @tparam Stream - Type of datastream buffer
     *  @return datastream<Stream>& - Reference to the datastream
     */
    template <typename Stream>
    datastream<Stream>& operator>>(datastream<Stream>& ds, bitcoin::uint256_t& v) {
        std::array<uint8_t, 32> buffer;
        ds.read((char*)buffer.data(), buffer.size());
        v = intx::le::unsafe::load<bitcoin::uint256_t>(buffer.data());
        return ds;
    }

}  // namespace eosio
