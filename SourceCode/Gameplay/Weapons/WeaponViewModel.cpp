#include "Gameplay/Weapons/WeaponViewModel.h"

#include "Physics/CharacterBody.h"

#include <glm/ext/matrix_transform.hpp>
#include <glm/geometric.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace Abomination::Gameplay
{
    glm::mat4 CalculateWeaponViewModelMatrix(const WeaponViewModel& weaponViewModel)
    {
        glm::vec3 position = weaponViewModel.offset;
        switch (weaponViewModel.side)
        {
        case WeaponViewModelSide::Right:
            break;
        case WeaponViewModelSide::Center:
            position.x = 0.0f;
            break;
        case WeaponViewModelSide::Left:
            position.x = -position.x;
            break;
        }

        position += CalculateWeaponViewModelMotionOffset(weaponViewModel.motion, weaponViewModel.motionSettings);

        // The recoil turns the muzzle up around the middle of the model: a positive angle around +X (the right) turns -Z
        // (forward) towards +Y (up).
        const glm::mat4 placed = glm::translate(glm::mat4(1.0f), position);
        return glm::rotate(placed, weaponViewModel.motion.recoilPitch, glm::vec3(1.0f, 0.0f, 0.0f));
    }

    glm::vec3 CalculateWeaponViewModelMuzzle(const WeaponViewModel& weaponViewModel)
    {
        return glm::vec3(CalculateWeaponViewModelMatrix(weaponViewModel) * glm::vec4(weaponViewModel.muzzle, 1.0f));
    }

    void UpdateWeaponViewModel(const GameplayState& state, entt::registry& registry, float deltaTime)
    {
        WeaponViewModel* weaponViewModel = registry.try_get<WeaponViewModel>(state.player);
        if (weaponViewModel == nullptr)
            return;

        weaponViewModel->flashTimeLeft -= deltaTime;

        const Physics::CharacterBody& body = registry.get<Physics::CharacterBody>(state.player);
        const WeaponViewModelMotionInput input{
            .horizontalSpeed = glm::length(glm::vec2(body.velocity.x, body.velocity.z)),
            .maxSpeed = state.movementSettings.maxSpeed,
            .isOnGround = body.isOnGround,
            .verticalSpeed = body.velocity.y,
            .look = registry.get<LookAngles>(state.player),
        };
        UpdateWeaponViewModelMotion(weaponViewModel->motion, weaponViewModel->motionSettings, input, deltaTime);
    }
}
