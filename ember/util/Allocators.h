#pragma once

#include <array>
#include <bitset>
#include <cstdlib>
#include <memory>
#include <vector>

#include <cstdio>

namespace ember::util {

    template<typename T, size_t E = 4000/sizeof(T)>
    class SlabAllocator {
    public:
        SlabAllocator() {}

        T* malloc() {
            for (auto& slab : m_slabs) {
                if ((slab.num_free != 0)) {
                    auto index = ffs(slab.freelist);
                    return slab.allocate(index);
                }
            }

            m_slabs.push_front(Slab());
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

}