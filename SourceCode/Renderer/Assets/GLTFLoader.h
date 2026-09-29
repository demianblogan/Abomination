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
    // Only what the game draws now is read: triangles with positions, normals and texture coordinates, and the base color
    // texture of their material. Other textures (normals, metalness), animations and skins are ignored for now.
    [[nodiscard]] std::expected<ModelData, std::string> LoadGLTFFile(const std::filesystem::path& path);
}
