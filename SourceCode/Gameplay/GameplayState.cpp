#include "Gameplay/GameplayState.h"

#include "Audio/AudioEngine.h"
#include "Gameplay/LandingDip.h"
#include "Gameplay/Player.h"
#include "Gameplay/ViewModel.h"
#include "Gameplay/ViewRecoil.h"
#include "Gameplay/Weapon.h"
#include "Renderer/Assets/RenderAssets.h"
#include "World/PlayerStart.h"

#include <glm/vec3.hpp>

namespace Abomination::Gameplay
{
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
        registry.emplace<ViewModel>(state.player, ViewModel{
            .model = shotgun,
            .shaderProgram = renderAssets.shaders.Load("Shaders/TexturedShaded"),

            // The model points along -Z, so its front is the end of the barrel.
            .muzzle = renderAssets.models.Get(shotgun).front,
        });

        state.effects.textures = LoadEffectTextures(renderAssets.textures);

        registry.emplace<LandingDip>(state.player);
        registry.emplace<ViewRecoil>(state.player);

        // The shotgun itself: what it shoots and how it sounds. One recording, its pitch changed by up to 5% every shot.
        Weapon& weapon = registry.emplace<Weapon>(state.player);
        weapon.fireSound = audio.LoadSoundEvent("Sounds/Weapons/Shotgun/Fire", 1, Core::AssetLifetime::Global);
        weapon.fireSound.pitchVariation = 0.05f;
        weapon.fireSound.maxVoices = 2;

        // The confirmation that a shot hurt or killed something (temporary sounds from Kenney's Impact Sounds).
        weapon.hitSound = audio.LoadSoundEvent("Sounds/Weapons/Hit", 3, Core::AssetLifetime::Global);
        weapon.hitSound.maxVoices = 2;
        weapon.killSound = audio.LoadSoundEvent("Sounds/Weapons/Kill", 3, Core::AssetLifetime::Global);
        weapon.killSound.maxVoices = 2;

        return state;
    }
}
