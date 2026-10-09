#pragma once

#include "Core/Scene/Transform.h"
#include "Renderer/Light.h"
#include "World/MapData.h"

#include <optional>

// The lights of a map: the point entities point_light and spot_light (Tools/TrenchBroom/Abomination/Abomination.fgd),
// named like the lights of Unity.
//
// Their properties, in the units of the map:
//   intensity  the light at 1 m (3 lights a white wall fully, like the made-up sun); 5 by default, 30 for a spot
//   range      where the light is faded out, in units (320 = 10 m)
//   _color     the color as TrenchBroom picks it, "R G B" from 0 to 255 (sRGB)
// and of a spot only:
//   angles     where it shines, "pitch yaw roll" in degrees, as the rotate tool of TrenchBroom writes them: yaw around
//              the vertical like the angle of other entities (0 along +X of the map), pitch from 90 (straight down) to
//              -90 (straight up); roll does nothing to a round cone. Without it the spot shines along +X, as
//              TrenchBroom shows an entity without angles.
//   cone       the angle from the axis to the edge of the light, in degrees
//   inner_cone the angle up to which the light is full, in degrees (it fades out between inner_cone and cone)
namespace Abomination::World
{
    // A light as the level places it: where it is, where it points and what it gives.
    struct MapLight
    {
        Core::Transform transform;
        Renderer::Light light;
    };

    // The light of a point_light or spot_light entity, or nothing for any other entity. A missing or unreadable property
    // keeps its default (see Renderer::Light).
    [[nodiscard]] std::optional<MapLight> ReadMapLight(const MapEntity& entity);
}
