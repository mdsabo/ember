#pragma once

// Disable constructors which enables designated initializers
#define VULKAN_HPP_NO_CONSTRUCTORS

#include <vulkan/vulkan.hpp>

#define vkGetProcAddr(instance, fn) reinterpret_cast<PFN_##fn>(instance.getProcAddr(#fn));

namespace ember::gpu {

    using vk::ArrayProxy;

}
