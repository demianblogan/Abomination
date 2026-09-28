#pragma once

#include "Renderer/Assets/MeshStore.h"
#include "Renderer/Assets/ShaderStore.h"
#include "Renderer/Assets/TextureStore.h"

namespace Abomination::Renderer
{
    // Component: "draw this entity as this mesh, with this texture and shader program". Together with Core::Transform
    // (where) it makes an entity visible; the render system draws every entity that has both.
    // Only handles, no pointers: many entities share the same mesh and texture, and a component stays valid when the
    // stores move (like everything owned by Application) or when it is saved (0.8).
    struct MeshRenderer
    {
        MeshHandle mesh;
        TextureHandle texture;
        ShaderHandle shaderProgram;
    };
}
