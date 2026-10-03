#pragma once

#include <array>
#include <cstddef>
#include <string_view>

namespace Abomination::Audio
{
    // The groups every sound belongs to, each with its own volume, so the player can turn one of them down (the options
    // menu, 0.8) without touching the others: the volume a sound plays at is master x group x its own volume.
    enum class SoundGroup
    {
        Effects, // Shots, hits, steps, the world.
        Voice,   // The voice of the player, later of the enemies.
        Music,
        Count,
    };

    inline constexpr std::size_t SoundGroupCount = static_cast<std::size_t>(SoundGroup::Count);

    // Names for the debug overlay, in the order of the enum values.
    inline constexpr std::array<std::string_view, SoundGroupCount> SoundGroupNames = {"Effects", "Voice", "Music"};
}
