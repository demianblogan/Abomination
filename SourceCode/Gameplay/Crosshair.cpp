#include "Gameplay/Crosshair.h"

#include <cmath>

namespace Abomination::Gameplay
{
    float CalculateSpreadRadiusOnScreen(float spreadAngle, float verticalFOV, float screenHeight)
    {
        // Half the screen height covers tan(verticalFOV / 2); the cone covers tan(spreadAngle) of it. Example: a spread
        // of 4 degrees with a field of view of 60 degrees on a 1080 pixel screen: tan(4) / tan(30) = 0.121, times 540 is
        // 65 pixels.
        return std::tan(spreadAngle) / std::tan(verticalFOV * 0.5f) * (screenHeight * 0.5f);
    }
}
