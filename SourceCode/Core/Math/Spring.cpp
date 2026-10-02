#include "Core/Math/Spring.h"

#include <algorithm>
#include <cmath>

namespace Abomination::Core
{
    namespace
    {
        // The spring is integrated in steps of at most this long (seconds): a spring moved by a whole slow frame at once
        // (a long frame of 0.1 s) would overshoot and could swing ever wider. 240 steps per second keep it stable.
        constexpr float SpringStepDuration = 1.0f / 240.0f;
    }

    void UpdateDampedSpring(float& offset, float& velocity, float stiffness, float deltaTime)
    {
        // The spring accelerates towards 0 in proportion to how far away it is (stiffness), the damping brakes in
        // proportion to the speed. Damping of 1.4 * sqrt(stiffness) is a little below the value at which it would creep
        // back without any swing (2 * sqrt(stiffness)), so it swings past 0 once, slightly, like a hand catching a weight.
        const float damping = 1.4f * std::sqrt(stiffness);
        float remainingTime = deltaTime;
        while (remainingTime > 0.0f)
        {
            const float step = std::min(remainingTime, SpringStepDuration);
            const float acceleration = -stiffness * offset - damping * velocity;

            // Semi-implicit Euler: the new velocity moves the position. Unlike the plain order, it does not add energy to
            // the spring, so the swing does not grow by itself.
            velocity += acceleration * step;
            offset += velocity * step;
            remainingTime -= step;
        }
    }

    float CalculateApproachFactor(float rate, float deltaTime)
    {
        return 1.0f - std::exp(-rate * deltaTime);
    }
}
