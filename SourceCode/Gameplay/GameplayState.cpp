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

#include <algorithm>
#include <cstddef>
#include <string>
#include <string_view>

namespace Abomination::Gameplay
{
    namespace
    {
        constexpr int StartingShells = 100;

        constexpr std::string_view ShotgunPath = "Models/Weapons/Shotgun.glb";
        constexpr std::string_view ShotgunBoltPartName = "Bolt";

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

        // The window of the shotgun that spent shells fly out of: on the right side of its body (the receiver), above the
        // loading gate. The model has no part for the window itself, so it is found from these two.
        glm::vec3 FindShellWindow(const Renderer::Model& model)
        {
            const auto findPart = [&model](std::string_view name) -> const Renderer::ModelPart*
            {
                const auto part = std::ranges::find(model.parts, name, &Renderer::ModelPart::name);
                return part != model.parts.end() ? &*part : nullptr;
            };

            const Renderer::ModelPart* body = findPart("Body_low_Shotgun_0");
            const Renderer::ModelPart* loader = findPart("Loader_low_Shotgun_0");
            if (body == nullptr || loader == nullptr)
                return glm::vec3(0.0f);

            return {body->center.x + body->size.x * 0.5f, body->center.y, loader->center.z};
        }
    }

    void PrepareGameplayModels(Renderer::RenderAssets& renderAssets)
    {
        // The bolt of the shotgun is a piece of its body: the curved light surface in the window on the right side
        // (+X), 7.7 cm long, inside a frame. The box and the texture rectangle were read from the model file (in the
        // coordinates of the body; the frame uses another part of the texture, so the rectangle leaves it in the body).
        // Behind the bolt stays a dark copy of it: the opening it uncovers when it slides back with the pump.
        renderAssets.models.SetPartSplits(std::string(ShotgunPath), {Renderer::ModelPartSplit{
            .sourcePartName = "Body_low_Shotgun_0",
            .partName = std::string(ShotgunBoltPartName),
            .boxMinimum = {0.0005f, 0.0098f, -0.0792f},
            .boxMaximum = {0.0089f, 0.0276f, -0.0017f},
            .texCoordMinimum = {0.285f, 0.19f},
            .texCoordMaximum = {0.33f, 0.31f},
            .backingPartName = "BoltOpening",
            .backingTexturePath = "Textures/Weapons/ShotgunOpening.png",
        }});
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
        const Renderer::ModelHandle shotgun = renderAssets.LoadModel(std::string(ShotgunPath), Core::AssetLifetime::Global);
        registry.emplace<WeaponViewModel>(state.player, WeaponViewModel{
            .model = shotgun,
            .shaderProgram = renderAssets.shaders.Load("Shaders/TexturedShaded"),

            // The model points along -Z, so its front is the end of the barrel.
            .muzzle = renderAssets.models.Get(shotgun).front,
            .shellWindow = FindShellWindow(renderAssets.models.Get(shotgun)),

            // The pump, the slide behind it and the bolt in the window (taken out of the body, see PrepareGameplayModels)
            // move together when the pump is worked; the weapon is turned to the chest around a point along its length.
            .pump = {
                .partNames = {"Pump_low_Shotgun_0", "Slide_low_Shotgun_0", std::string(ShotgunBoltPartName)},
                .modelLength = renderAssets.models.Get(shotgun).size.z,
            },
        });

        state.effects.textures = LoadEffectTextures(renderAssets.textures);
        state.shells = LoadShells(renderAssets, audio);

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
