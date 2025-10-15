#include "ArgParser.h"

#include <cstdio>

namespace ember::util {

    ArgParser::ArgParser(int argc, const char* const* argv): m_args(argc-1) {
        for (auto i = 1; i < argc; i++) {
            m_args[i-1] = argv[i];
        }
    }

    bool ArgParser::is_set(const std::string_view& arg) {
        return std::find(m_args.begin(), m_args.end(), arg) != m_args.end();
    }

}