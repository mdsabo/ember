#pragma once

#include <map>
#include <set>
#include <vector>

#include "System.h"

namespace ember::ecs {

    using SystemGraph = std::vector<std::set<SystemRunFn>>;

    class SystemGraphBuilder {
    public:
        template<System S>
        inline void add_system() {
            m_nodes.insert(S::run);
            if (!m_edges.contains(S::run)) m_edges.insert({ S::run, {} });
        }

        template<System Src, System Dst>
        inline void order_systems() {
            if (m_edges.contains(Dst::run)) {
                m_edges.at(Dst::run).insert(Src::run);
            } else {
                m_edges.insert({ Dst::run, {Src::run} });
            }
        }

        SystemGraph build();

    private:
        std::set<SystemRunFn> m_nodes;
        std::map<SystemRunFn, std::set<SystemRunFn>> m_edges;
    };

}