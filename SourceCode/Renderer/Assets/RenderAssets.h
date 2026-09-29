#pragma once

#include "Core/Assets/AssetLifetime.h"
#include "Renderer/Assets/MeshStore.h"
#include "Renderer/Assets/ShaderStore.h"
#include "Renderer/Assets/TextureStore.h"

namespace Abomination::Renderer
{
    // Every graphics asset of the game: the stores of all asset types the renderer draws with.
    // Code that draws gets this one object instead of one parameter per store, so a new asset type (materials in
    // 0.3) adds a member here instead of a parameter to every drawing function.
    //
    // Only graphics assets belong here: sounds get their own store in the Audio module, so code that draws never
    // sees sounds and code that plays sounds never sees shaders.
    //
    // Unlike UI::DebugOverlayContext, which only refers to systems owned by others, this struct owns the stores.
    // Owned by the application. Requires a current OpenGL context. Move-only.
    struct RenderAssets
    {
        TextureStore textures;
        ShaderStore shaders;
        MeshStore meshes;

        // Removes every texture and mesh of the lifetime group (loading the next level removes the Level group). Shader
        // programs are always global.
        void RemoveAll(Core::AssetLifetime lifetime)
        {
            textures.RemoveAll(lifetime);
            meshes.RemoveAll(lifetime);
        }
    };
}
