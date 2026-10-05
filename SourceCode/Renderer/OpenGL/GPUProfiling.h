#pragma once

namespace Abomination::Renderer
{
    // The GPU timeline of the Tracy profiler. The GPU draws later than the CPU asks it to, so the time of a GPU zone
    // (PROFILE_GPU_ZONE in the drawing code, see GPUProfileZone.h) is measured with OpenGL timestamp queries: the GPU
    // writes its clock into a query when it reaches the start and the end of the zone, and the results are read a few
    // frames later. Without ABOMINATION_PROFILING both functions do nothing.

    // Connects the profiler to the OpenGL context. Called once, after the OpenGL functions are loaded.
    void StartGPUProfiling();

    // Reads the timestamps the GPU has finished writing and sends them to the profiler. Called once per frame, after
    // the buffers are swapped.
    void CollectGPUProfiling();
}
