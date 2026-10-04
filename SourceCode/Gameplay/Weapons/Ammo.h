#pragma once

#include <array>
#include <cstddef>
#include <string_view>

namespace Abomination::Gameplay
{
    // The kinds of ammunition: every weapon uses one of them, and weapons that use the same kind share it, like in
    // Quake.
    enum class AmmoType
    {
        Shells,  // The shotgun and the double-barreled shotgun.
        Bullets, // The machine gun and the minigun.
        Rockets, // The rocket and the grenade launcher.
        Cells,   // The lightning gun.
        Count,
    };

    // The names of the icons of the HUD (Assets/UI/Icons/<name>.png), in the order of the enum values.
    inline constexpr std::array<std::string_view, 4> AmmoTypeNames = {"Shells", "Bullets", "Rockets", "Cells"};

    // Component of the player: how much of every kind of ammunition they carry, and the most they can carry.
    struct Ammo
    {
        std::array<int, 4> counts{};
        std::array<int, 4> maximums{100, 200, 100, 100};
    };

    [[nodiscard]] inline int GetAmmo(const Ammo& ammo, AmmoType type)
    {
        return ammo.counts[static_cast<std::size_t>(type)];
    }

    // Takes amount of the kind for a shot. Returns false (and takes nothing) if there is not enough.
    [[nodiscard]] inline bool TryUseAmmo(Ammo& ammo, AmmoType type, int amount)
    {
        int& count = ammo.counts[static_cast<std::size_t>(type)];
        if (count < amount)
            return false;

        count -= amount;
        return true;
    }

    // The shells the player starts with (pickups add more in 0.6).
    inline constexpr int StartingShells = 100;

    // Adds amount of the kind, never above its maximum.
    inline void AddAmmo(Ammo& ammo, AmmoType type, int amount)
    {
        const auto index = static_cast<std::size_t>(type);
        ammo.counts[index] = ammo.counts[index] + amount < ammo.maximums[index] ? ammo.counts[index] + amount
                                                                                 : ammo.maximums[index];
    }
}
