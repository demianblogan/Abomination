#pragma once

// A four-legged character stands on the ground with a box (see Physics::CharacterBody): on the edge of a stair the box
// rests on the upper step with a few centimeters of it, and most of the dog hangs in the air. Its model is fitted to the
// ground instead: the height of the ground is found under its front paws and under its hind paws, the model is tilted
// by the difference and lowered (or raised) to the middle of the two. Only the drawing changes; the box stays as it is.
namespace Abomination::Gameplay
{
    // Component: how the model of a character is fitted to the ground now and one tick earlier (drawn between the two,
    // see Renderer::DrawOffset). offset is how far the model is drawn above the bottom of its box (negative: below),
    // pitch its tilt in radians (positive: its front up).
    struct GroundFit
    {
        float offset = 0.0f;
        float previousOffset = 0.0f;
        float pitch = 0.0f;
        float previousPitch = 0.0f;
    };

    // Where the fit should be.
    struct GroundFitTarget
    {
        float offset = 0.0f;
        float pitch = 0.0f;
    };

    // The fit for ground at frontGround under the front paws and backGround under the hind paws (heights, in meters),
    // pawDistance in front of and behind the middle of the body, whose box stands at bottom. The tilt is at most
    // maximumTilt, the offset at most maximumOffset either way.
    [[nodiscard]] GroundFitTarget CalculateGroundFitTarget(float frontGround, float backGround, float bottom,
                                                           float pawDistance, float maximumTilt, float maximumOffset);

    // One tick: the fit moves towards target, the offset by at most heightSpeed × deltaTime, the tilt by at most
    // tiltSpeed × deltaTime, so the model glides from step to step. steppedUpHeight is how far the box just stepped up
    // (see Physics::CharacterBody): the model stays where it was and glides up after the box, but never more than
    // maximumLag from the target (a running dog would leave its model far behind on a long stair otherwise).
    void UpdateGroundFit(GroundFit& fit, const GroundFitTarget& target, float steppedUpHeight, float heightSpeed,
                         float tiltSpeed, float maximumLag, float deltaTime);
}
