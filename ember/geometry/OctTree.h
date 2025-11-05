#pragma once

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdio>
#include <glm/glm.hpp>

#include "Shapes.h"
#include "ember/collections/LinearLinkedLists.h"

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
            assert(node_encloses_aabb(m_nodes.at(0), e.aabb));
            insert(0, 0, e);
        }

        std::vector<T> query(const AABB& aabb) {
            std::vector<T> intersections;
            query(aabb, intersections, 0);
            return intersections;
        }

    private:
        static constexpr auto MAX_DEPTH = 8;
        static constexpr auto NODE_SPLIT_THRESHOLD = 8;
        static constexpr auto NO_ELEMENTS = collections::LinearLinkedLists<T, uint32_t>::END_OF_LIST;

        struct Node {
            AABB::Extent extent;
            uint32_t children;
            uint32_t elements;
            uint32_t num_elements;

            Node(const glm::vec3& min, const glm::vec3& max):
                extent({ min, max }), children(0), elements(NO_ELEMENTS), num_elements(0)
            { }
            glm::vec3 center() const { return (extent.min + extent.max) * 0.5f; }
            AABB aabb() const { return AABB::from_extent(extent); }
        };
        std::vector<Node> m_nodes;
        collections::LinearLinkedLists<Element, uint32_t> m_elements;

        void insert(uint32_t nodeid, unsigned int depth, const Element& e) {
            assert(depth <= MAX_DEPTH);
            assert(nodeid < m_nodes.size());

            auto node = &m_nodes.at(nodeid);

            if ((node->num_elements < NODE_SPLIT_THRESHOLD) || (depth == MAX_DEPTH)) {
                m_elements.insert_into(node->elements, e);
                node->num_elements++;
            } else {
                const auto node_center = node->center();
                if (node->children == 0) {
                    create_children(*node);
                    node = &m_nodes.at(nodeid); // creating new nodes invalidates our node pointer

                    std::vector<T> removed;
                    removed.reserve(NODE_SPLIT_THRESHOLD);
                    for (auto iter = m_elements.begin(node->elements); iter != m_elements.end(); iter++) {
                        const auto child = node->children + get_child_index(node_center, e.aabb.center);
                        if (node_encloses_aabb(m_nodes.at(child), e.aabb)) {
                            insert(child, depth + 1, e);
                            removed.push_back(e.data);
                        }
                    }


                    for (const auto r : removed) {
                        m_elements.erase_if(node->elements, [r](const Element& e){
                            return e.data == r;
                        });
                    }
                    node->num_elements -= removed.size();
                }

                const auto child = node->children + get_child_index(node_center, e.aabb.center);
                if (node_encloses_aabb(m_nodes.at(child), e.aabb)) {
                    insert(child, depth + 1, e);
                } else {
                    m_elements.insert_into(node->elements, e);
                    node->num_elements++;
                }
            }
        }

        void create_children(Node& node) {
            const auto extent = node.extent;
            const auto center = node.center();
            node.children = m_nodes.size();

            const std::array<Node, 8> children {
                // [0] = Lower Bottom Left (-x, -y, -z)
                Node(extent.min, center),
                // [1] = Lower Bottom Right (+x, -y, -z)
                Node(
                    glm::vec3(center.x, extent.min.y, extent.min.z),
                    glm::vec3(extent.max.x, center.y, center.z)
                ),
                // [2] = Upper Bottom Left (-x, +y, -z)
                Node(
                    glm::vec3(extent.min.x, center.y, extent.min.z),
                    glm::vec3(center.x, extent.max.y, center.z)
                ),
                // [3] = Upper Bottom Right (+x, +y, -z)
                Node(
                    glm::vec3(center.x, center.y, extent.min.z),
                    glm::vec3(extent.max.x, extent.max.y, center.z)
                ),
                // [4] = Lower Top Left (-x, -y, +z)
                Node(
                    glm::vec3(extent.min.x, extent.min.y, center.z),
                    glm::vec3(center.x, center.y, extent.max.z)
                ),
                // [5] = Lower Top Right (+x, -y, +z)
                Node(
                    glm::vec3(center.x, extent.min.y, center.z),
                    glm::vec3(extent.max.x, center.y, extent.max.z)
                ),
                // [6] = Upper Top Left (-x, +y, +z)
                Node(
                    glm::vec3(extent.min.x, center.y, center.z),
                    glm::vec3(center.x, extent.max.y, extent.max.z)
                ),
                // [7] = Upper Top Right (+x, +y, +z)
                Node(center, extent.max)
            };
            m_nodes.insert(m_nodes.end(), children.begin(), children.end());
        }

        uint32_t get_child_index(const glm::vec3& node_center, const glm::vec3& obj_center) {
            uint32_t index = 0;
            if (obj_center.x > node_center.x) index += 1;
            if (obj_center.y > node_center.y) index += 2;
            if (obj_center.z > node_center.z) index += 4;
            return index;
        }

        bool node_encloses_aabb(const Node& node, const AABB& aabb) {
            const auto aabb_extent = aabb.extent();

            return node.extent.min.x <= aabb_extent.min.x
                && node.extent.max.x >= aabb_extent.max.x
                && node.extent.min.y <= aabb_extent.min.y
                && node.extent.max.y >= aabb_extent.max.y
                && node.extent.min.z <= aabb_extent.min.z
                && node.extent.max.z >= aabb_extent.max.z;
        }

        void query(const AABB& aabb, std::vector<T>& intersections, unsigned int nodeid) {
            auto& node = m_nodes.at(nodeid);

            if (intersect(aabb, node.aabb())) {
                for (auto iter = m_elements.begin(node.elements); iter != m_elements.end(); iter++) {
                    if (intersect(aabb, iter->aabb)) intersections.push_back(iter->data);
                }

                if (node.children != 0) {
                    for (auto i = 0; i < 8; i++) {
                        query(aabb, intersections, node.children + i);
                    }
                }
            }
        }
    };

}