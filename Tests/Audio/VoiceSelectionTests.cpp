#include "Audio/VoiceSelection.h"

#include <gtest/gtest.h>

#include <array>

namespace Abomination::Audio
{
    namespace
    {
        constexpr SoundHandle Shot{.index = 1, .generation = 1};
        constexpr SoundHandle Jump{.index = 2, .generation = 1};
    }

    TEST(VoiceSelection, TakesFirstFreeVoice)
    {
        const std::array<VoiceInfo, 3> voices = {{
            {.isPlaying = true, .eventKey = Jump, .startOrder = 1},
            {.isPlaying = false},
            {.isPlaying = false},
        }};

        EXPECT_EQ(ChooseVoice(voices, Shot, 4), 1u);
    }

    TEST(VoiceSelection, EventAtItsLimitReplacesItsOldestCopy)
    {
        // Two shots already play and the limit is 2: the older shot (start order 3) makes room, although voice 3 is free.
        const std::array<VoiceInfo, 4> voices = {{
            {.isPlaying = true, .eventKey = Jump, .startOrder = 1},
            {.isPlaying = true, .eventKey = Shot, .startOrder = 5},
            {.isPlaying = true, .eventKey = Shot, .startOrder = 3},
            {.isPlaying = false},
        }};

        EXPECT_EQ(ChooseVoice(voices, Shot, 2), 2u);
    }

    TEST(VoiceSelection, AllBusyReplacesOldestVoiceOfAll)
    {
        const std::array<VoiceInfo, 3> voices = {{
            {.isPlaying = true, .eventKey = Jump, .startOrder = 7},
            {.isPlaying = true, .eventKey = Jump, .startOrder = 2},
            {.isPlaying = true, .eventKey = Shot, .startOrder = 9},
        }};

        EXPECT_EQ(ChooseVoice(voices, Shot, 4), 1u);
    }

    TEST(VoiceSelection, OldestIsFoundWhenFirstVoicesAreFreeLater)
    {
        // All voices busy, the oldest one is not the first.
        const std::array<VoiceInfo, 3> voices = {{
            {.isPlaying = true, .eventKey = Jump, .startOrder = 8},
            {.isPlaying = true, .eventKey = Jump, .startOrder = 6},
            {.isPlaying = true, .eventKey = Jump, .startOrder = 4},
        }};

        EXPECT_EQ(ChooseVoice(voices, Shot, 1), 2u);
    }
}
