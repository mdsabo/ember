#include "SystemGraph.h"

namespace ember::ecs {

    SystemGraph SystemGraphBuilder::build() {
        SystemGraph graph;


        std::set<SystemRunFn> remaining_systems = m_systems;
        while (!remaining_systems.empty()) {
            std::set<SystemRunFn> removal_set;
            auto& phase = graph.emplace_back();

            for (const auto& [sys, edges] : m_dependencies) {
                if (edges.empty()) {
                    phase.insert(sys);
                    remaining_systems.erase(sys);
                    removal_set.insert(sys);
                }
            }

            for (auto sys : phase) {
                for (auto& [s, edges] : m_dependencies) {
                    edges.erase(sys);
                }
            }

            for (const auto sys : removal_set) m_dependencies.erase(sys);
        }

        return graph;
    }

}