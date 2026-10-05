#pragma once

// GPU zones of the Tracy profiler (see GPUProfiling.h). Included only by .cpp files of the Renderer: it brings GLAD
// with it, which must stay private to the module.
//
// The order of the includes is not the usual one, because TracyOpenGL.hpp does not include what it uses itself:
// - it calls OpenGL functions (glGenQueries, glQueryCounter, ...) without declaring them, so GLAD comes first;
// - without profiling it still uses int32_t, so <cstdint> comes first too.
#include <cstdint>

#include <glad/gl.h>
#include <tracy/TracyOpenGL.hpp>

// The time the GPU spends on the drawing commands from here to the end of the block, as a zone on the GPU timeline of
// the profiler: PROFILE_GPU_ZONE("Sprites"). Only in drawing code, which runs only in the game after
// Core::StartProfiler, so unlike PROFILE_ZONE it does not ask whether the profiler is running.
#define PROFILE_GPU_ZONE(name) TracyGpuZone(name)
