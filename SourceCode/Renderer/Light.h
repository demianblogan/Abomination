#pragma once

#include <glm/vec3.hpp>

namespace Abomination::Renderer
{
    // How a light spreads.
    enum class LightType
    {
        // Into every direction from one point: a torch, a candle, a brazier.
        Point,

        // In a cone along the direction the entity faces (Core::LocalForward turned by its rotation): a lantern with a
        // shutter, the flashlight.
        Spot,
    };

    // Component: "this entity gives light". Together with Core::Transform (where it is and, for a spot, where it points)
    // it lights every surface within its range.
    //
    // Light from a point weakens with the square of the distance: at 2 m a surface gets a quarter of the light it gets at
    // 1 m, because the same light is spread over a sphere 4 times as large. intensity is the light at 1 m, in the units of
    // the made-up sun (3 makes a white surface facing it fully white). A real light reaches forever but less and less;
    // range is where it is faded out completely, so a light costs nothing beyond it.
    struct Light
    {
        LightType type = LightType::Point;

        // Linear values (an sRGB color picked in the editor is converted when the map is loaded).
        glm::vec3 color{1.0f};

        float intensity = 5.0f;

        // Meters.
        float range = 10.0f;

        // Only for a spot: the angles from its axis to the edge of the cone, in radians. Inside the inner angle the light
        // is full; between the two it fades to nothing at the outer one, so the edge of the circle of light is soft.
        // 20 and 30 degrees.
        float innerConeAngle = 0.34906585f;
        float outerConeAngle = 0.52359878f;
    };
}
