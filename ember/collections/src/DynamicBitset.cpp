#include "DynamicBitset.h"

#include <algorithm>
#include <stdexcept>

namespace ember::collections {
    bool DynamicBitset::operator[](size_t bit) const {
        auto [index, mask] = bit_indices(bit);
        return bool(m_data[index] & mask);
    }

    bool DynamicBitset::test(size_t bit) const {
        auto [index, mask] = bit_indices(bit);
        return (bit < m_size) && bool(m_data.at(index) & mask);
    }

    bool DynamicBitset::all() const {
        return std::all_of(m_data.begin(), m_data.end(), [](const uint64_t x){ return x == -1ULL; });
    }

    bool DynamicBitset::any() const {
        return std::any_of(m_data.begin(), m_data.end(), [](const uint64_t x){ return bool(x); });
    }

    bool DynamicBitset::none() const {
        return std::none_of(m_data.begin(), m_data.end(), [](const uint64_t x){ return bool(x); });
    }

    void DynamicBitset::resize(size_t nbits) {
        // If we need to grow the bitset and the current set covers
        // a partial word, clear the upper bits of the word.
        if ((nbits > m_size) && (m_size & 0x3f)) {
            auto [index, mask] = bit_indices(m_size);
            m_data.at(index) &= (mask - 1);
        }

        m_data.resize((nbits + 63) >> 6, 0);
        m_size = nbits;
    }

    void DynamicBitset::set(size_t bit) {
        if (bit >= m_size) throw std::out_of_range("bit out of bounds");

        auto [index, mask] = bit_indices(bit);
        m_data.at(index) |= mask;
    }

    void DynamicBitset::reset(size_t bit) {
        if (bit >= m_size) throw std::out_of_range("bit out of bounds");

        auto [index, mask] = bit_indices(bit);
        m_data.at(index) &= ~mask;
    }

    DynamicBitset& DynamicBitset::operator=(const DynamicBitset& rhs) {
        this->m_data = rhs.m_data;
        this->m_size = rhs.m_size;
        return *this;
    }

    DynamicBitset& DynamicBitset::operator=(DynamicBitset&& rhs) {
        this->m_data = rhs.m_data;
        this->m_size = rhs.m_size;
        return *this;
    }

    DynamicBitset& DynamicBitset::operator&=(const DynamicBitset& rhs) {
        const auto minsize = std::min(this->size(), rhs.size());
        resize(minsize);

        const auto num_words = minsize >> 6;
        for (auto i = 0; i < num_words; i++) {
            this->m_data[i] &= rhs.m_data[i];
        }

        return *this;
    }

    DynamicBitset operator&(const DynamicBitset& lhs, const DynamicBitset& rhs) {
        const auto minsize = std::min(lhs.size(), rhs.size());
        auto res = DynamicBitset(minsize);

        const auto num_words = (minsize >> 6) + 1;
        for (auto i = 0; i < num_words; i++) {
            res.m_data[i] = lhs.m_data[i] & rhs.m_data[i];
        }

        return res;
    }
}