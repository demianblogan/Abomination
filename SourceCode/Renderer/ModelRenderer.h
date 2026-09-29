#pragma once

#include "Renderer/Assets/ModelStore.h"
#include "Renderer/Assets/ShaderStore.h"

namespace Abomination::Renderer
{
    // Component: "draw this entity as this model, with this shader program". Like MeshRenderer, but for a model loaded
    // from a file, whose parts each have their own mesh, texture and place in the model. The render system draws every
    // part at the entity's Core::Transform combined with the part's transform.
    struct ModelRenderer
    {
        ModelHandle model;
        ShaderHandle shaderProgram;
    };
}
