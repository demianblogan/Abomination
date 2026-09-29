#include "World/BrushGeometry.h"
#include "World/MapParser.h"

#include <glm/geometric.hpp>
#include <glm/vec3.hpp>
#include <gtest/gtest.h>

#include <cmath>
#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace Abomination::World
{
    namespace
    {
        constexpr double Tolerance = 1e-6;

        // Parses a map text with one brush in the world and returns that brush.
        MapBrush ParseSingleBrush(std::string_view mapText)
        {
            const std::expected<MapData, std::string> map = ParseMap(mapText);
            EXPECT_TRUE(map.has_value()) << (map.has_value() ? "" : map.error());

            return map.value().entities.at(0).brushes.at(0);
        }

        // The direction a polygon faces: the normal of its first three vertices (counter-clockwise seen from the front).
        glm::dvec3 CalculateFacingDirection(const Core::ConvexPolygon& polygon)
        {
            return glm::normalize(glm::cross(polygon[1] - polygon[0], polygon[2] - polygon[0]));
        }

        glm::dvec3 CalculateCenter(const Core::ConvexPolygon& polygon)
        {
            glm::dvec3 sum(0.0);
            for (const glm::dvec3& vertex : polygon)
                sum += vertex;

            return sum / static_cast<double>(polygon.size());
        }

        bool IsOneOf(double value, double first, double second)
        {
            return std::abs(value - first) < Tolerance || std::abs(value - second) < Tolerance;
        }

        // Brush 0 of the test map: a wall 16 x 512 x 256 units, x from 48 to 64, y from -448 to 64, z from -16 to 240.
        constexpr std::string_view WallMap = R"({
"classname" "worldspawn"
{
( 48 64 240 ) ( 48 -448 240 ) ( 48 64 -16 ) Crate [ 0 1 0 0 ] [ 0 0 -1 0 ] 270 1 1
( 64 -448 -16 ) ( 48 -448 -16 ) ( 64 -448 240 ) Crate [ 1 0 0 0 ] [ 0 0 -1 0 ] 180 1 1
( 64 64 -16 ) ( 48 64 -16 ) ( 64 -448 -16 ) Crate [ -1 0 0 0 ] [ 0 -1 0 0 ] 0 1 1
( 64 -448 240 ) ( 48 -448 240 ) ( 64 64 240 ) Crate [ 1 0 0 0 ] [ 0 -1 0 0 ] 0 1 1
( 64 64 240 ) ( 48 64 240 ) ( 64 64 -16 ) Crate [ -1 0 0 0 ] [ 0 0 -1 0 ] 0 1 1
( 64 -448 240 ) ( 64 64 240 ) ( 64 -448 -16 ) Crate [ 0 1 0 0 ] [ 0 0 -1 0 ] 0 1 1
}
}
)";

        // A wedge: the triangle (0, 0, 0) - (64, 0, 0) - (0, 0, 64) in the XZ plane, stretched along Y from 0 to 64.
        // Faces: bottom (z = 0), back (x = 0), slope (x + z = 64), front (y = 0) and rear (y = 64).
        constexpr std::string_view WedgeMap = R"({
