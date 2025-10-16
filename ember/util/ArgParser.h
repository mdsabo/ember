#pragma once

#include <string_view>
#include <vector>

namespace ember::util {

    // ./program -opt1 --opt-2 -opt3 64 --opt-4 100 arg1 arg2

    class ArgParser {
    public:
        ArgParser(int argc, const char* const* argv);

        void add_argument(
            const std::string_view short_opt,
            const std::string_view long_opt,
            const std::string_view help_text
        );

        void show_help();

        bool is_set(const std::string_view arg);
        std::string_view arg_value(const std::string_view arg);
    private:
        struct ArgInfo {
            std::string_view short_opt;
            std::string_view long_opt;
            std::string_view help_text;
        };
        std::vector<ArgInfo> m_args;
        std::vector<std::string_view> m_argv;
    };

}