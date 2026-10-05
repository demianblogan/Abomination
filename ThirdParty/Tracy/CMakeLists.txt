# Tracy 0.14.1: a frame profiler. The game (the client) measures zones of code and sends them over a socket to the
# Tracy profiler, a separate program that shows them on a timeline.
# public/ is the client folder of the release tag (https://github.com/wolfpld/tracy/tree/v0.14.1/public), not edited by
# hand. vcpkg only has 0.13.1 so far. The profiler program must have the same version as the client: download
# tracy-0.14.1 for Windows from https://github.com/wolfpld/tracy/releases.
# To update: replace public/ and LICENSE with the folder of the new release tag, and download the matching profiler.

# The whole client is one source file that includes all the others ("unity build" of the library).
add_library(Tracy STATIC
    public/TracyClient.cpp
)

# SYSTEM: the headers are treated as external, so they produce no warnings in our code (/external:W0).
# Our code includes them as <tracy/Tracy.hpp>.
target_include_directories(Tracy SYSTEM PUBLIC public)

# Without TRACY_ENABLE the zone macros are empty and TracyClient.cpp compiles to nothing, so a build without profiling
# carries no trace of Tracy. The definitions are PUBLIC: every file that includes Tracy.hpp must see the same ones.
if(ABOMINATION_PROFILING)
    target_compile_definitions(Tracy PUBLIC
        TRACY_ENABLE
        TRACY_ON_DEMAND       # Collect data only while the profiler is connected; otherwise a zone costs a few nanoseconds
        TRACY_ONLY_LOCALHOST  # Listen on this computer only: nothing is open to the network, and no firewall question
    )
endif()

# Group the target under "ThirdParty" in the Visual Studio solution.
set_target_properties(Tracy PROPERTIES FOLDER ThirdParty)
