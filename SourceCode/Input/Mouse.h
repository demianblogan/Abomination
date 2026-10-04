#pragma once

#include <glm/vec2.hpp>

#include <array>
#include <cstddef>
#include <cstdint>

namespace Abomination::Input
{
    // The values are the button numbers SDL uses (SDL_BUTTON_LEFT = 1, ...), so an SDL button converts with a cast.
    enum class MouseButton : std::uint8_t
    {
        Left = 1,
        Middle = 2,
        Right = 3,
        Back = 4,    // The side button closer to the user ("X1")
        Forward = 5, // The side button farther from the user ("X2")
    };

    // State of the mouse for the current frame. Like Keyboard, it is filled by the platform layer
    // (Platform::Window::ProcessEvents) and only read by the game; it knows nothing about SDL.
    //
    //   movement - how far the mouse moved during this frame, in pixels of mouse movement. +X is right, +Y is DOWN
    //              (screen convention). It is not the cursor position: while the mouse is captured (relative mode)
    //              the cursor is hidden and the movement is not limited by the edges of the screen.
    //   wheel    - how far the wheel turned during this frame (a delta), positive away from the user. The unit is one
    //              notch: the "click" felt when turning a regular wheel, which gives exactly +1 or -1. Precise wheels,
    //              free-spinning wheels and touchpads give fractions (0.15); a fast spin sums up to several notches
    //              in one frame (3.0). There is no fixed range.
    //   buttons  - held / pressed this frame / released this frame, the same states as keys.
    class Mouse
    {
    public:
        // Starts a new frame: movement and wheel go back to zero, pressed/released flags are cleared.
        void StartFrame() noexcept;

        // Called by the platform layer for every movement event; several events in one frame add up.
        void Move(glm::vec2 offset) noexcept;

        // Called by the platform layer for every wheel event; delta is in notches (see the class comment), and
        // several events in one frame add up.
        void Scroll(float delta) noexcept;

        void PressButton(MouseButton button) noexcept;
        void ReleaseButton(MouseButton button) noexcept;

        // Releases every held button, for example when the window loses the focus.
        void ReleaseAllButtons() noexcept;

        [[nodiscard]] glm::vec2 GetMovement() const noexcept;
        [[nodiscard]] float GetWheelMovement() const noexcept;

        [[nodiscard]] bool IsButtonHeld(MouseButton button) const noexcept;
        [[nodiscard]] bool WasButtonPressed(MouseButton button) const noexcept;
        [[nodiscard]] bool WasButtonReleased(MouseButton button) const noexcept;

        // Some button went down in this frame.
        [[nodiscard]] bool WasAnyButtonPressed() const noexcept;

    private:
        // Index 0 is unused: button values start at 1.
        static constexpr std::size_t ButtonSlotCount = 6;

        glm::vec2 m_movement{0.0f, 0.0f};
        float m_wheelMovement = 0.0f;

        std::array<bool, ButtonSlotCount> m_heldButtons{};
        std::array<bool, ButtonSlotCount> m_pressedButtons{};
        std::array<bool, ButtonSlotCount> m_releasedButtons{};
    };
}
