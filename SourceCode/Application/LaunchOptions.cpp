#include "Application/LaunchOptions.h"

namespace Abomination
{
    LaunchOptions ParseLaunchOptions(std::span<const std::string_view> arguments)
    {
        LaunchOptions options;
        for (const std::string_view argument : arguments)
        {
            if (argument == "--benchmark")
                options.isBenchmark = true;
            else
                options.unknownArguments.emplace_back(argument);
        }

        return options;
    }
}
