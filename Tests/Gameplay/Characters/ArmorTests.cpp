#include "Gameplay/Characters/Armor.h"

#include <gtest/gtest.h>

namespace Abomination::Gameplay
{
    TEST(Armor, TakesTwoThirdsOfTheDamage)
    {
        Health health;
        Armor armor{.current = 50.0f};

        EXPECT_FALSE(ApplyDamage(health, armor, 30.0f));

        EXPECT_NEAR(armor.current, 30.0f, 1e-4f);
        EXPECT_NEAR(health.current, 90.0f, 1e-4f);
    }

    TEST(Armor, TakesOnlyWhatIsLeft)
    {
        Health health;
        Armor armor{.current = 5.0f};

        EXPECT_FALSE(ApplyDamage(health, armor, 30.0f));

        EXPECT_FLOAT_EQ(armor.current, 0.0f);
        EXPECT_NEAR(health.current, 75.0f, 1e-4f);
    }

    TEST(Armor, WithoutArmorTheHealthTakesEverything)
    {
        Health health;
        Armor armor;

        EXPECT_FALSE(ApplyDamage(health, armor, 40.0f));
        EXPECT_NEAR(health.current, 60.0f, 1e-4f);

        EXPECT_TRUE(ApplyDamage(health, armor, 100.0f));
        EXPECT_FLOAT_EQ(health.current, 0.0f);
    }
}
