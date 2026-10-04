#include "Input/Mouse.h"

#include <algorithm>
#include <utility>

namespace Abomination::Input
{
    namespace
    {
        std::size_t ConvertToIndex(MouseButton button) noexcept
        {
            return std::to_underlying(button);
        }

        // The platform layer casts the button number of SDL to MouseButton without checking it. A mouse with more buttons
        // can report numbers above Forward (5), which have no slot in the arrays; writing there would go past their end.
        bool IsKnownButton(std::size_t index) noexcept
        {
            return index >= std::to_underlying(MouseButton::Left) && index <= std::to_underlying(MouseButton::Forward);
        }
    }

    void Mouse::StartFrame() noexcept
    {
        m_movement = glm::vec2(0.0f, 0.0f);
        m_wheelMovement = 0.0f;
        m_pressedButtons.fill(false);
        m_releasedButtons.fill(false);
    }

    void Mouse::Move(glm::vec2 offset) noexcept
    {
        m_movement += offset;
    }

    void Mouse::Scroll(float delta) noexcept
    {
        m_wheelMovement += delta;
    }

    void Mouse::PressButton(MouseButton button) noexcept
    {
        const std::size_t index = ConvertToIndex(button);
        if (!IsKnownButton(index) || m_heldButtons[index])
            return;

        m_heldButtons[index] = true;
        m_pressedButtons[index] = true;
    }

    void Mouse::ReleaseButton(MouseButton button) noexcept
    {
        const std::size_t index = ConvertToIndex(button);
        if (!IsKnownButton(index) || !m_heldButtons[index])
            return;

        m_heldButtons[index] = false;
        m_releasedButtons[index] = true;
    }

    void Mouse::ReleaseAllButtons() noexcept
    {
        for (std::size_t index = 0; index < ButtonSlotCount; ++index)
        {
            if (m_heldButtons[index])
            {
                m_heldButtons[index] = false;
                m_releasedButtons[index] = true;
            }
        }
    }

    glm::vec2 Mouse::GetMovement() const noexcept
    {
        return m_movement;
    }

    float Mouse::GetWheelMovement() const noexcept
    {
        return m_wheelMovement;
    }

    bool Mouse::IsButtonHeld(MouseButton button) const noexcept
    {
        const std::size_t index = ConvertToIndex(button);
        return IsKnownButton(index) && m_heldButtons[index];
    }

    bool Mouse::WasButtonPressed(MouseButton button) const noexcept
    {
        const std::size_t index = ConvertToIndex(button);
        return IsKnownButton(index) && m_pressedButtons[index];
    }

    bool Mouse::WasButtonReleased(MouseButton button) const noexcept
    {
        const std::size_t index = ConvertToIndex(button);
        return IsKnownButton(index) && m_releasedButtons[index];
    }

    bool Mouse::WasAnyButtonPressed() const noexcept
    {
        return std::ranges::any_of(m_pressedButtons, [](bool isPressed) { return isPressed; });
    }
}
