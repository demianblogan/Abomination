#include "Core/Units.h"
#include "World/PlayerStart.h"

#include <gtest/gtest.h>

namespace Abomination::World
{
    TEST(PlayerStart, BoxMatchesTheEntityOfTheMapEditor)
    {
        // Abomination.fgd: info_player_start has size(-16 -16 -24, 16 16 32).
        EXPECT_DOUBLE_EQ(Core::MetersToMapUnits(PlayerHalfExtents.x), 16.0);
        EXPECT_DOUBLE_EQ(Core::MetersToMapUnits(PlayerHalfExtents.y), 28.0);
        EXPECT_DOUBLE_EQ(Core::MetersToMapUnits(PlayerHalfExtents.z), 16.0);

        // From the center the box reaches down to 24 units below the origin and up to 32 above it, so a player start
        // placed on the floor in the editor stands exactly on the floor in the game.
        const double halfHeightInUnits = Core::MetersToMapUnits(PlayerHalfExtents.y);
        EXPECT_DOUBLE_EQ(PlayerBoxCenterAboveOrigin - halfHeightInUnits, -24.0);
        EXPECT_DOUBLE_EQ(PlayerBoxCenterAboveOrigin + halfHeightInUnits, 32.0);
    }
}