"classname" "worldspawn"
{
( 0 64 0 ) ( 0 0 0 ) ( 64 0 0 ) Crate [ 1 0 0 0 ] [ 0 -1 0 0 ] 0 1 1
( 0 0 64 ) ( 0 0 0 ) ( 0 64 0 ) Crate [ 0 1 0 0 ] [ 0 0 -1 0 ] 0 1 1
( 64 64 0 ) ( 64 0 0 ) ( 0 0 64 ) Crate [ 0 1 0 0 ] [ 0 0 -1 0 ] 0 1 1
( 64 0 0 ) ( 0 0 0 ) ( 0 0 64 ) Crate [ 1 0 0 0 ] [ 0 0 -1 0 ] 0 1 1
( 0 64 64 ) ( 0 64 0 ) ( 64 64 0 ) Crate [ 1 0 0 0 ] [ 0 0 -1 0 ] 0 1 1
}
}
)";
    }

    TEST(BrushGeometry, BoxGivesSixRectanglesAtItsCorners)
    {
        const MapBrush brush = ParseSingleBrush(WallMap);

        const std::vector<Core::ConvexPolygon> polygons = BuildBrushPolygons(brush);

        ASSERT_EQ(polygons.size(), 6u);
        for (const Core::ConvexPolygon& polygon : polygons)
        {
            ASSERT_EQ(polygon.size(), 4u);

            // Every vertex is a corner of the box.
            for (const glm::dvec3& vertex : polygon)
            {
                EXPECT_TRUE(IsOneOf(vertex.x, 48.0, 64.0)) << vertex.x;
                EXPECT_TRUE(IsOneOf(vertex.y, -448.0, 64.0)) << vertex.y;
                EXPECT_TRUE(IsOneOf(vertex.z, -16.0, 240.0)) << vertex.z;
            }
        }
    }

    TEST(BrushGeometry, FacesLookOutOfBrush)
    {
        // The vertices of every face go counter-clockwise seen from outside, so the facing direction of each polygon
        // points away from the center of the brush (and matches the plane of its face).
        for (const std::string_view mapText : {WallMap, WedgeMap})
        {
            const MapBrush brush = ParseSingleBrush(mapText);
            const std::vector<Core::ConvexPolygon> polygons = BuildBrushPolygons(brush);

            glm::dvec3 brushCenter(0.0);
            for (const Core::ConvexPolygon& polygon : polygons)
                brushCenter += CalculateCenter(polygon);
            brushCenter /= static_cast<double>(polygons.size());

            for (const Core::ConvexPolygon& polygon : polygons)
            {
                ASSERT_GE(polygon.size(), 3u);
                const glm::dvec3 outwards = CalculateCenter(polygon) - brushCenter;
                EXPECT_GT(glm::dot(CalculateFacingDirection(polygon), outwards), 0.0);
            }
        }
    }

    TEST(BrushGeometry, WedgeHasThreeRectanglesAndTwoTriangles)
    {
        const MapBrush brush = ParseSingleBrush(WedgeMap);

        const std::vector<Core::ConvexPolygon> polygons = BuildBrushPolygons(brush);

        // In the order of the faces: bottom, back and slope are rectangles, front and rear are triangles.
        ASSERT_EQ(polygons.size(), 5u);
        EXPECT_EQ(polygons[0].size(), 4u);
        EXPECT_EQ(polygons[1].size(), 4u);
        EXPECT_EQ(polygons[2].size(), 4u);
        EXPECT_EQ(polygons[3].size(), 3u);
        EXPECT_EQ(polygons[4].size(), 3u);

        // Every vertex of the slope lies on the plane x + z = 64.
        for (const glm::dvec3& vertex : polygons[2])
            EXPECT_NEAR(vertex.x + vertex.z, 64.0, Tolerance);
    }

    TEST(BrushGeometry, PlaneThatMissesBrushGivesEmptyFace)
    {
        // The wall with a seventh plane x = 1000 facing +X: the whole brush is behind it, so it changes nothing,
        // and its own face lies outside the brush and disappears.
        std::string mapText(WallMap);
        const std::string extraFace = "( 1000 0 0 ) ( 1000 0 64 ) ( 1000 64 0 ) Crate [ 0 1 0 0 ] [ 0 0 -1 0 ] 0 1 1\n";
        mapText.insert(mapText.find("}\n}"), extraFace);

        const std::vector<Core::ConvexPolygon> polygons = BuildBrushPolygons(ParseSingleBrush(mapText));

        ASSERT_EQ(polygons.size(), 7u);
        for (std::size_t index = 0; index < 6; ++index)
            EXPECT_EQ(polygons[index].size(), 4u);
        EXPECT_TRUE(polygons[6].empty());
    }

    TEST(BrushGeometry, FaceWithoutPlaneGivesEmptyPolygon)
    {
        // A face whose three points lie on one line does not define a plane; the other faces are built as usual.
        MapBrush brush = ParseSingleBrush(WallMap);
        brush.faces.push_back(MapFace{.points = {glm::dvec3(0.0), glm::dvec3(1.0), glm::dvec3(2.0)}});

        const std::vector<Core::ConvexPolygon> polygons = BuildBrushPolygons(brush);

        ASSERT_EQ(polygons.size(), 7u);
        EXPECT_EQ(polygons[0].size(), 4u);
        EXPECT_TRUE(polygons[6].empty());
    }
}
