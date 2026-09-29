#include "Core/Math/Random.h"

#include <gtest/gtest.h>

#include <array>
#include <cstddef>

namespace Abomination::Core
{
    TEST(Random, SameSeedGivesSameNumbers)
    {
        Random first(42);
        Random second(42);

        for (int i = 0; i < 100; ++i)
            EXPECT_EQ(first.GetFloat(0.0f, 1.0f), second.GetFloat(0.0f, 1.0f));
    }

    TEST(Random, FloatsStayInRange)
    {
        Random random(7);

        for (int i = 0; i < 1000; ++i)
        {
            const float value = random.GetFloat(0.95f, 1.05f);
            EXPECT_GE(value, 0.95f);
            EXPECT_LE(value, 1.05f);
        }
    }

    TEST(Random, EmptyRangeGivesItsOnlyValue)
    {
        Random random(7);

        EXPECT_EQ(random.GetFloat(2.0f, 2.0f), 2.0f);
    }

    TEST(Random, IndicesCoverTheWholeRange)
    {
        Random random(1);
        std::array<int, 4> hits{};

        for (int i = 0; i < 1000; ++i)
        {
            const std::size_t index = random.GetIndex(hits.size());
            ASSERT_LT(index, hits.size());
            ++hits[index];
        }

        // 1000 picks of 4 indices: every index comes out about 250 times; none is left out.
        for (const int count : hits)
            EXPECT_GT(count, 150);
    }
}
