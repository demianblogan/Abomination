#pragma once

#include "Gameplay/GameplayState.h"
#include "Gameplay/Weapons/WeaponViewModelMotion.h"
#include "Renderer/Assets/ModelStore.h"
#include "Renderer/Assets/ShaderStore.h"

#include <entt/entt.hpp>
#include <glm/mat4x4.hpp>
#include <glm/trigonometric.hpp>
#include <glm/vec3.hpp>

#include <array>
#include <string_view>

namespace Abomination::Gameplay
{
    // Which side of the screen the weapon is held on. Modern shooters hold it on the right; Quake held it in the middle.
    enum class WeaponViewModelSide
    {
        Left,
        Center,
        Right,
    };

    // Names for the debug overlay, in the order of the enum values: from left to right, like the radio buttons.
    inline constexpr std::array<std::string_view, 3> WeaponViewModelSideNames = {"Left", "Center", "Right"};

    // Component of the player: the weapon in their hands as they see it (a "view model"). It is not in the world: it is
    // drawn over the world, placed relative to the eyes (see Renderer::DrawWeaponViewModel), so it follows the view exactly
    // and never goes into walls. What the weapon does in the world (shots) is decided elsewhere.
    struct WeaponViewModel
    {
        Renderer::ModelHandle model;
        Renderer::ShaderHandle shaderProgram;

        // Where the middle of the model is relative to the eyes when held on the right: meters to the right, up (negative:
        // down) and forward (negative: in front, the eyes look along -Z). The model points forward along -Z.
        glm::vec3 offset{0.082f, -0.176f, -0.355f};

        WeaponViewModelSide side = WeaponViewModelSide::Right;

        // The field of view the weapon is drawn with, apart from that of the world (radians, vertical). Narrower than the
        // world's 60 degrees: the weapon looks less stretched in depth, the way it is seen when held.
        float verticalFOV = glm::radians(34.0f);

        // How the weapon swings, lags and dips (see WeaponViewModelMotion.h).
        WeaponViewModelMotionSettings motionSettings;
        WeaponViewModelMotion motion;

        // Where the muzzle is in the model (meters): the flash appears there.
        glm::vec3 muzzle{0.0f};

        // How much longer the muzzle flash is shown (seconds; 0 or less: not shown) and its rotation, new every shot.
        float flashTimeLeft = 0.0f;
        float flashRotation = 0.0f;
    };

    // Where the muzzle of the weapon in the hands is relative to the eyes now (meters, eye space: see
    // CalculateWeaponViewModelMatrix).
    [[nodiscard]] glm::vec3 CalculateWeaponViewModelMuzzle(const WeaponViewModel& weaponViewModel);

    // The matrix that places the model relative to the eyes (see Renderer::DrawWeaponViewModel): the offset, with its
    // sideways part taken to the left for WeaponViewModelSide::Left and removed for WeaponViewModelSide::Center, plus the
    // motion.
    // The model itself is not mirrored on the left: a mirrored weapon would have its parts on the wrong side.
    [[nodiscard]] glm::mat4 CalculateWeaponViewModelMatrix(const WeaponViewModel& weaponViewModel);

    // Once per frame (deltaTime, seconds): moves the motion of the player's weapon with what the player does (see
    // UpdateWeaponViewModelMotion).
    void UpdateWeaponViewModel(const GameplayState& state, entt::registry& registry, float deltaTime);
}
