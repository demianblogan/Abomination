#pragma once

#include "Core/Files/Image.h"
#include "Renderer/Assets/MeshData.h"

#include <glm/mat4x4.hpp>

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace Abomination::Renderer
{
    // One part of a model: a piece with one texture that moves as a whole. The shotgun has five: the body, the pump,
    // the trigger, the shell and the loading gate. Keeping the parts apart (not merged into one mesh) lets the game move
    // one of them later, for example slide the pump back after a shot.
    struct ModelPartData
    {
        // The name the artist gave the part ("pump_shotgun_0"), to find it by.
        std::string name;

        // The geometry, in the coordinates of the part itself.
        MeshData mesh;

        // Where the part is in the model: moves the coordinates of the part into those of the whole model (meters, +Y up).
        // In a model file parts sit in a tree of nodes, each moved, turned and scaled relative to its parent; this is the
        // whole chain from the root to the part multiplied into one matrix.
        glm::mat4 transform{1.0f};

        // The index of the base color texture of the part in ModelData::images; none for an untextured part.
        std::optional<std::size_t> imageIndex;
    };

    // A model read from a file, in ordinary memory, before it is uploaded to the GPU. Needs no OpenGL, so it can be
    // tested.
    struct ModelData
    {
        std::vector<ModelPartData> parts;

        // The base color textures of the parts, decoded. An image that could not be decoded is left empty (width 0).
        std::vector<Core::Image> images;
    };
}
