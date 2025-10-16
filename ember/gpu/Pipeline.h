#pragma once

#include "Vulkan.h"

namespace ember::gpu {

    enum class PipelineType {
        Compute,
        Graphics
    };

    struct Pipeline {
        PipelineType type;
        vk::PipelineLayout layout;
        vk::Pipeline pipeline;
    };

}