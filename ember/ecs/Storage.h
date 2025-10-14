#pragma once

#include <concepts>
#include <vector>

#include "Entity.h"

namespace ember::ecs {

    template<typename T>
    concept ComponentStorage = requires(const T const_s, T s, Entity e, const T::Component& c) {
        typename T::Component;

        { const_s[e] } -> std::same_as<const typename T::Component&>;
        { const_s.at(e) } -> std::same_as<const typename T::Component&>;

        { s[e] } -> std::same_as<typename T::Component&>;
        { s.at(e) } -> std::same_as<typename T::Component&>;
        s.insert(e, c);
    };

    template<typename T>
    class VectorStorage {
    public:
        using Component = T;

        void insert(Entity e, const Component& c) {
            maybe_resize(e);
            m_data[e] = c;
        }

        const T& operator[](Entity e) const {
            return m_data[e];
        }
        const T& at(Entity e) const {
            return m_data.at(e);
        }

        T& operator[](Entity e) {
            return m_data[e];
        }
        T& at(Entity e) {
            return m_data.at(e);
        }
    private:
        std::vector<T> m_data;

        void maybe_resize(Entity e) {
            if (e >= m_data.capacity()) {
                m_data.reserve((e + 1024) & 0x3ff);
            }
            m_data.resize(e+1);
        }
    };
    static_assert(ComponentStorage<VectorStorage<int>>);

}