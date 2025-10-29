#include "Allocators.h"

#include <cassert>

namespace ember::util {

    StackAllocator::StackAllocator(size_t size): m_top(0), m_size(size) {
        m_memory = std::make_unique<uint8_t[]>(size);
    }

    StackAllocator::~StackAllocator() {
        reset();
    }

    void* StackAllocator::malloc(size_t size) {
        m_max_size = std::max(m_max_size, m_top + size);

        if ((m_top + size) <= m_size) {
            auto ptr = &m_memory[m_top];
            m_top += size;
            return ptr;
        } else {
#if defined(EMBER_STACK_ALLOCATOR_ASSERT_ON_OVERFLOW)
            assert(0 && "Stack allocator overflowed!");
#else
            auto ptr = malloc(size);
            m_overflow.push_back(ptr);
            return ptr;
#endif
        }
    }

    void StackAllocator::reset() {
        m_top = 0;
#if !defined(EMBER_STACK_ALLOCATOR_ASSERT_ON_OVERFLOW)
        for (const auto& ptr : m_overflow) free(ptr);
        m_overflow.clear();
#endif
    }

}