#include <eosio/eosio.hpp>
#include <bitcoin/utility/base58.hpp>
#include <bitcoin/utility/bech32.hpp>
#include <sstream>

namespace bitcoin::utility {
  static std::vector<uint8_t> convert_bits(const std::vector<uint8_t>& in, int from_bits, int to_bits, bool pad) {
    int acc = 0;
    int bits = 0;
    const int maxv = (1 << to_bits) - 1;
    const int max_acc = (1 << (from_bits + to_bits - 1)) - 1;
    std::vector<uint8_t> out;

    for (size_t i = 0; i < in.size(); ++i) {
      int value = in[i];
      acc = ((acc << from_bits) | value) & max_acc;
      bits += from_bits;
      while (bits >= to_bits) {
        bits -= to_bits;
        out.push_back((acc >> bits) & maxv);
      }
    }

    if (pad) {
      if (bits) {
        out.push_back((acc << (to_bits - bits)) & maxv);
      }
    } else if (bits >= from_bits || ((acc << (to_bits - bits)) & maxv)) {
      return {};
    }

    return out;
  }

  static std::vector<uint8_t> address_to_scriptpubkey(const std::string& address) {
    eosio::check(address.size() > 0, "Empty Bitcoin address");
    std::vector<uint8_t> scriptpubkey;
    if (address.substr(0, 2) == "bc" || address.substr(0, 2) == "tb") {
      bitcoin::bech32::DecodeResult decoded = bitcoin::bech32::Decode(address);
      eosio::check(decoded.encoding != bitcoin::bech32::Encoding::INVALID, "Invalid Bech32 address");
      eosio::check(decoded.hrp == "bc" || decoded.hrp == "tb" || decoded.hrp == "bcrt", "Not a valid Bitcoin address");
      // The first byte of decoded data is the witness version
      uint8_t witness_version = decoded.data[0];
      std::vector<uint8_t> witness_program_5bit(decoded.data.begin() + 1, decoded.data.end());
      std::vector<uint8_t> witness_program_8bit = convert_bits(witness_program_5bit, 5, 8, false);
      eosio::check(!witness_program_8bit.empty(), "Invalid witness program");

      // OP_0 (for version 0 witness program) or OP_1 to OP_16 for higher versions
      scriptpubkey.push_back(witness_version == 0 ? 0x00 : 0x50 + witness_version);

      // Push the witness program
      scriptpubkey.push_back(witness_program_8bit.size());
      scriptpubkey.insert(scriptpubkey.end(), witness_program_8bit.begin(), witness_program_8bit.end());
    } else {
      std::vector<unsigned char> decoded;
      bool success = bitcoin::DecodeBase58Check(address, decoded, 100);
      eosio::check(success, "Invalid Base58Check address");
      std::vector<unsigned char> pubkey_hash(decoded.begin() + 1, decoded.end());
      if (decoded[0] == 0x00 || decoded[0] == 0x6f) {
        scriptpubkey = { 0x76, 0xa9, 0x14 };
        scriptpubkey.insert(scriptpubkey.end(), pubkey_hash.begin(), pubkey_hash.end());
        scriptpubkey.push_back(0x88);
        scriptpubkey.push_back(0xac);
      } else if (decoded[0] == 0x05 || decoded[0] == 0xc4) {
        scriptpubkey = { 0xa9, 0x14 };
        scriptpubkey.insert(scriptpubkey.end(), pubkey_hash.begin(), pubkey_hash.end());
        scriptpubkey.push_back(0x87);
      } else {
        eosio::check(false, "Unsupported address type");
      }
    }
    return scriptpubkey;
  }
}