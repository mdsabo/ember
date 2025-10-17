#pragma once

#include "Vulkan.h"

namespace ember::gpu {

    struct Pipeline {
    // Maybe make it opaque to everything but the driver itself?
    // But then it's not an aggregate type
    //     friend class Renderer;
    //     friend class CommandRecorder;
    // private:
        vk::PipelineBindPoint bind_point;
        vk::PipelineLayout layout;
        vk::Pipeline pipeline;
    };

}