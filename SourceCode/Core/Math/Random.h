#pragma once

#include <cstddef>
#include <cstdint>
#include <random>

namespace Abomination::Core
{
    // A source of random numbers for the game: the pitch of a sound, the choice between sound variants, later the spread
    // of shotgun pellets.
    //
    // std::rand() is not used: it gives poor numbers, and its one hidden global state cannot be made repeatable for
    // one system only. Here every owner keeps its own generator; the same seed always gives the same sequence, so tests
    // (and later replays) can repeat it exactly.
    class Random
    {
    public:
        // A generator seeded from the operating system (std::random_device): different numbers every run.
        Random();

        // A generator with a fixed seed: the same numbers every run.
        explicit Random(std::uint32_t seed) noexcept;

        // A number from minimum to maximum (both may come out, minimum <= maximum).
        [[nodiscard]] float GetFloat(float minimum, float maximum);

        // An index from 0 to count - 1, each equally likely; count must not be 0.
        [[nodiscard]] std::size_t GetIndex(std::size_t count);

    private:
        // The Mersenne Twister: the standard general-purpose generator, fast and good enough for games (not for
        // cryptography).
        std::mt19937 m_engine;
    };
}
