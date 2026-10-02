#include "UI/GameUI.h"

#include "Core/Files/FileSystem.h"
#include "Core/Logging/Log.h"
#include "Platform/RmlUiPlatformBackend.h"
#include "Platform/Window.h"
#include "Renderer/RmlUiRendererBackend.h"

#include <RmlUi/Core.h>

#include <array>
#include <format>
#include <string_view>
#include <utility>

namespace Abomination::UI
{
    using Core::LogCategory;
    using Core::LogLevel;

    namespace
    {
        // The fonts of the game (relative to the assets folder): Oswald for the HUD and every ordinary text, Cormorant SC
        // for titles. Documents choose them by their family names, "Oswald" and "Cormorant SC". Both have every letter of
        // the five languages of the game (a test checks it).
        constexpr std::array<std::string_view, 2> FontPaths = {"Fonts/OswaldBold.ttf", "Fonts/CormorantSCBold.ttf"};

        // The window height the sizes in the documents are written for: 1 dp is 1 pixel in a window 1080 pixels high.
        constexpr float ReferenceHeight = 1080.0f;
    }

    std::expected<GameUI, std::string> GameUI::Create(const Platform::Window& window,
                                                  const std::filesystem::path& assetsDirectory)
    {
        GameUI gameUI;
        gameUI.m_platformBackend = std::make_unique<Platform::RmlUiPlatformBackend>(window.GetSDLWindow());
        gameUI.m_rendererBackend = std::make_unique<Renderer::RmlUiRendererBackend>();

        // The backends are given before RmlUi starts, as it asks.
        Rml::SetSystemInterface(gameUI.m_platformBackend.get());
        Rml::SetRenderInterface(gameUI.m_rendererBackend.get());
        if (!Rml::Initialise())
            return std::unexpected("Failed to start RmlUi");

        for (const std::string_view fontPath : FontPaths)
        {
            if (!Rml::LoadFontFace(Core::ToUTF8String(assetsDirectory / fontPath)))
            {
                Rml::Shutdown();
                return std::unexpected(std::format("Failed to load the font {}", fontPath));
            }
        }

        // A context is one screen of documents; the game has one, as big as the window.
        const Rml::Vector2i size(window.GetWidthInPixels(), window.GetHeightInPixels());
        gameUI.m_context = Rml::CreateContext("game", size);
        if (gameUI.m_context == nullptr)
        {
            Rml::Shutdown();
            return std::unexpected("Failed to create the RmlUi context");
        }

        gameUI.m_platformBackend->SetContext(gameUI.m_context);

        Core::Log::Write(LogCategory::UI, LogLevel::Info, "Game interface started (RmlUi {})", Rml::GetVersion());
        return gameUI;
    }

    GameUI::GameUI() = default;

    GameUI::GameUI(GameUI&& other) noexcept
        : m_platformBackend(std::move(other.m_platformBackend))
        , m_rendererBackend(std::move(other.m_rendererBackend))
        , m_context(std::exchange(other.m_context, nullptr))
    {}

    GameUI& GameUI::operator=(GameUI&& other) noexcept
    {
        if (this != &other)
        {
            Destroy();
            m_platformBackend = std::move(other.m_platformBackend);
            m_rendererBackend = std::move(other.m_rendererBackend);
            m_context = std::exchange(other.m_context, nullptr);
        }

        return *this;
    }

    GameUI::~GameUI()
    {
        Destroy();
    }

    void GameUI::Destroy() noexcept
    {
        // A moved-from object has no context and must not shut RmlUi down.
        if (m_context == nullptr)
            return;

        // RmlUi destroys its documents and gives back every texture and piece of geometry to the renderer backend, which
        // is destroyed only after that.
        Rml::Shutdown();
        m_context = nullptr;
    }

    void GameUI::Update(glm::vec2 viewportSize, bool isInputEnabled)
    {
        const Rml::Vector2i size(static_cast<int>(viewportSize.x), static_cast<int>(viewportSize.y));
        if (m_context->GetDimensions() != size)
            m_context->SetDimensions(size);

        // 1 dp = 1 pixel at 1080 pixels of height: 2 pixels on a 4K screen, 0.67 at 720.
        m_context->SetDensityIndependentPixelRatio(viewportSize.y / ReferenceHeight);

        m_platformBackend->SetInputEnabled(isInputEnabled);
        m_context->Update();
    }

    int GameUI::Render(glm::vec2 viewportSize, const Renderer::GLShaderProgram& program)
    {
        m_rendererBackend->BeginFrame(viewportSize, program);
        m_context->Render();
        m_rendererBackend->EndFrame();
        return m_rendererBackend->GetDrawCallCount();
    }

    Platform::RmlUiPlatformBackend* GameUI::GetPlatformBackend() const noexcept
    {
        return m_platformBackend.get();
    }
}
