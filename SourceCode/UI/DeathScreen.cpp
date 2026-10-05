#include "UI/DeathScreen.h"

#include "Core/Logging/Log.h"
#include "Gameplay/GameplayState.h"
#include "Gameplay/Player/PlayerDeath.h"
#include "UI/StyleValues.h"

#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>

#include <algorithm>
#include <cmath>
#include <format>
#include <numbers>
#include <string>

namespace Abomination::UI
{
    namespace
    {
        // How GAME OVER appears: in TitleTime it comes out of the dark and shrinks from TitleStartScale to its size,
        // slowing down at the end.
        constexpr float TitleTime = 0.8f;
        constexpr float TitleStartScale = 1.15f;

        // The glow behind it comes up with it, then breathes between GlowLowOpacity and GlowHighOpacity, once every
        // GlowBreathTime (slower than the hint, like embers).
        constexpr float GlowLowOpacity = 0.35f;
        constexpr float GlowHighOpacity = 0.8f;
        constexpr float GlowBreathTime = 3.0f;

        // The hint fades in in HintFadeTime, then breathes between HintLowOpacity and 1, once every HintBreathTime.
        constexpr float HintFadeTime = 0.4f;
        constexpr float HintLowOpacity = 0.45f;
        constexpr float HintBreathTime = 2.0f;

        // How high an eyelid is, in percent of the screen (DeathScreen.rcss): fully open it is just above (or below)
        // the screen, closed it covers this much of it from its edge.
        constexpr float EyelidHeight = 60.0f;
    }

    DeathScreen DeathScreen::Load(Rml::Context& context, const std::string& documentPath)
    {
        DeathScreen screen;
        screen.m_document = context.LoadDocument(documentPath);
        if (screen.m_document == nullptr)
        {
            Core::Log::Write(Core::LogCategory::UI, Core::LogLevel::Warning, "Death screen not loaded: {}", documentPath);
            return screen;
        }

        Rml::ElementDocument& document = *screen.m_document;
        screen.m_vignette = document.GetElementById("vignette");
        screen.m_upperEyelid = document.GetElementById("eyelid-top");
        screen.m_lowerEyelid = document.GetElementById("eyelid-bottom");
        screen.m_black = document.GetElementById("black");
        screen.m_message = document.GetElementById("message");
        screen.m_title = document.GetElementById("title");
        screen.m_titleGlow = document.GetElementById("title-glow");
        screen.m_hint = document.GetElementById("hint");
        return screen;
    }

    void DeathScreen::Update(const Gameplay::GameplayState& gameplay)
    {
        if (m_document == nullptr)
            return;

        const Gameplay::PlayerDeath& death = gameplay.playerDeath;
        SetShown(*m_document, death.isDead);
        if (!death.isDead)
            return;

        // The heavy eyes: the darkened edges, the eyelids coming together (an eyelid moves from just off the screen,
        // -60%, to its edge, 0%), and plain black once they have met.
        SetOpacity(m_vignette, Gameplay::CalculateDeathVignette(death));
        const float closed = Gameplay::CalculateEyesClosed(death);
        const std::string eyelidPosition = std::format("{:.2f}%", -EyelidHeight * (1.0f - closed));
        if (m_upperEyelid != nullptr)
            m_upperEyelid->SetProperty("top", eyelidPosition);
        if (m_lowerEyelid != nullptr)
            m_lowerEyelid->SetProperty("bottom", eyelidPosition);
        SetOpacity(m_black, std::clamp((closed - 0.9f) * 10.0f, 0.0f, 1.0f));

        // GAME OVER on the black, then the hint.
        const bool isShown = Gameplay::IsGameOverShown(death);
        if (m_message != nullptr)
            m_message->SetProperty("display", isShown ? "block" : "none");
        if (!isShown)
            return;

        const Gameplay::PlayerDeathSettings& settings = death.settings;
        const float sinceTitle = death.time - (settings.eyesCloseStart + settings.eyesCloseTime);
        const float appear = std::clamp(sinceTitle / TitleTime, 0.0f, 1.0f);
        const float settle = 1.0f - (1.0f - appear) * (1.0f - appear); // fast at first, slowing down at the end
        if (m_title != nullptr)
        {
            SetOpacity(m_title, appear);
            const float scale = TitleStartScale + (1.0f - TitleStartScale) * settle;
            m_title->SetProperty("transform", std::format("scale({:.4f})", scale));
        }

        // The glow: up with the title, then slowly breathing (a cosine wave, starting at its brightest).
        const float glowBreath = 0.5f + 0.5f * std::cos(2.0f * std::numbers::pi_v<float> * sinceTitle / GlowBreathTime);
        SetOpacity(m_titleGlow, appear * (GlowLowOpacity + (GlowHighOpacity - GlowLowOpacity) * glowBreath));

        // The hint: faded in, then breathing (a sine wave between the low opacity and 1).
        const float sinceHint = sinceTitle - settings.hintDelay;
        float hintOpacity = 0.0f;
        if (sinceHint > 0.0f)
        {
            const float breath = 0.5f + 0.5f * std::cos(2.0f * std::numbers::pi_v<float> * sinceHint / HintBreathTime);
            hintOpacity = std::min(sinceHint / HintFadeTime, 1.0f) * (HintLowOpacity + (1.0f - HintLowOpacity) * breath);
        }
        SetOpacity(m_hint, hintOpacity);
    }
}
