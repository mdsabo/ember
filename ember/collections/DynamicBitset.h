#pragma once

#include <cstdlib>
#include <tuple>
#include <vector>

namespace ember::collections {

    class DynamicBitset {
    public:
        DynamicBitset(size_t nbits = 0): m_data((nbits+63)>>6, 0), m_size(nbits) { }
        DynamicBitset(const DynamicBitset&) = default;
        DynamicBitset(DynamicBitset&&) = default;

        bool operator[](size_t bit) const;
        bool test(size_t bit) const;
        bool all() const;
        bool any() const;
        bool none() const;
        size_t ffs() const;

        inline size_t size() const { return m_size; }
        void resize(size_t nbits);
        void reserve(size_t nbits);

        void set(size_t bit);
        void reset(size_t bit);

        DynamicBitset& operator=(const DynamicBitset& rhs);
        DynamicBitset& operator=(DynamicBitset&& rhs);

        DynamicBitset& operator&=(const DynamicBitset& rhs);
        friend DynamicBitset operator&(const DynamicBitset& lhs, const DynamicBitset& rhs);

        inline const std::vector<uint64_t>& data() const { return m_data; }
        inline std::vector<uint64_t>& data() { return m_data; }
    private:
        std::vector<uint64_t> m_data;
        size_t m_size;

        static constexpr std::tuple<size_t, size_t> bit_indices(size_t bit) {
            return std::make_tuple(bit >> 6, 1ULL << (bit & 0x3f));
        }
    };

}