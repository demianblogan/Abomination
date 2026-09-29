#include "UI/AssetsWindow.h"

#include "Core/Assets/AssetLifetime.h"
#include "Renderer/Assets/RenderAssets.h"
#include "UI/UIScale.h"

#include <imgui.h>

#include <cstddef>
#include <format>
#include <string>

namespace Abomination::UI
{
    namespace
    {
        // The window opens for the first time to the right of the Performance window; later ImGui restores the position
        // and size it was left at.
        constexpr ImVec2 InitialPosition(450.0f, 40.0f);
        constexpr ImVec2 InitialSize(620.0f, 360.0f);

        // The color of assets replaced by a fallback: the same magenta as the fallbacks themselves.
        constexpr ImVec4 FallbackTextColor(1.0f, 0.0f, 1.0f, 1.0f);

        // The Status cell of an asset: "Loaded", or a magenta "Fallback" with a tooltip that explains it.
        void DrawAssetStatus(bool isFallback)
        {
            if (!isFallback)
            {
                ImGui::TextUnformatted("Loaded");

                return;
            }

            ImGui::TextColored(FallbackTextColor, "Fallback");
            ImGui::SetItemTooltip("The file is missing or broken; the log says why.");
        }

        // "512 B", "21.3 KB" or "4.0 MB": a size in bytes for people to read.
        std::string FormatByteSize(std::size_t byteCount)
        {
            constexpr double BytesPerKilobyte = 1024.0;
            constexpr double BytesPerMegabyte = BytesPerKilobyte * 1024.0;

            const auto bytes = static_cast<double>(byteCount);
            if (bytes >= BytesPerMegabyte)
                return std::format("{:.1f} MB", bytes / BytesPerMegabyte);
            if (bytes >= BytesPerKilobyte)
                return std::format("{:.1f} KB", bytes / BytesPerKilobyte);

            return std::format("{} B", byteCount);
        }

