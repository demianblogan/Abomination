#include "Gameplay/GameplayState.h"

#include "Audio/AudioEngine.h"
#include "Gameplay/Characters/Armor.h"
#include "Gameplay/Characters/Health.h"
#include "Gameplay/Player/DamageReaction.h"
#include "Gameplay/Player/LandingDip.h"
#include "Gameplay/Player/Player.h"
#include "Gameplay/Weapons/Ammo.h"
#include "Gameplay/Weapons/ViewRecoil.h"
#include "Gameplay/Weapons/Weapon.h"
#include "Gameplay/Weapons/WeaponViewModel.h"
#include "Renderer/Assets/RenderAssets.h"
#include "World/PlayerStart.h"

#include <glm/vec3.hpp>

#include <cstddef>
#include <string>

namespace Abomination::Gameplay
{
    namespace
    {
        constexpr int StartingShells = 100;

        // Loads a sound event for the whole game (see AudioEngine::LoadSoundEvent) and sets how many copies of it may play at
        // once. The volumes are tuned in the Audio window of the debug overlay.
        Audio::SoundEventHandle LoadEvent(Audio::AudioEngine& audio, const std::string& path, int variantCount,
                                          Audio::SoundGroup group, int maxVoices)
        {
            const Audio::SoundEventHandle event =
                audio.LoadSoundEvent(path, variantCount, group, Core::AssetLifetime::Global);
            audio.GetSoundEvent(event)->maxVoices = maxVoices;
            return event;
        }
    }

    GameplayState CreateGameplayState(entt::registry& registry, const World::PlayerStart& playerStart,
                                      Audio::AudioEngine& audio, Renderer::RenderAssets& renderAssets)
    {
        GameplayState state;
        state.player = SpawnPlayer(registry, playerStart);

        const glm::vec3 eyePosition = playerStart.boxCenter + glm::vec3(0.0f, PlayerEyeHeight, 0.0f);
        state.freeFlyCamera = SpawnFreeFlyCamera(registry, eyePosition, playerStart.yaw);

        // The feet: one landing at a time, a new one cuts the old one off. The voice of the player (a jump is a short
        // effort sound, the feet only push off): one cry at a time. The blows and the heart are heard in the body.
        PlayerSounds& sounds = state.playerSounds;
        sounds.land = LoadEvent(audio, "Sounds/Player/Land", 3, Audio::SoundGroup::Effects, 1);
        sounds.jump = LoadEvent(audio, "Sounds/Player/Voice/Jump", 3, Audio::SoundGroup::Voice, 1);
        sounds.hurt = LoadEvent(audio, "Sounds/Player/Voice/Hurt", 3, Audio::SoundGroup::Voice, 1);
        sounds.death = LoadEvent(audio, "Sounds/Player/Voice/Death", 3, Audio::SoundGroup::Voice, 1);
        sounds.relief = LoadEvent(audio, "Sounds/Player/Voice/Relief", 3, Audio::SoundGroup::Voice, 1);
        sounds.hits[static_cast<std::size_t>(DamageKind::Melee)] =
            LoadEvent(audio, "Sounds/Player/HitMelee", 1, Audio::SoundGroup::Effects, 2);
        sounds.heartbeat = LoadEvent(audio, "Sounds/Player/Heartbeat", 1, Audio::SoundGroup::Effects, 2);

        // A heart always beats at the same pitch: a changing one does not sound like a heart.
        audio.GetSoundEvent(sounds.heartbeat)->pitchVariation = 0.0f;


        // The shotgun in the hands, for the whole game (the pickups and weapon switching of later versions will change
        // which model it is).
        const Renderer::ModelHandle shotgun = renderAssets.LoadModel("Models/Weapons/Shotgun.glb", Core::AssetLifetime::Global);
        registry.emplace<WeaponViewModel>(state.player, WeaponViewModel{
            .model = shotgun,
            .shaderProgram = renderAssets.shaders.Load("Shaders/TexturedShaded"),

            // The model points along -Z, so its front is the end of the barrel.
            .muzzle = renderAssets.models.Get(shotgun).front,

            // The pump and the slide behind it move together when the pump is worked; the weapon is turned to the chest
            // around a point along its length.
            .pump = {
                .partNames = {"Pump_low_Shotgun_0", "Slide_low_Shotgun_0"},
                .modelLength = renderAssets.models.Get(shotgun).size.z,
            },
        });

        state.effects.textures = LoadEffectTextures(renderAssets.textures);

        registry.emplace<LandingDip>(state.player);
        registry.emplace<DamageReaction>(state.player);
        registry.emplace<ViewRecoil>(state.player);

        // The player starts with full health, no armor and 100 shells for the shotgun (pickups add more in 0.6).
        registry.emplace<Health>(state.player);
        registry.emplace<Armor>(state.player);
        Ammo& ammo = registry.emplace<Ammo>(state.player);
        AddAmmo(ammo, AmmoType::Shells, StartingShells);

        // The shotgun itself: what it shoots and how it sounds. One recording, its pitch changed by up to 5% every shot.
        Weapon& weapon = registry.emplace<Weapon>(state.player);
        weapon.fireSound = LoadEvent(audio, "Sounds/Weapons/Shotgun/Fire", 1, Audio::SoundGroup::Effects, 2);

        // The click of the trigger when the shells run out.
        weapon.emptySound = LoadEvent(audio, "Sounds/Weapons/Shotgun/DryFire", 1, Audio::SoundGroup::Effects, 1);

        // The pump after a shot: one recording of both clacks, back and forward (see PumpSoundBackClackTime).
        weapon.pumpSound = LoadEvent(audio, "Sounds/Weapons/Shotgun/Pump", 1, Audio::SoundGroup::Effects, 1);

        // The confirmation that a shot hurt or killed something (temporary sounds from Kenney's Impact Sounds).
        weapon.hitSound = LoadEvent(audio, "Sounds/Weapons/Hit", 3, Audio::SoundGroup::Effects, 2);
        weapon.killSound = LoadEvent(audio, "Sounds/Weapons/Kill", 3, Audio::SoundGroup::Effects, 2);

        return state;
    }
}
