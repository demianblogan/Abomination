#pragma once

#include "Platform/ScreenMode.h"

#include <expected>
#include <string>

// SDL types are only declared here, so code that includes Window.h does not see SDL headers.
struct SDL_Window;
struct SDL_GLContextState;

namespace Abomination::Input
{
    struct InputDevices;
}

namespace Abomination::Platform
{
    class RmlUiPlatformBackend;

    struct WindowSettings
    {
        std::string title = "Abomination";

        // There is no size here: the window takes its size from the monitor (see CalculateWindowedSize in WindowSizing.h).

        bool isResizable = true;

        // Borderless by default: the game covers the screen like fullscreen, without the slow Alt+Tab of exclusive
        // fullscreen. Not saved between runs until the Config module (0.8).
        ScreenMode screenMode = ScreenMode::Borderless;

        // V-Sync: SwapBuffers() waits for the monitor refresh. No tearing, and the frame rate never exceeds the refresh rate.
        bool isVSyncEnabled = true;
    };

    // An operating system window with an OpenGL 4.6 Core context attached to it.
    // Requires an initialized SDLLibrary. Move-only.
    class Window
    {
    public:
        [[nodiscard]] static std::expected<Window, std::string> Create(const WindowSettings& settings);

        Window(const Window&) = delete;
        Window& operator=(const Window&) = delete;

        Window(Window&& other) noexcept;
        Window& operator=(Window&& other) noexcept;

        ~Window();

        // Handles all events the operating system has sent since the last call: closing and resizing the window,
        // keys, the mouse. Starts a new input frame of every device and fills it with the input of this frame.
        void ProcessEvents(Input::InputDevices& input);

        // The game interface that gets the keyboard and mouse events after ImGui (nullptr: none). Events the interface uses
        // (a click on a button) do not reach the game. The backend must outlive the window or be reset to nullptr.
        void SetRmlUiBackend(RmlUiPlatformBackend* backend) noexcept;

        // Shows the frame that has just been drawn: swaps the back buffer and the front buffer.
        void SwapBuffers();

        // Relative mouse mode: the cursor is hidden and locked inside the window, and the mouse reports only how far
        // it moved, without stopping at the edges of the screen. Used for looking around with the mouse.
        void SetRelativeMouseMode(bool isEnabled);

        // Turns V-Sync on or off (see WindowSettings::isVSyncEnabled). Takes effect from the next SwapBuffers(),
        // the window does not have to be created again. The graphics driver settings can force V-Sync on or off
        // regardless of this call.
        void SetVSyncEnabled(bool isEnabled);

        [[nodiscard]] bool IsVSyncEnabled() const noexcept;

        // Switches between windowed, borderless and exclusive fullscreen (see ScreenMode). The size in pixels changes a
        // moment later, when the operating system has resized the window (GetWidthInPixels() follows it). Returning to
        // windowed mode restores the size and position the window had before. On failure the mode stays and the error
        // is logged.
        void SetScreenMode(ScreenMode mode);

        [[nodiscard]] ScreenMode GetScreenMode() const noexcept;

        // Asks to close the window, like its close button does: IsCloseRequested() becomes true and the main loop ends.
        void RequestClose() noexcept;

        [[nodiscard]] bool IsCloseRequested() const noexcept;

        // Size of the drawable area in pixels. OpenGL works in pixels, so these are the values for glViewport.
        [[nodiscard]] int GetWidthInPixels() const noexcept;
        [[nodiscard]] int GetHeightInPixels() const noexcept;

        // The display scale of Windows for the monitor the window is on: 1.0 at 100%, 1.5 at 150%, 2.0 at 200% (usual for
        // 4K monitors). Things drawn in pixels (the debug overlay) are multiplied by it to keep the size the user expects.
        // Updated when the setting changes or the window moves to a monitor with another scale.
        [[nodiscard]] float GetDisplayScale() const noexcept;

        // The SDL handles of the window and its OpenGL context, for other Platform classes (ImGuiPlatformBackend).
        // Code outside Platform cannot use them: SDL types are only declared there, never defined.
        [[nodiscard]] SDL_Window* GetSDLWindow() const noexcept;
        [[nodiscard]] SDL_GLContextState* GetSDLContext() const noexcept;

    private:
        Window(SDL_Window* window, SDL_GLContextState* context) noexcept;

        void Destroy() noexcept;

        SDL_Window* m_window = nullptr;
        SDL_GLContextState* m_context = nullptr;
        bool m_isCloseRequested = false;
        bool m_isVSyncEnabled = false;
        ScreenMode m_screenMode = ScreenMode::Windowed;
        int m_widthInPixels = 0;
        int m_heightInPixels = 0;
        float m_displayScale = 1.0f;
        RmlUiPlatformBackend* m_rmlUiBackend = nullptr;
    };

    // A pointer to a function without parameters and return value. OpenGL functions have different signatures,
    // so the address is returned in this general form and GLAD casts it to the real type of each function.
    using OpenGLFunction = void (*)();

    // Asks the graphics driver for the address of an OpenGL function by its name ("glClear", ...).
    // Requires a current OpenGL context. Returns nullptr if the driver does not provide the function.
    [[nodiscard]] OpenGLFunction GetOpenGLFunctionAddress(const char* name);
}
