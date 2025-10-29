#pragma once

#include <concepts>

#include "Storage.h"

namespace ember::ecs {

    // A component is required to be
    // 1. Default initializable (required by most data structures)
    // 2. Trivially desctructible: 'static in Rust terms, POD types only
    // 3. Have an associated storage type

    template<typename T>
    concept Component = std::default_initializable<T>
        && std::is_trivially_destructible_v<T>
        && ComponentStorage<typename T::Storage>;

}