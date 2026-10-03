#include "Gameplay/GameplayState.h"

#include "Audio/AudioEngine.h"
#include "Gameplay/Characters/Armor.h"
#include "Gameplay/Characters/Health.h"
#include "Gameplay/Player/LandingDip.h"
#include "Gameplay/Player/Player.h"
#include "Gameplay/Weapons/Ammo.h"
#include "Gameplay/Weapons/ViewRecoil.h"
#include "Gameplay/Weapons/Weapon.h"
#include "Gameplay/Weapons/WeaponViewModel.h"
#include "Renderer/Assets/RenderAssets.h"
#include "World/PlayerStart.h"

#include <glm/vec3.hpp>

namespace Abomination::Gameplay
{
    namespace
    {
        constexpr int StartingShells = 100;
    }

    GameplayState CreateGameplayState(entt::registry& registry, const World::PlayerStart& playerStart,
                                      Audio::AudioEngine& audio, Renderer::RenderAssets& renderAssets)
    {
        GameplayState state;
        state.player = SpawnPlayer(registry, playerStart);

        const glm::vec3 eyePosition = playerStart.boxCenter + glm::vec3(0.0f, PlayerEyeHeight, 0.0f);
        state.freeFlyCamera = SpawnFreeFlyCamera(registry, eyePosition, playerStart.yaw);

        // A jump itself makes no sound: the feet only push off. The jump will be the voice of the player (a short effort
        // sound), recorded separately; until then the jump event has no variants and plays nothing.
        // One landing at a time: a new one cuts the old one off.
        state.playerSounds.land = audio.LoadSoundEvent("Sounds/Player/Land", 3, Core::AssetLifetime::Global);
        state.playerSounds.land.maxVoices = 1;

        // The shotgun in the hands, for the whole game (the pickups and weapon switching of later versions will change
        // which model it is).
        const Renderer::ModelHandle shotgun = renderAssets.LoadModel("Models/Weapons/Shotgun.glb", Core::AssetLifetime::Global);
        registry.emplace<WeaponViewModel>(state.player, WeaponViewModel{
            .model = shotgun,
            .shaderProgram = renderAssets.shaders.Load("Shaders/TexturedShaded"),

            // The model points along -Z, so its front is the end of the barrel.
            .muzzle = renderAssets.models.Get(shotgun).front,
        });

        state.effects.textures = LoadEffectTextures(renderAssets.textures);

        registry.emplace<LandingDip>(state.player);
        registry.emplace<ViewRecoil>(state.player);

        // The player starts with full health, no armor and 100 shells for the shotgun (pickups add more in 0.6).
        registry.emplace<Health>(state.player);
        registry.emplace<Armor>(state.player);
        Ammo& ammo = registry.emplace<Ammo>(state.player);
        AddAmmo(ammo, AmmoType::Shells, StartingShells);

        // The shotgun itself: what it shoots and how it sounds. One recording, its pitch changed by up to 5% every shot.
        Weapon& weapon = registry.emplace<Weapon>(state.player);
        weapon.fireSound = audio.LoadSoundEvent("Sounds/Weapons/Shotgun/Fire", 1, Core::AssetLifetime::Global);
        weapon.fireSound.pitchVariation = 0.05f;
        weapon.fireSound.maxVoices = 2;

        // The click of the trigger when the shells run out.
        weapon.emptySound = audio.LoadSoundEvent("Sounds/Weapons/Shotgun/DryFire", 1, Core::AssetLifetime::Global);
        weapon.emptySound.maxVoices = 1;

        // The confirmation that a shot hurt or killed something (temporary sounds from Kenney's Impact Sounds).
        weapon.hitSound = audio.LoadSoundEvent("Sounds/Weapons/Hit", 3, Core::AssetLifetime::Global);
        weapon.hitSound.maxVoices = 2;
        weapon.killSound = audio.LoadSoundEvent("Sounds/Weapons/Kill", 3, Core::AssetLifetime::Global);
        weapon.killSound.maxVoices = 2;

        return state;
    }
}
