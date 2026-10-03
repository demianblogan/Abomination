#include "Gameplay/Weapons/PumpAction.h"

namespace Abomination::Gameplay
{
    namespace
    {
        // 3t^2 - 2t^3: from 0 to 1, starting and stopping softly.
        float Smoothstep(float t)
        {
            return t * t * (3.0f - 2.0f * t);
        }
    }

    float CalculatePumpStartTime(const PumpActionSettings& settings)
    {
        return settings.recoilDuration + settings.raiseDuration;
    }

    float CalculatePumpProgress(const PumpActionSettings& settings, float secondsSinceShot)
    {
        float time = secondsSinceShot - CalculatePumpStartTime(settings);
        if (time <= 0.0f)
            return 0.0f;

        // Back: 1 - (1 - t)^2 starts fast and slows down to a stop at the back.
        if (time < settings.backDuration)
        {
            const float remaining = 1.0f - time / settings.backDuration;
            return 1.0f - remaining * remaining;
        }
        time -= settings.backDuration;

        if (time < settings.holdDuration)
            return 1.0f;
        time -= settings.holdDuration;

        // Forward: from all the way back to rest, softly.
        if (time < settings.forwardDuration)
            return 1.0f - Smoothstep(time / settings.forwardDuration);

        return 0.0f;
    }

    float CalculateChestPose(const PumpActionSettings& settings, float secondsSinceShot)
    {
        float time = secondsSinceShot - settings.recoilDuration;
        if (time <= 0.0f)
            return 0.0f;

        if (time < settings.raiseDuration)
            return Smoothstep(time / settings.raiseDuration);
        time -= settings.raiseDuration;

        // Held at the chest while the pump is worked.
        const float pumpDuration = settings.backDuration + settings.holdDuration + settings.forwardDuration;
        if (time < pumpDuration)
            return 1.0f;
        time -= pumpDuration;

        if (time < settings.lowerDuration)
            return 1.0f - Smoothstep(time / settings.lowerDuration);

        return 0.0f;
    }

    float CalculateShotCycleDuration(const PumpActionSettings& settings)
    {
        return settings.recoilDuration + settings.raiseDuration + settings.backDuration + settings.holdDuration +
               settings.forwardDuration + settings.lowerDuration;
    }
}
