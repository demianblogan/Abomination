#include "Gameplay/Player/DamageReaction.h"

#include "Audio/AudioEngine.h"
#include "Core/Scene/Transform.h"
#include "Gameplay/Characters/Armor.h"
#include "Gameplay/Characters/Health.h"

#include <gtest/gtest.h>

#include <filesystem>

namespace Abomination::Gameplay
{
    namespace
    {
        // A player with the components the reaction needs. The sounds are not loaded: their handles play nothing.
        struct TestPlayer
        {
            entt::registry registry;
            GameplayState state;
            Audio::AudioEngine audio{std::filesystem::temp_directory_path() / "AbominationDamageReactionTest"};

            TestPlayer()
            {
                state.player = registry.create();
                registry.emplace<Core::Transform>(state.player);
                registry.emplace<Health>(state.player);
                registry.emplace<Armor>(state.player);
                registry.emplace<DamageReaction>(state.player);
            }

            Health& GetHealth() { return registry.get<Health>(state.player); }
            DamageReaction& GetReaction() { return registry.get<DamageReaction>(state.player); }
        };
    }

    TEST(DamageReaction, BlowHurtsThroughArmorAndIsCounted)
    {
        TestPlayer player;
        player.registry.get<Armor>(player.state.player).current = 50.0f;

        const bool isKilled = DamagePlayer(player.state, player.registry, player.audio, {.amount = 30.0f});

        EXPECT_FALSE(isKilled);
        EXPECT_NEAR(player.GetHealth().current, 90.0f, 1e-4f);
        EXPECT_EQ(player.GetReaction().damageCount, 1);
        EXPECT_FLOAT_EQ(player.GetReaction().lastDamage, 30.0f);
    }

    TEST(DamageReaction, DirectionOfTheBlowIsAlongTheGround)
    {
        TestPlayer player;
        DamagePlayer(player.state, player.registry, player.audio, {.amount = 10.0f, .sourcePosition = glm::vec3(3, 5, 0)});

        const auto& direction = player.GetReaction().lastDamageDirection;
        ASSERT_TRUE(direction.has_value());
        EXPECT_NEAR(direction->x, 1.0f, 1e-5f);
        EXPECT_NEAR(direction->y, 0.0f, 1e-5f);

        // A blow without a source has no direction.
        DamagePlayer(player.state, player.registry, player.audio, {.amount = 10.0f});
        EXPECT_FALSE(player.GetReaction().lastDamageDirection.has_value());
    }

    TEST(DamageReaction, DeadPlayerIsNotHurtOrHealed)
    {
        TestPlayer player;
        EXPECT_TRUE(DamagePlayer(player.state, player.registry, player.audio, {.amount = 200.0f}));
        EXPECT_FALSE(DamagePlayer(player.state, player.registry, player.audio, {.amount = 10.0f}));
        EXPECT_EQ(player.GetReaction().damageCount, 1);

        HealPlayer(player.state, player.registry, player.audio, 50.0f);
        EXPECT_FLOAT_EQ(player.GetHealth().current, 0.0f);
        EXPECT_EQ(player.GetReaction().healCount, 0);
    }

    TEST(DamageReaction, HealingStopsAtTheMaximumAndIsCounted)
    {
        TestPlayer player;
        player.GetHealth().current = 70.0f;

        HealPlayer(player.state, player.registry, player.audio, 50.0f);
        EXPECT_FLOAT_EQ(player.GetHealth().current, 100.0f);
        EXPECT_EQ(player.GetReaction().healCount, 1);

        // At full health nothing happens, not even the sigh.
        HealPlayer(player.state, player.registry, player.audio, 50.0f);
        EXPECT_EQ(player.GetReaction().healCount, 1);
    }

    TEST(DamageReaction, PunchSpringsBack)
    {
        TestPlayer player;
        DamagePlayer(player.state, player.registry, player.audio, {.amount = 20.0f});
        for (int frame = 0; frame < 5; ++frame)
            UpdateDamageReaction(player.state, player.registry, player.audio, 1.0f / 60.0f);

        const DamageReaction& reaction = player.GetReaction();
        EXPECT_GT(std::abs(reaction.punchPitch) + std::abs(reaction.punchYaw) + std::abs(reaction.punchRoll), 0.0f);

        for (int frame = 0; frame < 180; ++frame)
            UpdateDamageReaction(player.state, player.registry, player.audio, 1.0f / 60.0f);
        EXPECT_NEAR(reaction.punchPitch, 0.0f, 1e-3f);
        EXPECT_NEAR(reaction.punchYaw, 0.0f, 1e-3f);
        EXPECT_NEAR(reaction.punchRoll, 0.0f, 1e-3f);
        EXPECT_FLOAT_EQ(reaction.muffle, 0.0f);
    }

    TEST(DamageReaction, HeartBeatsOnlyAtLowHealth)
    {
        TestPlayer player;
        // 0.9 s: not exactly 1 s, where the next heartbeat would fall on the very last frame.
        const auto runAlmostOneSecond = [&player]
        {
            for (int frame = 0; frame < 54; ++frame)
                UpdateDamageReaction(player.state, player.registry, player.audio, 1.0f / 60.0f);
        };

        runAlmostOneSecond();
        EXPECT_EQ(player.GetReaction().heartbeatCount, 0);

        // At 20 health the heartbeat sound starts at once: its two beats (at 0.3 and 0.6 s) are counted in 0.9 s, the next
        // sound comes only at 1 s.
        player.GetHealth().current = 20.0f;
        runAlmostOneSecond();
        EXPECT_EQ(player.GetReaction().heartbeatCount, 2);

        // Dead: silence.
        player.GetHealth().current = 0.0f;
        runAlmostOneSecond();
        EXPECT_EQ(player.GetReaction().heartbeatCount, 2);
    }
}
