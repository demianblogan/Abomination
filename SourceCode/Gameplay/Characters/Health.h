#pragma once

namespace Abomination::Gameplay
{
    // Component: how much damage an entity can still take. Pellets hurt every entity that has it; at 0 it is dead.
    struct Health
    {
        float current = 100.0f;
        float maximum = 100.0f;

        // The damage it took beyond death: what the killing blow had left over, and every blow after it. A body bursts
        // into gibs once this is large enough (see GibSettings::burstDamage), like a monster below -40 health in Quake.
        float overkill = 0.0f;
    };

    // Takes damage from the health (never below 0; what is left over goes to overkill) and tells whether this damage
    // killed the entity: it was alive before and is at 0 now. Damage to a dead entity kills nothing, it only adds to
    // overkill.
    [[nodiscard]] inline bool ApplyDamage(Health& health, float damage)
    {
        if (health.current <= 0.0f)
        {
            health.overkill += damage;
            return false;
        }

        if (health.current > damage)
        {
            health.current -= damage;
            return false;
        }

        health.overkill += damage - health.current;
        health.current = 0.0f;
        return true;
    }
}
