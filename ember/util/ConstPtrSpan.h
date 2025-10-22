#pragma once

#include <span>

namespace ember::util {
    // Turns out the compile is pretty restrictive when it comes to spans and type resolution
    // so add a bit of syntatic sugar to help it out.
    template<typename T>
    using ConstPtrSpan = std::span<const T* const>;
}