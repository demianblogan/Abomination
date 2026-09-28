#include "Renderer/OpenGL/OpenGLLoader.h"

#include "Core/Logging/Log.h"
#include "Platform/Window.h"

#include <glad/gl.h>

#include <format>

namespace Abomination::Renderer
{
    using Core::LogCategory;
    using Core::LogLevel;

    namespace
    {
        // glGetString returns the text as unsigned bytes; std::string works with char.
        std::string GetOpenGLString(GLenum name)
        {
            const GLubyte* text = glGetString(name);
            if (text == nullptr)
                return "unknown";

            return reinterpret_cast<const char*>(text);
        }
    }

    std::expected<void, std::string> LoadOpenGLFunctions()
    {
        // GLAD asks the driver for the address of every OpenGL function through our Platform function
        // and stores it in its own pointer: glClear, glCreateBuffers, ... become callable after this line.
        // The result is the version of the context that was actually created, encoded as major * 10000 + minor,
        // or 0 if loading failed.
        const int version = gladLoadGL(Platform::GetOpenGLFunctionAddress);
        if (version == 0)
            return std::unexpected("Failed to load OpenGL functions");

        const int majorVersion = GLAD_VERSION_MAJOR(version);
        const int minorVersion = GLAD_VERSION_MINOR(version);

        // GLAD sets a flag for every OpenGL version the context supports.
        if (GLAD_GL_VERSION_4_6 == 0)
        {
            return std::unexpected(std::format("OpenGL 4.6 is required, but the graphics driver provides only {}.{}",
                                               majorVersion, minorVersion));
        }

        const GraphicsDeviceInfo deviceInfo = GetGraphicsDeviceInfo();

        Core::Log::Write(LogCategory::Renderer, LogLevel::Info, "OpenGL {}.{} loaded", majorVersion, minorVersion);
        Core::Log::Write(LogCategory::Renderer, LogLevel::Info, "GPU: {} ({})", deviceInfo.GPUName, deviceInfo.vendor);
        Core::Log::Write(LogCategory::Renderer, LogLevel::Info, "Driver: {}", deviceInfo.driverVersion);
        Core::Log::Write(LogCategory::Renderer, LogLevel::Info, "GLSL: {}", deviceInfo.shadingLanguageVersion);

        return {};
    }

    GraphicsDeviceInfo GetGraphicsDeviceInfo()
    {
        return GraphicsDeviceInfo{
            .GPUName = GetOpenGLString(GL_RENDERER),
            .vendor = GetOpenGLString(GL_VENDOR),
            .driverVersion = GetOpenGLString(GL_VERSION),
            .shadingLanguageVersion = GetOpenGLString(GL_SHADING_LANGUAGE_VERSION),
        };
    }
}
