#pragma once

#include <cstdint>
#include <utility>

namespace ember::ecs {

    union Entity {
        uint64_t raw;
        struct {
            uint32_t id;
            uint32_t generation;
        };

        constexpr Entity(uint64_t raw): raw(raw) { }
        constexpr Entity(uint32_t generation, uint32_t id): generation(generation), id(id) { }
        constexpr Entity(const Entity&) = default;
        constexpr Entity(Entity&&) = default;

        constexpr inline Entity& operator=(const Entity& rhs) { this->raw = rhs.raw; return *this; }
        constexpr inline Entity& operator=(Entity&& rhs) { this->raw = rhs.raw; return *this; }

        friend constexpr inline std::strong_ordering operator<=>(const Entity& lhs, const Entity& rhs) {
            return lhs.id <=> rhs.id;
        }

        friend constexpr inline bool operator==(const Entity& lhs, const Entity& rhs) {
            return (lhs.id <=> rhs.id) == std::strong_ordering::equal;
        }
    };

}

template<>
struct std::hash<ember::ecs::Entity> {
    std::size_t operator()(const ember::ecs::Entity& e) const noexcept {
        return std::hash<uint64_t>{}(e.raw);
    }
};