#pragma once

#include <filesystem>
#include <shaderc/shaderc.hpp>
#include <vector>

namespace ember::gpu {
    using SPIRVWord = uint32_t;
    using SPIRVCode = std::vector<SPIRVWord>;

    namespace {
        inline size_t spirv_code_size(const SPIRVCode& spirv) {
            return spirv.size() * sizeof(SPIRVWord);
        }
    }

    SPIRVCode compile_glsl_to_spirv(
        const std::string& glsl,
        const std::string& filename,
        shaderc_shader_kind shader_kind, // #pragma shader_stage(something)
        bool optimize
    );
    SPIRVCode compile_glsl_to_spirv(const std::filesystem::path& path, bool optimize);
}
