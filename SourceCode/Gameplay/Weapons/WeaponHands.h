#pragma once

#include "Renderer/Assets/ModelStore.h"
#include "Renderer/ModelPose.h"

#include <glm/mat4x4.hpp>

namespace Abomination::Gameplay
{
    struct WeaponViewModel;

    // The hands of the player holding the weapon in the hands (see WeaponViewModel): a model with a skeleton (WRAD ARMS),
    // posed in Blender on the weapon (Tools/Blender/HandsPoseScene.py and ExportHandsPoses.py) and exported in the
    // coordinates of the weapon, with two poses as clips:
    //   Hold         - the hands holding the weapon;
    //   HoldPumpBack - the same, the left hand on the pump pulled back.
    // The game draws them with the weapon's matrix, so they follow everything it does, and blends the two poses as the
    // pump goes back and forth.
    struct WeaponHands
    {
        Renderer::ModelHandle model;

        // Debug: the hands can be hidden (the Weapon window, In hands tab).
        bool isVisible = true;

        // The pose of now, drawn with the hands (see CalculateWeaponHandsPose).
        Renderer::ModelPose pose;
    };

    // The matrix that places the hands model relative to the eyes: the weapon's (see CalculateWeaponViewModelMatrix), the
    // centering the game gives every model undone, since the hands are posed around the middle of the weapon.
    [[nodiscard]] glm::mat4 CalculateWeaponHandsMatrix(const WeaponViewModel& weaponViewModel,
                                                       const Renderer::Model& handsModel);

    // Sets hands.pose for the pump as it is now: Hold at rest, HoldPumpBack with the pump all the way back, mixed in
    // between. A model without these clips is drawn at rest.
    void CalculateWeaponHandsPose(WeaponViewModel& weaponViewModel, const Renderer::Model& handsModel);
}
