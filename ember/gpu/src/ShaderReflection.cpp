#include "ShaderReflection.h"

namespace ember::gpu {

    ShaderReflection::ShaderReflection(const SPIRVCode& spirv) {
        auto result = spvReflectCreateShaderModule(spirv_code_size(spirv), spirv.data(), &m_module);
        if (result != SPV_REFLECT_RESULT_SUCCESS) throw std::runtime_error("Failed to reflect spirv code");
    }

    ShaderReflection::~ShaderReflection() {
        spvReflectDestroyShaderModule(&m_module);
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
    std::vector<std::vector<vk::DescriptorSetLayoutBinding>> ShaderReflection::get_descriptor_set_bindings() {
        uint32_t count = 0;
        auto result = spvReflectEnumerateDescriptorSets(&m_module, &count, nullptr);
        if (result != SPV_REFLECT_RESULT_SUCCESS) throw std::runtime_error("Failed to enumerate shader descriptor sets");

        std::vector<SpvReflectDescriptorSet*> descriptor_sets(count);
        result = spvReflectEnumerateDescriptorSets(&m_module, &count, descriptor_sets.data());
        if (result != SPV_REFLECT_RESULT_SUCCESS) throw std::runtime_error("Failed to enumerate shader descriptor sets");

        std::vector<std::vector<vk::DescriptorSetLayoutBinding>> vk_layout_bindings;
        for (const auto reflect_set : descriptor_sets) {
            auto vk_set_layout_bindings = reflect_layout_bindings(
                m_module.shader_stage, 
                reflect_set->binding_count, 
                reflect_set->bindings
            );
            vk_layout_bindings.push_back(vk_set_layout_bindings);
        }

        return vk_layout_bindings;
    }

    std::vector<vk::PushConstantRange> ShaderReflection::get_push_constant_ranges() {
        uint32_t count = 0;
        auto result = spvReflectEnumeratePushConstantBlocks(&m_module, &count, nullptr);
        if (result != SPV_REFLECT_RESULT_SUCCESS) throw std::runtime_error("Failed to enumerate push constant blocks");

        std::vector<SpvReflectBlockVariable*> push_constant_blocks(count);
        result = spvReflectEnumeratePushConstantBlocks(&m_module, &count, push_constant_blocks.data());
        if (result != SPV_REFLECT_RESULT_SUCCESS) throw std::runtime_error("Failed to enumerate push constant blocks");

        std::vector<vk::PushConstantRange> push_constant_ranges(count);
        for (auto i = 0; i < count; i++) {
            push_constant_ranges[i].stageFlags = static_cast<vk::ShaderStageFlags>(m_module.shader_stage),
            push_constant_ranges[i].size = push_constant_blocks[i]->size;
            push_constant_ranges[i].offset = push_constant_blocks[i]->offset;
        }
        return push_constant_ranges;
    }

}