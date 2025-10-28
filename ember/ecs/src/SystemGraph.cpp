#include "SystemGraph.h"

#include <algorithm>
#include <stack>

namespace ember::ecs {

    SystemGraph SystemGraphBuilder::build() {
        SystemGraph graph;

        // Remove any edges that point at a non-existant systems just in case
        for (auto& [sys, edge_set] : m_edges) {
            for (auto e : edge_set) {
                if (std::find(m_nodes.begin(), m_nodes.end(), e) == edge_set.end()) {
                    edge_set.erase(e);
                }
            }
        }

        std::set<SystemRunFn> remaining_systems = m_nodes;
        while (!remaining_systems.empty()) {
            graph.emplace_back();
            auto& phase = graph.back();
            for (const auto& [sys, edges] : m_edges) {
                if (edges.empty()) {
                    phase.insert(sys);
                    remaining_systems.erase(sys);
                }
            }

            for (auto sys : phase) {
                for (auto& [s, edges] : m_edges) {
                    edges.erase(sys);
                }
            }
        }

        return graph;
    }

}