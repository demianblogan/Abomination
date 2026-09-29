#include "Core/Math/Random.h"

#include <cassert>

namespace Abomination::Core
{
    Random::Random()
        : m_engine(std::random_device{}())
    {
    }

    Random::Random(std::uint32_t seed) noexcept
        : m_engine(seed)
    {
    }

    float Random::GetFloat(float minimum, float maximum)
    {
        assert(minimum <= maximum);

        // A distribution turns the raw 32-bit numbers of the engine into numbers of the wanted range, evenly spread.
        // It is cheap to create, so it is made for every call instead of being kept for every range.
        return std::uniform_real_distribution<float>(minimum, maximum)(m_engine);
    }

    std::size_t Random::GetIndex(std::size_t count)
    {
        assert(count > 0);

        return std::uniform_int_distribution<std::size_t>(0, count - 1)(m_engine);
    }
}
