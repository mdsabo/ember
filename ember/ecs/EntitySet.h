#pragma once

#include <vector>

#include "collections/DynamicBitset.h"

namespace ember::ecs {

    class EntitySet {
    public:

    private:
        collections::DynamicBitset m_valid;
    }

}