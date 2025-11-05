#pragma once

#include <array>
#include <initializer_list>
#include <span>
#include <stdexcept>

namespace ember::collections {

    template<typename T, size_t S = 24>
    class SmallVector {
    public:
        static_assert(sizeof(T) <= 16);

        using iterator = T*;
        using const_iterator = const T*;

        SmallVector(): m_size(0) { }
        ~SmallVector() {
            if (m_size > SMALLVEC_LEN) {
                delete[] m_dynamic.data();
            }
        }

        SmallVector(const SmallVector<T>& other) {
            m_size = other.m_size;

            if (m_size <= SMALLVEC_LEN) {
                m_local = other.m_local;
            } else {
                m_dynamic = std::span(new T[m_size], m_size);
                std::copy(other.m_dynamic.begin(), other.m_dynamic.end(), m_dynamic.begin());
            }
        }
        SmallVector(SmallVector<T>&& other) {
            m_size = other.m_size;
            other.m_size = 0;

            if (m_size <= SMALLVEC_LEN) {
                m_local = std::move(other.m_local);
            } else {
                m_dynamic = other.m_dynamic;
                other.m_dynamic = std::span<T>();
            }
        }

        SmallVector(size_t count): m_size(count) {
            if (count > SMALLVEC_LEN) {
                m_dynamic = std::span(new T[count], count);
            }
        }
        SmallVector(size_t count, const T& value): SmallVector(count) {
            if (m_size <= SMALLVEC_LEN) {
                std::fill(m_local.begin(), m_local.end(), value);
            } else {
                std::fill(m_dynamic.begin(), m_dynamic.end(), value);
            }
        }
        template<typename Iter>
        SmallVector(Iter begin, Iter end) {
            m_size = std::distance(begin, end);
            if (m_size <= SMALLVEC_LEN) {
                std::copy(begin, end, m_local.begin());
            } else {
                m_dynamic = std::span(new T[m_size], m_size);
                std::copy(begin, end, m_dynamic.begin());
            }
        }
        SmallVector(std::initializer_list<T> list): SmallVector(list.begin(), list.end()) { }


        SmallVector& operator=(SmallVector<T>& other) {
            m_size = other.m_size;

            if (m_size <= SMALLVEC_LEN) {
                m_local = other.m_local;
            } else {
                m_dynamic = std::span(new T[m_size], m_size);
                std::copy(other.m_dynamic.begin(), other.m_dynamic.end(), m_dynamic.begin());
            }

            return *this;
        }
        SmallVector& operator=(SmallVector<T>&& other) {
            m_size = other.m_size;
            other.m_size = 0;

            if (m_size <= SMALLVEC_LEN) {
                m_local = std::move(other.m_local);
            } else {
                m_dynamic = other.m_dynamic;
                other.m_dynamic = std::span<T>();
            }

            return *this;
        }
        SmallVector& operator=(std::initializer_list<T> list) {
            m_size = list.size();

            if (m_size <= SMALLVEC_LEN) {
                std::copy(list.begin(), list.end(), m_local.begin());
            } else {
                m_dynamic = std::span(new T[m_size], m_size);
                std::copy(list.begin(), list.end(), m_dynamic.begin());
            }

            return *this;
        }

        const T& at(size_t pos) const {
            if (m_size <= SMALLVEC_LEN) {
                return m_local.at(pos);
            } else {
                if (pos >= m_size) {
                    throw std::out_of_range("SmallVector at() out of range!");
                }
                return m_dynamic[pos];
            }
        }
        T& at(size_t pos) {
            if (m_size <= SMALLVEC_LEN) {
                return m_local.at(pos);
            } else {
                if (pos >= m_size) {
                    throw std::out_of_range("SmallVector at() out of range!");
                }
                return m_dynamic[pos];
            }
        }

        const T& operator[](size_t pos) const {
            if (m_size <= SMALLVEC_LEN) return m_local[pos];
            else return m_dynamic[pos];
        }
        T& operator[](size_t pos) {
            if (m_size <= SMALLVEC_LEN) return m_local[pos];
            else return m_dynamic[pos];
        }

        const T& front() const {
            if (m_size <= SMALLVEC_LEN) return m_local.front();
            else return m_dynamic.front();
        }
        T& front() {
            if (m_size <= SMALLVEC_LEN) return m_local.front();
            else return m_dynamic.front();
        }

