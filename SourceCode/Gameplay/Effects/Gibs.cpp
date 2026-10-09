#include "Gameplay/Effects/Gibs.h"

#include "Core/Scene/Name.h"
#include "Core/Scene/Transform.h"
#include "Gameplay/Effects/Effects.h"
#include "Gameplay/GameplayState.h"
#include "Renderer/Assets/RenderAssets.h"
#include "Renderer/ModelRenderer.h"

#include <glm/geometric.hpp>
#include <glm/gtc/quaternion.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <format>
#include <numbers>
#include <vector>

namespace Abomination::Gameplay
{
    namespace
    {
        // The chunks collide as small boxes: 3 cm from the middle to each side, smaller than the meat they show, so they
        // settle into the floor a little instead of floating on their corners.
        constexpr double GibHalfSize = 0.03;

        // How fast a chunk of an old group sinks into the floor (half a second), and how deep before it is gone.
        constexpr float SinkSpeed = 0.3f;
        constexpr float SinkDepth = 0.15f;

        // How many sprays of blood a burst throws, in all directions.
        constexpr int BloodSprayCount = 8;

        // A direction of length 1 in a random direction (a random point in a cube, kept if it lies in the ball).
        glm::vec3 RandomDirection(Core::Random& random)
        {
            for (;;)
            {
                const glm::vec3 point(random.GetFloat(-1.0f, 1.0f), random.GetFloat(-1.0f, 1.0f),
                                      random.GetFloat(-1.0f, 1.0f));
                const float length = glm::length(point);
                if (length > 0.1f && length <= 1.0f)
                    return point / length;
            }
        }

        // A new chunk of the group, drawn as one of the gib models.
        Gib& CreateGib(Gibs& gibs, entt::registry& registry, std::uint64_t group, std::size_t modelIndex)
        {
            const entt::entity entity = registry.create();
            registry.emplace<Core::Name>(entity, std::format("Gib of burst {}", group + 1));
            registry.emplace<Core::Transform>(entity);
            registry.emplace<Renderer::ModelRenderer>(entity, Renderer::ModelRenderer{
                .model = gibs.models[modelIndex],
                .shaderProgram = gibs.shaderProgram,
            });
            return gibs.gibs.emplace_back(Gib{.entity = entity, .group = group});
        }

        // Too many groups lying: the oldest ones that do not sink yet start to, every chunk of each together.
        void SinkOldestGroups(Gibs& gibs)
        {
            std::vector<std::uint64_t> lying;
            for (const Gib& gib : gibs.gibs)
                if (!gib.isSinking && std::ranges::find(lying, gib.group) == lying.end())
                    lying.push_back(gib.group);
            std::ranges::sort(lying);

            const auto excess = static_cast<std::ptrdiff_t>(lying.size()) - std::max(gibs.settings.maximumGroups, 0);
            for (std::ptrdiff_t index = 0; index < excess; ++index)
                for (Gib& gib : gibs.gibs)
                    if (gib.group == lying[static_cast<std::size_t>(index)])
                        gib.isSinking = true;
        }
    }

    Gibs LoadGibs(Renderer::RenderAssets& renderAssets, Audio::AudioEngine& audio)
    {
        Gibs gibs;
        for (std::size_t index = 0; index < gibs.models.size(); ++index)
            gibs.models[index] = renderAssets.LoadModel(std::format("Models/Enemies/Gibs/Gib{}.glb", index + 1),
                                                        Core::AssetLifetime::Global);
        gibs.shaderProgram = renderAssets.shaders.Load("Shaders/Lit");

        // Two bodies bursting together are enough noise.
        gibs.burstSound = LoadGameSound(audio, "Sounds/Enemies/Gib", 1, Audio::SoundGroup::Effects, 2);
        return gibs;
    }

    void BurstIntoGibs(GameplayState& state, entt::registry& registry, Audio::AudioEngine& audio, const glm::vec3& center,
                       const glm::vec3& halfExtents, const glm::vec3& shotDirection)
    {
        Gibs& gibs = state.gibs;
        const GibSettings& settings = gibs.settings;
        Core::Random& random = gibs.random;

        audio.Play(gibs.burstSound, center);
        for (int spray = 0; spray < BloodSprayCount; ++spray)
            SpawnBloodImpact(state.effects, center, RandomDirection(random));

        const std::uint64_t group = gibs.nextGroup++;
        const float shotLength = glm::length(shotDirection);
        const glm::vec3 shot = shotLength > 1e-4f ? shotDirection / shotLength : glm::vec3(0.0f);
        for (int index = 0; index < settings.count; ++index)
        {
            // The three models in turn: a lump, a leg, a scrap, a lump, ...
            Gib& gib = CreateGib(gibs, registry, group, static_cast<std::size_t>(index) % gibs.models.size());

            // Somewhere in the body, flying out from its middle, more upwards, and along the shot.
            Core::Transform& transform = registry.get<Core::Transform>(gib.entity);
            const glm::vec3 offset(random.GetFloat(-1.0f, 1.0f) * halfExtents.x * 0.6f,
                                   random.GetFloat(-0.5f, 0.8f) * halfExtents.y,
                                   random.GetFloat(-1.0f, 1.0f) * halfExtents.z * 0.6f);
            transform.position = center + offset;
            transform.rotation = glm::angleAxis(random.GetFloat(0.0f, 2.0f * std::numbers::pi_v<float>),
                                                RandomDirection(random));

            gib.motion.velocity = RandomDirection(random) * settings.speed * random.GetFloat(0.5f, 1.0f) +
                           glm::vec3(0.0f, settings.upSpeed * random.GetFloat(0.6f, 1.0f), 0.0f) +
                           shot * settings.shotSpeed;
            gib.motion.spinAxis = RandomDirection(random);
            gib.motion.spinSpeed = settings.spinSpeed * random.GetFloat(0.5f, 1.0f);
        }

        SinkOldestGroups(gibs);
    }

    void UpdateGibs(GameplayState& state, entt::registry& registry, std::span<const World::CollisionBrush> brushes,
                    float deltaTime)
    {
        Gibs& gibs = state.gibs;
        const GibSettings& settings = gibs.settings;
        for (Gib& gib : gibs.gibs)
        {
            Core::Transform& transform = registry.get<Core::Transform>(gib.entity);

            // Its group is too old: it sinks into the floor (the chunks still in the air too), and is gone once under it.
            if (gib.isSinking)
            {
                const float step = SinkSpeed * deltaTime;
                gib.sunkDepth += step;
                transform.position.y -= step;
                continue;
            }

            if (gib.isResting)
                continue;

            // It falls, tumbles and bounces (see Tumble); meat hardly bounces.
            const BounceSettings bounce{.bounce = settings.bounce, .slide = settings.slide, .spinKept = 0.4f,
                                        .restSpeed = settings.restSpeed};
            gib.isResting = Tumble(gib.motion, transform, brushes, state.physicsSettings.gravity, GibHalfSize, bounce,
                                   deltaTime)
                                .hasStopped;
        }

        // The chunks under the floor are gone.
        std::erase_if(gibs.gibs, [&registry](const Gib& gib)
        {
            if (gib.sunkDepth < SinkDepth)
                return false;
            registry.destroy(gib.entity);
            return true;
        });
    }

    void ClearGibs(Gibs& gibs, entt::registry& registry)
    {
        for (const Gib& gib : gibs.gibs)
            registry.destroy(gib.entity);
        gibs.gibs.clear();
    }
}
