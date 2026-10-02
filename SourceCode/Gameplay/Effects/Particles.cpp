#include "Gameplay/Effects/Particles.h"

#include <glm/common.hpp>

#include <cmath>
#include <utility>

namespace Abomination::Gameplay
{
    void ParticleSystem::Emit(const Particle& particle)
    {
        if (m_particles.size() < MaximumParticleCount)
        {
            m_particles.push_back(particle);
            return;
        }

        m_particles[m_nextReplaced] = particle;
        m_nextReplaced = (m_nextReplaced + 1) % MaximumParticleCount;
    }

    void ParticleSystem::Update(float deltaTime, float gravity)
    {
        // The drag keeps exp(-drag * deltaTime) of the speed each frame: the same slowdown at any frame rate (see
        // Core::CalculateApproachFactor for the same idea).
        for (Particle& particle : m_particles)
        {
            particle.age += deltaTime;
            particle.velocity.y -= gravity * particle.gravityScale * deltaTime;
            particle.velocity *= std::exp(-particle.drag * deltaTime);
            particle.position += particle.velocity * deltaTime;
            particle.rotation += particle.rotationSpeed * deltaTime;
        }

        // Removes the dead particles without keeping the order: the last particle takes the place of a dead one (the order
        // does not matter, the renderer sorts them anyway). Much cheaper than erasing from the middle.
        for (std::size_t index = 0; index < m_particles.size();)
        {
            if (m_particles[index].age < m_particles[index].lifetime)
            {
                ++index;
                continue;
            }

            m_particles[index] = std::move(m_particles.back());
            m_particles.pop_back();
        }

        if (m_nextReplaced >= m_particles.size())
            m_nextReplaced = 0;
    }

    void ParticleSystem::AddSprites(Renderer::SpriteBatch& batch) const
    {
        for (const Particle& particle : m_particles)
        {
            const float life = glm::clamp(particle.age / particle.lifetime, 0.0f, 1.0f);
            const float halfSize = glm::mix(particle.startHalfSize, particle.endHalfSize, life);
            const glm::vec4 color = glm::mix(particle.startColor, particle.endColor, life);
            batch.AddBillboard(particle.position, halfSize, particle.rotation, color, particle.texture, particle.blend);
        }
    }

    void ParticleSystem::Clear() noexcept
    {
        m_particles.clear();
        m_nextReplaced = 0;
    }

    std::size_t ParticleSystem::GetCount() const noexcept
    {
        return m_particles.size();
    }
}
