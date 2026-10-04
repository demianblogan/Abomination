#include "World/NavMeshGeometry.h"

#include "World/BrushGeometry.h"
#include "World/MapCoordinates.h"

#include <vector>

namespace Abomination::World
{
    Navigation::NavMeshGeometry BuildNavMeshGeometry(const MapEntity& entity)
    {
        Navigation::NavMeshGeometry geometry;
        for (const MapBrush& brush : entity.brushes)
        {
            for (const Core::ConvexPolygon& polygon : BuildBrushPolygons(brush))
            {
                if (polygon.size() < 3)
                    continue;

                // The polygon as a fan of triangles, counter-clockwise seen from outside the brush like the polygon (see
                // BuildLevelMesh): Recast takes a triangle whose normal points up enough as floor.
                const auto first = static_cast<int>(geometry.vertices.size());
                for (const glm::dvec3& vertex : polygon)
                    geometry.vertices.push_back(ConvertMapPosition(vertex));
                for (int corner = 1; corner + 1 < static_cast<int>(polygon.size()); ++corner)
                    geometry.indices.insert(geometry.indices.end(), {first, first + corner, first + corner + 1});
            }
        }
        return geometry;
    }
}
