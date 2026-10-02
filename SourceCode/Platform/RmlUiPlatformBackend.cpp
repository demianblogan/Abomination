#include "Platform/RmlUiPlatformBackend.h"

#include "Core/Logging/Log.h"

#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Input.h>
#include <SDL3/SDL_clipboard.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_keyboard.h>
#include <SDL3/SDL_mouse.h>
#include <SDL3/SDL_timer.h>
#include <SDL3/SDL_video.h>

#include <cstddef>

namespace Abomination::Platform
{
    using Core::LogCategory;
    using Core::LogLevel;

    namespace
    {
        // The key RmlUi knows for an SDL key. SDL_Keycode is the key as the layout names it (on a German layout the key
        // left of U is Z), which is what typing and shortcuts like Ctrl+Z need. Keys RmlUi does nothing with are unknown.
        Rml::Input::KeyIdentifier ToKeyIdentifier(SDL_Keycode key)
        {
            using namespace Rml::Input;
            if (key >= SDLK_A && key <= SDLK_Z)
                return static_cast<KeyIdentifier>(KI_A + (key - SDLK_A));
            if (key >= SDLK_0 && key <= SDLK_9)
                return static_cast<KeyIdentifier>(KI_0 + (key - SDLK_0));

            switch (key)
            {
                case SDLK_SPACE: return KI_SPACE;
                case SDLK_BACKSPACE: return KI_BACK;
                case SDLK_TAB: return KI_TAB;
                case SDLK_RETURN: return KI_RETURN;
                case SDLK_KP_ENTER: return KI_NUMPADENTER;
                case SDLK_ESCAPE: return KI_ESCAPE;
                case SDLK_PAGEUP: return KI_PRIOR;
                case SDLK_PAGEDOWN: return KI_NEXT;
                case SDLK_END: return KI_END;
                case SDLK_HOME: return KI_HOME;
                case SDLK_LEFT: return KI_LEFT;
                case SDLK_UP: return KI_UP;
                case SDLK_RIGHT: return KI_RIGHT;
                case SDLK_DOWN: return KI_DOWN;
                case SDLK_INSERT: return KI_INSERT;
                case SDLK_DELETE: return KI_DELETE;
                default: return KI_UNKNOWN;
            }
        }

        // The modifier keys held now, in the bits RmlUi uses (Ctrl+A selects all text, Shift+arrows select).
        int GetKeyModifierState()
        {
            const SDL_Keymod modifiers = SDL_GetModState();
            int state = 0;
            if ((modifiers & SDL_KMOD_CTRL) != 0)
                state |= Rml::Input::KM_CTRL;
            if ((modifiers & SDL_KMOD_SHIFT) != 0)
                state |= Rml::Input::KM_SHIFT;
            if ((modifiers & SDL_KMOD_ALT) != 0)
                state |= Rml::Input::KM_ALT;
            if ((modifiers & SDL_KMOD_CAPS) != 0)
                state |= Rml::Input::KM_CAPSLOCK;
            return state;
        }

        // SDL numbers the mouse buttons 1 (left), 2 (middle), 3 (right); RmlUi 0 (left), 1 (right), 2 (middle).
        int ToMouseButtonIndex(std::uint8_t button)
        {
            switch (button)
            {
                case SDL_BUTTON_LEFT: return 0;
                case SDL_BUTTON_RIGHT: return 1;
                case SDL_BUTTON_MIDDLE: return 2;
                default: return button - 1;
            }
        }
    }

    RmlUiPlatformBackend::RmlUiPlatformBackend(SDL_Window* window)
        : m_window(window)
    {
        const auto createCursor = [this](CursorShape shape, SDL_SystemCursor systemCursor)
        {
            m_cursors[static_cast<std::size_t>(shape)] = SDL_CreateSystemCursor(systemCursor);
        };
        createCursor(CursorShape::Arrow, SDL_SYSTEM_CURSOR_DEFAULT);
        createCursor(CursorShape::Pointer, SDL_SYSTEM_CURSOR_POINTER);
        createCursor(CursorShape::Text, SDL_SYSTEM_CURSOR_TEXT);
        createCursor(CursorShape::Move, SDL_SYSTEM_CURSOR_MOVE);
        createCursor(CursorShape::ResizeHorizontal, SDL_SYSTEM_CURSOR_EW_RESIZE);
        createCursor(CursorShape::ResizeVertical, SDL_SYSTEM_CURSOR_NS_RESIZE);
    }

    RmlUiPlatformBackend::~RmlUiPlatformBackend()
    {
        for (SDL_Cursor* cursor : m_cursors)
            SDL_DestroyCursor(cursor);
    }

    void RmlUiPlatformBackend::SetContext(Rml::Context* context) noexcept
    {
        m_context = context;
    }

    void RmlUiPlatformBackend::SetInputEnabled(bool isEnabled) noexcept
    {
        // The interface forgets what was under the mouse, so nothing stays highlighted after the mouse went to the game.
        if (m_isInputEnabled && !isEnabled && m_context != nullptr)
            m_context->ProcessMouseLeave();

        m_isInputEnabled = isEnabled;
    }

