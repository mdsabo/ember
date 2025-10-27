#pragma once

#include <atomic>
#include <unordered_map>

namespace ember::util {

    template<typename T>
    class ResourceManager {
    public:
        ResourceManager(): m_next_id(0) { }

        uint64_t insert(const T& obj) {
            auto id = m_next_id++;
            m_objects.insert({ id, obj });
            return id;
        }

        void remove(uint64_t id) {
            m_objects.erase(id);
        }

        const T& at(uint64_t id) const {
            return m_objects.at(id);
        }

        T& at(uint64_t id) {
            return m_objects.at(id);
        }

    private:
        std::uint64_t m_next_id;
        std::unordered_map<uint64_t, T> m_objects;
    };

}