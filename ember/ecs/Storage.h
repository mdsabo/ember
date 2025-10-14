#pragma once

#include <concepts>
#include <vector>

#include "Entity.h"
#include "EntitySet.h"

namespace ember::ecs {

    template<typename T>
    concept ComponentStorage = requires(const T const_s, T s, Entity e, const T::Component& c) {
        typename T::Component;
        typename T::iterator;
        typename T::const_iterator;

        { const_s.contains(e) } -> std::convertible_to<bool>;
        { const_s[e] } -> std::same_as<const typename T::Component&>;
        { const_s.at(e) } -> std::same_as<const typename T::Component&>;
        { const_s.begin() } -> std::same_as<typename T::const_iterator>;
        { const_s.end() } -> std::same_as<typename T::const_iterator>;
        { const_s.entities() } -> std::same_as<const EntitySet&>;

        s.insert(e, c);
        s.remove(e);
        { s[e] } -> std::same_as<typename T::Component&>;
        { s.at(e) } -> std::same_as<typename T::Component&>;
        { s.begin() } -> std::same_as<typename T::iterator>;
        { s.end() } -> std::same_as<typename T::iterator>;
    };

    template<typename T>
    class VectorStorage {
    public:
        using Component = T;
        using iterator = std::vector<T>::iterator;
        using const_iterator = std::vector<T>::const_iterator;

        inline bool contains(Entity e) const { return m_valid.contains(e); }
        inline const EntitySet& entities() const { return m_valid; }

        void insert(Entity e, const Component& c) {
            maybe_resize(e);
            m_valid.insert(e);
            m_data[e.id] = c;
        }

        void remove(Entity e) {
            m_valid.remove(e);
        }

        const T& operator[](Entity e) const {
            return m_data[e.id];
        }
        T& operator[](Entity e) {
            return m_data[e.id];
        }

        const T& at(Entity e) const {
            if (!m_valid[e]) throw std::out_of_range("Attempted to access invalid component from VectorStorage!");
            return m_data.at(e.id);
        }
        T& at(Entity e) {
            if (!m_valid[e]) throw std::out_of_range("Attempted to access invalid component from VectorStorage!");
            return m_data.at(e.id);
        }

        const_iterator begin() const {
            return m_data.begin();
        }
        iterator begin() {
            return m_data.begin();
        }

        const_iterator end() const {
            return m_data.end();
        }
        iterator end() {
            return m_data.end();
        }

    private:
        std::vector<T> m_data;
        EntitySet m_valid;

        void maybe_resize(Entity e) {
            if (e.id >= m_data.size()) {
                m_data.resize(e.id+1);
            }
        }
    };
    static_assert(ComponentStorage<VectorStorage<int>>);

}