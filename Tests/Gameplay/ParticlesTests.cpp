#include "Gameplay/Particles.h"

#include <gtest/gtest.h>

namespace Abomination::Gameplay
{
    TEST(Particles, ParticleMovesFallsAndDies)
    {
        ParticleSystem system;
        system.Emit(Particle{.velocity = {1.0f, 0.0f, 0.0f}, .lifetime = 0.5f, .gravityScale = 1.0f});

        system.Update(0.25f, 10.0f);
        ASSERT_EQ(system.GetCount(), 1u);

        system.Update(0.3f, 10.0f);
        EXPECT_EQ(system.GetCount(), 0u);
    }

    TEST(Particles, SpritesFadeOverLife)
    {
        ParticleSystem system;
        system.Emit(Particle{.lifetime = 1.0f, .startColor = {1.0f, 1.0f, 1.0f, 1.0f}, .endColor = {1.0f, 1.0f, 1.0f, 0.0f}});
        system.Update(0.75f, 0.0f);

        Renderer::SpriteBatch batch;
        system.AddSprites(batch);

        ASSERT_EQ(batch.GetSprites().size(), 1u);
        EXPECT_NEAR(batch.GetSprites()[0].color.a, 0.25f, 1e-5f);
    }

    TEST(Particles, FullSystemReplacesOldest)
    {
        ParticleSystem system;
        for (std::size_t index = 0; index < ParticleSystem::MaximumParticleCount + 10; ++index)
            system.Emit(Particle{.lifetime = 10.0f});

        EXPECT_EQ(system.GetCount(), ParticleSystem::MaximumParticleCount);
    }
}
