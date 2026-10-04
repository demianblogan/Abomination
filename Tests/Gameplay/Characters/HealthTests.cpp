#include "Gameplay/Characters/Health.h"

#include <gtest/gtest.h>

namespace Abomination::Gameplay
{
    TEST(Health, KillingBlowLeavesItsRestAsOverkill)
    {
        Health health{.current = 25.0f, .maximum = 60.0f};

        EXPECT_FALSE(ApplyDamage(health, 10.0f));
        EXPECT_FLOAT_EQ(health.overkill, 0.0f);

        // 15 health left, a blow of 40: dead, with 25 over.
        EXPECT_TRUE(ApplyDamage(health, 40.0f));
        EXPECT_FLOAT_EQ(health.current, 0.0f);
        EXPECT_FLOAT_EQ(health.overkill, 25.0f);
    }

    TEST(Health, BlowsAtTheDeadAddToOverkill)
    {
        Health health{.current = 0.0f, .maximum = 60.0f};

        EXPECT_FALSE(ApplyDamage(health, 10.0f));
        EXPECT_FALSE(ApplyDamage(health, 10.0f));

        EXPECT_FLOAT_EQ(health.overkill, 20.0f);
    }
}
