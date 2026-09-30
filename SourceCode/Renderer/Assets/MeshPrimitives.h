#pragma once

#include "Renderer/Assets/MeshData.h"

// Simple meshes built by code, for tests, debugging and placeholders (the fallback cube of the mesh store).
namespace Abomination::Renderer
{
    // A cube of size 1 centered at the origin. Every face has its own 4 vertices with texture coordinates 0-1,
    // so a texture covers each face once.
    [[nodiscard]] MeshData CreateCubeMeshData();
}
