#pragma once

#include "Core/Transform.h"
#include "World/Level.h"

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
    inline constexpr float PlayerEyeHeight = 18.0f / 32.0f;

    // Creates the player at the player start of the level: Name, Transform (the center of the box, the eyes at
    // playerStart.eyePosition), PreviousTransform (the player moves in ticks), Physics::CharacterBody with the box of
    // the Quake player (World::PlayerHalfExtents), PlayerLook and a Renderer::CameraLens (the player is also a camera).
    entt::entity SpawnPlayer(entt::registry& registry, const World::PlayerStart& playerStart);

    // Where the camera of the player is: at the eyes above the body, looking along the look angles. body is the transform
    // of the player (it may be interpolated).
    [[nodiscard]] Core::Transform CalculatePlayerEyeTransform(const Core::Transform& body, const PlayerLook& look);
}
