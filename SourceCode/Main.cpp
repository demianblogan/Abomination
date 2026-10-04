#include "Application/Application.h"
#include "Core/BuildConfiguration.h"
#include "Core/Logging/Log.h"
#include "Core/Logging/LogHistory.h"
#include "Core/Version.h"
#include "Platform/SystemServices.h"

#include <expected>
#include <filesystem>
#include <string>

// On laptops with hybrid graphics (an integrated and a discrete GPU) the NVIDIA / AMD driver decides which GPU runs
// a process, using its database of known games. A game that is not in the database may silently run on the weak
// integrated GPU. When the executable exports these two variables, the drivers run it on the discrete GPU instead.
// On computers with one GPU they have no effect.
// The names and types are defined by NVIDIA (Optimus) and AMD (PowerXpress) and must not be changed.
// They must be defined in the executable itself: in a static library the linker would drop them as unused.
extern "C"
{
    __declspec(dllexport) unsigned long NvOptimusEnablement = 1;
    __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}

namespace
{
    using namespace Abomination;
    using namespace Abomination::Core;

    // Creates and runs the application. It is a separate function so that the application (the window, SDL)
    // is destroyed when the function returns, while logging still works and can record the shutdown.
    int RunApplication(LogHistory& logHistory)
    {
        // CMake copies the Assets folder of the repository next to the executable on every build: the game finds its
        // files there, the same way it will after installation. A Debug build reads the Assets folder of the
        // repository itself instead, so a map saved in TrenchBroom (or any edited asset) is seen after a level reload,
        // without a build. If that folder is gone (the executable was moved to another computer), the copy is used.
        std::filesystem::path assetsDirectory = Platform::GetExecutableDirectory() / "Assets";
#ifdef ABOMINATION_SOURCE_ASSETS_DIRECTORY
        if (std::filesystem::is_directory(ABOMINATION_SOURCE_ASSETS_DIRECTORY))
            assetsDirectory = ABOMINATION_SOURCE_ASSETS_DIRECTORY;
#endif

        std::expected<Application, std::string> application = Application::Create(assetsDirectory, logHistory);
        if (!application.has_value())
        {
            Log::Write(LogCategory::Core, LogLevel::Critical, "{}", application.error());
            Platform::ShowErrorDialog("Abomination", application.error());

            return 1;
        }

        return application->Run();
    }
}

int main()
{
    // The last messages, kept for the in-game console. It lives until the end of main(), after Log::Shutdown().
    LogHistory logHistory;

    Log::Initialize(LogSettings{
        .filePath = Platform::GetExecutableDirectory() / "Abomination.log",
        .minimumLevel = IsDebugBuild ? LogLevel::Debug : LogLevel::Info,
        .history = &logHistory,
    });

    Log::Write(LogCategory::Core, LogLevel::Info, "Abomination {}", GetGameVersionString());

    const int exitCode = RunApplication(logHistory);

    Log::Shutdown();

    return exitCode;
}