        const T& back() const {
            if (m_size <= SMALLVEC_LEN) return m_local.back();
            else return m_dynamic.front();
        }
        T& back() {
            if (m_size <= SMALLVEC_LEN) return m_local.back();
            else return m_dynamic.front();
        }

        const T* data() const {
            if (m_size <= SMALLVEC_LEN) return m_local.data();
            else return m_dynamic.data();
        }
        T* data() {
            if (m_size <= SMALLVEC_LEN) return m_local.data();
            else return m_dynamic.data();
        }

        inline iterator begin() { return data(); }
        inline const_iterator begin() const { return data(); }

        inline iterator end() { return data() + m_size; }
        inline const_iterator end() const { return data() + m_size; }

        inline iterator rbegin() { return end() - 1; }
        inline const_iterator rbegin() const { return end() - 1; }

        inline iterator rend() { return begin() - 1; }
        inline const_iterator rend() const { return begin() - 1; }

        inline bool empty() const { return bool(m_size == 0); }
        inline size_t size() const { return m_size; }

        void reserve(size_t count) {
            if (count > SMALLVEC_LEN && count > m_dynamic.size()) {
                realloc_dynamic_memory(count);
            }
        }

        size_t capacity() const {
            if (m_size <= SMALLVEC_LEN) return SMALLVEC_LEN;
            else return m_dynamic.size();
        }

        void shrink_to_fit() {
            if (m_size > SMALLVEC_LEN) {
                realloc_dynamic_memory(m_size);
            }
        }

        void clear() {
            if (m_size > SMALLVEC_LEN) {
                delete[] m_dynamic.data();
                m_dynamic = nullptr;
            }
            m_size = 0;
        }

        iterator insert(const_iterator pos, const T& value) {
            insert(pos, 1, value);
        }
        iterator insert(const_iterator pos, T&& value) {
            const auto ofs = std::distance(begin(), pos);
            assert(ofs >= 0 && ofs < m_size);
            resize(m_size+1);

            for (auto i = ofs+1; i < m_size; i++) {
                this->at(i) = std::move(this->at(i-1));
            }
            this->at(ofs) = std::move(value);
        }
        iterator insert(const_iterator pos, size_t count, const T& value) {
            const auto ofs = std::distance(begin(), pos);
            assert(ofs >= 0 && ofs < m_size);
            resize(m_size+count);

            for (auto i = ofs+count; i < m_size; i++) {
                this->at(i) = this->at(i-count);
            }
            this->at(ofs) = value;
        }
        template<typename InputIter>
        iterator insert(const_iterator pos, InputIter begin, InputIter end) {
            for (auto iter = begin; iter != end; iter++, pos++) {
                insert(pos, *iter);
            }
        }
        iterator insert(const_iterator pos, std::initializer_list<T> list) {
            return insert(pos, list.begin(), list.end());
        }

        template<typename... Args>
        iterator emplace(const_iterator pos, Args&&... args) {
            const auto ofs = std::distance(begin(), pos);
            assert(ofs >= 0 && ofs < m_size);
            resize(m_size+1);

            for (auto i = ofs+1; i < m_size; i++) {
                this->at(i) = std::move(this->at(i-1));
            }
            std::construct_at(&this->at(ofs), args...);
        }

        iterator erase(iterator pos) {
            erase(static_cast<const_iterator>(pos));
        }
        iterator erase(const_iterator pos) {
            const auto ofs = std::distance(begin(), pos);
            assert(ofs >= 0 && ofs < m_size);
            for (auto i = pos; i < m_size-1; i++) {
                this->at(i) = this->at(i+1);
            }
            m_size--;
        }
        iterator erase(iterator first, iterator last) {
            erase(static_cast<const_iterator>(first), static_cast<const_iterator>(last));
        }
        iterator erase(const_iterator first, const_iterator last) {
            const auto first_ofs = std::distance(begin(), first);
            assert(first_ofs >= 0 && first_ofs < m_size);

            const auto last_ofs = std::distance(begin(), last);
            assert(last_ofs >= 0 && last_ofs < m_size);

            assert(first_ofs < last_ofs);
            const auto count = last_ofs - first_ofs;

            for (auto i = 0; i < count && (last_ofs + i) < m_size; i++) {
                this->at(first_ofs + i) = this->at(last_ofs + i);
            }

            m_size -= count;
        }

