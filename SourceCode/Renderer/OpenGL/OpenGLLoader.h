#pragma once

#include <expected>
#include <string>

namespace Abomination::Renderer
{
    // What the graphics driver reports about itself.
    struct GraphicsDeviceInfo
    {
        std::string GPUName;                // "NVIDIA GeForce RTX 4060 Laptop GPU/PCIe/SSE2"
        std::string vendor;                 // "NVIDIA Corporation"
        std::string driverVersion;          // "4.6.0 NVIDIA 560.94"
        std::string shadingLanguageVersion; // "4.60 NVIDIA"
    };

    // Loads the addresses of all OpenGL 4.6 functions from the graphics driver and checks that 4.6 is supported.
    // Requires a current OpenGL context. Must be called once, after the window is created and before any other
    // OpenGL call: until then every gl* function pointer is nullptr.
    [[nodiscard]] std::expected<void, std::string> LoadOpenGLFunctions();

    // Asks the driver for the GPU name, vendor and versions. Requires loaded OpenGL functions.
    [[nodiscard]] GraphicsDeviceInfo GetGraphicsDeviceInfo();
}
