#pragma once

#include <spirv_reflect.h>
#include <vector>
#include <vulkan/vulkan.hpp>

#include "SPIRV.h"

namespace ember::gpu {

    class ShaderReflection {
    public:
        ShaderReflection(const SPIRVCode& spirv);
        ~ShaderReflection();

        std::vector<std::vector<vk::DescriptorSetLayoutBinding>> get_descriptor_set_bindings();
        std::vector<vk::PushConstantRange> get_push_constant_ranges();
    private:
        SpvReflectShaderModule m_module;
    };

}