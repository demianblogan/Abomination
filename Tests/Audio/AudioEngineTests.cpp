#include "Audio/AudioEngine.h"

#include <gtest/gtest.h>

#include <filesystem>

namespace Abomination::Audio
{
    // The engine opens the sound card if there is one; without one (a build server) it runs silently, and everything
    // tested here works the same way. The sound files do not exist: every variant becomes the fallback beep.
    namespace
    {
        std::filesystem::path GetEmptyAssetsDirectory()
        {
            return std::filesystem::temp_directory_path() / "AbominationAudioEngineTest";
        }
    }

    TEST(AudioEngine, SameEventNameGivesSameHandleAndKeepsItsValues)
    {
        AudioEngine audio(GetEmptyAssetsDirectory());
        const SoundEventHandle first = audio.LoadSoundEvent("Sounds/Test/Step", 2, SoundGroup::Effects,
                                                            Core::AssetLifetime::Global);
        ASSERT_NE(audio.GetSoundEvent(first), nullptr);
        EXPECT_EQ(audio.GetSoundEvent(first)->variants.size(), 2u);

        // A volume tuned once (in the Audio window) stays when the same event is asked for again.
        audio.GetSoundEvent(first)->volume = 0.4f;
        const SoundEventHandle second = audio.LoadSoundEvent("Sounds/Test/Step", 2, SoundGroup::Effects,
                                                             Core::AssetLifetime::Global);
        EXPECT_EQ(first, second);
        EXPECT_FLOAT_EQ(audio.GetSoundEvent(second)->volume, 0.4f);
    }

    TEST(AudioEngine, EventsAreListedWithTheirGroup)
    {
        AudioEngine audio(GetEmptyAssetsDirectory());
        const SoundEventHandle voice = audio.LoadSoundEvent("Sounds/Test/Hurt", 1, SoundGroup::Voice,
                                                            Core::AssetLifetime::Global);

        const auto events = audio.ListSoundEvents();
        ASSERT_EQ(events.size(), 1u);
        EXPECT_EQ(events[0].first, "Sounds/Test/Hurt");
        EXPECT_EQ(events[0].second, voice);
        EXPECT_EQ(audio.GetSoundEvent(voice)->group, SoundGroup::Voice);
    }

    TEST(AudioEngine, EventsOfALevelAreRemovedWithItsSounds)
    {
        AudioEngine audio(GetEmptyAssetsDirectory());
        const SoundEventHandle global = audio.LoadSoundEvent("Sounds/Test/Global", 1, SoundGroup::Effects,
                                                             Core::AssetLifetime::Global);
        const SoundEventHandle level = audio.LoadSoundEvent("Sounds/Test/Level", 1, SoundGroup::Effects,
                                                            Core::AssetLifetime::Level);

        audio.RemoveSounds(Core::AssetLifetime::Level);

        EXPECT_NE(audio.GetSoundEvent(global), nullptr);
        EXPECT_EQ(audio.GetSoundEvent(level), nullptr);
    }

    TEST(AudioEngine, InvalidEventPlaysNothing)
    {
        AudioEngine audio(GetEmptyAssetsDirectory());
        EXPECT_EQ(audio.Play(SoundEventHandle{}).startOrder, 0u);
    }

    TEST(AudioEngine, GroupVolumeStaysBetweenZeroAndOne)
    {
        AudioEngine audio(GetEmptyAssetsDirectory());
        audio.SetGroupVolume(SoundGroup::Voice, 0.5f);
        EXPECT_FLOAT_EQ(audio.GetGroupVolume(SoundGroup::Voice), 0.5f);
        EXPECT_FLOAT_EQ(audio.GetGroupVolume(SoundGroup::Effects), 1.0f);

        audio.SetGroupVolume(SoundGroup::Music, 3.0f);
        EXPECT_FLOAT_EQ(audio.GetGroupVolume(SoundGroup::Music), 1.0f);
    }
}
