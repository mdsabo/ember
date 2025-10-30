#pragma once

#include <any>
#include <memory>
#include <queue>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>

#include "Component.h"
#include "Entity.h"
#include "EntitySet.h"
#include "SystemGraph.h"

namespace ember::ecs {

    class World {
    public:
        World();
        World(const SystemGraph& systems);

        void run(float dt);

        Entity create_entity();
        void destroy_entity(Entity e);

        template<Component T>
        void add_component() { add_resource<typename T::Storage>(); }

        template<Component T>
        const typename T::Storage& read_component() const {
            return read_resource<typename T::Storage>();
        }

        template<Component T>
        typename T::Storage& write_component() {
            return write_resource<typename T::Storage>();
        }

        template<Component... T>
        EntitySet query() {
            return (read_component<T>().entities() & ...);
        }

        template<typename T>
        void add_resource() {
            const auto index = std::type_index(typeid(T));
            m_components.insert({ index, std::any(T()) });
        }

        template<typename T>
        void add_resource(T resource) {
            const auto index = std::type_index(typeid(T));
            m_components.insert({ index, std::any(resource) });
        }

        template<typename T>
        const T& read_resource() const {
            const auto index = std::type_index(typeid(T));
            return std::any_cast<const T&>(m_components.at(index));
        }

        template<typename T>
        T& write_resource() {
            const auto index = std::type_index(typeid(T));
            return std::any_cast<T&>(m_components.at(index));
        }

    private:
        Entity m_next_entity;
        std::queue<Entity> m_recycled_entities;

        std::unordered_map<std::type_index, std::any> m_components;

        SystemGraph m_systems;
    };

}