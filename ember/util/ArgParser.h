#pragma once

#include <string_view>
#include <vector>

namespace ember::util {

    class ArgParser {
    public:
        ArgParser(int argc, const char* const* argv);

        bool is_set(const std::string_view& arg);
    private:
        std::vector<std::string_view> m_args;
    };

}