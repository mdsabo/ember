#pragma once

#include <algorithm>
#include <array>
#include <glm/glm.hpp>

#include "Shapes.h"
#include "ember/collections/SmallVector.h"

namespace ember::geometry {

    template<typename T>
    class QuadTree {
    public:
        struct Element {
            T data;
            AABB aabb;
        };

        QuadTree(const AABB& bounds, uint32_t elements = 5000) {
            m_nodes.reserve(1000);
            m_elements.reserve(elements);
            m_nodes.emplace_back(bounds);
        }

        ~QuadTree() {
            // size_t elements = 0;
            // for (const auto& node : m_nodes) {
            //     printf("Element Count: %zu\n", node.elements.size());
            //     elements += node.elements.size();
            // }
            // printf("Nodes: %zu, Elements: %zu\n", m_nodes.size(), m_elements.size());
        }

        void insert(const Element& e) {
            assert(node_encloses_obj(ROOT_NODE_ID, ExtentXZ(e.aabb)));
            const auto eid = static_cast<ElementId>(m_elements.size());
            m_elements.push_back(e);
            insert(0, 0, eid);
        }

        std::vector<T> query(const AABB& aabb) {
            std::vector<T> intersections;
            query(aabb, intersections, 0);
            return intersections;
        }

    private:
        using NodeId = uint32_t;
        using ElementId = uint32_t;

        struct ExtentXZ {
            // .x = x
            // .y = z
            glm::vec2 min;
            glm::vec2 max;

            ExtentXZ(const glm::vec2& min, const glm::vec2& max): min(min), max(max) { }
            ExtentXZ(const AABB& aabb) {
                min.x = aabb.center.x - aabb.half_size.x;
                min.y = aabb.center.z - aabb.half_size.z;
                max.x = aabb.center.x + aabb.half_size.x;
                max.y = aabb.center.z + aabb.half_size.z;
            }
        };

        struct Node {
            ExtentXZ extent;
            uint32_t children;
            collections::SmallVector<ElementId> elements;

            Node(const ExtentXZ& extent):
                extent(extent), children(0), elements()
            { }
            glm::vec2 center() const { return (extent.min + extent.max) * 0.5f; }
        };

        static constexpr auto NULL_CHILDREN = 0;
        static constexpr auto MAX_DEPTH = 8;
        static constexpr auto NODE_SPLIT_THRESHOLD = 8;
        static constexpr NodeId ROOT_NODE_ID = 0;

        std::vector<Node> m_nodes;
        std::vector<Element> m_elements;

        inline Node& get_node(NodeId nodeid) { return m_nodes.at(nodeid); }

        void insert(NodeId nodeid, unsigned int depth, ElementId eid) {
            assert(depth <= MAX_DEPTH);
            assert(nodeid < m_nodes.size());

            Node& node = get_node(nodeid);
            if (node.elements.size() < NODE_SPLIT_THRESHOLD || depth == MAX_DEPTH) {
                node.elements.push_back(eid);
            } else {
                if (node.children == NULL_CHILDREN) {
                    create_children(nodeid);
                    move_elements_to_children(nodeid, depth);
                }

                insert_element_including_children(nodeid, eid, depth);
            }
        }

        void create_children(NodeId nodeid) {
            auto& node = get_node(nodeid);
            const auto node_center = node.center();

            const std::array<Node, 4> children {
                Node({node.extent.min, node_center}),
                Node({
                    glm::vec2(node_center.x, node.extent.min.y),
                    glm::vec2(node.extent.max.x, node_center.y)
                }),
                Node({
                    glm::vec2(node.extent.min.x, node_center.y),
                    glm::vec2(node_center.x, node.extent.max.y)
                }),
                Node({node_center, node.extent.max})
            };

            node.children = m_nodes.size();
            m_nodes.insert(m_nodes.end(), children.begin(), children.end());
        }

        void move_elements_to_children(NodeId nodeid, size_t depth) {
            const auto node_center = get_node(nodeid).center();
            auto elements = std::move(get_node(nodeid).elements);

            for (const auto eid : elements) {
                insert_element_including_children(nodeid, eid, depth);
            }
        }

        uint32_t get_child_index(const glm::vec2& node_center, const glm::vec3& obj_center) {
            uint32_t index = 0;
            if (obj_center.x > node_center.x) index += 1;
            if (obj_center.z > node_center.y) index += 2;
            return index;
        }

        bool node_encloses_obj(NodeId nodeid, const ExtentXZ& obj) {
            const auto& node = get_node(nodeid);

            return node.extent.min.x <= obj.min.x
                && node.extent.max.x > obj.max.x
                && node.extent.min.y <= obj.min.y
                && node.extent.max.y > obj.max.y;
        }

        void insert_element_including_children(NodeId nodeid, ElementId eid, size_t depth) {
            auto& node = get_node(nodeid);
            const auto node_center = node.center();

            const auto& element = m_elements.at(eid);

            auto child = node.children + get_child_index(node_center, element.aabb.center);
            if (node_encloses_obj(child, ExtentXZ(element.aabb))) {
                insert(child, depth + 1, eid);
            } else {
                node.elements.push_back(eid);
            }
        }

        void query(const AABB& aabb, std::vector<T>& intersections, unsigned int nodeid) {
            if (intersects_node(nodeid, aabb)) {
                const auto& node = get_node(nodeid);
                for (const auto eid : node.elements) {
                    const auto& element = m_elements.at(eid);
                    if (intersect(aabb, element.aabb)) intersections.push_back(element.data);
                }

                if (node.children != 0) {
                    for (auto i = 0; i < 4; i++) {
                        query(aabb, intersections, node.children + i);
                    }
                }
            }
        }

        bool intersects_node(NodeId nodeid, const AABB& aabb) {
            // SAT test over x/z axes
            // If the abs distance from center to center is less than the sum of half_widths
            // then there must be some overlap on that axis, continue to the next axis
            // If no axis can separate them, then they must intersect
            const auto& node = get_node(nodeid);
            const auto node_center = node.center();
            const auto node_half_size = node_center - node.extent.min;

            const auto dx = node_center.x - aabb.center.x;
            if (std::abs(dx) >= (node_half_size.x + aabb.half_size.x)) return false;

            const auto dz = node_center.y - aabb.center.z;
            if (std::abs(dz) >= (node_half_size.y + aabb.half_size.z)) return false;

            // No separating axis, they intersect
            return true;
        }
    };

}