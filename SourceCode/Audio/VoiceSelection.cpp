#include "Audio/VoiceSelection.h"

#include <cassert>
#include <optional>

namespace Abomination::Audio
{
    std::size_t ChooseVoice(std::span<const VoiceInfo> voices, SoundHandle eventKey, int maxEventVoices)
    {
        assert(!voices.empty());
        assert(maxEventVoices >= 1);

        // One pass collects everything the three rules need.
        int eventVoiceCount = 0;
        std::optional<std::size_t> oldestEventVoice;
        std::optional<std::size_t> freeVoice;
        std::optional<std::size_t> oldestVoice;

        for (std::size_t index = 0; index < voices.size(); ++index)
        {
            const VoiceInfo& voice = voices[index];
            if (!voice.isPlaying)
            {
                if (!freeVoice.has_value())
                    freeVoice = index;
                continue;
            }

            if (!oldestVoice.has_value() || voice.startOrder < voices[*oldestVoice].startOrder)
                oldestVoice = index;

            if (voice.eventKey == eventKey)
            {
                ++eventVoiceCount;
                if (!oldestEventVoice.has_value() || voice.startOrder < voices[*oldestEventVoice].startOrder)
                    oldestEventVoice = index;
            }
        }

        if (eventVoiceCount >= maxEventVoices)
            return *oldestEventVoice;

        if (freeVoice.has_value())
            return *freeVoice;

        // No free voice, so every voice is playing and the oldest one exists.
        return *oldestVoice;
    }
}
