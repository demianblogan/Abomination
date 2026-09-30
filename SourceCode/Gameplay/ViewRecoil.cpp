#include "Gameplay/ViewRecoil.h"

#include "Core/Math/Spring.h"

namespace Abomination::Gameplay
{
    void KickViewRecoil(ViewRecoil& recoil)
    {
        recoil.pitchVelocity += recoil.kick;
    }

    void UpdatePlayerViewRecoil(const GameplayState& state, entt::registry& registry, float deltaTime)
    {
        if (ViewRecoil* recoil = registry.try_get<ViewRecoil>(state.player); recoil != nullptr)
            Core::UpdateDampedSpring(recoil->pitch, recoil->pitchVelocity, recoil->springStiffness, deltaTime);
    }
}
