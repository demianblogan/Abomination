#pragma once

#include "Audio/SoundStore.h"

#include <cstddef>
#include <cstdint>
#include <span>

namespace Abomination::Audio
{
    // What the choice of a voice needs to know about one voice (see ChooseVoice).
    struct VoiceInfo
    {
        // The voice is playing a sound right now.
        bool isPlaying = false;

        // The sound event it plays, identified by the first variant of the event (see SoundEvent): all variants of the
        // same event count together.
        SoundHandle eventKey;

        // When the voice was started: a number that grows with every started voice, so a smaller number is older.
        std::uint64_t startOrder = 0;
    };

    // Chooses the voice that plays a new sound of the event eventKey. A voice is one sound playing; the engine has a
    // fixed number of them, like the channels of a mixing desk.
    //
    // The problem it solves: 20 pellets hitting a wall at once would play 20 copies of the same sound. They add up to
    // a loud mess, take every voice, and a new important sound (an enemy behind the player) would find none free.
    // So an event plays at most maxEventVoices copies at once, and the oldest copy makes room for a new one: the start
    // of a sound is what the ear notices, the end of an old copy is missed by nobody.
    //
    //   1. The event already plays maxEventVoices copies: its oldest copy is replaced.
    //   2. Otherwise a free voice (one that is not playing).
    //   3. Otherwise every voice is busy: the oldest voice of all is replaced.
    //
    // voices must not be empty; maxEventVoices must be at least 1.
    [[nodiscard]] std::size_t ChooseVoice(std::span<const VoiceInfo> voices, SoundHandle eventKey, int maxEventVoices);
}
