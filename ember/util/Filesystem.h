#pragma once

#include <string>
#include <filesystem>

namespace ember::util {
    std::string read_file_contents(const std::filesystem::path& path);
}