#pragma once

#include <bitset>
#include <cstddef>
#include <cstdint>

namespace Abomination::Input
{
    // A physical key, identified by its position on the keyboard (scancode), not by the character printed on it:
    // Key::W is the key to the right of Q even on a French AZERTY keyboard, where it prints "Z". This is what games
    // need for movement keys.
    //
    // The values are the USB HID usage codes of the keys. SDL uses the same codes for its scancodes, so an SDL
    // scancode converts to Key with a simple cast. New keys are added here when the game starts using them.
    enum class Key : std::uint16_t
    {
        // Letters
        A = 4,
        D = 7,
        E = 8,
        Q = 20,
        S = 22,
        W = 26,

        // Control keys
        Enter = 40,
        Escape = 41,
        Grave = 53, // ` and ~, left of 1 (Ё on a Russian keyboard)

        // Function keys
        F1 = 58,

        // Modifiers
        LeftShift = 225,
        LeftAlt = 226,
        RightAlt = 230,
    };

    // Number of possible scancodes. Must match SDL_SCANCODE_COUNT; Platform/Window.cpp checks this with a static_assert.
    inline constexpr std::size_t KeyCount = 512;

    // State of every key of the keyboard for the current frame.
    //
    // The platform layer (Platform::Window::ProcessEvents) fills it from the operating system events once per frame;
    // the game only reads it. This class knows nothing about SDL or the operating system. A key can be:
    //   held     - it is down right now (true for every frame while it is held);
    //   pressed  - it went down during this frame (true for exactly one frame);
    //   released - it went up during this frame (true for exactly one frame).
    // A toggle such as "F1 shows the overlay" uses WasKeyPressed(), movement uses IsKeyHeld().
    class Keyboard
    {
    public:
        // Starts a new frame: forgets which keys were pressed or released during the previous frame.
        // Held keys stay held. Called by the platform layer before it processes the events.
        void StartFrame() noexcept;

        // Called by the platform layer when a key goes down. A key that is already held is ignored, so the repeated "down"
        // events the operating system sends while a key is held do not count as new presses.
        void PressKey(Key key) noexcept;

        // Called by the platform layer when a key goes up.
        void ReleaseKey(Key key) noexcept;

        // Releases every held key. Called when the window loses the focus (Alt+Tab): the "up" events of keys
        // released in another window never reach the game, and without this they would stay held forever.
        void ReleaseAllKeys() noexcept;

        [[nodiscard]] bool IsKeyHeld(Key key) const noexcept;
        [[nodiscard]] bool WasKeyPressed(Key key) const noexcept;
        [[nodiscard]] bool WasKeyReleased(Key key) const noexcept;

    private:
        // One bit per key; the index is the numeric value of Key.
        std::bitset<KeyCount> m_heldKeys;
        std::bitset<KeyCount> m_pressedKeys;
        std::bitset<KeyCount> m_releasedKeys;
    };
}
