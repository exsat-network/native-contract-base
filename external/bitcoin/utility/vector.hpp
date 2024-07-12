#pragma once

namespace bitcoin {

    /** Concatenate two vectors, moving elements. */
    template <typename V>
    inline V Cat(V v1, V&& v2) {
        v1.reserve(v1.size() + v2.size());
        for (auto& arg : v2) {
            v1.push_back(std::move(arg));
        }
        return v1;
    }

    /** Concatenate two vectors. */
    template <typename V>
    inline V Cat(V v1, const V& v2) {
        v1.reserve(v1.size() + v2.size());
        for (const auto& arg : v2) {
            v1.push_back(arg);
        }
        return v1;
    }

    template <typename IteratorT>
    inline std::string join(IteratorT Begin, IteratorT End, std::string Separator) {
        std::string S;
        if (Begin == End) return S;

        S += (*Begin);
        while (++Begin != End) {
            S += Separator;
            S += (*Begin);
        }
        return S;
    }
}  // namespace bitcoin