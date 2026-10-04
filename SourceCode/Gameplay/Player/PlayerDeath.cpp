#include "Gameplay/Player/PlayerDeath.h"

#include "Audio/AudioEngine.h"
#include "Core/Scene/Transform.h"
#include "Core/Scene/TransformInterpolation.h"
#include "Gameplay/Camera/MouseLook.h"
#include "Gameplay/Characters/Armor.h"
#include "Gameplay/Characters/Health.h"
#include "Gameplay/GameplayState.h"
#include "Gameplay/Player/DamageReaction.h"
#include "Gameplay/Player/LandingDip.h"
#include "Gameplay/Player/Player.h"
#include "Gameplay/Weapons/Ammo.h"
#include "Gameplay/Weapons/ViewRecoil.h"
#include "Gameplay/Weapons/Weapon.h"
#include "Physics/CharacterBody.h"
#include "World/PlayerStart.h"

#include <algorithm>

namespace Abomination::Gameplay
{
    namespace
    {
        // The part of the way from start to end that time has come: 0 before start, 1 after end.
        float Progress(float time, float start, float end)
        {
            return std::clamp((time - start) / std::max(end - start, 0.01f), 0.0f, 1.0f);
        }

        // Starting slowly and speeding up, like a fall: t².
        float EaseIn(float t)
        {
            return t * t;
        }

        // Starting fast and slowing down at the end: 1 - (1 - t)².
        float EaseOut(float t)
        {
            return 1.0f - (1.0f - t) * (1.0f - t);
        }

        // Slow at both ends: 3t² - 2t³ (smoothstep).
        float EaseInOut(float t)
        {
            return t * t * (3.0f - 2.0f * t);
        }
    }

    bool IsPlayerDead(const GameplayState& state) noexcept
    {
        return state.playerDeath.isDead;
    }

    void UpdatePlayerDeath(GameplayState& state, const entt::registry& registry, Audio::AudioEngine& audio, float deltaTime)
    {
        PlayerDeath& death = state.playerDeath;
        if (!death.isDead)
        {
            const Health* health = registry.try_get<Health>(state.player);
            if (health == nullptr || health->current > 0.0f)
                return;

            death.isDead = true;
            death.time = 0.0f;
            death.hasShownGameOver = false;
            return;
        }

        death.time += deltaTime;

        // GAME OVER appears with its sound, once.
        if (IsGameOverShown(death) && !death.hasShownGameOver)
        {
            death.hasShownGameOver = true;
            audio.Play(state.playerSounds.gameOver);
        }
    }

    DeathView CalculateDeathView(const PlayerDeath& death, float eyesAboveFloor, float lookPitch)
    {
        if (!death.isDead)
            return DeathView{.pitch = lookPitch};

        // The nod: the head drops from where it looked; the fall: from the bottom of the nod back to looking up, the
        // body going down faster and faster (a fall) and landing on its back.
        const PlayerDeathSettings& settings = death.settings;
        const float nod = EaseOut(Progress(death.time, 0.0f, settings.nodTime));
        const float fall = Progress(death.time, settings.nodTime, settings.fallEndTime);
        const float nodPitch = lookPitch - settings.nodPitch * nod;
        const float fallen = EaseIn(fall);

        return DeathView{
            .drop = std::max(eyesAboveFloor - settings.eyeHeight, 0.0f) * fallen,
            .back = settings.fallBack * fallen,
            .pitch = nodPitch + (settings.endPitch - nodPitch) * EaseInOut(fall),
            .roll = settings.roll * fallen,
        };
    }

    float CalculateWeaponLowering(const PlayerDeath& death)
    {
        return death.isDead ? EaseIn(Progress(death.time, 0.0f, death.settings.weaponLowerTime)) : 0.0f;
    }

    float CalculateHUDOpacity(const PlayerDeath& death)
    {
        return death.isDead ? 1.0f - Progress(death.time, 0.0f, death.settings.hudFadeTime) : 1.0f;
    }

    float CalculateDeathVignette(const PlayerDeath& death)
    {
        if (!death.isDead)
            return 0.0f;
        const PlayerDeathSettings& settings = death.settings;
        return settings.vignetteOpacity * EaseInOut(Progress(death.time, settings.vignetteStart, settings.fallEndTime));
    }

    float CalculateEyesClosed(const PlayerDeath& death)
    {
        if (!death.isDead)
            return 0.0f;
        const PlayerDeathSettings& settings = death.settings;
        return EaseIn(Progress(death.time, settings.eyesCloseStart, settings.eyesCloseStart + settings.eyesCloseTime));
    }

    bool IsGameOverShown(const PlayerDeath& death)
    {
        return death.isDead && death.time >= death.settings.eyesCloseStart + death.settings.eyesCloseTime;
    }

    bool CanRestartAfterDeath(const PlayerDeath& death)
    {
        const PlayerDeathSettings& settings = death.settings;
        return death.isDead && death.time >= settings.eyesCloseStart + settings.eyesCloseTime + settings.hintDelay;
    }

    void RespawnPlayer(GameplayState& state, entt::registry& registry, const World::PlayerStart& playerStart)
    {
        const entt::entity player = state.player;

        // Where the map puts them, standing still, looking along the start; the transform of the last tick too, so the
        // view does not glide from where they died.
        const Core::Transform start{.position = playerStart.boxCenter};
        registry.replace<Core::Transform>(player, start);
        registry.replace<Core::PreviousTransform>(player, Core::PreviousTransform{.value = start});
        registry.replace<Physics::CharacterBody>(player, Physics::CharacterBody{.halfExtents = World::PlayerHalfExtents});
        registry.replace<LookAngles>(player, LookAngles{.yaw = playerStart.yaw});
        registry.replace<StepSmoothing>(player);
        registry.replace<LandingDip>(player);

        // Full health, no armor, the shells of the start.
        Health& health = registry.get<Health>(player);
        health = Health{.current = health.maximum, .maximum = health.maximum};
        registry.replace<Armor>(player);
        Ammo& ammo = registry.get<Ammo>(player);
        ammo.counts.fill(0);
        AddAmmo(ammo, AmmoType::Shells, StartingShells);

        // What was moving in the view stops; the tuned values and the counters the HUD follows are kept.
        DamageReaction& reaction = registry.get<DamageReaction>(player);
        reaction.punchPitch = reaction.punchPitchVelocity = 0.0f;
        reaction.punchYaw = reaction.punchYawVelocity = 0.0f;
        reaction.punchRoll = reaction.punchRollVelocity = 0.0f;
        reaction.muffle = 0.0f;
        reaction.secondsSinceHeartbeat = -1.0f;
        reaction.lastDamageDirection.reset();
        ViewRecoil& recoil = registry.get<ViewRecoil>(player);
        recoil.pitch = recoil.pitchVelocity = 0.0f;
        Weapon& weapon = registry.get<Weapon>(player);
        weapon.cooldown = 0.0f;
        weapon.isFireRequested = false;

        state.playerDeath.isDead = false;
        state.playerDeath.time = 0.0f;
        state.playerDeath.hasShownGameOver = false;
    }
}