    bool RmlUiPlatformBackend::ProcessEvent(const SDL_Event& event)
    {
        if (m_context == nullptr || !m_isInputEnabled)
            return false;

        // The Process functions return true while the event was NOT used by the interface (it may go on to the game).
        const int modifiers = GetKeyModifierState();
        // SDL gives the mouse position in window coordinates, which on a screen with a display scale are not pixels (at 200%
        // one coordinate is 2 pixels); the interface is laid out in pixels.
        const float pixelDensity = SDL_GetWindowPixelDensity(m_window);
        switch (event.type)
        {
            case SDL_EVENT_MOUSE_MOTION:
                return !m_context->ProcessMouseMove(static_cast<int>(event.motion.x * pixelDensity),
                                                    static_cast<int>(event.motion.y * pixelDensity), modifiers);
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
                return !m_context->ProcessMouseButtonDown(ToMouseButtonIndex(event.button.button), modifiers);
            case SDL_EVENT_MOUSE_BUTTON_UP:
                return !m_context->ProcessMouseButtonUp(ToMouseButtonIndex(event.button.button), modifiers);
            case SDL_EVENT_MOUSE_WHEEL:
                // RmlUi scrolls down for a positive value, SDL reports turning the wheel away from the user as positive.
                return !m_context->ProcessMouseWheel(Rml::Vector2f(event.wheel.x, -event.wheel.y), modifiers);
            case SDL_EVENT_KEY_DOWN:
                // Held keys repeat here (unlike in the game), so holding Backspace deletes letter after letter.
                return !m_context->ProcessKeyDown(ToKeyIdentifier(event.key.key), modifiers);
            case SDL_EVENT_KEY_UP:
                return !m_context->ProcessKeyUp(ToKeyIdentifier(event.key.key), modifiers);
            case SDL_EVENT_TEXT_INPUT:
                // The typed text, in UTF-8: the operating system has already applied the layout, Shift and dead keys.
                return !m_context->ProcessTextInput(Rml::String(event.text.text));
            case SDL_EVENT_WINDOW_MOUSE_LEAVE:
                m_context->ProcessMouseLeave();
                return false;
            default:
                return false;
        }
    }

    double RmlUiPlatformBackend::GetElapsedTime()
    {
        // Seconds since SDL started; animations and the blinking caret of a text field are timed by it.
        return static_cast<double>(SDL_GetTicksNS()) / 1'000'000'000.0;
    }

    bool RmlUiPlatformBackend::LogMessage(Rml::Log::Type type, const Rml::String& message)
    {
        LogLevel level = LogLevel::Debug;
        switch (type)
        {
            case Rml::Log::LT_ALWAYS:
            case Rml::Log::LT_INFO: level = LogLevel::Info; break;
            case Rml::Log::LT_ERROR:
            case Rml::Log::LT_ASSERT: level = LogLevel::Error; break;
            case Rml::Log::LT_WARNING: level = LogLevel::Warning; break;
            default: level = LogLevel::Debug; break;
        }

        Core::Log::Write(LogCategory::UI, level, "RmlUi: {}", message);
        return true;
    }

    void RmlUiPlatformBackend::SetMouseCursor(const Rml::String& cursorName)
    {
        // The names of the CSS "cursor" property.
        CursorShape shape = CursorShape::Arrow;
        if (cursorName == "pointer")
            shape = CursorShape::Pointer;
        else if (cursorName == "text")
            shape = CursorShape::Text;
        else if (cursorName == "move")
            shape = CursorShape::Move;
        else if (cursorName == "ew-resize")
            shape = CursorShape::ResizeHorizontal;
        else if (cursorName == "ns-resize")
            shape = CursorShape::ResizeVertical;

        SDL_SetCursor(m_cursors[static_cast<std::size_t>(shape)]);
    }

    void RmlUiPlatformBackend::SetClipboardText(const Rml::String& text)
    {
        SDL_SetClipboardText(text.c_str());
    }

    void RmlUiPlatformBackend::GetClipboardText(Rml::String& text)
    {
        // SDL allocates the text; it must be freed with SDL_free.
        char* clipboardText = SDL_GetClipboardText();
        text = clipboardText != nullptr ? clipboardText : "";
        SDL_free(clipboardText);
    }

    void RmlUiPlatformBackend::ActivateKeyboard(Rml::Vector2f caretPosition, float lineHeight)
    {
        // A text field got the focus: SDL starts sending typed text (SDL_EVENT_TEXT_INPUT), and the input method of
        // languages like Chinese opens next to the caret.
        const SDL_Rect area{static_cast<int>(caretPosition.x), static_cast<int>(caretPosition.y), 1,
                            static_cast<int>(lineHeight)};
        SDL_SetTextInputArea(m_window, &area, 0);
        SDL_StartTextInput(m_window);
    }

    void RmlUiPlatformBackend::DeactivateKeyboard()
    {
        SDL_StopTextInput(m_window);
    }
}
