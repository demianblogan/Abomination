#pragma once

#include "Core/Math/ConvexPolygon.h"
#include "World/MapData.h"

#include <vector>

namespace Abomination::World
{
    // Calculates the vertices of every face of a brush from the planes of its faces (a .map file stores only planes).
    // The result has one polygon per face, in the order of brush.faces, so result[i] belongs to brush.faces[i].
    //
    // For every face: a huge square is laid on the face's plane and clipped by every other plane of the brush; what is
    // left is exactly the face. The vertices go counter-clockwise when looked at from outside the brush (the front side
    // of the face), which is the order OpenGL treats as the front of a triangle.
    //
    // A face whose three points do not define a plane, or that is cut away completely (a plane that does not touch
    // the brush), gets an empty polygon. Coordinates stay in map units and axes.
    [[nodiscard]] std::vector<Core::ConvexPolygon> BuildBrushPolygons(const MapBrush& brush);
}
