#include "ShaderReflection.h"

namespace ember::gpu {

    ShaderReflection::ShaderReflection(const SPIRVCode& spirv): m_module(spirv) { }

    vk::ShaderStageFlagBits ShaderReflection::get_shader_stage() const {
        return static_cast<vk::ShaderStageFlagBits>(m_module.GetShaderStage());
    }

    namespace {
        constexpr uint32_t vertex_stride(SpvReflectFormat format) {
            switch(format) {
                case SPV_REFLECT_FORMAT_R16_UINT:
                case SPV_REFLECT_FORMAT_R16_SINT:
                case SPV_REFLECT_FORMAT_R16_SFLOAT:
                    return 2;

                case SPV_REFLECT_FORMAT_R16G16_UINT:
                case SPV_REFLECT_FORMAT_R16G16_SINT:
                case SPV_REFLECT_FORMAT_R16G16_SFLOAT:
                case SPV_REFLECT_FORMAT_R32_UINT:
                case SPV_REFLECT_FORMAT_R32_SINT:
                case SPV_REFLECT_FORMAT_R32_SFLOAT:
                    return 4;

                case SPV_REFLECT_FORMAT_R16G16B16_UINT:
                case SPV_REFLECT_FORMAT_R16G16B16_SINT:
                case SPV_REFLECT_FORMAT_R16G16B16_SFLOAT:
                    return 6;

                case SPV_REFLECT_FORMAT_R16G16B16A16_UINT:
                case SPV_REFLECT_FORMAT_R16G16B16A16_SINT:
                case SPV_REFLECT_FORMAT_R16G16B16A16_SFLOAT:
                case SPV_REFLECT_FORMAT_R32G32_UINT:
                case SPV_REFLECT_FORMAT_R32G32_SINT:
                case SPV_REFLECT_FORMAT_R32G32_SFLOAT:
                case SPV_REFLECT_FORMAT_R64_UINT:
                case SPV_REFLECT_FORMAT_R64_SINT:
                case SPV_REFLECT_FORMAT_R64_SFLOAT:
                    return 8;

                case SPV_REFLECT_FORMAT_R32G32B32_UINT:
                case SPV_REFLECT_FORMAT_R32G32B32_SINT:
                case SPV_REFLECT_FORMAT_R32G32B32_SFLOAT:
                    return 12;

                case SPV_REFLECT_FORMAT_R32G32B32A32_UINT:
                case SPV_REFLECT_FORMAT_R32G32B32A32_SINT:
                case SPV_REFLECT_FORMAT_R32G32B32A32_SFLOAT:
                case SPV_REFLECT_FORMAT_R64G64_UINT:
                case SPV_REFLECT_FORMAT_R64G64_SINT:
                case SPV_REFLECT_FORMAT_R64G64_SFLOAT:
                    return 16;

                case SPV_REFLECT_FORMAT_R64G64B64_UINT:
                case SPV_REFLECT_FORMAT_R64G64B64_SINT:
                case SPV_REFLECT_FORMAT_R64G64B64_SFLOAT:
                    return 24;

                case SPV_REFLECT_FORMAT_R64G64B64A64_UINT:
                case SPV_REFLECT_FORMAT_R64G64B64A64_SINT:
                case SPV_REFLECT_FORMAT_R64G64B64A64_SFLOAT:
                    return 32;

                // This shouldn't occur right?
                case SPV_REFLECT_FORMAT_UNDEFINED:
                    return 0;
            }
        }
    }

    std::vector<vk::VertexInputAttributeDescription> ShaderReflection::get_input_attributes() const {
        uint32_t count = 0;
        auto result = m_module.EnumerateInputVariables(&count, nullptr);
        if (result != SPV_REFLECT_RESULT_SUCCESS) throw std::runtime_error("Failed to enumerate shader interface variables");

        std::vector<SpvReflectInterfaceVariable*> input_variables(count);
        result = m_module.EnumerateInputVariables(&count, input_variables.data());
        if (result != SPV_REFLECT_RESULT_SUCCESS) throw std::runtime_error("Failed to enumerate shader interface variables");

        uint32_t offset = 0;
        std::vector<vk::VertexInputAttributeDescription> input_attributes(input_variables.size());
        for (auto i = 0; i < input_variables.size(); i++) {
            // FIXME: Really not sure how these indexes map to each other
            input_attributes[i].binding = input_variables[i]->location;
            input_attributes[i].location = input_variables[i]->component;
            input_attributes[i].format = static_cast<vk::Format>(input_variables[i]->format);
            input_attributes[i].offset = offset;
            offset += vertex_stride(input_variables[i]->format);
        }
        return input_attributes;
    }

