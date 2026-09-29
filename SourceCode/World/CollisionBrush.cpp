#include "World/CollisionBrush.h"

#include "World/BrushGeometry.h"
#include "World/MapCoordinates.h"

#include <glm/geometric.hpp>

#include <array>
#include <cstddef>
#include <optional>
#include <utility>

namespace Abomination::World
{
    namespace
    {
        // Two normals count as the same direction when their dot product (the cosine of the angle between them) is this
        // close to 1. TrenchBroom writes axis-aligned faces with exact integer points, so their normals are exact.
        constexpr double SameDirectionTolerance = 1e-6;

        // The six directions of the sides of a bounding box.
        constexpr std::array<glm::dvec3, 6> AxisDirections = {
            glm::dvec3(1.0, 0.0, 0.0), glm::dvec3(-1.0, 0.0, 0.0),
            glm::dvec3(0.0, 1.0, 0.0), glm::dvec3(0.0, -1.0, 0.0),
            glm::dvec3(0.0, 0.0, 1.0), glm::dvec3(0.0, 0.0, -1.0),
        };

        bool HasPlaneFacing(const std::vector<Core::Plane>& planes, const glm::dvec3& direction)
        {
            for (const Core::Plane& plane : planes)
                if (glm::dot(plane.normal, direction) > 1.0 - SameDirectionTolerance)
                    return true;

            return false;
        }

        // The side of the box that faces the direction, as a plane. A plane holds the points with dot(normal, p) ==
        // distance: for +X that is x == maximum.x, for -X it is -x == -minimum.x, that is x == minimum.x.
        Core::Plane CreateBoxSidePlane(const Core::BoundingBox& box, const glm::dvec3& direction)
        {
            const bool facesPositive = direction.x + direction.y + direction.z > 0.0;
            const glm::dvec3& corner = facesPositive ? box.maximum : box.minimum;

            return {.normal = direction, .distanceFromOrigin = glm::dot(direction, corner)};
        }
    }

    std::vector<CollisionBrush> BuildCollisionBrushes(const MapEntity& entity)
    {
        std::vector<CollisionBrush> brushes;
        brushes.reserve(entity.brushes.size());

        for (const MapBrush& mapBrush : entity.brushes)
        {
            CollisionBrush brush;
            std::vector<glm::dvec3> vertices;

            // The shapes of the faces tell which faces really exist, and their vertices give the bounding box.
            const std::vector<Core::ConvexPolygon> polygons = BuildBrushPolygons(mapBrush);
            for (std::size_t faceIndex = 0; faceIndex < polygons.size(); ++faceIndex)
            {
                if (polygons[faceIndex].empty())
                    continue;

                const MapFace& face = mapBrush.faces[faceIndex];
                const std::optional<Core::Plane> mapPlane = Core::CreatePlaneFromPoints(face.points[0], face.points[1],
                                                                                        face.points[2]);
                if (!mapPlane.has_value())
                    continue;

                brush.planes.push_back(ConvertMapPlane(*mapPlane));
                for (const glm::dvec3& vertex : polygons[faceIndex])
                    vertices.push_back(ConvertMapPositionPrecise(vertex));
            }

            if (brush.planes.empty())
                continue;

            brush.bounds = Core::CalculateBoundingBox(vertices);

            // Bevel planes: the sides of the box that the brush does not have as faces. A box brush has all six already
            // and gets none; a ramp gets the ones its slanted face replaces.
            for (const glm::dvec3& direction : AxisDirections)
                if (!HasPlaneFacing(brush.planes, direction))
                    brush.planes.push_back(CreateBoxSidePlane(brush.bounds, direction));

            brushes.push_back(std::move(brush));
        }

        return brushes;
    }
}
