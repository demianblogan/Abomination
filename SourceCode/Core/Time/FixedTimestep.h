#pragma once

namespace Abomination::Core
{
    // Turns frames of any duration into ticks of the simulation that always last exactly the same time
    // (1/60 s at 60 ticks per second), so movement and physics give the same result at any frame rate.
    //
    // Frame times are added to an accumulator, and whole ticks are taken out of it; the rest waits for the next frame:
    //
    //   tick = 16.7 ms, frame = 40 ms:   accumulator 40 -> tick -> 23.3 -> tick -> 6.7 ms left for the next frame
    //   tick = 16.7 ms, frame =  7 ms:   accumulator  7 -> no tick this frame, 14 -> no tick, 21 -> tick -> 4.3 left
    //
    // Usage, once per frame:
    //   const int tickCount = fixedTimestep.Advance(frameTime);
    //   for (int tick = 0; tick < tickCount; ++tick)
    //       Simulate(fixedTimestep.GetTickDuration());
    //   Draw(fixedTimestep.GetInterpolationFactor());
    class FixedTimestep
    {
    public:
        // At most this many ticks are run in one frame. If the computer cannot simulate in real time (the ticks take
        // longer than the time they simulate), every frame would need even more ticks, which would make the frames
        // even longer: the "spiral of death", ending in a frozen game. With the limit the game slows down instead.
        // 8 ticks keep the game in real time down to 7.5 FPS.
        static constexpr int MaxTicksPerFrame = 8;

        // ticksPerSecond must be greater than 0.
        explicit FixedTimestep(int ticksPerSecond) noexcept;

        // Adds the duration of a frame in seconds and returns how many ticks must be run now (0 to MaxTicksPerFrame).
        [[nodiscard]] int Advance(float frameTime) noexcept;

        // Duration of one tick in seconds: the delta time every tick of the simulation uses.
        [[nodiscard]] float GetTickDuration() const noexcept;

        [[nodiscard]] int GetTicksPerSecond() const noexcept;

        // Which part of the next tick has already passed, from 0 up to (not including) 1: the time left in the
        // accumulator divided by the tick duration. Objects are drawn at this fraction of the way between their state
        // before the last tick and after it, so movement looks smooth even when frames and ticks do not line up.
        [[nodiscard]] float GetInterpolationFactor() const noexcept;

    private:
        int m_ticksPerSecond = 0;
        float m_tickDuration = 0.0f;

        // Time that has passed but has not been simulated yet, in seconds. Always less than one tick after Advance().
        float m_accumulator = 0.0f;
    };
}
