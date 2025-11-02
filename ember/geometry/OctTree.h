#pragma once

#include <algorithm>
#include <glm/glm.hpp>

#include "Shapes.h"

namespace ember::geometry {

    template<typename T>
    class OctTree {
    public:
        struct Element {
            T data;
            AABB aabb;
        };

        OctTree(const AABB& bounds) {
            const auto extent = bounds.extent();
            m_nodes.emplace_back(extent.min, extent.max);
        }

        void insert(const Element& e) {
            insert(0, 0, e);
        }

        std::vector<T> query(const AABB& aabb) {
            std::vector<T> intersections;
            query(aabb, intersections, 0);
            return intersections;
        }

    private:
        static constexpr auto MAX_DEPTH = 8;
        static constexpr auto NODE_SPLIT_THRESHOLD = 2;

        enum NodeLocation {
            LowerBottomLeft, LowerBottomRight,
            UpperBottomLeft, UpperBottomRight,
            LowerTopLeft, LowerTopRight,
            UpperTopLeft, UpperTopRight
        };
        struct Node {
            AABB::Extent extent;
            size_t first_child;
            std::vector<Element> elements;

            Node(const glm::vec3& min, const glm::vec3& max): extent({ min, max }), first_child(0) { }
            AABB aabb() const { return AABB::from_extent(extent); }
        };
        std::vector<Node> m_nodes;

        void insert(size_t nodeid, unsigned int depth, const Element& e) {
            auto node = &m_nodes.at(nodeid);

            if (node->elements.size() < NODE_SPLIT_THRESHOLD || depth == MAX_DEPTH) {
                node->elements.push_back(e);
            } else {
                if (node->first_child == 0) {
                    create_children(*node);
                    node = &m_nodes.at(nodeid); // creating new nodes invalidates our node pointer
                    const auto insert_child = [this, nodeid, depth](const Element& e) -> bool{
                        return try_insert_element_into_children(nodeid, e, depth);
                    };
                    std::erase_if(node->elements, insert_child);
                }

                if (!try_insert_element_into_children(nodeid, e, depth)) {
                    node->elements.push_back(e);
                } // no else since element was already pushed into the children by move_element if needed
            }
        }

        void create_children(Node& node) {
            const auto extent = node.extent;
            const auto center = (node.extent.min + node.extent.max) * 0.5f;
            node.first_child = m_nodes.size();
            // [0] = Lower Bottom Left (-x, -y, -z)
            m_nodes.emplace_back(extent.min, center);
            // [1] = Lower Bottom Right (+x, -y, -z)
            m_nodes.emplace_back(
                glm::vec3(center.x, extent.min.y, extent.min.z),
                glm::vec3(extent.max.x, center.y, center.z)
            );
            // [2] = Upper Bottom Left (-x, +y, -z)
            m_nodes.emplace_back(
                glm::vec3(extent.min.x, center.y, extent.min.z),
                glm::vec3(center.x, extent.max.y, center.z)
            );
            // [3] = Upper Bottom Right (+x, +y, -z)
            m_nodes.emplace_back(
                glm::vec3(center.x, center.y, extent.min.z),
                glm::vec3(extent.max.x, extent.max.y, center.z)
            );

            // [4] = Lower Top Left (-x, -y, +z)
            m_nodes.emplace_back(
                glm::vec3(extent.min.x, extent.min.y, center.z),
                glm::vec3(center.x, center.y, extent.max.z)
            );
            // [5] = Lower Top Right (+x, -y, +z)
            m_nodes.emplace_back(
                glm::vec3(center.x, extent.min.y, center.z),
                glm::vec3(extent.max.x, center.y, extent.max.z)
            );
            // [6] = Upper Top Left (-x, +y, +z)
            m_nodes.emplace_back(
                glm::vec3(extent.min.x, center.y, center.z),
                glm::vec3(center.x, extent.max.y, extent.max.z)
            );
            // [7] = Upper Top Right (+x, +y, +z)
            m_nodes.emplace_back(center, extent.max);
        }

        bool try_insert_element_into_children(size_t nodeid, const Element& e, size_t depth) {
            auto& node = m_nodes.at(nodeid);
            assert(node.first_child != 0);

            for (auto i = 0; i < 8; i++) {
                const auto child = node.first_child + i;
                if (node_encloses_aabb(child, e.aabb)) {
                    insert(child, depth+1, e);
                    return true;
                }
            }
            return false;
        }

        bool node_encloses_aabb(unsigned int node_index, const AABB& aabb) {
            const auto& node = m_nodes.at(node_index);
            const auto aabb_extent = aabb.extent();

            return glm::all(glm::lessThanEqual(node.extent.min, aabb_extent.min))
                && glm::all(glm::greaterThanEqual(node.extent.max, aabb_extent.max));
        }

        void query(const AABB& aabb, std::vector<T>& intersections, unsigned int nodeid) {
            auto& node = m_nodes.at(nodeid);

            if (intersect(aabb, node.aabb())) {
                for (const auto& e : node.elements) {
                    if (intersect(aabb, e.aabb)) intersections.push_back(e.data);
                }

                if (node.first_child != 0) {
                    for (auto i = 0; i < 8; i++) {
                        query(aabb, intersections, node.first_child + i);
                    }
                }
            }
        }
    };

}