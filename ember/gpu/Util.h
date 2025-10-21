#pragma once

namespace ember::gpu {

    template<typename T>
    inline constexpr bool vkFlagContains(const T& flags, const T& expected) {
        return (flags & expected) == expected;
    }

}