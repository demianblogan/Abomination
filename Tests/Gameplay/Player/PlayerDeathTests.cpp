#include "Gameplay/Player/PlayerDeath.h"

#include <gtest/gtest.h>

namespace Abomination::Gameplay
{
    namespace
    {
        PlayerDeath DeadFor(float seconds)
        {
            return PlayerDeath{.isDead = true, .time = seconds};
        }
    }

    TEST(PlayerDeath, TheLivingSeeNothingOfIt)
    {
        const PlayerDeath alive;
        const DeathView view = CalculateDeathView(alive, 1.4f, 0.3f);

        EXPECT_FLOAT_EQ(view.drop, 0.0f);
        EXPECT_FLOAT_EQ(view.pitch, 0.3f);
        EXPECT_FLOAT_EQ(CalculateWeaponLowering(alive), 0.0f);
        EXPECT_FLOAT_EQ(CalculateHUDOpacity(alive), 1.0f);
        EXPECT_FLOAT_EQ(CalculateEyesClosed(alive), 0.0f);
        EXPECT_FALSE(CanRestartAfterDeath(alive));
    }

    TEST(PlayerDeath, HeadDropsThenFallsOnTheBackLookingUp)
    {
        const PlayerDeathSettings settings;

        // At the end of the nod the head looks nodPitch lower than it did, and the body has not fallen yet.
        const DeathView nod = CalculateDeathView(DeadFor(settings.nodTime), 1.4f, 0.0f);
        EXPECT_FLOAT_EQ(nod.pitch, -settings.nodPitch);
        EXPECT_FLOAT_EQ(nod.drop, 0.0f);

        // Fallen: on the back, the eyes eyeHeight above the floor and fallBack behind, looking up.
        const DeathView fallen = CalculateDeathView(DeadFor(settings.fallEndTime), 1.4f, 0.0f);
        EXPECT_FLOAT_EQ(fallen.pitch, settings.endPitch);
        EXPECT_FLOAT_EQ(fallen.drop, 1.4f - settings.eyeHeight);
        EXPECT_FLOAT_EQ(fallen.back, settings.fallBack);
    }

    TEST(PlayerDeath, WeaponAndHUDGoFirst)
    {
        const PlayerDeathSettings settings;

        EXPECT_FLOAT_EQ(CalculateWeaponLowering(DeadFor(settings.weaponLowerTime)), 1.0f);
        EXPECT_FLOAT_EQ(CalculateHUDOpacity(DeadFor(settings.hudFadeTime * 0.5f)), 0.5f);
        EXPECT_FLOAT_EQ(CalculateHUDOpacity(DeadFor(settings.hudFadeTime)), 0.0f);
    }

    TEST(PlayerDeath, EyesCloseThenGameOverThenTheHint)
    {
        const PlayerDeathSettings settings;
        const float closedTime = settings.eyesCloseStart + settings.eyesCloseTime;

        EXPECT_FLOAT_EQ(CalculateEyesClosed(DeadFor(settings.eyesCloseStart)), 0.0f);
        EXPECT_FLOAT_EQ(CalculateEyesClosed(DeadFor(closedTime)), 1.0f);

        EXPECT_FALSE(IsGameOverShown(DeadFor(closedTime - 0.01f)));
        EXPECT_TRUE(IsGameOverShown(DeadFor(closedTime)));
        EXPECT_FALSE(CanRestartAfterDeath(DeadFor(closedTime + settings.hintDelay - 0.01f)));
        EXPECT_TRUE(CanRestartAfterDeath(DeadFor(closedTime + settings.hintDelay)));
    }
}
