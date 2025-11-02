#include "ParticleCollisionSystem.h"

#include <cassert>

#include "ParticleComponent.h"
#include "ember/ecs/World.h"
#include "ember/geometry/Intersect.h"
#include "ember/util/Allocators.h"

namespace ember::physics {
    void ParticleCollisionSystem::init(ecs::World& world) {
        world.add_component<ParticleComponent>();
    }

    namespace {
        struct ParticleContact {
            ParticleComponent& p1, p2;
            geometry::IntersectInfo contact;
            float cor;
        };

        float calculate_separating_velocity(const ParticleContact& contact) {
            auto relative_vel = contact.p1.velocity - contact.p2.velocity;
            return glm::dot(relative_vel, contact.contact.normal);
        }

        void resolve_contact_velocity(ParticleContact& contact, float dt) {
            // If both objects are infinite mass then skip everything
            const auto total_inv_mass = contact.p1.inverse_mass + contact.p2.inverse_mass;
            if (total_inv_mass <= 0.0f) return;

            const auto separating_velocity = calculate_separating_velocity(contact);
            // objects are moving away from each other or stationary
            if (separating_velocity >= 0.0f) return;

            auto restitution_velocity = separating_velocity * contact.cor;

            // If the res. velocity frame is from forces adding accel. then this is a resting contact
            // That is the objects are already stuck together and are just being pushed into each other
            // continuously.
            const auto vel_from_acc = dt * glm::dot(
                contact.p1.acceleration - contact.p2.acceleration,
                contact.contact.normal
            );
            if (vel_from_acc < 0.0f) {
                // Add back just enough velocity to keep the objects from penetrating into each other
                // but do allow them to slide along each other.
                restitution_velocity += contact.cor * vel_from_acc;
                restitution_velocity = std::max(restitution_velocity, 0.0f);
            }

            const auto delta_velocity = restitution_velocity - separating_velocity;

            const auto impulse = delta_velocity / total_inv_mass;
            const auto impulse_mass = contact.contact.normal * impulse;

            contact.p1.velocity += impulse_mass * contact.p1.inverse_mass;
            contact.p2.velocity -= impulse_mass * contact.p2.inverse_mass;
        }

        void resolve_interpenetration(ParticleContact& contact, float dt) {
            if (contact.contact.depth <= 0.0f) return; // objects are not penetrating

            const auto total_inv_mass = contact.p1.inverse_mass + contact.p2.inverse_mass;
            if (total_inv_mass <= 0.0f) return;

            const auto pos_mass = contact.contact.normal * (contact.contact.depth / total_inv_mass);

            contact.p1.position += pos_mass * contact.p1.inverse_mass;
            contact.p2.position -= pos_mass * contact.p2.inverse_mass;
        }

        void resolve_contacts(const std::vector<ParticleContact*> contacts, float dt) {
            constexpr auto MAX_ITERATIONS = 10000000;

            size_t max_index = contacts.size();
            for (auto i = 0; i < MAX_ITERATIONS; i++) {
                float max_closing_velocity = std::numeric_limits<float>::max();
                for (const auto& contact : contacts) {
                    const auto separating_vel = calculate_separating_velocity(*contact);
                    if (
                        (separating_vel < max_closing_velocity)
                        && ((separating_vel < 0.0f) || (contact->contact.depth > 0.0f))
                    ) {
                        max_closing_velocity = separating_vel;
                        max_index = i;
                    }
                }
            }

            if (max_index == contacts.size()) return; // nothing to fix, all contacts are resolved

            auto& c = *contacts[max_index];
            resolve_contact_velocity(c, dt);
            resolve_interpenetration(c, dt);
        }
    }

    void ParticleCollisionSystem::run(ecs::World& world, float dt) {
        util::PoolAllocator<ParticleContact> contact_allocator;
        std::vector<ParticleContact*> contacts;

        auto& particles = world.write_component<ParticleComponent>();

        // Build list of collisions ...

        resolve_contacts(contacts, dt);
    }
}
