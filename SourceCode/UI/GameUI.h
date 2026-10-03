#pragma once

#include <entt/entt.hpp>
#include <glm/vec2.hpp>

#include <expected>
#include <filesystem>
#include <memory>
#include <string>

namespace Rml
{
    class Context;
}

namespace Abomination::Gameplay
{
    struct GameplayState;
}

namespace Abomination::Platform
{
    class RmlUiPlatformBackend;
    class Window;
}

namespace Abomination::Renderer
{
    class GLShaderProgram;
    class RmlUiRendererBackend;
}

namespace Abomination::UI
{
    class HUD;

    // The game interface (the HUD, the death screen, later the menus), built with RmlUi: documents written like web pages
    // (.rml, like HTML) with style sheets (.rcss, like CSS) in Assets/UI. RmlUi lays them out, handles the mouse and the
    // keyboard, and draws them through the two backends (Platform for SDL, Renderer for OpenGL), the way the debug overlay
    // uses Dear ImGui.
    //
    // Sizes in the documents are given in dp ("density-independent pixels") for a window 1080 pixels high; the interface
    // scales with the height of the window, so it fills the same share of the screen at any resolution.
    //
    // RmlUi has one global state, so only one GameUI may exist. Move-only.
    class GameUI
    {
    public:
        // Starts RmlUi, loads the fonts of the game and the documents. assetsDirectory holds Fonts/ and UI/. The window
        // (and its OpenGL context) must live longer than the GameUI: RmlUi gives its textures back when it shuts down.
        [[nodiscard]] static std::expected<GameUI, std::string> Create(const Platform::Window& window,
                                                                       const std::filesystem::path& assetsDirectory);

        GameUI(const GameUI&) = delete;
        GameUI& operator=(const GameUI&) = delete;

        GameUI(GameUI&& other) noexcept;
        GameUI& operator=(GameUI&& other) noexcept;

        ~GameUI();

        // Once per frame: the size of the window in pixels (the layout follows it), and whether the interface gets the
        // mouse and the keyboard (only while the cursor is free). Lets RmlUi update its animations and layout.
        void Update(glm::vec2 viewportSize, bool isInputEnabled);

        // Once per frame, before Update: the HUD shows the state of the player (see HUD).
        void UpdateHUD(const Gameplay::GameplayState& gameplay, const entt::registry& registry, glm::vec2 viewportSize,
                       float deltaTime);

        // Draws the visible documents over the game with the program (Shaders/GameUI). Returns the number of draw calls.
        int Render(glm::vec2 viewportSize, const Renderer::GLShaderProgram& program);

        // For Window::SetRmlUiBackend: the window passes its keyboard and mouse events to it.
        [[nodiscard]] Platform::RmlUiPlatformBackend* GetPlatformBackend() const noexcept;

    private:
        GameUI();

        void Destroy() noexcept;

        // RmlUi keeps pointers to both backends, so they live on the heap and keep their address when GameUI moves.
        std::unique_ptr<Platform::RmlUiPlatformBackend> m_platformBackend;
        std::unique_ptr<Renderer::RmlUiRendererBackend> m_rendererBackend;

        // Owned by RmlUi; destroyed with it in Rml::Shutdown().
        Rml::Context* m_context = nullptr;

        // On the heap: its data model keeps the addresses of its values (see HUD::Load).
        std::unique_ptr<HUD> m_hud;
    };
}
