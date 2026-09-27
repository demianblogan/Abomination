#pragma once

#include "Gameplay/MouseLook.h"

namespace Abomination::Gameplay
{
    // Component: the view direction of a free-fly camera entity as two angles, in radians (no roll: the horizon always
    // stays level). FreeFlyCameraController changes these angles and builds the Transform rotation from them; angles are
    // kept because mouse movement adds to them directly, and a limit on pitch is easy to apply to an angle.
    //   yaw   - turn left/right around the world up axis (+Y). 0 looks along -Z (OpenGL's "forward");
    //           a positive yaw turns to the LEFT (counter-clockwise when seen from above).
    //   pitch - tilt up/down. 0 looks at the horizon, a positive pitch looks up.
    struct FreeFlyCamera
    {
        // Pitch is kept within this range (see MaxLookPitch).
        static constexpr float MaxPitch = MaxLookPitch;

        float yaw = 0.0f;
        float pitch = 0.0f;
    };
}
