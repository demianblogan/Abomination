#pragma once

#include "Renderer/Assets/MeshData.h"

// Simple meshes built by code, for tests, debugging and placeholders (the fallback cube of the mesh store).
namespace Abomination::Renderer
{
    // A cube of size 1 centered at the origin. Every face has its own 4 vertices with texture coordinates 0-1,
    // so a texture covers each face once.
    [[nodiscard]] MeshData CreateCubeMeshData();

    // A cylinder standing along Y, centered at the origin: sideCount flat sides (8 looks round enough for something
    // small), closed at both ends. The texture wraps around it once: u goes around (from +X towards -Z), v along it from
    // the bottom (0) to the top (1). The bottom and the top end take the color of the texture at the bottom and the top
    // row of the sides.
    [[nodiscard]] MeshData CreateCylinderMeshData(int sideCount, float radius, float length);
}
