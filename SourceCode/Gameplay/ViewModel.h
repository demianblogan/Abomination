#pragma once

#include "Renderer/Assets/ModelStore.h"
#include "Renderer/Assets/ShaderStore.h"

#include <glm/mat4x4.hpp>
#include <glm/trigonometric.hpp>
#include <glm/vec3.hpp>

#include <array>
#include <string_view>

namespace Abomination::Gameplay
{
    // Which side of the screen the weapon is held on. Modern shooters hold it on the right; Quake held it in the middle.
    enum class ViewModelSide
    {
        Right,
        Center,
        Left,
    };

    // Names for the debug overlay, in the order of the enum values.
    inline constexpr std::array<std::string_view, 3> ViewModelSideNames = {"Right", "Center", "Left"};

    // Component of the player: the weapon in their hands as they see it (a "view model"). It is not in the world: it is
    // drawn over the world, placed relative to the eyes (see Renderer::DrawViewModel), so it follows the view exactly
    // and never goes into walls. What the weapon does in the world (shots) is decided elsewhere.
    struct ViewModel
    {
        Renderer::ModelHandle model;
        Renderer::ShaderHandle shaderProgram;

        // Where the middle of the model is relative to the eyes when held on the right: meters to the right, up (negative:
        // down) and forward (negative: in front, the eyes look along -Z). The model points forward along -Z.
        glm::vec3 offset{0.082f, -0.176f, -0.355f};

        ViewModelSide side = ViewModelSide::Right;

        // The field of view the weapon is drawn with, apart from that of the world (radians, vertical). Narrower than the
        // world's 60 degrees: the weapon looks less stretched in depth, the way it is seen when held.
        float verticalFOV = glm::radians(34.0f);
    };

    // The matrix that places the model relative to the eyes (see Renderer::DrawViewModel): the offset, with its
    // sideways part taken to the left for ViewModelSide::Left and removed for ViewModelSide::Center. The model itself
    // is not mirrored on the left: a mirrored weapon would have its parts on the wrong side.
    [[nodiscard]] glm::mat4 CalculateViewModelMatrix(const ViewModel& viewModel);
}
