#include "Gameplay/LandingDip.h"

#include "Gameplay/ViewModelMotion.h"
#include "Physics/CharacterBody.h"

namespace Abomination::Gameplay
{
    void UpdateLandingDip(LandingDip& dip, bool isOnGround, float verticalSpeed, float deltaTime)
    {
        // The fall speed comes from the last frame: by now the physics has stopped the fall.
        const float fallSpeed = -dip.previousVerticalSpeed;
        if (!dip.wasOnGround && isOnGround && fallSpeed > MinimumLandingDipSpeed)
            dip.velocity -= fallSpeed * dip.kickPerFallSpeed;

        dip.wasOnGround = isOnGround;
        dip.previousVerticalSpeed = verticalSpeed;

        UpdateDampedSpring(dip.offset, dip.velocity, dip.springStiffness, deltaTime);
    }

    void UpdatePlayerLandingDip(const GameplayState& state, entt::registry& registry, float deltaTime)
    {
        LandingDip* dip = registry.try_get<LandingDip>(state.player);
        if (dip == nullptr)
            return;

        const Physics::CharacterBody& body = registry.get<Physics::CharacterBody>(state.player);
        UpdateLandingDip(*dip, body.isOnGround, body.velocity.y, deltaTime);
    }
}