        void push_back(const T& value) {
            T* ptr;
            const auto newsize = m_size + 1;

            if (newsize <= SMALLVEC_LEN) {
                ptr = m_local.data();
            } else if (newsize == (SMALLVEC_LEN + 1)) {
                move_to_dynamic_storage(newsize);
                ptr = m_dynamic.data();
            } else {
                if (newsize > m_dynamic.size()) {
                    realloc_dynamic_memory(m_dynamic.size() * 2);
                }
                ptr = m_dynamic.data();
            }

            ptr[m_size] = value;
            m_size = newsize;
        }

        void push_back(T&& value) {
            T* ptr;
            const auto newsize = m_size + 1;

            if (newsize <= SMALLVEC_LEN) {
                ptr = m_local.data();
            } else if (newsize == (SMALLVEC_LEN + 1)) {
                move_to_dynamic_storage(newsize);
                ptr = m_dynamic.data();
            } else {
                if (newsize > m_dynamic.size()) {
                    realloc_dynamic_memory(m_dynamic.size() * 2);
                }
                ptr = m_dynamic.ptr();
            }

            ptr[m_size] = std::move(value);
            m_size = newsize;
        }

        template<typename... Args>
        T& emplace_back(Args&&... args) {
            T* ptr;
            const auto newsize = m_size + 1;
            if (newsize <= SMALLVEC_LEN) {
                ptr = m_local.data() + m_size;
            } else if (newsize == (SMALLVEC_LEN + 1)) {
                move_to_dynamic_storage(newsize);
                ptr = m_dynamic.data() + m_size;
            } else {
                if (newsize > m_dynamic.size()) {
                    realloc_dynamic_memory(m_dynamic.size() * 2);
                }
                ptr = m_dynamic.data() + m_size;
            }

            std::construct_at(ptr, args...);
            m_size = newsize;
        }

        inline void pop_back() { if (m_size > 0) m_size--; }

        void resize(size_t count) {
            resize(count, T());
        }

        void resize(size_t count, const T& value) {
            if (count < m_size) {
                if (m_size <= SMALLVEC_LEN) {
                    m_size = count;
                } else if (count <= SMALLVEC_LEN) {
                    for (auto i = 0; i < count; i++) {
                        m_local[i] = m_dynamic[i];
                    }
                }
            } else if (count > m_size) {
                T* dst = m_dynamic.data();
                if (m_size <= SMALLVEC_LEN && count <= SMALLVEC_LEN) {
                    dst = m_local.data();
                } else if (m_size <= SMALLVEC_LEN && count > SMALLVEC_LEN) {
                    move_to_dynamic_storage(count);
                } else if (count > m_dynamic.size()) {
                    realloc_dynamic_memory(std::max(count, m_dynamic.size() * 2));
                }

                assert(dst != nullptr);
                for (auto i = m_size; i < count; i++) {
                    dst[i] = value;
                }
            }

            m_size = count;
        }

        void swap(SmallVector<T>& other) noexcept {
            if (m_size <= SMALLVEC_LEN && other.m_size <= SMALLVEC_LEN) {
                std::swap(m_local, other.m_local);
            } else if (m_size > SMALLVEC_LEN && other.m_size <= SMALLVEC_LEN) {
                std::array<T, SMALLVEC_LEN> tmp = other.m_local;
                other.m_dynamic = m_dynamic;
                m_local = tmp;
            } else if (m_size <= SMALLVEC_LEN && other.m_size > SMALLVEC_LEN) {
                std::array<T, SMALLVEC_LEN> tmp = m_local;
                m_dynamic = other.m_dynamic;
                other.m_local = tmp;
            } else {
                std::swap(m_dynamic, other.m_dynamic);
            }

            std::swap(m_size, other.m_size);
        }

    private:
        static constexpr auto SMALLVEC_LEN = S/sizeof(T);

        union {
            std::array<T, SMALLVEC_LEN> m_local;
            std::span<T> m_dynamic;
        };
        size_t m_size;

        void realloc_dynamic_memory(size_t count) {
            auto tmp = std::span(new T[count], count);
            std::memcpy(tmp.data(), m_dynamic.data(), m_size * sizeof(T));
            delete[] m_dynamic.data();
            m_dynamic = tmp;
        }

        void move_to_dynamic_storage(size_t count) {
            count = std::max(count, SMALLVEC_LEN * 2);
            const auto tmp = std::move(m_local);
            m_dynamic = std::span(new T[count], count);
            std::copy(tmp.begin(), tmp.end(), m_dynamic.begin());
        }
    };
    static_assert(sizeof(SmallVector<int32_t>) == 32);
    static_assert(sizeof(SmallVector<int32_t, 120>) == 128);

}