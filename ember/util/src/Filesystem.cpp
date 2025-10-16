#include "Filesystem.h"

#include <fstream>
#include <sstream>

namespace ember::util {
    std::string read_file_contents(const std::filesystem::path& path) {
        std::ifstream file(path.c_str(), std::ios::binary);
        if (!file.is_open()) throw std::runtime_error("Failed to open file!");

        std::stringstream ss;
        ss << file.rdbuf();
        return ss.str();
    }
}