#pragma once

#include <spirv_reflect.h>
#include <vector>
#include <vulkan/vulkan.hpp>

#include "SPIRV.h"

namespace ember::gpu {

    class ShaderReflection {
    public:
        ShaderReflection(const SPIRVCode& spirv);
        ShaderReflection(const ShaderReflection&) = delete;

        vk::ShaderStageFlagBits get_shader_stage() const;
        std::vector<vk::VertexInputAttributeDescription> get_input_attributes() const;
        std::vector<vk::Format> get_output_formats() const;
        std::vector<std::vector<vk::DescriptorSetLayoutBinding>> get_descriptor_set_bindings() const;
        std::vector<vk::PushConstantRange> get_push_constant_ranges() const;
    private:
        spv_reflect::ShaderModule m_module;
    };

}