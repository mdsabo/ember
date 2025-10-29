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
        void add_system() {
            m_systems.insert(S::run);
            m_dependencies.insert({ S::run, {} });
        }

        template<System Src, System Dst>
        void order_systems() {
            add_system<Src>();
            add_system<Dst>();
            m_dependencies.at(Dst::run).insert(Src::run);
        }

        SystemGraph build();

    private:
        std::set<SystemRunFn> m_systems;
        std::map<SystemRunFn, std::set<SystemRunFn>> m_dependencies;
    };

}