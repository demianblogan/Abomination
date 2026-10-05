#pragma once

#include <string_view>

namespace Abomination::Renderer
{
    // A named group of OpenGL commands, for frame debuggers such as RenderDoc: every command from the constructor to the
    // destructor is shown under this name ("World", "Weapon"), so the list of a frame reads as its passes instead of
    // hundreds of bare draw calls. Groups may be nested. Costs nothing measurable without a debugger.
    //
    // A scope guard, used as a local variable: { GLDebugGroup group("World"); ... }. Not copyable and not movable, because
    // a group must close exactly once, in the scope it was opened in. Requires a current OpenGL context.
    class GLDebugGroup
    {
    public:
        explicit GLDebugGroup(std::string_view name);

        GLDebugGroup(const GLDebugGroup&) = delete;
        GLDebugGroup& operator=(const GLDebugGroup&) = delete;

        GLDebugGroup(GLDebugGroup&&) = delete;
        GLDebugGroup& operator=(GLDebugGroup&&) = delete;

        ~GLDebugGroup();
    };
}
