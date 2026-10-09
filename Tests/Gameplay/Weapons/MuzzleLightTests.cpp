#include "Gameplay/GameplayState.h"
#include "Gameplay/Weapons/MuzzleLight.h"
#include "Gameplay/Weapons/WeaponViewModel.h"
#include "Renderer/Light.h"

#include <gtest/gtest.h>

namespace Abomination::Gameplay
{
    namespace
    {
        // A player with a weapon in the hands, whose muzzle is half a meter in front of the eyes.
        GameplayState MakeState(entt::registry& registry)
        {
            GameplayState state;
            state.player = registry.create();
            registry.emplace<WeaponViewModel>(state.player, WeaponViewModel{.muzzle = {0.0f, 0.0f, -0.5f}});
            return state;
        }
    }

    TEST(MuzzleLight, ShotLightsAtTheMuzzleAndFadesOut)
    {
        entt::registry registry;
        GameplayState state = MakeState(registry);
        const Core::Transform eyes{.position = {0.0f, 1.0f, 0.0f}};
        const float duration = state.effects.settings.flashLightDuration;

        FlashMuzzleLight(state);
        UpdateMuzzleLight(state, registry, eyes, duration / 2.0f);

        // Full at the shot, at the muzzle seen from the eyes.
        ASSERT_TRUE(registry.valid(state.muzzleLight.entity));
        const auto* light = registry.try_get<Renderer::Light>(state.muzzleLight.entity);
        ASSERT_NE(light, nullptr);
        EXPECT_FLOAT_EQ(light->intensity, state.effects.settings.flashLightIntensity);
        const glm::vec3 muzzle = eyes.position + CalculateWeaponViewModelMuzzle(registry.get<WeaponViewModel>(state.player));
        EXPECT_EQ(registry.get<Core::Transform>(state.muzzleLight.entity).position, muzzle);

        // Halfway, a quarter: the intensity falls with the square of the time left.
        UpdateMuzzleLight(state, registry, eyes, duration / 2.0f);
        EXPECT_NEAR(registry.get<Renderer::Light>(state.muzzleLight.entity).intensity,
                    state.effects.settings.flashLightIntensity * 0.25f, 1e-3f);

        // Then dark: no Light at all.
        UpdateMuzzleLight(state, registry, eyes, duration);
        EXPECT_FALSE(registry.all_of<Renderer::Light>(state.muzzleLight.entity));
    }

    TEST(MuzzleLight, NoShotNoLight)
    {
        entt::registry registry;
        GameplayState state = MakeState(registry);

        UpdateMuzzleLight(state, registry, Core::Transform{}, 0.016f);

        EXPECT_FALSE(registry.valid(state.muzzleLight.entity));
    }
}
