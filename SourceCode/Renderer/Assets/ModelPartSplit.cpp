#include "Renderer/Assets/ModelPartSplit.h"

#include <glm/common.hpp>
#include <glm/vector_relational.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <utility>

namespace Abomination::Renderer
{
    namespace
    {
        bool IsVertexTaken(const MeshVertex& vertex, const ModelPartSplit& split)
        {
            return glm::all(glm::greaterThanEqual(vertex.position, split.boxMinimum)) &&
                   glm::all(glm::lessThanEqual(vertex.position, split.boxMaximum)) &&
                   glm::all(glm::greaterThanEqual(vertex.texCoord, split.texCoordMinimum)) &&
                   glm::all(glm::lessThanEqual(vertex.texCoord, split.texCoordMaximum));
        }
    }

    bool SplitModelPart(ModelData& model, const ModelPartSplit& split)
    {
        const auto source = std::ranges::find(model.parts, split.sourcePartName, &ModelPartData::name);
        if (source == model.parts.end())
            return false;

        // Every triangle goes either to the new mesh or stays. The new mesh gets copies of the vertices of its triangles,
        // numbered anew: newIndices maps the number of a vertex in the source to its number in the new mesh. The source
        // keeps all its vertices (the ones only the taken triangles used are left unused, which costs nothing to draw).
        const MeshData& sourceMesh = source->mesh;
        MeshData taken;
        std::vector<std::uint32_t> keptIndices;
        std::unordered_map<std::uint32_t, std::uint32_t> newIndices;
        for (std::size_t first = 0; first + 2 < sourceMesh.indices.size(); first += 3)
        {
            const std::uint32_t triangle[3] = {sourceMesh.indices[first], sourceMesh.indices[first + 1],
                                               sourceMesh.indices[first + 2]};
            const bool isTaken = std::ranges::all_of(triangle, [&](std::uint32_t index)
                                                     { return IsVertexTaken(sourceMesh.vertices[index], split); });
            if (!isTaken)
            {
                keptIndices.insert(keptIndices.end(), std::begin(triangle), std::end(triangle));
                continue;
            }

            for (const std::uint32_t index : triangle)
            {
                const auto [found, isNew] = newIndices.try_emplace(index, static_cast<std::uint32_t>(taken.vertices.size()));
                if (isNew)
                    taken.vertices.push_back(sourceMesh.vertices[index]);
                taken.indices.push_back(found->second);
            }
        }

        if (taken.indices.empty())
            return false;

        source->mesh.indices = std::move(keptIndices);

        // The new parts sit where the source does. push_back may move the parts, so the source is copied out first.
        const glm::mat4 transform = source->transform;
        const std::optional<ModelMaterialData> material = source->material;
        if (!split.backingPartName.empty())
        {
            model.parts.push_back(ModelPartData{
                .name = split.backingPartName,
                .mesh = CreateInsetMeshCopy(taken, split.backingInset),
                .transform = transform,
            });
        }
        model.parts.push_back(ModelPartData{
            .name = split.partName,
            .mesh = std::move(taken),
            .transform = transform,
            .material = material,
        });
        return true;
    }

    MeshData CreateInsetMeshCopy(const MeshData& mesh, float inset)
    {
        MeshData copy = mesh;
        for (MeshVertex& vertex : copy.vertices)
            vertex.position -= vertex.normal * inset;
        return copy;
    }
}
