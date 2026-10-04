#pragma once

#include "Core/Math/BoundingBox.h"
#include "Core/Math/Plane.h"
#include "World/MapData.h"

#include <vector>

namespace Abomination::World
{
    // A brush as collision sees it: a convex solid given only by its planes (no triangles), and the box around it.
    // A point is inside the brush when it is behind all planes. Everything is in game meters and axes, in doubles.
    struct CollisionBrush
    {
        // The planes of the faces (normals pointing out of the brush), followed by the bevel planes (see
        // BuildCollisionBrushes).
        std::vector<Core::Plane> planes;

        // The axis-aligned box around the brush: a quick test before the planes. Objects far from the box cannot touch
        // the brush, so its planes are not even looked at.
        Core::BoundingBox bounds;
    };

    // Which brushes BuildCollisionBrushes builds: all (what characters collide with), or all but clip (what shots and
    // sight are stopped by; see IsClipBrush).
    enum class BrushSelection
    {
        All,
        WithoutClip,
    };

    // Builds the collision brushes of the brushes of an entity (the world) that selection picks.
    //
    // Every brush also gets bevel planes: the sides of its bounding box (normals along +X, -X, +Y, -Y, +Z, -Z) that the
    // brush does not have as faces yet. They cut nothing off the brush itself, which is inside its box anyway. They are
    // needed when a box is traced against the brush (see TraceBox): the planes are moved out by the size of the box,
    // and at sharp corners moved planes meet far from the brush, so the brush would reach out too far; the moved bevel
    // planes cut that off.
    //
    // Faces without a plane (three points on one line) and faces that do not touch the brush are left out. A brush with
    // no faces left is left out completely.
    [[nodiscard]] std::vector<CollisionBrush> BuildCollisionBrushes(const MapEntity& entity, BrushSelection selection);

    // A brush in the shape of an axis-aligned box (game meters): its six planes and bounds. Characters collide with each
    // other through such brushes: the box of every other character is added to the brushes of the level while a
    // character moves, so the same traces stop it at walls and at other characters. A box needs no bevel planes: its
    // faces are the sides of its bounding box already.
    [[nodiscard]] CollisionBrush CreateBoxCollisionBrush(const glm::dvec3& center, const glm::dvec3& halfExtents);
}
