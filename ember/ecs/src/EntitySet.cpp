#include "EntitySet.h"

#include <cassert>

namespace ember::ecs {
    void EntitySet::insert(Entity e) {
        if (e.id >= m_ids.size()) {
            m_ids.resize(e.id+1);
            m_generations.resize(e.id+1);
        }
        
        m_ids.set(e.id);
        m_generations.at(e.id) = e.generation;
    }

    void EntitySet::remove(Entity e) {
        m_ids.reset(e.id);
    }

    std::set<Entity> EntitySet::as_set() const {
        auto set = std::set<Entity>();
        for (auto i = 0; i < this->size(); i++) {
            if (this->contains(i)) set.insert(Entity(this->m_generations[i], i));
        }
        return set;
    }

    std::unordered_set<Entity> EntitySet::as_unordered_set() const {
        auto set = std::unordered_set<Entity>();
        for (auto i = 0; i < this->size(); i++) {
            if (this->contains(i)) set.insert(Entity(this->m_generations[i], i));
        }
        return set;
    }

    // For sparse sets doing it this way is ~25x speedup so it might be ugly
    // but speed is speed.
    namespace {
        void intersect_generations(
            std::vector<uint64_t>& ids, 
            const std::vector<uint32_t>& lhs, 
            const std::vector<uint32_t>& rhs
        ) {
            const auto max_index = std::min(lhs.size(), rhs.size());

            for (auto i = 0; i < ids.size(); i++) {
                auto& word = ids[i];
                if (word != 0) {
                    for (auto j = 0; j < 64 ; j++) {
                        auto index = (i<<6) + j;
                        if (index >= max_index) break;

                        const auto mask = (1ULL << j);
                        if (bool(word & mask) && (lhs[index] != rhs[index])) {
                            word &= ~mask;
                        }
                    }
                }
            }
        }
    }

    EntitySet& EntitySet::operator&=(const EntitySet& rhs) {
        this->m_ids &= rhs.m_ids;
        this->m_generations.resize(this->m_ids.size());

        intersect_generations(this->m_ids.data(), this->m_generations, rhs.m_generations);

        return *this;
    }

    EntitySet operator&(const EntitySet& lhs, const EntitySet& rhs) {
        auto res = EntitySet();

        res.m_ids = lhs.m_ids & rhs.m_ids;

        if (lhs.size() < rhs.size()) res.m_generations = lhs.m_generations;
        else res.m_generations = lhs.m_generations;
        assert(res.m_ids.size() == res.m_generations.size());

        intersect_generations(res.m_ids.data(), lhs.m_generations, rhs.m_generations);

        return res;
    }
}