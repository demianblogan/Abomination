#pragma once

#include "Core/Math/Random.h"
#include "Gameplay/Characters/DamageKind.h"
#include "Gameplay/GameplayState.h"

#include <entt/entt.hpp>
#include <glm/vec3.hpp>

#include <optional>

namespace Abomination::Audio
{
    class AudioEngine;
}

namespace Abomination::Gameplay
{
    // One blow to the player.
    struct PlayerDamage
    {
        float amount = 0.0f;
        DamageKind kind = DamageKind::Melee;

        // Where it came from (game meters), for the indicator of the HUD that shows its direction; nothing if it has no
        // direction (falling, poison).
        std::optional<glm::vec3> sourcePosition;
    };

    // Component of the player: how they feel damage and healing, beyond the numbers.
    //   - The view punch: a blow jerks the picture in a random direction (pitch, yaw and a tilt) and springs back. Only
    //     the picture moves, the aim stays, like the recoil of a shot (see ViewRecoil).
    //   - The muffle: after a strong blow the world sounds muffled for a moment, as if the ears were ringing (see
    //     Audio::AudioEngine::SetMuffle).
    //   - The heartbeat: at low health the heart beats audibly, and the HUD pulses with it.
    //   - The counts and the direction of the last blow, for the HUD (the shake, the vignette, the direction indicator).
    struct DamageReaction
    {
        // How hard a blow of 20 damage jerks the view (radians per second of its own motion; weaker and stronger blows
        // in proportion, between half and one and a half of it) and how stiff the springs are that bring it back.
        float punchKick = 1.2f;
        float punchStiffness = 180.0f;

        // How far the view is turned by the punch now (radians) and how fast it turns: up, to the left, tilted.
        float punchPitch = 0.0f;
        float punchPitchVelocity = 0.0f;
        float punchYaw = 0.0f;
        float punchYawVelocity = 0.0f;
        float punchRoll = 0.0f;
        float punchRollVelocity = 0.0f;

        // A blow of muffleFullDamage or more muffles the world fully; it clears up over muffleDuration seconds.
        float muffleFullDamage = 30.0f;
        float muffleDuration = 0.6f;
        float muffle = 0.0f;

        // At lowHealth or less the heart is heard once a second: one recording of two beats ("lub-dub",
        // Sounds/Player/Heartbeat1.ogg). The seconds until the next sound, and since the last one started (negative: none).
        float lowHealth = 25.0f;
        float secondsToHeartbeat = 0.0f;
        float secondsSinceHeartbeat = -1.0f;

        // How many blows, heals and single beats of the heart (two per heartbeat sound) so far; the HUD pulses with every
        // beat. They only grow: the HUD remembers the numbers it has seen and
        // reacts when one grows (like the hit markers and Weapon::hitCount).
        int damageCount = 0;
        int healCount = 0;
        int heartbeatCount = 0;

        // The last blow: how much damage it did and where it came from (a direction in the world, from the player to
        // the source, along the ground), if it had a source.
        float lastDamage = 0.0f;
        std::optional<glm::vec3> lastDamageDirection;

        // Chooses the direction of the punch.
        Core::Random random;
    };

    // Hurts the player, through their armor (see ApplyDamage). If they survive, the view punches, their voice cries
    // out, the blow is heard and may muffle the world; if it kills them, their death cry is heard instead. A dead player
    // is not hurt again. Returns whether this blow killed them.
    bool DamagePlayer(GameplayState& state, entt::registry& registry, Audio::AudioEngine& audio,
                      const PlayerDamage& damage);

    // Heals the player by amount (never above their maximum health), with a sigh of relief. A dead player is not
    // healed; healing a player at full health does nothing.
    void HealPlayer(GameplayState& state, entt::registry& registry, Audio::AudioEngine& audio, float amount);

    // Once per frame: the punch springs back, the muffle clears up, and at low health the heart beats.
    void UpdateDamageReaction(GameplayState& state, entt::registry& registry, Audio::AudioEngine& audio, float deltaTime);
}
