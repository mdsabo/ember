#include "SPIRV.h"

#include <algorithm>
#include <array>
#include <shaderc/shaderc.hpp>

#include "ember/util/Filesystem.h"
#include "ember/util/Log.h"

namespace ember::gpu {

    namespace {
        shaderc_shader_kind file_shader_kind(const std::filesystem::path& path) {
            static constexpr std::array SHADER_KIND_MAP = {
                std::make_pair(".comp", shaderc_compute_shader),
                std::make_pair(".vert", shaderc_vertex_shader),
                std::make_pair(".frag", shaderc_fragment_shader),
            };

            auto extension = path.extension();
            for (const auto& [ext, shader_kind] : SHADER_KIND_MAP) {
                if (extension == ext) return shader_kind;
            }
            throw std::runtime_error("Invalid shader file extension");
        }
    }

    SPIRVCode compile_glsl_to_spirv(
        const std::string& glsl,
        const std::string& filename,
        shaderc_shader_kind shader_kind,
        bool optimize
    ) {
        shaderc::CompileOptions options;
        options.SetGenerateDebugInfo();
        if (optimize) options.SetOptimizationLevel(shaderc_optimization_level_size);

        shaderc::Compiler compiler;
        auto spv = compiler.CompileGlslToSpv(
            glsl,
            shader_kind,
            filename.c_str(),
            options
        );

        if (spv.GetCompilationStatus() != shaderc_compilation_status_success) {
            printf("%s\n", spv.GetErrorMessage().c_str());
            throw std::runtime_error("Failed to compile shader!");
        } else {
            return std::vector(spv.begin(), spv.end());
        }
    }

    SPIRVCode compile_glsl_to_spirv(const std::filesystem::path& path, bool optimize) {
        auto source_text = ember::util::read_file_contents(path);
        auto shader_kind = file_shader_kind(path);
        info(EMBER_GPU_LOG, "Loaded shader from file {}, inferred shader type: {}", path.string(), (int)shader_kind);
        return compile_glsl_to_spirv(source_text, path.filename().string(), shader_kind, optimize);
    }

}