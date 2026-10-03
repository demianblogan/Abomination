#pragma once

#include "Gameplay/Characters/Health.h"

namespace Abomination::Gameplay
{
    // Component: armor that takes a part of the damage before the health does, until it is used up. The player starts
    // without armor and picks it up (0.6); the HUD shows it only while there is some.
    struct Armor
    {
        float current = 0.0f;
        float maximum = 100.0f;

        // The part of every hit the armor takes, as long as it has enough; the health takes the rest. Two thirds, between
        // the green (30%) and the red (80%) armor of Quake.
        float absorption = 2.0f / 3.0f;
    };

    // Damage to an entity with armor: the armor takes its part (as much as it still has), the health the rest (see the
    // ApplyDamage of Health). Returns whether this damage killed the entity.
    //
    // Example: 30 damage with 50 armor: the armor takes 20 (two thirds) and has 30 left, the health loses 10. With only
    // 5 armor left, it takes those 5 and the health loses 25.
    [[nodiscard]] inline bool ApplyDamage(Health& health, Armor& armor, float damage)
    {
        float armorDamage = damage * armor.absorption;
        if (armorDamage > armor.current)
            armorDamage = armor.current;

        armor.current -= armorDamage;
        return ApplyDamage(health, damage - armorDamage);
    }
}
