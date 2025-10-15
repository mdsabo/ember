#include "World.h"

namespace ember::ecs {
    Entity World::create_entity() {
        if (!m_recycled_entities.empty()) {
            auto e = m_recycled_entities.front();
            m_recycled_entities.pop();
            return e;
        } else {
            auto e = m_next_entity;
            m_next_entity.id++;
            return e;
        }
    }

    void World::destroy_entity(Entity e) {
        m_recycled_entities.push(e.generation++);
    }
}