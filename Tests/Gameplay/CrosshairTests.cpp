#include "Gameplay/Crosshair.h"

#include <glm/trigonometric.hpp>
#include <gtest/gtest.h>

namespace Abomination::Gameplay
{
    TEST(Crosshair, SpreadAsWideAsHalfTheViewReachesTheTopEdge)
    {
        // A cone as wide as half the field of view touches the top edge of the screen: half the screen height.
        EXPECT_NEAR(CalculateSpreadRadiusOnScreen(glm::radians(30.0f), glm::radians(60.0f), 1080.0f), 540.0f, 0.01f);
    }

    TEST(Crosshair, ShotgunSpreadGivesSmallCircle)
    {
        // 4 degrees at 60 degrees on 1080 pixels: tan(4) / tan(30) * 540, about 65 pixels.
        EXPECT_NEAR(CalculateSpreadRadiusOnScreen(glm::radians(4.0f), glm::radians(60.0f), 1080.0f), 65.4f, 0.1f);
    }

    TEST(Crosshair, WiderFieldOfViewShrinksTheCircle)
    {
        const float narrow = CalculateSpreadRadiusOnScreen(glm::radians(4.0f), glm::radians(60.0f), 1080.0f);
        const float wide = CalculateSpreadRadiusOnScreen(glm::radians(4.0f), glm::radians(90.0f), 1080.0f);

        EXPECT_LT(wide, narrow);
    }
}
