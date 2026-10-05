#pragma once

#include <entt/entt.hpp>

namespace Abomination::Navigation
{
    class NavMesh;
}

namespace Abomination::Renderer
{
    class DebugLines;
}

// The debug lines of the monsters: what they sense and the way they find (see Monsters.h), and the navmesh they find it
// on. Only for looking at their behavior; they change nothing.
namespace Abomination::Gameplay
{
    struct GameplayState;

    // Debug, with state.areDogSensesVisible: the senses of every dog as lines on the floor around it: its field of view
    // and the range of its sight (yellow), the radius it smells the player in (orange), the range it hears shots in
    // (blue), the area it patrols around where it appeared (green), and its path around walls (white).
    // interpolationFactor places it as it is drawn.
    void AddMonsterDebugLines(const GameplayState& state, const entt::registry& registry, float interpolationFactor,
                              Renderer::DebugLines& lines);

    // Debug, with state.isNavMeshVisible: the polygons of the navmesh as lines a little above the floor (none if the level
    // has no navmesh).
    void AddNavMeshDebugLines(const GameplayState& state, const Navigation::NavMesh* navMesh, Renderer::DebugLines& lines);
}
