#pragma once

#include <string>

namespace bitcoin {

    inline bool ValidAsCString(const std::string& str) noexcept { return str.size() == strlen(str.c_str()); }

    constexpr size_t strlen(const char* str) {
        int i = 0;
        while (*(str + i) != '\0') i++;
        return i;
    }

    constexpr inline bool IsSpace(char c) noexcept {
        return c == ' ' || c == '\f' || c == '\n' || c == '\r' || c == '\t' || c == '\v';
    }

};  // namespace bitcoin