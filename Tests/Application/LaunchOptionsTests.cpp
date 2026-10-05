#include "Application/LaunchOptions.h"

#include <gtest/gtest.h>

#include <array>
#include <string_view>

namespace Abomination
{
    TEST(LaunchOptions, NoArgumentsStartTheGameNormally)
    {
        const LaunchOptions options = ParseLaunchOptions({});

        EXPECT_FALSE(options.isBenchmark);
        EXPECT_TRUE(options.unknownArguments.empty());
    }

    TEST(LaunchOptions, BenchmarkIsRecognized)
    {
        const std::array<std::string_view, 1> arguments{"--benchmark"};

        const LaunchOptions options = ParseLaunchOptions(arguments);

        EXPECT_TRUE(options.isBenchmark);
        EXPECT_TRUE(options.unknownArguments.empty());
    }

    TEST(LaunchOptions, UnknownArgumentsAreKeptForTheLog)
    {
        const std::array<std::string_view, 3> arguments{"--fast", "--benchmark", "level2"};

        const LaunchOptions options = ParseLaunchOptions(arguments);

        EXPECT_TRUE(options.isBenchmark);
        ASSERT_EQ(options.unknownArguments.size(), 2u);
        EXPECT_EQ(options.unknownArguments[0], "--fast");
        EXPECT_EQ(options.unknownArguments[1], "level2");
    }
}
