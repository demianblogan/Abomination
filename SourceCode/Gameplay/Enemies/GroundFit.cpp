#include "Gameplay/Enemies/GroundFit.h"

#include <algorithm>
#include <cmath>

namespace Abomination::Gameplay
{
    namespace
    {
        // Moves value towards target by at most maximumStep.
        float MoveTowards(float value, float target, float maximumStep)
        {
            return value + std::clamp(target - value, -maximumStep, maximumStep);
        }
    }

    GroundFitTarget CalculateGroundFitTarget(float frontGround, float backGround, float bottom, float pawDistance,
                                             float maximumTilt, float maximumOffset)
    {
        // The line from the hind paws to the front paws rises frontGround - backGround over 2 × pawDistance: its angle
        // is the arctangent of rise over run. For example, a step of 0.5 m under paws 0.4 m from the middle: atan(0.5 /
        // 0.8) = 32 degrees.
        const float pitch = std::atan2(frontGround - backGround, 2.0f * pawDistance);

        // Tilted around its middle, the body goes up at one end as much as it goes down at the other: its middle is
        // at the height halfway between the paws.
        const float middle = (frontGround + backGround) * 0.5f;

        return GroundFitTarget{
            .offset = std::clamp(middle - bottom, -maximumOffset, maximumOffset),
            .pitch = std::clamp(pitch, -maximumTilt, maximumTilt),
        };
    }

    void UpdateGroundFit(GroundFit& fit, const GroundFitTarget& target, float steppedUpHeight, float heightSpeed,
                         float tiltSpeed, float maximumLag, float deltaTime)
    {
        fit.previousOffset = fit.offset;
        fit.previousPitch = fit.pitch;

        // The box jumped up the step at once: the model stays where it was in the world, now below the box. (Only the
        // offset now: the previous offset belongs to the previous position of the box, below the step.)
        fit.offset -= steppedUpHeight;

        fit.offset = MoveTowards(fit.offset, target.offset, heightSpeed * deltaTime);
        fit.offset = std::clamp(fit.offset, target.offset - maximumLag, target.offset + maximumLag);
        fit.pitch = MoveTowards(fit.pitch, target.pitch, tiltSpeed * deltaTime);
    }
}
