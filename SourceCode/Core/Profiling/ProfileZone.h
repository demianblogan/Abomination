#pragma once

#include <tracy/Tracy.hpp>

// Zones of the Tracy profiler: a zone measures the time from where it is written to the end of its block and shows it
// on the timeline of the profiler. Macros, because only a macro can disappear completely from a build without profiling
// (ABOMINATION_PROFILING off, see CODE_STYLE.md).
//
// The profiler runs only while the game has started it (Core::StartProfiler in main()). Every zone first asks whether
// it is running (TracyIsStarted, one atomic load): the tests and tools that link the same code never start it, so for
// them a zone is that one check, with no profiler threads, sockets or the 200 ms timer calibration at the start of
// every process.

// A zone named after the function it is in. The first line of a function body.
#define PROFILE_ZONE() ZoneNamed(___tracy_scoped_zone, TracyIsStarted)

// A zone with a name in plain words, for a part of a function: { PROFILE_ZONE_NAMED("Swap buffers"); ... }
#define PROFILE_ZONE_NAMED(name) ZoneNamedN(___tracy_scoped_zone, name, TracyIsStarted)

// The end of a frame: the profiler cuts its timeline into frames here. Once per frame of the main loop.
#define PROFILE_FRAME_MARK() FrameMark
