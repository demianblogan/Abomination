#pragma once

#include "Core/Math/Units.h"
#include "Core/Scene/Transform.h"
#include "Gameplay/MouseLook.h"
#include "World/PlayerStart.h"

#include <entt/entt.hpp>

// The body of the player (its box) never turns: the view turns with the mouse (the LookAngles component of the player
// entity), and walking goes along the yaw.
namespace Abomination::Gameplay
{
    // The eyes of the player above the center of their box: 18 units, as in Quake (the box of the Quake player goes from
    // 24 units below its origin to 32 above it, the eyes are 22 above the origin, so 18 above the center of the box).
    inline constexpr float PlayerEyeHeight = Core::MapUnitsToMeters(18.0f);

    // Component of a character: how far what is seen of it is below where its body is after the body stepped up a stair
    // (0 or negative), now and one tick earlier (it is drawn between the two, like interpolated transforms). The body
    // goes up a step at once (see Physics::StepSlideMove); what is seen follows it smoothly, like in Quake, so climbing
    // stairs does not jerk. For the player it is the eyes; for other characters their model (see Renderer::DrawOffset).
    // Only the picture uses it: the body is where the physics put it.
    struct StepSmoothing
    {
        float offset = 0.0f;
        float previousOffset = 0.0f;
    };

    // How fast the eyes catch up with the body (80 units/s in Quake) and how far they may fall behind (12 units).
    inline constexpr float StepSmoothingSpeed = Core::MapUnitsToMeters(80.0f);
    inline constexpr float MaximumStepLag = Core::MapUnitsToMeters(12.0f);

    // One tick of the smoothing: the eyes stay behind by the height the body just stepped up (at most MaximumStepLag),
    // then catch up by StepSmoothingSpeed.
    void UpdateStepSmoothing(StepSmoothing& smoothing, float steppedUpHeight, float deltaTime);

    // Creates the player at the player start of the level: Name, Transform (the center of the box, at
    // playerStart.boxCenter), PreviousTransform (the player moves in ticks), Physics::CharacterBody with the box of
    // the Quake player (World::PlayerHalfExtents), LookAngles, StepSmoothing and a Renderer::CameraLens (the
    // player is also a camera).
    entt::entity SpawnPlayer(entt::registry& registry, const World::PlayerStart& playerStart);

    // Where the camera of the player is: at the eyes above the body (moved by stepOffset, see StepSmoothing),
    // looking along the look angles. body is the transform of the player (it may be interpolated).
    [[nodiscard]] Core::Transform CalculatePlayerEyeTransform(const Core::Transform& body, const LookAngles& look,
                                                              float stepOffset = 0.0f);
}
