#include "Core/Math/Units.h"

#include <gtest/gtest.h>

namespace Abomination::Core
{
    TEST(Units, ConvertsMapUnitsToMeters)
    {
        EXPECT_DOUBLE_EQ(MapUnitsToMeters(32.0), 1.0);
        EXPECT_FLOAT_EQ(MapUnitsToMeters(270.0f), 8.4375f); // the jump speed of Quake in m/s
    }

    TEST(Units, ConvertsMetersToMapUnits)
    {
        EXPECT_FLOAT_EQ(MetersToMapUnits(10.0f), 320.0f); // running in Quake: 10 m/s is 320 units/s
        EXPECT_DOUBLE_EQ(MetersToMapUnits(MapUnitsToMeters(18.0)), 18.0);
    }

    // The conversions are constexpr, so constants of the game can be written with them.
    static_assert(MapUnitsToMeters(64.0) == 2.0);
}
