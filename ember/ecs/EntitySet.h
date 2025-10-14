#pragma once

#include <set>
#include <unordered_set>
#include <vector>

#include "Entity.h"
#include "ember/collections/DynamicBitset.h"

namespace ember::ecs {

    class EntitySet {
    public:
        EntitySet(size_t size = 0): m_ids(size), m_generations(size) { }
        EntitySet(const EntitySet&) = default;
        EntitySet(EntitySet&&) = default;

        inline bool operator[](Entity e) const { return m_ids[e.id]; }
        inline bool contains(Entity e) const { return m_ids.test(e.id); }

        inline size_t size() const { return m_ids.size(); }
        inline void resize(size_t size) { m_ids.resize(size); }

        void insert(Entity e);
        void remove(Entity e);

        std::set<Entity> as_set() const;
        std::unordered_set<Entity> as_unordered_set() const;

        EntitySet& operator&=(const EntitySet& rhs);
        friend EntitySet operator&(const EntitySet& lhs, const EntitySet& rhs);
        
    private:
        collections::DynamicBitset m_ids;
        std::vector<uint32_t> m_generations;
    };

}