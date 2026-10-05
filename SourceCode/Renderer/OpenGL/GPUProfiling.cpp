#include "Renderer/OpenGL/GPUProfiling.h"

#include "Renderer/OpenGL/GPUProfileZone.h"

namespace Abomination::Renderer
{
    void StartGPUProfiling()
    {
        TracyGpuContext;
    }

    void CollectGPUProfiling()
    {
        TracyGpuCollect;
    }
}
