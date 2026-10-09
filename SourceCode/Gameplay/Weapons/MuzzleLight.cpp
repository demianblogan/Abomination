#include "Gameplay/Weapons/MuzzleLight.h"

#include "Core/Scene/Name.h"
#include "Gameplay/GameplayState.h"
#include "Gameplay/Weapons/WeaponViewModel.h"
#include "Renderer/ColorSpace.h"
#include "Renderer/Light.h"

#include <algorithm>

namespace Abomination::Gameplay
{
    void FlashMuzzleLight(GameplayState& state)
    {
        state.muzzleLight.timeLeft = state.effects.settings.flashLightDuration;
    }

    void UpdateMuzzleLight(GameplayState& state, entt::registry& registry, const Core::Transform& eyes, float deltaTime)
    {
        MuzzleLight& muzzleLight = state.muzzleLight;
        const WeaponViewModel* weaponViewModel = registry.try_get<WeaponViewModel>(state.player);
        if (muzzleLight.timeLeft <= 0.0f || weaponViewModel == nullptr)
        {
            // Dark: without a Light the light buffer does not even hold it.
            if (registry.valid(muzzleLight.entity))
                registry.remove<Renderer::Light>(muzzleLight.entity);
            return;
        }

        // The entity may be gone with a level that was replaced; it is made again when needed.
        if (!registry.valid(muzzleLight.entity))
        {
            muzzleLight.entity = registry.create();
            registry.emplace<Core::Name>(muzzleLight.entity, "Muzzle flash light");
            registry.emplace<Core::Transform>(muzzleLight.entity);
        }

        // The muzzle relative to the eyes, placed by the eyes (like the smoke of the shot, see WeaponSystem).
        registry.get<Core::Transform>(muzzleLight.entity).position =
            eyes.position + eyes.rotation * CalculateWeaponViewModelMuzzle(*weaponViewModel);

        const EffectSettings& settings = state.effects.settings;
        const float remaining = std::clamp(muzzleLight.timeLeft / std::max(settings.flashLightDuration, 1e-4f), 0.0f, 1.0f);
        registry.emplace_or_replace<Renderer::Light>(muzzleLight.entity, Renderer::Light{
            .color = Renderer::ConvertSRGBToLinear(settings.flashLightColor),
            .intensity = settings.flashLightIntensity * remaining * remaining,
            .range = settings.flashLightRange,
        });

        muzzleLight.timeLeft -= deltaTime;
    }
}
