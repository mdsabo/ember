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

        vk::ShaderStageFlagBits get_shader_stage() const;
        std::vector<std::vector<vk::DescriptorSetLayoutBinding>> get_descriptor_set_bindings() const;
        std::vector<vk::PushConstantRange> get_push_constant_ranges() const;
    private:
        SpvReflectShaderModule m_module;
    };

}