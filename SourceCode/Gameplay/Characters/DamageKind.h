#pragma once

#include <cstddef>

namespace Abomination::Gameplay
{
    // What hurt a character: it decides the sound of the blow (a strike, later a bite, a bullet, an explosion).
    enum class DamageKind
    {
        Melee,
        Count,
    };

    inline constexpr std::size_t DamageKindCount = static_cast<std::size_t>(DamageKind::Count);
}
