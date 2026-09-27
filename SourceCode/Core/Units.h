#pragma once

#include <concepts>

// Map units and meters. Maps are built in the units of Quake and TrenchBroom; the game works in meters. Most movement
// values of the game come from Quake, where they are given in units (a jump starts at 270 units/s), so they are written
// as MapUnitsToMeters(270.0f): the original number stays visible and can be compared with the Quake source.
namespace Abomination::Core
{
    // 32 units are about 1 meter: the Quake player is 56 units tall (about 1.75 m), and the grid sizes of TrenchBroom
    // (8, 16, 32, 64) are based on this scale.
    inline constexpr double UnitsPerMeter = 32.0;

    // A length in map units in meters; also works for speeds (units/s -> m/s) and accelerations. For float and double.
    template <std::floating_point Number>
    [[nodiscard]] constexpr Number MapUnitsToMeters(Number units) noexcept
    {
        return units / static_cast<Number>(UnitsPerMeter);
    }

    // The other way: meters in map units (to show a value in the units of Quake).
    template <std::floating_point Number>
    [[nodiscard]] constexpr Number MetersToMapUnits(Number meters) noexcept
    {
        return meters * static_cast<Number>(UnitsPerMeter);
    }
}
