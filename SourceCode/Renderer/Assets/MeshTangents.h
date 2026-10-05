#pragma once

#include "Renderer/Assets/MeshData.h"

namespace Abomination::Renderer
{
    // Fills MeshVertex::tangent of every vertex from the positions, normals and texture coordinates of the triangles,
    // with MikkTSpace: the standard way to calculate tangents, used by Blender, Substance and most programs that bake
    // normal maps. A normal map is only right with the tangents it was baked with, so the models of glTF files get them
    // this way (see GLTFLoader for why the tangents of a file are not used). The level calculates its own (see
    // World::CalculateTangent).
    //
    // MikkTSpace works per corner of a triangle; corners that share a vertex get the same tangent from it, because a
    // vertex is only shared where the position, the normal and the texture coordinates are the same (a seam of the
    // texture has separate vertices on both sides). Returns false and leaves the tangents as they were if MikkTSpace
    // fails (a mesh without triangles).
    [[nodiscard]] bool GenerateTangents(MeshData& mesh);
}