        void DrawTextureSection(const Renderer::TextureStore& textures)
        {
            // The header shows the totals, so they are summed before the list is drawn.
            std::size_t totalMemory = 0;
            textures.VisitTextures([&](const std::string&, const Renderer::GLTexture& texture, bool, Core::AssetLifetime)
            {
                totalMemory += texture.GetVideoMemorySize();
            });

            // A collapsing header is a clickable bar that shows or hides what follows it. The text after "###" is the ID
            // ImGui remembers the header by: the visible label changes with the numbers, the ID must stay the same.
            const std::string header =
                std::format("Textures: {}, {} of video memory###Textures", textures.GetCount(), FormatByteSize(totalMemory));
            if (!ImGui::CollapsingHeader(header.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
                return;

            // A table: columns are set up once, then every row is filled cell by cell with TableNextColumn().
            // RowBg alternates the row background, Borders draws the lines between cells.
            if (!ImGui::BeginTable("TextureTable", 5, ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders))
                return;

            // The path takes all the width the other columns leave; the others are as wide as their contents.
            ImGui::TableSetupColumn("Path", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableSetupColumn("Video memory", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableSetupColumn("Status", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableSetupColumn("Lifetime", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableHeadersRow();

            textures.VisitTextures([](const std::string& path, const Renderer::GLTexture& texture, bool isFallback,
                                      Core::AssetLifetime lifetime)
            {
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(path.c_str());

                ImGui::TableNextColumn();
                const std::string sizeText = std::format("{}x{}", texture.GetWidth(), texture.GetHeight());
                ImGui::TextUnformatted(sizeText.c_str());

                ImGui::TableNextColumn();
                ImGui::TextUnformatted(FormatByteSize(texture.GetVideoMemorySize()).c_str());

                ImGui::TableNextColumn();
                DrawAssetStatus(isFallback);

                ImGui::TableNextColumn();
                ImGui::TextUnformatted(Core::GetAssetLifetimeName(lifetime).data());
            });

            ImGui::EndTable();
        }

        void DrawMeshSection(const Renderer::MeshStore& meshes)
        {
            std::size_t totalMemory = 0;
            meshes.VisitMeshes([&](const std::string&, const Renderer::Mesh& mesh, Core::AssetLifetime)
            {
                totalMemory += mesh.GetVideoMemorySize();
            });

            const std::string header =
                std::format("Meshes: {}, {} of video memory###Meshes", meshes.GetCount(), FormatByteSize(totalMemory));
            if (!ImGui::CollapsingHeader(header.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
                return;

            if (!ImGui::BeginTable("MeshTable", 5, ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders))
                return;

            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Vertices", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableSetupColumn("Triangles", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableSetupColumn("Video memory", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableSetupColumn("Lifetime", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableHeadersRow();

            meshes.VisitMeshes([](const std::string& name, const Renderer::Mesh& mesh, Core::AssetLifetime lifetime)
            {
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(name.c_str());

                ImGui::TableNextColumn();
                ImGui::TextUnformatted(std::format("{}", mesh.GetVertexCount()).c_str());

                // Every 3 indices are one triangle.
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(std::format("{}", mesh.GetIndexCount() / 3).c_str());

                ImGui::TableNextColumn();
                ImGui::TextUnformatted(FormatByteSize(mesh.GetVideoMemorySize()).c_str());

                ImGui::TableNextColumn();
                ImGui::TextUnformatted(Core::GetAssetLifetimeName(lifetime).data());
            });

            ImGui::EndTable();
        }

        void DrawModelSection(const Renderer::ModelStore& models)
        {
            const std::string header = std::format("Models: {}###Models", models.GetCount());
            if (!ImGui::CollapsingHeader(header.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
                return;

            if (!ImGui::BeginTable("ModelTable", 4, ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders))
                return;

            // The meshes and textures of a model are listed in their own sections, named after the model file.
            ImGui::TableSetupColumn("Path", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Parts", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableSetupColumn("Status", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableSetupColumn("Lifetime", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableHeadersRow();

            models.VisitModels([](const std::string& path, const Renderer::Model& model, bool isFallback,
                                  Core::AssetLifetime lifetime)
            {
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(path.c_str());

                ImGui::TableNextColumn();
                ImGui::TextUnformatted(std::format("{}", model.parts.size()).c_str());

                ImGui::TableNextColumn();
                DrawAssetStatus(isFallback);

                ImGui::TableNextColumn();
                ImGui::TextUnformatted(Core::GetAssetLifetimeName(lifetime).data());
            });

            ImGui::EndTable();
        }

        void DrawShaderProgramSection(const Renderer::ShaderStore& shaders)
        {
            const std::string header = std::format("Shader programs: {}###ShaderPrograms", shaders.GetCount());
            if (!ImGui::CollapsingHeader(header.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
                return;

            if (!ImGui::BeginTable("ShaderProgramTable", 2, ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders))
                return;

            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Status", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableHeadersRow();

            shaders.VisitPrograms([](const std::string& name, bool isFallback)
            {
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(name.c_str());

                ImGui::TableNextColumn();
                DrawAssetStatus(isFallback);
            });

            ImGui::EndTable();
        }
    }

    void DrawAssetsWindow(bool* isOpen, const Renderer::RenderAssets& assets)
    {
        // ImGuiCond_FirstUseEver applies the position and size only when the settings file does not know the window yet:
        // afterwards the window opens where it was left.
        ImGui::SetNextWindowPos(ScaleToUI(InitialPosition), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ScaleToUI(InitialSize), ImGuiCond_FirstUseEver);

        if (ImGui::Begin("Assets", isOpen))
        {
            DrawTextureSection(assets.textures);
            DrawMeshSection(assets.meshes);
            DrawModelSection(assets.models);
            DrawShaderProgramSection(assets.shaders);
        }
        ImGui::End();
    }
}
