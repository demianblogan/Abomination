#pragma once

#include <RmlUi/Core/SystemInterface.h>

#include <array>
#include <cstddef>

// SDL types are only declared here, like in Window.h.
struct SDL_Cursor;
struct SDL_Window;
union SDL_Event;

namespace Rml
{
    class Context;
}

namespace Abomination::Platform
{
    // The SDL3 part of RmlUi, the library of the game interface: the time, the log, the mouse cursor, the clipboard and the
    // on-screen text input, and the keyboard and mouse events passed on to the interface (Window::ProcessEvents forwards
    // every SDL event here, after ImGui).
    //
    // Not copyable or movable: RmlUi and the window keep a pointer to it.
    class RmlUiPlatformBackend final : public Rml::SystemInterface
    {
    public:
        explicit RmlUiPlatformBackend(SDL_Window* window);

        RmlUiPlatformBackend(const RmlUiPlatformBackend&) = delete;
        RmlUiPlatformBackend& operator=(const RmlUiPlatformBackend&) = delete;

        ~RmlUiPlatformBackend() override;

        // The context that gets the input, or nullptr while there is none.
        void SetContext(Rml::Context* context) noexcept;

        // Whether the interface gets the mouse and the keyboard: only while it shows something to click (a menu), never
        // while the mouse turns the view.
        void SetInputEnabled(bool isEnabled) noexcept;

        // Passes one event to the context. Returns true if the interface used it (a click on a button), so the game
        // should ignore it.
        bool ProcessEvent(const SDL_Event& event);

        // Rml::SystemInterface.
        double GetElapsedTime() override;
        bool LogMessage(Rml::Log::Type type, const Rml::String& message) override;
        void SetMouseCursor(const Rml::String& cursorName) override;
        void SetClipboardText(const Rml::String& text) override;
        void GetClipboardText(Rml::String& text) override;
        void ActivateKeyboard(Rml::Vector2f caretPosition, float lineHeight) override;
        void DeactivateKeyboard() override;

    private:
        // The cursors a document may ask for with the "cursor" property.
        enum class CursorShape
        {
            Arrow,
            Pointer,
            Text,
            Move,
            ResizeHorizontal,
            ResizeVertical,
            Count,
        };

        SDL_Window* m_window = nullptr;
        Rml::Context* m_context = nullptr;
        bool m_isInputEnabled = false;
        std::array<SDL_Cursor*, static_cast<std::size_t>(CursorShape::Count)> m_cursors{};
    };
}
