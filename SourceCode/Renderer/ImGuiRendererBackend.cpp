#include "Renderer/ImGuiRendererBackend.h"

#include "Renderer/OpenGL/GPUProfileZone.h"

#include <imgui.h>
#include <imgui_impl_opengl3.h>

#include <utility>

namespace Abomination::Renderer
{
    std::expected<ImGuiRendererBackend, std::string> ImGuiRendererBackend::Initialize()
    {
        // The backend compiles its own small shaders; this line is put at the top of them.
        if (!ImGui_ImplOpenGL3_Init("#version 460 core"))
            return std::unexpected("Failed to initialize the OpenGL backend of ImGui");

        ImGuiRendererBackend backend;
        backend.m_isActive = true;

        return backend;
    }

    ImGuiRendererBackend::ImGuiRendererBackend(ImGuiRendererBackend&& other) noexcept
        : m_isActive(std::exchange(other.m_isActive, false))
    {}

    ImGuiRendererBackend& ImGuiRendererBackend::operator=(ImGuiRendererBackend&& other) noexcept
    {
        if (this != &other)
        {
            if (m_isActive)
                ImGui_ImplOpenGL3_Shutdown();

            m_isActive = std::exchange(other.m_isActive, false);
        }

        return *this;
    }

    ImGuiRendererBackend::~ImGuiRendererBackend()
    {
        if (m_isActive)
            ImGui_ImplOpenGL3_Shutdown();
    }

    void ImGuiRendererBackend::StartFrame()
    {
        ImGui_ImplOpenGL3_NewFrame();
    }

    void ImGuiRendererBackend::DrawFrame()
    {
        PROFILE_GPU_ZONE("Debug overlay");

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }
}
