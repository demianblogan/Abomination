#pragma once

namespace Abomination::Core
{
    // Starts the Tracy profiler client: its threads, and a socket on this computer that the profiler program connects
    // to. Data is collected only while the program is connected. Only the game starts it (in main(), before anything
    // else is measured); the tests never do, so their zones cost nothing (see ProfileZone.h).
    // Without ABOMINATION_PROFILING both functions do nothing.
    void StartProfiler();

    // Stops the client; called at the end of main(), after everything that is measured is gone.
    void StopProfiler();
}
