#include "DescriptorPool.h"

namespace ember::gpu {

    DescriptorPool::DescriptorPool(vk::DescriptorPool pool, DescriptorSetCounts max_sets):
        m_pool(pool), m_max_allocated(max_sets)
    { }

}