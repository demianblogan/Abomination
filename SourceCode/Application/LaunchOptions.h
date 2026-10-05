#pragma once

#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace Abomination
{
    // What the command line asks the game to do: "Abomination.exe --benchmark".
    struct LaunchOptions
    {
        // --benchmark: a repeatable scene for the profiler (see Tools/Profiling/Capture.ps1): the start map, nobody at the
        // controls, an invulnerable player the dogs attack, and the game closes itself after BenchmarkDuration.
        bool isBenchmark = false;

        // Arguments the game does not know, kept so that main() can write them to the log.
        std::vector<std::string> unknownArguments;
    };

    // How long a benchmark runs, in seconds of game time.
    constexpr int BenchmarkDuration = 20;

    // arguments: the command line without the name of the executable (argv[1] ... argv[argc - 1]).
    [[nodiscard]] LaunchOptions ParseLaunchOptions(std::span<const std::string_view> arguments);
}
