#pragma once

#include <utility>

#include "Shapes.h"

namespace ember::geometry {

    std::pair<float, float> intersect(const Ray& ray, const Sphere& sphere);

}