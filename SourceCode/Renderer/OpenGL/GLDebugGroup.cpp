#include "Renderer/OpenGL/GLDebugGroup.h"

#include <glad/gl.h>

namespace Abomination::Renderer
{
    GLDebugGroup::GLDebugGroup(std::string_view name)
    {
        // GL_DEBUG_SOURCE_APPLICATION: the group comes from our code, not from the driver. The id (0) is ours to choose and
        // is not used. The length is passed, so the name needs no terminating zero.
        glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, static_cast<GLsizei>(name.size()), name.data());
    }

    GLDebugGroup::~GLDebugGroup()
    {
        glPopDebugGroup();
    }
}