    std::vector<vk::Format> ShaderReflection::get_output_formats() const {
        uint32_t count = 0;
        auto result = m_module.EnumerateOutputVariables(&count, nullptr);
        if (result != SPV_REFLECT_RESULT_SUCCESS) throw std::runtime_error("Failed to enumerate shader interface variables");

        std::vector<SpvReflectInterfaceVariable*> output_variables(count);
        result = m_module.EnumerateOutputVariables(&count, output_variables.data());
        if (result != SPV_REFLECT_RESULT_SUCCESS) throw std::runtime_error("Failed to enumerate shader interface variables");

        uint32_t offset = 0;
        std::vector<vk::Format> output_formats(output_variables.size());
        for (auto i = 0; i < output_variables.size(); i++) {
            output_formats[i] = static_cast<vk::Format>(output_variables[i]->format);
        }
        return output_formats;
    }

    namespace {
        uint32_t descriptor_count(const size_t dims_count, const uint32_t* dims) {
            uint32_t count = 1;
            for (auto i = 0; i < dims_count; i++) {
                count *= dims[i];
            }
            return count;
        }

        std::vector<vk::DescriptorSetLayoutBinding> reflect_layout_bindings(
            SpvReflectShaderStageFlagBits shader_stage,
            size_t binding_count,
            const SpvReflectDescriptorBinding* const* reflect_bindings
        ) {
            std::vector<vk::DescriptorSetLayoutBinding> vk_layout_bindings(binding_count);
            for (auto i = 0; i < binding_count; i++) {
                auto& vk_layout_binding = vk_layout_bindings[i];
                auto reflect_binding = reflect_bindings[i];

                vk_layout_binding.binding = reflect_binding->binding;
                vk_layout_binding.descriptorType = static_cast<vk::DescriptorType>(reflect_binding->descriptor_type);
                vk_layout_binding.descriptorCount = descriptor_count(reflect_binding->array.dims_count, reflect_binding->array.dims);
                vk_layout_binding.stageFlags = static_cast<vk::ShaderStageFlags>(shader_stage);
            }

            return vk_layout_bindings;
        }
    }

    // See https://github.com/KhronosGroup/SPIRV-Reflect/blob/main/examples/main_descriptors.cpp
    std::vector<std::vector<vk::DescriptorSetLayoutBinding>> ShaderReflection::get_descriptor_set_bindings() const {
        uint32_t count = 0;
        auto result = m_module.EnumerateDescriptorSets(&count, nullptr);
        if (result != SPV_REFLECT_RESULT_SUCCESS) throw std::runtime_error("Failed to enumerate shader descriptor sets");

        std::vector<SpvReflectDescriptorSet*> descriptor_sets(count);
        result = m_module.EnumerateDescriptorSets(&count, descriptor_sets.data());
        if (result != SPV_REFLECT_RESULT_SUCCESS) throw std::runtime_error("Failed to enumerate shader descriptor sets");

        std::vector<std::vector<vk::DescriptorSetLayoutBinding>> vk_layout_bindings;
        vk_layout_bindings.reserve(descriptor_sets.size());
        for (const auto reflect_set : descriptor_sets) {
            auto vk_set_layout_bindings = reflect_layout_bindings(
                m_module.GetShaderStage(),
                reflect_set->binding_count,
                reflect_set->bindings
            );
            vk_layout_bindings.push_back(vk_set_layout_bindings);
        }

        return vk_layout_bindings;
    }

    std::vector<vk::PushConstantRange> ShaderReflection::get_push_constant_ranges() const {
        uint32_t count = 0;
        auto result = m_module.EnumeratePushConstantBlocks(&count, nullptr);
        if (result != SPV_REFLECT_RESULT_SUCCESS) throw std::runtime_error("Failed to enumerate push constant blocks");

        std::vector<SpvReflectBlockVariable*> push_constant_blocks(count);
        result = m_module.EnumeratePushConstantBlocks(&count, push_constant_blocks.data());
        if (result != SPV_REFLECT_RESULT_SUCCESS) throw std::runtime_error("Failed to enumerate push constant blocks");

        std::vector<vk::PushConstantRange> push_constant_ranges(count);
        for (auto i = 0; i < count; i++) {
            push_constant_ranges[i].stageFlags = static_cast<vk::ShaderStageFlags>(m_module.GetShaderStage()),
            push_constant_ranges[i].size = push_constant_blocks[i]->size;
            push_constant_ranges[i].offset = push_constant_blocks[i]->offset;
        }
        return push_constant_ranges;
    }

}