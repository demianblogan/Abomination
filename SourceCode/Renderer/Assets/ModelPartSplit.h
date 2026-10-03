#pragma once

#include "Renderer/Assets/MeshData.h"
#include "Renderer/Assets/ModelData.h"

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <string>

namespace Abomination::Renderer
{
    // Takes some triangles out of a part of a model into a new part of their own, so the game can move them apart from the
    // rest: the shotgun model has its bolt (seen in the window) in its body, and the bolt must slide back with the pump.
    // A triangle is taken when all three of its vertices lie inside the box (in the coordinates of the source part, as
    // the model file has them) and their texture coordinates inside the rectangle (as the game reads them: v goes up).
    // Both together pick a piece exactly: the box alone may catch the edge of a frame around it, the rectangle alone
    // another piece painted with the same part of the texture.
    struct ModelPartSplit
    {
        std::string sourcePartName;
        std::string partName;

        glm::vec3 boxMinimum{0.0f};
        glm::vec3 boxMaximum{0.0f};
        glm::vec2 texCoordMinimum{0.0f};
        glm::vec2 texCoordMaximum{1.0f};

        // If not empty, one more part with this name: a copy of the taken triangles moved backingInset meters inwards
        // (against their normals), which stays where they were. Without it, the place the new part moved away from
        // would be a hole through the model. Its texture is backingTexturePath (relative to the assets directory), not one
        // of the model: a dark opening, for example.
        std::string backingPartName;
        std::string backingTexturePath;
        float backingInset = 0.0003f;
    };

    // Splits the part (see ModelPartSplit) and adds the new parts at the end of model.parts. Returns false and changes
    // nothing if there is no source part or no triangle is taken.
    bool SplitModelPart(ModelData& model, const ModelPartSplit& split);

    // A copy of the mesh with every vertex moved inset meters against its normal (into the surface).
    [[nodiscard]] MeshData CreateInsetMeshCopy(const MeshData& mesh, float inset);
}
