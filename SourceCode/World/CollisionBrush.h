#pragma once

#include "Core/BoundingBox.h"
#include "Core/Plane.h"
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

    // Builds the collision brushes of all brushes of an entity (the world).
    //
    // Every brush also gets bevel planes: the sides of its bounding box (normals along +X, -X, +Y, -Y, +Z, -Z) that the
    // brush does not have as faces yet. They cut nothing off the brush itself, which is inside its box anyway. They are
    // needed when a box is traced against the brush (the next step): the planes are moved out by the size of the box,
    // and at sharp corners moved planes meet far from the brush, so the brush would reach out too far; the moved bevel
    // planes cut that off.
    //
    // Faces without a plane (three points on one line) and faces that do not touch the brush are left out. A brush with
    // no faces left is left out completely.
    [[nodiscard]] std::vector<CollisionBrush> BuildCollisionBrushes(const MapEntity& entity);
}
