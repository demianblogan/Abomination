#pragma once

#include "Renderer/Sprites/SpriteBatch.h"

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <cstddef>
#include <vector>

namespace Abomination::Gameplay
{
    // A particle: a small sprite that flies, falls, slows down, grows or shrinks and fades during its short life (a spark,
    // a puff of dust, a drop of blood). Particles are only for the eyes: they do not collide and nothing collides with them.
    struct Particle
    {
        glm::vec3 position{0.0f};
        glm::vec3 velocity{0.0f};

        // Seconds lived and seconds it lives in all.
        float age = 0.0f;
        float lifetime = 1.0f;

        // Half its size at birth and at death (meters); it changes evenly in between.
        float startHalfSize = 0.05f;
        float endHalfSize = 0.05f;

        // Its rotation in the plane of the screen and how fast it turns (radians, radians per second).
        float rotation = 0.0f;
        float rotationSpeed = 0.0f;

        // Its color at birth and at death (the alpha fades it out); it changes evenly in between.
        glm::vec4 startColor{1.0f};
        glm::vec4 endColor{1.0f, 1.0f, 1.0f, 0.0f};

        // 1 falls like everything else, 0 floats, negative rises (smoke).
        float gravityScale = 0.0f;

        // How fast the air slows it down: the part of its speed lost per second (2: most of it within half a second).
        float drag = 0.0f;

        Renderer::TextureHandle texture;
        Renderer::SpriteBlend blend = Renderer::SpriteBlend::Alpha;
    };

    // All particles of the game, moved and drawn every frame (not in ticks: they are only for the eyes, and moving them
    // every frame keeps them as smooth as the view).
    class ParticleSystem
    {
    public:
        // Particles beyond this many replace the oldest ones: a storm of shots never slows the game down.
        static constexpr std::size_t MaximumParticleCount = 2000;

        void Emit(const Particle& particle);

        // Moves every particle by deltaTime (seconds) and removes the ones whose life is over. gravity is the pull of the
        // world (meters per second squared, positive: down).
        void Update(float deltaTime, float gravity);

        // Adds every particle as a billboard.
        void AddSprites(Renderer::SpriteBatch& batch) const;

        void Clear() noexcept;

        [[nodiscard]] std::size_t GetCount() const noexcept;

    private:
        std::vector<Particle> m_particles;

        // Where the next particle goes once the list is full: it goes around, replacing the oldest.
        std::size_t m_nextReplaced = 0;
    };
}
