#include "World.h"

#include <glm/ext/matrix_transform.hpp>
#include "TransformComponent.h"

namespace ember::ecs {
    World::World(): m_next_entity(Entity(WORLD_ORIGIN_ENTITY.id + 1)) {
        add_component<TransformComponent>();
        write_component<TransformComponent>().insert(WORLD_ORIGIN_ENTITY, {});
    }

    World::World(const SystemGraph& systems): World() {
        m_systems = systems;
    }

    void World::run(float dt) {
        for (const auto& phase : m_systems) {
            for (const auto sys : phase) {
                sys(*this, dt);
            }
        }
    }

    Entity World::create_entity() {
        Entity e;
        if (!m_recycled_entities.empty()) {
            e = m_recycled_entities.front();
            m_recycled_entities.pop();
        } else {
            e = m_next_entity;
            m_next_entity.id++;
        }

        auto& storage = write_component<TransformComponent>();
        storage.insert(e, {});
        return e;
    }

    void World::destroy_entity(Entity e) {
        assert(e != WORLD_ORIGIN_ENTITY);
        m_recycled_entities.push(e.generation++);
    }
}