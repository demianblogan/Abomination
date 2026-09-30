#pragma once

#include <glm/vec4.hpp>

namespace Abomination::Gameplay
{
    // A hit marker: four short diagonal lines around the crosshair that flash for a moment when a shot hurts something,
    // moving a little outwards (or inwards) and fading out. Sizes are in pixels at 100% UI scale.
    struct HitMarkerSettings
    {
        glm::vec4 color{1.0f, 1.0f, 1.0f, 1.0f};

        // How long one line is, how far from the circle it starts, and how far it moves during the marker (negative:
        // towards the middle).
        float lineLength = 15.0f;
        float startGap = 3.0f;
        float travelDistance = 6.0f;

        // Seconds from appearing to gone.
        float duration = 0.18f;

        float thickness = 2.0f;
    };

    // How the crosshair of a weapon looks. Every weapon has its own (see Weapon): the shotgun shows a circle as wide as
    // the spread of its pellets; later weapons may have other shapes (0.7). Sizes are in pixels at 100% UI scale.
    struct CrosshairSettings
    {
        glm::vec4 color{1.0f, 1.0f, 1.0f, 0.85f};

        // The circle: its radius on the screen is the spread of the weapon, so every pellet lands inside it.
        float circleThickness = 1.5f;

        // The dot in the middle (0: none).
        float dotRadius = 1.5f;

        // The markers of a shot that hurt and of one that killed.
        HitMarkerSettings hitMarker;
        HitMarkerSettings killMarker{
            .color = {1.0f, 0.15f, 0.1f, 1.0f},
            .lineLength = 20.0f,
            .travelDistance = 9.0f,
            .duration = 0.35f,
            .thickness = 3.0f,
        };
    };

    // The radius on the screen (pixels) of a cone of half angle spreadAngle around the middle of the view: where the
    // cone crosses the screen. verticalFOV is the vertical field of view of the camera (radians), screenHeight the
    // height of the screen in pixels. The middle of the screen is at distance 1 from the eyes in the units where the top
    // edge is tan(verticalFOV / 2) above the middle; the cone reaches tan(spreadAngle) there.
    [[nodiscard]] float CalculateSpreadRadiusOnScreen(float spreadAngle, float verticalFOV, float screenHeight);
}
