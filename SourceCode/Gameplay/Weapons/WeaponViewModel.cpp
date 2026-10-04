#include "Gameplay/Weapons/WeaponViewModel.h"

#include "Audio/AudioEngine.h"
#include "Gameplay/Weapons/Weapon.h"
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

        // The matrices apply from the right (the last one first): the model is tilted on its side around its barrel (+Z:
        // a positive angle turns the right side up, counter-clockwise as the player sees it); turned around a point along
        // it (see PumpActionSettings::turnPivot), to the left (+Y) and up (+X: -Z, forward, towards +Y, up), so the barrel
        // swings across; turned up around its middle by the recoil; and placed relative to the eyes.
        const PumpAction& pump = weaponViewModel.pump;
        constexpr glm::vec3 Right(1.0f, 0.0f, 0.0f);
        constexpr glm::vec3 Up(0.0f, 1.0f, 0.0f);
        constexpr glm::vec3 Back(0.0f, 0.0f, 1.0f);
        glm::mat4 matrix = glm::translate(glm::mat4(1.0f), position);
        matrix = glm::rotate(matrix, weaponViewModel.motion.recoilPitch, Right);
        const glm::vec3 pivot(0.0f, 0.0f, (0.5f - pump.turnPivot) * pump.modelLength);
        matrix = glm::translate(matrix, pivot);
        matrix = glm::rotate(matrix, pump.turn, Up);
        matrix = glm::rotate(matrix, pump.lift, Right);
        matrix = glm::translate(matrix, -pivot);
        return glm::rotate(matrix, pump.tilt, Back);
    }

    glm::vec3 CalculateWeaponViewModelMuzzle(const WeaponViewModel& weaponViewModel)
    {
        return glm::vec3(CalculateWeaponViewModelMatrix(weaponViewModel) * glm::vec4(weaponViewModel.muzzle, 1.0f));
    }

    void UpdateWeaponViewModel(const GameplayState& state, entt::registry& registry, Audio::AudioEngine& audio,
                               float deltaTime)
    {
        WeaponViewModel* weaponViewModel = registry.try_get<WeaponViewModel>(state.player);
        if (weaponViewModel == nullptr)
            return;

        weaponViewModel->flashTimeLeft -= deltaTime;

        // The cycle of a shot (set by the weapon, see PumpAction.h): the pose at the chest and the pump now, and the sound of
        // the pump, started so that its first clack comes when the pump reaches the back.
        if (const Weapon* weapon = registry.try_get<Weapon>(state.player); weapon != nullptr)
        {
            const PumpActionSettings& settings = weapon->settings.pumpAction;
            PumpAction& pump = weaponViewModel->pump;
            const float previous = pump.secondsSinceShot;
            pump.secondsSinceShot += deltaTime;
            const float soundStart = CalculatePumpStartTime(settings) + settings.backDuration - PumpSoundBackClackTime;
            if (previous < soundStart && pump.secondsSinceShot >= soundStart)
                audio.Play(weapon->pumpSound);

            // At the back the spent shell flies out of the window.
            const float backTime = CalculatePumpStartTime(settings) + settings.backDuration;
            if (previous < backTime && pump.secondsSinceShot >= backTime)
                pump.isShellEjectRequested = true;

            // To the chest means towards the middle of the body: to the left (positive angles) for a weapon held on the right
            // or in the middle, to the right for one held on the left.
            const float towardsBody = weaponViewModel->side == WeaponViewModelSide::Left ? -1.0f : 1.0f;
            const float pose = CalculateChestPose(settings, pump.secondsSinceShot);
            pump.progress = CalculatePumpProgress(settings, pump.secondsSinceShot);
            pump.travel = pump.progress * settings.travel;
            pump.turn = pose * towardsBody * settings.turnAngle;
            pump.lift = pose * settings.liftAngle;
            pump.turnPivot = settings.turnPivot;
            pump.tilt = pose * towardsBody * settings.tiltAngle;
        }

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
