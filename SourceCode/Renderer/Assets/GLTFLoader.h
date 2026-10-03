#pragma once

#include "Renderer/Assets/ModelData.h"

#include <expected>
#include <filesystem>
#include <string>

namespace Abomination::Renderer
{
    // Reads a glTF 2.0 model (see ModelData): a .glb file (everything in one binary file) or a .gltf file (JSON with
    // separate buffer and image files next to it). glTF is the standard format for 3D models of games and the web; every
    // modelling program exports it.
    //
    // What the game uses is read: triangles with positions, normals and texture coordinates, the base color texture of
    // their material, and a skeleton with its skin and animation clips (one per model). Other textures (normals,
    // metalness) and morph targets are ignored.
    [[nodiscard]] std::expected<ModelData, std::string> LoadGLTFFile(const std::filesystem::path& path);
}
