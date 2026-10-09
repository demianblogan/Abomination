#pragma once

#include "Renderer/Assets/MeshStore.h"
#include "Renderer/Assets/ShaderStore.h"
#include "Renderer/Material.h"

namespace Abomination::Renderer
{
    // Component: "draw this entity as this mesh, of this material, with this shader program". Together with
    // Core::Transform (where) it makes an entity visible; the render system draws every entity that has both.
    // Only handles and numbers, no pointers: many entities share the same mesh and textures, and a component stays valid
    // when the stores move (like everything owned by Application) or when it is saved (0.8).
    struct MeshRenderer
    {
        MeshHandle mesh;
        Material material;
        ShaderHandle shaderProgram;
    };
}
