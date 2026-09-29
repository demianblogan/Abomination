#include "Gameplay/GameplayState.h"

#include "Audio/AudioEngine.h"
#include "Gameplay/Player.h"
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
                                      Audio::AudioEngine& audio)
    {
        GameplayState state;
        state.player = SpawnPlayer(registry, playerStart);

        const glm::vec3 eyePosition = playerStart.boxCenter + glm::vec3(0.0f, PlayerEyeHeight, 0.0f);
        state.freeFlyCamera = SpawnFreeFlyCamera(registry, eyePosition, playerStart.yaw);

        state.playerSounds.jump = LoadPlayerSoundEvent(audio, "Jump", 3);
        state.playerSounds.jump.volume = 0.5f;
        state.playerSounds.land = LoadPlayerSoundEvent(audio, "Land", 3);

        return state;
    }
}
