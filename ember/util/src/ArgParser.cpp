#include "ArgParser.h"

#include <cstdio>
#include <format>
#include <stdexcept>

namespace ember::util {

    ArgParser::ArgParser(int argc, const char* const* argv): m_argv(argc) {
        for (auto i = 0; i < argc; i++) {
            m_argv[i] = argv[i];
        }

        add_argument("-h", "--help", "Show this help text and exit");
    }

    void ArgParser::add_argument(
        const std::string_view short_opt,
        const std::string_view long_opt,
        const std::string_view help_text
    ) {
        m_args.push_back(ArgInfo{ short_opt, long_opt, help_text });
    }

    void ArgParser::show_help() {
        printf("%s:\n", m_argv[0].data());
        for (const auto& arg : m_args) {
            auto opts = std::format(
                "{}{}{}",
                arg.short_opt.size() ? arg.short_opt.data() : "",
                (arg.short_opt.size() && arg.long_opt.size()) ? ", " : "",
                arg.long_opt.size() ? arg.long_opt.data() : ""
            );
            printf(
                "%-24s| %s\n",
                opts.data(),
                arg.help_text.size() ? arg.help_text.data() : ""
            );
        }
        exit(0);
    }

    namespace {
        template <class Iter>
        Iter find_arg(
            Iter begin,
            Iter end,
            const std::string_view arg
        ) {
            return std::find_if(
                begin,
                end,
                [arg](const std::string_view& argv) { return argv.starts_with(arg); }
            );
        }
    } // namespace

    bool ArgParser::is_set(const std::string_view arg) {
        return find_arg(m_argv.begin(), m_argv.end(), arg) != m_argv.end();
    }

    std::string_view ArgParser::arg_value(const std::string_view arg) {
        auto iter = find_arg(m_argv.begin(), m_argv.end(), arg);
        if (iter != m_argv.end()) {
            auto argv = *iter;
            auto equal = argv.find("=");
            if (equal != std::string_view::npos) {
                return argv.substr(equal+1);
            } else {
                iter++;
                if (iter != m_argv.end()) return *iter;
                else throw std::out_of_range("Argument index out of range!");
            }
        } else {
            return {};
        }
    }

}
