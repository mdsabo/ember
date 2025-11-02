#pragma once

#include <concepts>
#include <vector>

#include "Entity.h"
#include "ember/collections/DynamicBitset.h"

namespace ember::ecs {

    class EntitySet {
    public:
        EntitySet(size_t size = 0): m_ids(size), m_generations(size, 0) { }
        EntitySet(const EntitySet&) = default;
        EntitySet(EntitySet&&) = default;

        inline bool operator[](Entity e) const {
            return m_ids[e.id] && (e.generation == m_generations[e.id]);
        }
        inline bool contains(Entity e) const {
            return m_ids.test(e.id) && (e.generation == m_generations.at(e.id));
        }

        inline size_t size() const { return m_ids.size(); }
        inline void resize(size_t size) { m_ids.resize(size); }

        void insert(Entity e);
        void remove(Entity e);

        class iterator {
        public:
            using value_type = Entity;
            using difference_type = ptrdiff_t;

            iterator() = default;
            iterator(uint32_t id, const collections::DynamicBitset* ids, const uint32_t* generations):
                id(id), ids(ids), generations(generations)
            {
                find_valid_id();
            }
            iterator(const iterator& other):
                id(other.id), ids(other.ids), generations(other.generations)
            { }

            Entity operator*() const {
                return Entity(generations[id], id);
            }

            iterator& operator++() {
                id++;
                find_valid_id();
                return *this;
            }

            iterator operator++(int) {
                auto tmp = *this;
                ++*this;
                return tmp;
            }

            bool operator==(const iterator& rhs) const {
                return (id == rhs.id) && (ids == rhs.ids) && (generations == rhs.generations);
            }
            bool operator!=(const iterator& rhs) const {
                return !(*this == rhs);
            }
        private:
            uint32_t id;
            const collections::DynamicBitset* ids;
            const uint32_t* generations;

            void find_valid_id() {
                while (!ids->test(id) && id < ids->size()) id++;
            }
        };
        static_assert(std::forward_iterator<iterator>);

        inline iterator begin() const {
            return iterator(0, &m_ids, m_generations.data());
        }
        inline iterator end() const {
            return iterator(m_generations.size(), &m_ids, m_generations.data());\
        }

        std::vector<Entity> as_vec() const;

        EntitySet& operator&=(const EntitySet& rhs);
        friend EntitySet operator&(const EntitySet& lhs, const EntitySet& rhs);

    private:
        collections::DynamicBitset m_ids;
        std::vector<uint32_t> m_generations;
    };

}