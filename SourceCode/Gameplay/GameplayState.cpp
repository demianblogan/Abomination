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

#include <string>

namespace Abomination::Gameplay
{
    namespace
    {
        // Loads the variants Sounds/Player/<name>1.ogg ... <name><count>.ogg of one sound event, for the whole game.
        Audio::SoundEvent LoadPlayerSoundEvent(Audio::AudioEngine& audio, const std::string& name, int variantCount)
        {
            Audio::SoundEvent event;
            for (int variant = 1; variant <= variantCount; ++variant)
            {
                const std::string path = "Sounds/Player/" + name + std::to_string(variant) + ".ogg";
                event.variants.push_back(audio.LoadSound(path, Core::AssetLifetime::Global));
            }

            // One jump or landing at a time: a new one cuts the old one off.
            event.maxVoices = 1;

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

        // A jump itself makes no sound: the feet only push off. The jump will be the voice of the player (a short effort
        // sound), recorded separately; until then the jump event has no variants and plays nothing.
        state.playerSounds.land = LoadPlayerSoundEvent(audio, "Land", 3);

        // The shotgun in the hands, for the whole game (the pickups and weapon switching of later versions will change
        // which model it is).
        registry.emplace<ViewModel>(state.player, ViewModel{
            .model = renderAssets.LoadModel("Models/Weapons/Shotgun.glb", Core::AssetLifetime::Global),
            .shaderProgram = renderAssets.shaders.Load("Shaders/TexturedShaded"),
        });

        registry.emplace<LandingDip>(state.player);
        registry.emplace<ViewRecoil>(state.player);

        // The shotgun itself: what it shoots and how it sounds. One recording, its pitch changed a little every shot.
        Weapon& weapon = registry.emplace<Weapon>(state.player);
        weapon.fireSound = Audio::SoundEvent{
            .variants = {audio.LoadSound("Sounds/Weapons/Shotgun/Fire1.ogg", Core::AssetLifetime::Global)},
            .pitchVariation = 0.03f,
            .maxVoices = 2,
        };

        return state;
    }
}
