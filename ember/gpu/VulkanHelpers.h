#pragma once

#include <vulkan/vulkan.hpp>

namespace ember::gpu {

    constexpr vk::ClearValue CLEAR_COLOR(float r, float g, float b, float a) {
        return vk::ClearValue{.color = { .float32 = {{ r, g, b, a}} }};
    }
    constexpr vk::ClearValue CLEAR_COLOR(int32_t r, int32_t g, int32_t b, int32_t a) {
        return vk::ClearValue{.color = { .int32 = {{ r, g, b, a}} }};
    }

}