#include "Core/Profiling/Profiler.h"

#include <tracy/Tracy.hpp>

namespace Abomination::Core
{
    // StartupProfiler and ShutdownProfiler exist only when Tracy is built in with a manual lifetime (TRACY_ENABLE,
    // TRACY_DELAYED_INIT and TRACY_MANUAL_LIFETIME, see ThirdParty/Tracy/CMakeLists.txt).
    void StartProfiler()
    {
#ifdef TRACY_ENABLE
        tracy::StartupProfiler();
#endif
    }

    void StopProfiler()
    {
#ifdef TRACY_ENABLE
        tracy::ShutdownProfiler();
#endif
    }
}
