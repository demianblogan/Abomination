#pragma once

#include "Navigation/NavMesh.h"
#include "World/MapData.h"

namespace Abomination::World
{
    // The triangles of every brush of an entity (the world), clip included, in game meters and axes: what the navmesh
    // of the level is built from. Clip counts: a clip ramp over stairs is what the dogs walk on.
    [[nodiscard]] Navigation::NavMeshGeometry BuildNavMeshGeometry(const MapEntity& entity);
}
