#pragma once

#include "Core/Transform.h"
#include "Core/Units.h"
#include "World/PlayerStart.h"

#include <entt/entt.hpp>

namespace Abomination::Gameplay
{
    // Component: where the player looks, as two angles (see MouseLook.h). The body of the player (its box) never turns;
    // the view turns with the mouse, and walking goes along the yaw.
    struct PlayerLook
    {
        float yaw = 0.0f;
        float pitch = 0.0f;
    };

    // The eyes of the player above the center of their box: 18 units, as in Quake (the box of the Quake player goes from
    // 24 units below its origin to 32 above it, the eyes are 22 above the origin, so 18 above the center of the box).
    inline constexpr float PlayerEyeHeight = Core::MapUnitsToMeters(18.0f);

    // Component: how far the eyes are below where they belong after the body stepped up a stair (0 or negative), now and
    // one tick earlier (the view is drawn between the two, like interpolated transforms). The body goes up a step at once
    // (see Physics::StepSlideMove); the eyes follow it smoothly, like in Quake, so climbing stairs does not jerk the view.
    // Only the view uses it: the body is where the physics put it.
    struct PlayerStepSmoothing
    {
        float offset = 0.0f;
        float previousOffset = 0.0f;
    };

    // How fast the eyes catch up with the body (80 units/s in Quake) and how far they may fall behind (12 units).
    inline constexpr float StepSmoothingSpeed = Core::MapUnitsToMeters(80.0f);
    inline constexpr float MaximumStepLag = Core::MapUnitsToMeters(12.0f);

    // One tick of the smoothing: the eyes stay behind by the height the body just stepped up (at most MaximumStepLag),
    // then catch up by StepSmoothingSpeed.
    void UpdateStepSmoothing(PlayerStepSmoothing& smoothing, float steppedUpHeight, float deltaTime);

    // Creates the player at the player start of the level: Name, Transform (the center of the box, at
    // playerStart.boxCenter), PreviousTransform (the player moves in ticks), Physics::CharacterBody with the box of
    // the Quake player (World::PlayerHalfExtents), PlayerLook, PlayerStepSmoothing and a Renderer::CameraLens (the
    // player is also a camera).
    entt::entity SpawnPlayer(entt::registry& registry, const World::PlayerStart& playerStart);

    // Where the camera of the player is: at the eyes above the body (moved by stepOffset, see PlayerStepSmoothing),
    // looking along the look angles. body is the transform of the player (it may be interpolated).
    [[nodiscard]] Core::Transform CalculatePlayerEyeTransform(const Core::Transform& body, const PlayerLook& look,
                                                              float stepOffset = 0.0f);
}
