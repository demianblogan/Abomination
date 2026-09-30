#include "Gameplay/Weapon.h"

#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>
#include <gtest/gtest.h>

#include <cmath>
#include <vector>

namespace Abomination::Gameplay
{
    namespace
    {
        constexpr glm::vec3 Forward{0.0f, 0.0f, -1.0f};
        constexpr glm::vec3 Up{0.0f, 1.0f, 0.0f};
    }

    TEST(Weapon, PelletsStayInsideTheCone)
    {
        Core::Random random(3);
        const float spread = glm::radians(4.0f);

        const std::vector<glm::vec3> directions = GeneratePelletDirections(Forward, Up, spread, 1000, random);

        ASSERT_EQ(directions.size(), 1000u);
        for (const glm::vec3& direction : directions)
        {
            EXPECT_NEAR(glm::length(direction), 1.0f, 1e-5f);

            // The angle from the middle of the screen is at most the spread (with a little rounding).
            EXPECT_LE(std::acos(glm::clamp(glm::dot(direction, Forward), -1.0f, 1.0f)), spread + 1e-4f);
        }
    }

    TEST(Weapon, PelletsFillTheWholeCone)
    {
        // Pellets land in every quarter of the cone and not only near the middle.
        Core::Random random(5);
        const float spread = glm::radians(4.0f);

        const std::vector<glm::vec3> directions = GeneratePelletDirections(Forward, Up, spread, 1000, random);

        int left = 0;
        int up = 0;
        int farFromMiddle = 0;
        for (const glm::vec3& direction : directions)
        {
            left += direction.x < 0.0f ? 1 : 0;
            up += direction.y > 0.0f ? 1 : 0;
            farFromMiddle += std::acos(glm::dot(direction, Forward)) > spread * 0.5f ? 1 : 0;
        }

        EXPECT_NEAR(left, 500, 80);
        EXPECT_NEAR(up, 500, 80);

        // Outside half the radius lies three quarters of the area of the disc.
        EXPECT_NEAR(farFromMiddle, 750, 80);
    }

    TEST(Weapon, NoSpreadShootsStraight)
    {
        Core::Random random(1);

        const std::vector<glm::vec3> directions = GeneratePelletDirections(Forward, Up, 0.0f, 6, random);

        for (const glm::vec3& direction : directions)
            EXPECT_NEAR(glm::dot(direction, Forward), 1.0f, 1e-6f);
    }
}
