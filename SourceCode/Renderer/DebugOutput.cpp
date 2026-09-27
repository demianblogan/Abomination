#include "Renderer/DebugOutput.h"

#include "Core/Log.h"

#include <glad/gl.h>

#include <string_view>

namespace Abomination::Renderer
{
    using Core::LogCategory;
    using Core::LogLevel;

    namespace
    {
        // Which part of the system produced the message.
        constexpr std::string_view ConvertSourceToString(GLenum source) noexcept
        {
            switch (source)
            {
                case GL_DEBUG_SOURCE_API:
                    return "API";
                case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
                    return "Window system";
                case GL_DEBUG_SOURCE_SHADER_COMPILER:
                    return "Shader compiler";
                case GL_DEBUG_SOURCE_THIRD_PARTY:
                    return "Third party";
                case GL_DEBUG_SOURCE_APPLICATION:
                    return "Application";
                default:
                    return "Other";
            }
        }

        // What kind of problem the message describes.
        constexpr std::string_view ConvertTypeToString(GLenum type) noexcept
        {
            switch (type)
            {
                case GL_DEBUG_TYPE_ERROR:
                    return "Error";
                case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
                    return "Deprecated behavior";
                case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
                    return "Undefined behavior";
                case GL_DEBUG_TYPE_PORTABILITY:
                    return "Portability";
                case GL_DEBUG_TYPE_PERFORMANCE:
                    return "Performance";
                case GL_DEBUG_TYPE_MARKER:
                    return "Marker";
                default:
                    return "Other";
            }
        }

        // Driver severities mapped to our log levels. Notifications are informational messages that some drivers
        // send very often (for example, where a buffer is stored), so they go to Trace, which is normally disabled.
        constexpr LogLevel ConvertSeverityToLogLevel(GLenum severity) noexcept
        {
            switch (severity)
            {
                case GL_DEBUG_SEVERITY_HIGH:
                    return LogLevel::Error;
                case GL_DEBUG_SEVERITY_MEDIUM:
                    return LogLevel::Warning;
                case GL_DEBUG_SEVERITY_LOW:
                    return LogLevel::Debug;
                default:
                    return LogLevel::Trace;
            }
        }

        // Called by the driver for every debug message. The signature is defined by OpenGL (GLDEBUGPROC);
        // GLAPIENTRY is the calling convention OpenGL uses on Windows (__stdcall).
        void GLAPIENTRY HandleDebugMessage(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei /*length*/,
                                           const GLchar* message, const void* /*userParameter*/)
        {
            // Performance messages are hints about how the driver works, not mistakes, so they are at most Debug. For
            // example, NVIDIA reports as a medium-severity warning that it recompiled a vertex shader when the wireframe
            // mode is switched on, which is expected and costs nothing noticeable.
            LogLevel level = ConvertSeverityToLogLevel(severity);
            if (type == GL_DEBUG_TYPE_PERFORMANCE && level > LogLevel::Debug)
                level = LogLevel::Debug;

            Core::Log::Write(LogCategory::Renderer, level, "OpenGL {} ({}, #{}): {}",
                             ConvertTypeToString(type), ConvertSourceToString(source), id, message);
        }
    }

    void EnableDebugOutput()
    {
        GLint contextFlags = 0;
        glGetIntegerv(GL_CONTEXT_FLAGS, &contextFlags);

        if ((contextFlags & GL_CONTEXT_FLAG_DEBUG_BIT) == 0)
        {
            Core::Log::Write(LogCategory::Renderer, LogLevel::Info, "OpenGL debug output is unavailable: not a debug context");

            return;
        }

        glEnable(GL_DEBUG_OUTPUT);

        // The driver calls HandleDebugMessage inside the OpenGL call that caused the message, not later from another
        // thread. Slower, but a breakpoint in HandleDebugMessage then shows exactly which of our calls is wrong.
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);

        glDebugMessageCallback(HandleDebugMessage, nullptr);

        Core::Log::Write(LogCategory::Renderer, LogLevel::Info, "OpenGL debug output enabled");
    }
}
