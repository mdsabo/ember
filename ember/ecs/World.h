#pragma once

#include <any>
#include <shared_mutex>
#include <memory>
#include <typeindex>
#include <unordered_map>

#include "Component.h"

namespace ember::ecs {

    class World {
    public:

    private:
        std::unordered_map<std::type_index, std::any> m_components;
    };

}