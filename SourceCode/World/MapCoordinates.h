#pragma once

#include "Core/Math/Plane.h"
#include "Core/Scene/Transform.h"
#include "World/MapData.h"

#include <glm/gtc/quaternion.hpp>
#include <glm/vec3.hpp>

#include <optional>
#include <string_view>

// Converting from the coordinates of .map files to the coordinates of the game. It happens once, when a map is loaded,
// so everything after loading works in the game's own units and axes.
//
//   map (Quake, TrenchBroom):  units, Z is up, X and Y are horizontal
//   game (OpenGL, glm):        meters, Y is up, -Z is forward
namespace Abomination::World
{
    // A position: axes turned (x, y, z) -> (x, z, -y), then units -> meters (see Core/Math/Units.h).
    // The turn is a rotation by 90 degrees around X: map +Z (up) becomes game +Y (up), map +Y becomes game -Z.
    // A rotation keeps the handedness of the coordinate system, so the counter-clockwise order of face corners, and
    // with it the front side of faces, stays the same.
    [[nodiscard]] glm::vec3 ConvertMapPosition(const glm::dvec3& mapPosition);

    // The same as ConvertMapPosition, but in doubles: for collision data, which stays in doubles like the map.
    [[nodiscard]] glm::dvec3 ConvertMapPositionPrecise(const glm::dvec3& mapPosition);

    // A plane: its normal turns like a direction, its distance from the origin only changes units (turning around the
    // origin does not change distances from it). In doubles, like Core::Plane.
    [[nodiscard]] Core::Plane ConvertMapPlane(const Core::Plane& mapPlane);

    // The "angle" of a map entity (degrees counter-clockwise from map +X, seen from above) as the yaw of the game
    // (radians counter-clockwise from game -Z, see Gameplay/Camera/MouseLook.h): an angle of 90 (map +Y) is yaw 0.
    [[nodiscard]] float ConvertMapAngleToYaw(double mapAngleDegrees);

    // How a model placed on a map (misc_model, a torch) is turned by its angle. TrenchBroom shows the +Z of a glTF model
    // (its front in glTF) towards the angle, while a yaw of the game turns the -Z of an entity there (see
    // Core::LocalForward): half a turn more, and the game shows the model the way the editor does.
    [[nodiscard]] glm::quat CalculateMapModelRotation(double mapAngleDegrees);

    // Where a model placed on a map stands: its origin, turned by its angle (see CalculateMapModelRotation).
    [[nodiscard]] Core::Transform ReadMapModelTransform(const MapEntity& entity);

    // Reads a vector property like "origin" "-48 -176 88". Returns nothing if the text is not three numbers.
    [[nodiscard]] std::optional<glm::dvec3> ParseVectorProperty(std::string_view text);
}
