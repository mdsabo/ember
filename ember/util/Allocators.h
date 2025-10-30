#pragma once

#include <array>
#include <bitset>
#include <cassert>
#include <cstdlib>
#include <forward_list>
#include <memory>
#include <stdexcept>
#include <vector>

namespace ember::util {

    template<typename T, size_t E = 4000/sizeof(T)>
    class SlabAllocator {
    public:
        SlabAllocator() {}

        [[nodiscard]] T* malloc() {
            for (auto& slab : m_slabs) {
                if ((slab.num_free != 0)) {
                    auto index = ffs(slab.freelist);
                    return slab.allocate(index);
                }
            }

            m_slabs.emplace_front();
            auto& new_slab = m_slabs.front();
            return new_slab.allocate(0);
        }

        void free(T* obj) {
            auto ptr = reinterpret_cast<uintptr_t>(obj);
            for (auto& slab : m_slabs) {
                auto offset = ptr - reinterpret_cast<uintptr_t>(slab.elements.data());
                if (offset < sizeof(std::array<T, E>)) {
                    auto index = offset / sizeof(T);
                    slab.free(index);
                    return;
                }
            }

            throw std::runtime_error("Attempted to free object from a slab to which it does not belong");
        }

        size_t allocated_count() {
            size_t cnt = 0;
            for (const auto& slab : m_slabs) cnt += (E - slab.num_free);
            return cnt;
        }

    private:
        struct Slab {
            std::array<T, E> elements;
            std::bitset<E> freelist;
            size_t num_free;

            Slab() {
                freelist.flip();
                num_free = E;
            }

            T* allocate(size_t i) {
                assert(freelist.test(i));
                assert(num_free >= 1);

                freelist.reset(i);
                num_free--;

                return &(elements[i]);
            }

            void free(size_t i) {
                assert(!freelist.test(i));

                std::destroy_at(&elements[i]);
                std::construct_at(&elements[i]);

                freelist.set(i);
                num_free++;

                assert(num_free <= E);
            }
        };
        std::forward_list<Slab> m_slabs;

        size_t ffs(const std::bitset<E>& freelist) {
            for (auto i = 0; i < E; i++) {
                if (freelist.test(i)) {
                    return i;
                }
            }
            return E;
        }
    };

    class StackAllocator {
    public:
        StackAllocator(size_t bytes);
        ~StackAllocator();

        [[nodiscard]] void* malloc(size_t size);

        template<typename T>
        [[nodiscard]] T* malloc(size_t count) {
            return reinterpret_cast<T*>(malloc(sizeof(T) * count));
        }

        void reset();

        inline size_t max_size() { return m_max_size; }

    private:
        std::unique_ptr<uint8_t[]> m_memory;
        size_t m_top;
        size_t m_size;
        size_t m_max_size;

#if !defined(EMBER_STACK_ALLOCATOR_ASSERT_ON_OVERFLOW)
        std::vector<void*> m_overflow;
#endif
    };

}