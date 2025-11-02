#pragma once

#include <vector>

#include "DynamicBitset.h"

namespace ember::collections {

    template<typename T>
    class FreeableVector {
    public:
        FreeableVector() = default;
        FreeableVector(const FreeableVector&) = default;
        FreeableVector(FreeableVector&& rhs) = default;
        FreeableVector& operator=(const FreeableVector& rhs) {
            m_vector = rhs.m_vector;
            m_freelist = rhs.m_freelist;
            return *this;
        }

        size_t size() const {
            return m_freelist.size();
        }

        size_t insert(const T& obj) {
            auto index = m_freelist.ffs();
            if (index == m_freelist.size()) {
                m_vector.push_back(obj);
                m_freelist.resize(m_vector.size());
            } else {
                m_vector.at(index) = obj;
            }
            return index;
        }

        void erase(size_t index) {
            assert(index < m_freelist.size());
            m_freelist.set(index);
        }

        T& at(size_t index) {
            assert(!m_freelist.test(index));
            return m_vector.at(index);
        }
        const T& at(size_t index) const {
            assert(!m_freelist.test(index));
            return m_vector.at(index);
        }
        T& operator[](size_t index) {
            return m_vector[index];
        }
        const T& operator[](size_t index) const {
            return m_vector[index];
        }
    private:
        std::vector<T> m_vector;
        DynamicBitset m_freelist;
    };

}