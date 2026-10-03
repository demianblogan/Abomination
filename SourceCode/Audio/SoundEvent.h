#pragma once

#include "Audio/SoundGroup.h"
#include "Audio/SoundStore.h"
#include "Core/Assets/AssetHandle.h"

#include <vector>

namespace Abomination::Audio
{
    // Something the game plays as a sound: "the player jumped", "a shotgun shot". It says which sound files may be played
    // and how, so every play sounds a little different.
    //
    // The problem it solves: the same file played ten times in a row sounds like a machine; the ear hears the repetition
    // at once. Real sounds never repeat exactly. So an event has a few variants (Jump1, Jump2, ...) and one of them is
    // chosen at random, and its pitch is changed a little at random.
    //
    // The events are kept by the AudioEngine, which hands out handles (SoundEventHandle): whoever plays a sound keeps only
    // the handle, so a volume tuned in the Audio window of the debug overlay changes the sound everywhere at once.
    struct SoundEvent
    {
        // The sounds to choose from, at random; at least one. The first one also identifies the event for the voice limit
        // (see ChooseVoice).
        std::vector<SoundHandle> variants;

        // The group whose volume it follows (see SoundGroup).
        SoundGroup group = SoundGroup::Effects;

        // 1 plays the sound as recorded, 0.5 at half the volume.
        float volume = 1.0f;

        // The pitch changes at random by up to this part: 0.05 plays the sound between 5% lower and 5% higher (and
        // shorter or longer by as much, like a record played a bit faster or slower).
        float pitchVariation = 0.05f;

        // At most this many copies of the event play at once (see ChooseVoice).
        int maxVoices = 4;
    };

    using SoundEventHandle = Core::AssetHandle<SoundEvent>;
}
