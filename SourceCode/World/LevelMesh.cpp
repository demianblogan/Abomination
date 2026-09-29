#include "World/LevelMesh.h"

#include "Core/Math/Plane.h"
#include "World/BrushGeometry.h"
#include "World/MapCoordinates.h"
#include "World/TextureCoordinates.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace Abomination::World
{
    LevelMesh BuildLevelMesh(const MapEntity& entity, const TextureSizeLookup& getTextureSize)
    {
        LevelMesh result;
        LevelMeshStatistics& counts = result.statistics;

        // Texture name -> index of its part in result.parts, to find the part of a face without searching the list.
        std::unordered_map<std::string, std::size_t> partIndices;

        for (const MapBrush& brush : entity.brushes)
        {
            ++counts.brushCount;

            // One polygon per face, in the order of brush.faces: polygons[i] is the shape of brush.faces[i].
            const std::vector<Core::ConvexPolygon> polygons = BuildBrushPolygons(brush);
            for (std::size_t faceIndex = 0; faceIndex < polygons.size(); ++faceIndex)
            {
                const Core::ConvexPolygon& polygon = polygons[faceIndex];
                const MapFace& face = brush.faces[faceIndex];

                // A face that does not exist (a plane that misses the brush) has no vertices.
                if (polygon.empty())
                    continue;

                // The normal of every vertex is the normal of the face's plane, turned into game axes. It is not
                // calculated from the vertices (the cross product of two edges): after clipping, two neighbouring vertices
                // can lie a hair apart, and such a short edge gives an inaccurate normal, or NaN when the vertices are
                // equal. The plane is exact. A face with vertices always has a plane (BuildBrushPolygons skips the rest).
                const std::optional<Core::Plane> mapPlane =
                    Core::CreatePlaneFromPoints(face.points[0], face.points[1], face.points[2]);
                if (!mapPlane.has_value())
                    continue;
                const glm::vec3 normal(ConvertMapPlane(*mapPlane).normal);

                ++counts.faceCount;

                // try_emplace adds the texture with the index of a new part only if it is not there yet; either way it
                // returns the entry of the texture (and whether it was just added).
                const auto [entry, isNewTexture] = partIndices.try_emplace(face.textureName, result.parts.size());
                if (isNewTexture)
                    result.parts.push_back(LevelMeshPart{.textureName = face.textureName});
                Renderer::MeshData& data = result.parts[entry->second].data;

                // The vertices of the face in game coordinates. Texture coordinates are calculated from the vertex in map
                // coordinates, the space the texture axes of the face are given in.
                const glm::ivec2 textureSize = getTextureSize(face.textureName);
                const auto firstVertex = static_cast<std::uint32_t>(data.vertices.size());
                for (const glm::dvec3& polygonVertex : polygon)
                    data.vertices.push_back(Renderer::MeshVertex{
                        .position = ConvertMapPosition(polygonVertex),
                        .texCoord = CalculateTextureCoordinates(face, polygonVertex, textureSize),
                        .normal = normal,
                    });

                // A convex polygon is cut into triangles like a fan: vertex 0 with every pair of neighbours after it,
                // (0, 1, 2), (0, 2, 3), (0, 3, 4), ... A polygon with N vertices gives N - 2 triangles, all
                // counter-clockwise like the polygon itself.
                const auto polygonVertexCount = static_cast<std::uint32_t>(polygon.size());
                for (std::uint32_t vertex = 1; vertex + 1 < polygonVertexCount; ++vertex)
                {
                    data.indices.insert(data.indices.end(), {firstVertex, firstVertex + vertex, firstVertex + vertex + 1});
                    ++counts.triangleCount;
                }
            }
        }

        return result;
    }
}
