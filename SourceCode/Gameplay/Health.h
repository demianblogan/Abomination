#pragma once

namespace Abomination::Gameplay
{
    // Component: how much damage an entity can still take. Pellets hurt every entity that has it; at 0 it is dead.
    struct Health
    {
        float current = 100.0f;
        float maximum = 100.0f;
    };

    // Takes damage from the health (never below 0) and tells whether this damage killed the entity: it was alive before
    // and is at 0 now. Damage to a dead entity kills nothing.
    [[nodiscard]] inline bool ApplyDamage(Health& health, float damage)
    {
        if (health.current <= 0.0f)
            return false;

        health.current = health.current > damage ? health.current - damage : 0.0f;

        return health.current <= 0.0f;
    }
}
