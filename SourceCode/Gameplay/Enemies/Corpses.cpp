#include "Gameplay/Enemies/Corpses.h"

#include "Core/Scene/Transform.h"
#include "Core/Scene/TransformInterpolation.h"
#include "Gameplay/Characters/Health.h"
#include "Gameplay/Effects/Gibs.h"
#include "Gameplay/Enemies/GroundFit.h"
#include "Gameplay/GameplayState.h"
#include "Physics/CharacterBody.h"
#include "Physics/CharacterMovement.h"
#include "Renderer/DrawOffset.h"
#include "World/CollisionBrush.h"

#include <algorithm>
#include <cstddef>
#include <utility>
#include <vector>

namespace Abomination::Gameplay
{
    void LayDownCorpse(GameplayState& state, entt::registry& registry, entt::entity entity)
    {
        // The box is lowered to half its height, the bottom where it was; the model is drawn that much higher, where it
        // was (see Corpse::raise).
        Physics::CharacterBody& body = registry.get<Physics::CharacterBody>(entity);
        const auto lowered = static_cast<float>(body.halfExtents.y * 0.5);
        body.halfExtents.y *= 0.5;
        registry.get<Core::Transform>(entity).position.y -= lowered;
        registry.get<Core::PreviousTransform>(entity).value.position.y -= lowered;

        registry.emplace<Corpse>(entity, Corpse{.order = state.nextCorpseOrder++, .raise = lowered});
    }

    bool BurstIfTornApart(GameplayState& state, entt::registry& registry, Audio::AudioEngine& audio, entt::entity entity,
                          const glm::vec3& shotFrom)
    {
        const Health* health = registry.try_get<Health>(entity);
        if (health == nullptr || health->overkill < state.gibs.settings.burstDamage)
            return false;

        const glm::vec3 center = registry.get<Core::Transform>(entity).position;
        const glm::vec3 halfExtents(registry.get<Physics::CharacterBody>(entity).halfExtents);
        BurstIntoGibs(state, registry, audio, center, halfExtents, center - shotFrom);
        registry.destroy(entity);
        return true;
    }

    void UpdateCorpses(GameplayState& state, entt::registry& registry, std::span<const World::CollisionBrush> brushes,
                       Audio::AudioEngine& audio, const glm::vec3& shotFrom, float tickDuration)
    {
        // Bodies shot to pieces burst (collected first: a burst destroys its entity).
        std::vector<entt::entity> corpseEntities;
        for (const auto [entity, corpse] : registry.view<const Corpse>().each())
            corpseEntities.push_back(entity);
        for (const entt::entity entity : corpseEntities)
            static_cast<void>(BurstIfTornApart(state, registry, audio, entity, shotFrom));

        // Too many bodies: the oldest ones that do not sink yet start to. A sinking body takes no more shots.
        std::vector<std::pair<std::uint64_t, entt::entity>> lying;
        for (const auto [entity, corpse] : registry.view<const Corpse>().each())
            if (!corpse.isSinking)
                lying.emplace_back(corpse.order, entity);
        const auto excess = static_cast<std::ptrdiff_t>(lying.size()) - std::max(state.maximumCorpses, 0);
        if (excess > 0)
        {
            std::ranges::sort(lying);
            for (std::ptrdiff_t index = 0; index < excess; ++index)
            {
                const entt::entity entity = lying[static_cast<std::size_t>(index)].second;
                registry.get<Corpse>(entity).isSinking = true;
                registry.remove<Health>(entity);
            }
        }

        // A body lies still: no wish to move, and friction stops a slide.
        Physics::MovementSettings movement = state.movementSettings;
        movement.maxSpeed = 0.0f;

        std::vector<entt::entity> gone;
        const auto corpses = registry.view<Corpse, Core::Transform, Physics::CharacterBody, GroundFit>();
        for (const auto [entity, corpse, transform, body, fit] : corpses.each())
        {
            Physics::UpdateCharacter(body, transform, brushes, state.physicsSettings, movement, {}, tickDuration);

            // Drawn where it lay down (the fit to the ground of its last moment alive), lower as it sinks; gone once
            // its whole body is under the floor.
            const float depthBefore = corpse.sunkDepth;
            if (corpse.isSinking)
                corpse.sunkDepth += CorpseSinkSpeed * tickDuration;
            registry.replace<Renderer::DrawOffset>(entity, Renderer::DrawOffset{
                .offset = glm::vec3(0.0f, fit.offset + corpse.raise - corpse.sunkDepth, 0.0f),
                .previousOffset = glm::vec3(0.0f, fit.offset + corpse.raise - depthBefore, 0.0f),
                .pitch = fit.pitch,
                .previousPitch = fit.pitch,
            });
            if (corpse.sunkDepth > 2.0f * (static_cast<float>(body.halfExtents.y) + corpse.raise))
                gone.push_back(entity);
        }
        registry.destroy(gone.begin(), gone.end());
    }
}
