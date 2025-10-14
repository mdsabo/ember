#include "../DynamicBitset.h"

namespace ember::collections {
    bool DynamicBitset::operator[](size_t bit) const {
        auto [index, mask] = bit_indices(bit);
        return bool(m_data[index] & mask);
    }

    bool DynamicBitset::test(size_t bit) const {
        auto [index, mask] = bit_indices(bit);
        return bool(m_data.at(index) & mask);
    }

    bool DynamicBitset::all() const {
        for (const auto& word : m_data) {
            if (word != -1ULL) return false;
        }
        return true;
    }

    bool DynamicBitset::any() const {
        for (const auto& word: m_data) {
            if (word != 0) return true;
        }
        return false;
    }

    bool DynamicBitset::none() const {
        return !any();
    }

    size_t DynamicBitset::size() const {
        return m_data.size() * 64;
    }

    void DynamicBitset::resize(size_t nbits) {
        m_data.resize((nbits + 63) >> 6, 0);
    }

    void DynamicBitset::set(size_t bit) {
        auto [index, mask] = bit_indices(bit);
        m_data.at(index) |= mask;
    }

    void DynamicBitset::reset(size_t bit) {
        auto [index, mask] = bit_indices(bit);
        m_data.at(index) &= ~mask;
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

        const auto num_words = minsize >> 6;
        for (auto i = 0; i < num_words; i++) {
            res.m_data[i] = lhs.m_data[i] & rhs.m_data[i];
        }

        return res;
    }
}