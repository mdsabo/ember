#pragma once

#include <concepts>
#include <map>
#include <stdexcept>
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
            m_components[e.id] = c;
        }

        void remove(Entity e) {
            m_valid.remove(e);
        }

        const T& operator[](Entity e) const {
            return m_components[e.id];
        }
        T& operator[](Entity e) {
            return m_components[e.id];
        }

        const T& at(Entity e) const {
            if (!m_valid[e]) throw std::out_of_range("Attempted to access invalid component!");
            return m_components.at(e.id);
        }
        T& at(Entity e) {
            if (!m_valid[e]) throw std::out_of_range("Attempted to access invalid component!");
            return m_components.at(e.id);
        }

        const_iterator begin() const {
            return m_components.begin();
        }
        iterator begin() {
            return m_components.begin();
        }

        const_iterator end() const {
            return m_components.end();
        }
        iterator end() {
            return m_components.end();
        }

    private:
        std::vector<T> m_components;
        EntitySet m_valid;

        void maybe_resize(Entity e) {
            if (e.id >= m_components.size()) {
                m_components.resize(e.id+1);
            }
        }
    };
    static_assert(ComponentStorage<VectorStorage<int>>);

    template<typename T>
    class DenseVectorStorage {
    public:
        using Component = T;
        using iterator = std::vector<T>::iterator;
        using const_iterator = std::vector<T>::const_iterator;

        inline bool contains(Entity e) const { return m_id_map.contains(e); }
        inline const EntitySet& entities() const { return m_id_map.entities(); }

        void insert(Entity e, const Component& c) {
            if (m_id_map.contains(e)) {
                this->at(e) = c;
            } else {
                m_id_map.insert(e, m_components.size());
                m_components.push_back(c);
            }
        }

        void remove(Entity e) {
            auto index = m_id_map.at(e);
            m_components[index] = m_components.back();
            m_components.resize(m_components.size()-1);
            for (auto& id : m_id_map) {
                if (id == m_components.size()) {
                    id = index;
                    break;
                }
            }
        }

        const T& operator[](Entity e) const {
            return m_components[m_id_map[e]];
        }
        T& operator[](Entity e) {
            return m_components[m_id_map[e]];
        }

        const T& at(Entity e) const {
            return m_components.at(m_id_map.at(e));
        }
        T& at(Entity e) {
            return m_components.at(m_id_map.at(e));
        }

        const_iterator begin() const {
            return m_components.begin();
        }
        iterator begin() {
            return m_components.begin();
        }

        const_iterator end() const {
            return m_components.end();
        }
        iterator end() {
            return m_components.end();
        }

    private:
        VectorStorage<size_t> m_id_map;
        std::vector<T> m_components;
    };
    static_assert(ComponentStorage<DenseVectorStorage<int>>);

    template<typename T>
    class MapStorage {
    public:
        using Component = T;
        using iterator = std::map<Entity, T>::iterator;
        using const_iterator = std::map<Entity, T>::const_iterator;

        inline bool contains(Entity e) const { return m_valid.contains(e); }
        inline const EntitySet& entities() const { return m_valid; }

        void insert(Entity e, const Component& c) {
            m_valid.insert(e);
            m_components.insert_or_assign(e, c);
        }

        void remove(Entity e) {
            m_components.erase(e);
            m_valid.remove(e);
        }

        const T& operator[](Entity e) const {
            return m_components[e];
        }
        T& operator[](Entity e) {
            return m_components[e];
        }

        const T& at(Entity e) const {
            return m_components.at(e);
        }
        T& at(Entity e) {
            return m_components.at(e);
        }

        const_iterator begin() const {
            return m_components.begin();
        }
        iterator begin() {
            return m_components.begin();
        }

        const_iterator end() const {
            return m_components.end();
        }
        iterator end() {
            return m_components.end();
        }

    private:
        std::map<Entity, T> m_components;
        EntitySet m_valid;
    };
    static_assert(ComponentStorage<MapStorage<int>>);

    template<typename T>
    class SingletonStorage {
    public:
        using Component = T;
        using iterator = std::nullptr_t;
        using const_iterator = std::nullptr_t;

        SingletonStorage(): m_singleton() { }

        inline const T& instance() const {
            return m_singleton;
        }
        inline T& instance() {
            return m_singleton;
        }

        inline bool contains(Entity e) const {
            assert(0 && "Attempted to get entity set of singleton component");
        }
        const EntitySet& entities() const {
            assert(0 && "Attempted to get entity set of singleton component");
        }
        void insert(Entity e, const Component& c) {
            assert(0 && "Attempted to attach singleton component to entity");
        }
        void remove(Entity e) {
            assert(0 && "Attempted to remove singleton component to entity");
        }
        const T& operator[](Entity e) const {
            assert(0 && "Attempted to index singleton component");
        }
        T& operator[](Entity e) {
            assert(0 && "Attempted to index singleton component");
        }
        const T& at(Entity e) const {
            assert(0 && "Attempted to index singleton component");
        }
        T& at(Entity e) {
            assert(0 && "Attempted to index singleton component");
        }
        const_iterator begin() const {
            assert(0 && "Attempted to create iterator over singleton component");
        }
        iterator begin() {
            assert(0 && "Attempted to create iterator over singleton component");
        }
        const_iterator end() const {
            assert(0 && "Attempted to create iterator over singleton component");
        }
        iterator end() {
            assert(0 && "Attempted to create iterator over singleton component");
        }
    private:
        T m_singleton;
    };
    static_assert(ComponentStorage<SingletonStorage<int>>);

}