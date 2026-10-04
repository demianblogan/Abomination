#include "Navigation/NavMesh.h"
#include "World/MapParser.h"
#include "World/NavMeshGeometry.h"

#include <glm/geometric.hpp>
#include <gtest/gtest.h>

#include <expected>
#include <filesystem>
#include <format>
#include <string>

namespace Abomination::Navigation
{
    namespace
    {
        // A box brush of the map format (Valve 220) from its corners in map units (Z up), the planes as TrenchBroom
        // writes them.
        std::string Box(int x0, int y0, int z0, int x1, int y1, int z1)
        {
            const auto face = [](int ax, int ay, int az, int bx, int by, int bz, int cx, int cy, int cz)
            {
                return std::format("( {} {} {} ) ( {} {} {} ) ( {} {} {} ) Tex [ 1 0 0 0 ] [ 0 -1 0 0 ] 0 1 1\n", ax, ay, az,
                                   bx, by, bz, cx, cy, cz);
            };
            return "{\n" + face(x0, y0, z0, x0, y0 + 1, z0, x0, y0, z0 + 1) + face(x0, y0, z0, x0, y0, z0 + 1, x0 + 1, y0, z0) +
                   face(x0, y0, z0, x0 + 1, y0, z0, x0, y0 + 1, z0) + face(x1, y1, z1, x1, y1 + 1, z1, x1 + 1, y1, z1) +
                   face(x1, y1, z1, x1 + 1, y1, z1, x1, y1, z1 + 1) + face(x1, y1, z1, x1, y1, z1 + 1, x1, y1 + 1, z1) + "}\n";
        }

        // A floor 512 x 512 units (16 m) with its top at height 0, and a pillar 128 units (4 m) across in its middle.
        NavMesh BuildRoomWithPillar()
        {
            const std::string map = "{\n\"classname\" \"worldspawn\"\n" + Box(-256, -256, -16, 256, 256, 0) +
                                    Box(-64, -64, 0, 64, 64, 128) + "}\n";
            const auto parsed = World::ParseMap(map);
            EXPECT_TRUE(parsed.has_value());
            std::expected<NavMesh, std::string> navMesh =
                NavMesh::Build(World::BuildNavMeshGeometry(parsed->entities.at(0)), NavMeshSettings{});
            EXPECT_TRUE(navMesh.has_value()) << (navMesh.has_value() ? "" : navMesh.error());
            return std::move(*navMesh);
        }

        // Game meters: 32 map units are a meter, the map's Y is the game's -Z. The floor is at y = 0.
        constexpr glm::vec3 West{-6.0f, 0.4f, 0.0f};
        constexpr glm::vec3 East{6.0f, 0.4f, 0.0f};
        constexpr glm::vec3 NorthWest{-6.0f, 0.4f, -6.0f};
        constexpr glm::vec3 NorthEast{6.0f, 0.4f, -6.0f};
    }

    TEST(NavMesh, FloorAroundPillarIsWalkable)
    {
        const NavMesh navMesh = BuildRoomWithPillar();

        EXPECT_FALSE(navMesh.GetPolygons().empty());
        EXPECT_TRUE(navMesh.IsStraightWayClear(NorthWest, NorthEast));
    }

    TEST(NavMesh, PathGoesAroundPillar)
    {
        const NavMesh navMesh = BuildRoomWithPillar();

        // Straight across, the pillar (2 m to each side of the middle) is in the way: the path turns at its corners.
        EXPECT_FALSE(navMesh.IsStraightWayClear(West, East));
        const std::vector<glm::vec3> path = navMesh.FindPath(West, East);
        ASSERT_GE(path.size(), 3u);
        EXPECT_NEAR(path.front().x, West.x, 0.2f);
        EXPECT_NEAR(path.back().x, East.x, 0.2f);

        // At every corner the box of the dog (0.53 m to each side, never turned) stays out of the pillar (2 m to each
        // side of the middle): it is clear of it along at least one axis.
        float length = 0.0f;
        for (std::size_t index = 1; index < path.size(); ++index)
        {
            length += glm::distance(path[index - 1], path[index]);
            const glm::vec3& corner = path[index];
            const bool isInPillar = std::max(std::abs(corner.x), std::abs(corner.z)) < 2.0f + 0.53f;
            EXPECT_FALSE(isInPillar) << corner.x << " " << corner.z;
        }
        EXPECT_GT(length, 12.0f);
    }

    TEST(NavMesh, RandomPointIsNearCenterAndOnFloor)
    {
        const NavMesh navMesh = BuildRoomWithPillar();
        float seed = 0.0f;
        const auto random = [&seed] { seed = seed + 0.37f > 1.0f ? seed + 0.37f - 1.0f : seed + 0.37f; return seed; };

        for (int attempt = 0; attempt < 10; ++attempt)
        {
            const std::optional<glm::vec3> point = navMesh.FindRandomPointAround(NorthWest, 3.0f, random);
            ASSERT_TRUE(point.has_value());
            EXPECT_NEAR(point->y, 0.0f, 0.3f);
            EXPECT_FALSE(std::abs(point->x) < 2.0f && std::abs(point->z) < 2.0f);
        }
    }

    TEST(NavMesh, TestMapHasPathFromPlayerToDog)
    {
        const auto map = World::LoadMapFile(std::filesystem::path(ABOMINATION_TEST_ASSETS_DIRECTORY) / "Maps" / "Test.map");
        ASSERT_TRUE(map.has_value());
        const auto navMesh = NavMesh::Build(World::BuildNavMeshGeometry(map->entities.at(0)), NavMeshSettings{});
        ASSERT_TRUE(navMesh.has_value()) << navMesh.error();

        // From the floor below the player start (the player appears in the air and falls) to the ledge of the dog, up
        // the clip ramp of the stairs: 2 m higher.
        const glm::vec3 player(10.0f, 0.9f, 31.5f);
        const glm::vec3 dog(-6.0f, 2.0f, 8.3f);
        const std::vector<glm::vec3> path = navMesh->FindPath(player, dog);
        ASSERT_GE(path.size(), 2u);
        EXPECT_NEAR(path.back().x, dog.x, 0.2f);
        EXPECT_NEAR(path.back().y, 2.05f, 0.2f);
    }
}
