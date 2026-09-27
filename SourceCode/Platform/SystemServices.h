#pragma once

#include <chrono>
#include <filesystem>
#include <string>

// Small operating system services that do not need an initialized SDL library.
namespace Abomination::Platform
{
    // The folder that contains the executable. Files next to the game (the log, the window settings of the debug overlay)
    // are placed relative to it, so they end up in the same place no matter from which folder the game was started.
    [[nodiscard]] std::filesystem::path GetExecutableDirectory();

    // Pauses the calling thread for the given time, accurate to a fraction of a millisecond.
    // A plain sleep of the operating system can oversleep by a whole scheduler tick (up to ~15 ms on Windows),
    // which is as long as a frame; this one sleeps most of the time and busy-waits the last moment.
    void SleepPrecisely(std::chrono::nanoseconds duration);

    // Shows a modal error dialog and waits until the user closes it. Used for fatal errors at startup.
    void ShowErrorDialog(const std::string& title, const std::string& message);
}
