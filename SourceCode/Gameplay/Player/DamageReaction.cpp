#include "Gameplay/Player/DamageReaction.h"

#include "Audio/AudioEngine.h"
#include "Core/Math/Spring.h"
#include "Core/Scene/Transform.h"
#include "Gameplay/Characters/Armor.h"
#include "Gameplay/Characters/Health.h"

#include <glm/common.hpp>
#include <glm/geometric.hpp>

#include <algorithm>
#include <array>
#include <cstddef>

namespace Abomination::Gameplay
{
    namespace
    {
        // The damage the punch kick is given for; other blows punch in proportion, within these limits.
        constexpr float ReferencePunchDamage = 20.0f;
        constexpr float WeakestPunch = 0.5f;
        constexpr float StrongestPunch = 1.5f;

        // The tilt of the punch is this part of its kick: a blow tilts the head less than it turns it.
        constexpr float PunchRollShare = 0.6f;

        // The heartbeat sound plays once a second; the recording is exactly one second long, so one sound ends where the
        // next begins. Its two beats are heard this many seconds after it starts.
        constexpr float HeartbeatInterval = 1.0f;
        constexpr std::array<float, 2> HeartbeatBeatTimes = {0.30f, 0.60f};
    }

    bool DamagePlayer(GameplayState& state, entt::registry& registry, Audio::AudioEngine& audio,
                      const PlayerDamage& damage)
    {
        Health& health = registry.get<Health>(state.player);
        if (health.current <= 0.0f || damage.amount <= 0.0f)
            return false;

        // An invulnerable player (debug) feels the blow like any other, but loses nothing.
        Armor* armor = registry.try_get<Armor>(state.player);
        bool isKilled = false;
        if (!state.isPlayerInvulnerable)
            isKilled = armor != nullptr ? ApplyDamage(health, *armor, damage.amount) : ApplyDamage(health, damage.amount);

        // The blow is heard whatever it did; the voice either cries out or dies.
        PlayerSounds& sounds = state.playerSounds;
        audio.Play(sounds.hits[static_cast<std::size_t>(damage.kind)]);
        audio.Play(isKilled ? sounds.death : sounds.hurt);

        DamageReaction* reaction = registry.try_get<DamageReaction>(state.player);
        if (reaction == nullptr)
            return isKilled;

        ++reaction->damageCount;
        reaction->lastDamage = damage.amount;

        // The direction of the blow along the ground, from the player to where it came from.
        reaction->lastDamageDirection.reset();
        if (damage.sourcePosition.has_value())
        {
            glm::vec3 direction = *damage.sourcePosition - registry.get<Core::Transform>(state.player).position;
            direction.y = 0.0f;
            if (glm::length(direction) > 0.001f)
                reaction->lastDamageDirection = glm::normalize(direction);
        }

        // The punch: a kick in a random direction, stronger for a bigger blow. Every axis gets its own random share, so
        // no two blows look the same.
        const float strength = glm::clamp(damage.amount / ReferencePunchDamage, WeakestPunch, StrongestPunch);
        const float kick = reaction->punchKick * strength;
        Core::Random& random = reaction->random;
        reaction->punchPitchVelocity += kick * random.GetFloat(-1.0f, 1.0f);
        reaction->punchYawVelocity += kick * random.GetFloat(-1.0f, 1.0f);
        reaction->punchRollVelocity += kick * PunchRollShare * random.GetFloat(-1.0f, 1.0f);

        // A strong blow muffles the world; a weaker one a little.
        reaction->muffle = std::max(reaction->muffle, std::min(1.0f, damage.amount / reaction->muffleFullDamage));

        return isKilled;
    }

    void HealPlayer(GameplayState& state, entt::registry& registry, Audio::AudioEngine& audio, float amount)
    {
        Health& health = registry.get<Health>(state.player);
        if (health.current <= 0.0f || health.current >= health.maximum || amount <= 0.0f)
            return;

        health.current = std::min(health.maximum, health.current + amount);
        audio.Play(state.playerSounds.relief);

        if (DamageReaction* reaction = registry.try_get<DamageReaction>(state.player); reaction != nullptr)
            ++reaction->healCount;
    }

    void UpdateDamageReaction(GameplayState& state, entt::registry& registry, Audio::AudioEngine& audio, float deltaTime)
    {
        DamageReaction* reaction = registry.try_get<DamageReaction>(state.player);
        if (reaction == nullptr)
            return;

        Core::UpdateDampedSpring(reaction->punchPitch, reaction->punchPitchVelocity, reaction->punchStiffness, deltaTime);
        Core::UpdateDampedSpring(reaction->punchYaw, reaction->punchYawVelocity, reaction->punchStiffness, deltaTime);
        Core::UpdateDampedSpring(reaction->punchRoll, reaction->punchRollVelocity, reaction->punchStiffness, deltaTime);

        // The muffle clears up evenly.
        if (reaction->muffleDuration > 0.0f)
            reaction->muffle = std::max(0.0f, reaction->muffle - deltaTime / reaction->muffleDuration);
        else
            reaction->muffle = 0.0f;
        audio.SetMuffle(reaction->muffle);

        // The heartbeat: only while alive and low. The first sound comes at once, the next ones every interval; when the
        // health is restored, the next time it drops low the heart starts at once again.
        const Health& health = registry.get<Health>(state.player);
        const bool isHeartAudible = health.current > 0.0f && health.current <= reaction->lowHealth;
        if (!isHeartAudible)
        {
            reaction->secondsToHeartbeat = 0.0f;
            reaction->secondsSinceHeartbeat = -1.0f;
            return;
        }

        // The beats of the sound playing now are counted when their moment in the recording passes, for the pulse of the
        // HUD: a beat at time t counts in the frame that goes from before t to t or after.
        if (reaction->secondsSinceHeartbeat >= 0.0f)
        {
            const float previous = reaction->secondsSinceHeartbeat;
            reaction->secondsSinceHeartbeat += deltaTime;
            for (const float beatTime : HeartbeatBeatTimes)
                if (previous < beatTime && reaction->secondsSinceHeartbeat >= beatTime)
                    ++reaction->heartbeatCount;
        }

        reaction->secondsToHeartbeat -= deltaTime;
        if (reaction->secondsToHeartbeat <= 0.0f)
        {
            audio.Play(state.playerSounds.heartbeat);
            reaction->secondsSinceHeartbeat = 0.0f;
            reaction->secondsToHeartbeat = std::max(reaction->secondsToHeartbeat + HeartbeatInterval, 0.0f);
        }
    }
}
