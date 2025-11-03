#pragma once

#include <vector>

#include "DynamicBitset.h"

namespace ember::collections {

    template<typename T>
    class FreeableVector {
    public:
        FreeableVector(): m_num_free(0) { }
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
            if (m_num_free == 0) {
                auto index = m_vector.size();
                m_vector.push_back(obj);
                m_freelist.resize(m_vector.size());
                return index;
            } else {
                auto index = m_freelist.ffs();
                m_vector.at(index) = obj;
                m_freelist.reset(index);
                m_num_free--;
                return index;
            }
        }

        void erase(size_t index) {
            assert(index < m_freelist.size());
            m_freelist.set(index);
            m_num_free++;
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

        inline T* data() { return m_vector.data(); }
        inline const T* data() const { return m_vector.data(); }
    private:
        std::vector<T> m_vector;
        DynamicBitset m_freelist;
        size_t m_num_free;
    };

}