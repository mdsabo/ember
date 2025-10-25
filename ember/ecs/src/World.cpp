#include "World.h"

#include <Eigen/Dense>
#include "TransformComponent.h"

namespace ember::ecs {
    World::World(): m_next_entity(Entity(WORLD_ORIGIN_ENTITY.id + 1)) {
        add_component<TransformComponent>();
        write_component<TransformComponent>().insert(WORLD_ORIGIN_ENTITY, TransformComponent{
            .transform = Eigen::Affine3f::Identity()
        });
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
        storage.insert(e, { Eigen::Affine3f::Identity() });
        return e;
    }

    void World::destroy_entity(Entity e) {
        assert(e != WORLD_ORIGIN_ENTITY);
        m_recycled_entities.push(e.generation++);
    }
}