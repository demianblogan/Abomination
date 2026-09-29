#include "World/BrushGeometry.h"

#include <glm/common.hpp>
#include <glm/geometric.hpp>

#include <cstddef>
#include <optional>
#include <utility>

namespace Abomination::World
{
    namespace
    {
        // Half the side of the square laid on every plane before clipping: far larger than any map (TrenchBroom limits
        // maps to about +-32768 units), so the square is guaranteed to cover the whole face. Doubles keep the
        // intersections exact to well below 1e-6 units even at this size.
        constexpr double HugeSquareHalfSize = 1'000'000.0;

        // A square around the point of the plane closest to the origin, lying in the plane, with its corners
        // counter-clockwise when looked at from the front (against the normal).
        Core::ConvexPolygon CreateHugeSquare(const Core::Plane& plane)
        {
            // Two directions along the plane, perpendicular to each other and to the normal. The cross product of the
            // normal with any direction that is not parallel to it gives a direction along the plane; world up (+Z in
            // map coordinates) works unless the plane is (nearly) horizontal, then world X is used instead.
            const glm::dvec3 helper = glm::abs(plane.normal.z) < 0.9 ? glm::dvec3(0.0, 0.0, 1.0) : glm::dvec3(1.0, 0.0, 0.0);
            const glm::dvec3 right = glm::normalize(glm::cross(helper, plane.normal));

            // up = normal x right. With this order cross(right, up) equals the normal: seen from the front, "right" points
            // right and "up" points up, so the corners below (left-down, right-down, right-up, left-up) go counter-clockwise.
            const glm::dvec3 up = glm::cross(plane.normal, right);

            const glm::dvec3 center = plane.normal * plane.distanceFromOrigin;
            const glm::dvec3 toRight = right * HugeSquareHalfSize;
            const glm::dvec3 toUp = up * HugeSquareHalfSize;

            return {
                center - toRight - toUp,
                center + toRight - toUp,
                center + toRight + toUp,
                center - toRight + toUp,
            };
        }
    }

    std::vector<Core::ConvexPolygon> BuildBrushPolygons(const MapBrush& brush)
    {
        // First the plane of every face; a face without a valid plane gets nothing.
        std::vector<std::optional<Core::Plane>> planes;
        planes.reserve(brush.faces.size());
        for (const MapFace& face : brush.faces)
            planes.push_back(Core::CreatePlaneFromPoints(face.points[0], face.points[1], face.points[2]));

        std::vector<Core::ConvexPolygon> polygons(brush.faces.size());
        for (std::size_t faceIndex = 0; faceIndex < brush.faces.size(); ++faceIndex)
        {
            if (!planes[faceIndex].has_value())
                continue;

            // Start with the huge square on the face's plane and cut it by every other plane of the brush: only the part
            // behind all of them stays (the brush is behind all its planes), and that part is the face.
            Core::ConvexPolygon polygon = CreateHugeSquare(*planes[faceIndex]);
            for (std::size_t otherIndex = 0; otherIndex < brush.faces.size() && !polygon.empty(); ++otherIndex)
                if (otherIndex != faceIndex && planes[otherIndex].has_value())
                    polygon = Core::ClipPolygon(polygon, *planes[otherIndex]);

            polygons[faceIndex] = std::move(polygon);
        }

        return polygons;
    }
}
