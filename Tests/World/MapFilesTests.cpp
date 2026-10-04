#include "World/BrushGeometry.h"
#include "World/MapData.h"
#include "World/MapParser.h"

#include <gtest/gtest.h>

#include <cstddef>
#include <filesystem>

namespace Abomination::World
{
    // Every map of the game reads without errors, and every brush of it is a closed solid: each of its faces is a real
    // polygon (none is cut away, none has three points on one line). A map saved from TrenchBroom or
    // edited by hand is checked here before the game loads it.
    TEST(MapFiles, EveryMapReadsAndEveryBrushIsClosed)
    {
        const std::filesystem::path mapsDirectory = std::filesystem::path(ABOMINATION_TEST_ASSETS_DIRECTORY) / "Maps";
        int mapCount = 0;
        for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(mapsDirectory))
        {
            if (entry.path().extension() != ".map")
                continue;
            ++mapCount;

            const auto map = LoadMapFile(entry.path());
            ASSERT_TRUE(map.has_value()) << entry.path().filename().string() << ": " << map.error();
            for (const MapEntity& entity : map->entities)
                for (std::size_t brushIndex = 0; brushIndex < entity.brushes.size(); ++brushIndex)
                    for (const auto& polygon : BuildBrushPolygons(entity.brushes[brushIndex]))
                        EXPECT_GE(polygon.size(), 3u) << entry.path().filename().string() << ", brush " << brushIndex;
        }
        EXPECT_GE(mapCount, 2);
    }
}
