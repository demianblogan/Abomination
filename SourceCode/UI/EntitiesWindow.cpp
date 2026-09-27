#include "UI/EntitiesWindow.h"

#include "Core/Name.h"
#include "Core/Transform.h"
#include "Core/TransformInterpolation.h"
#include "Gameplay/MouseLook.h"
#include "Gameplay/Spin.h"
#include "Renderer/CameraLens.h"
#include "Renderer/MeshRenderer.h"
#include "Renderer/RenderAssets.h"
#include "UI/UIScale.h"

#include <glm/gtc/quaternion.hpp>
#include <glm/trigonometric.hpp>
#include <glm/vec3.hpp>
#include <imgui.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <format>
#include <string>

namespace Abomination::UI
{
    namespace
    {
        // The window opens for the first time below the Assets window; later ImGui restores where it was left.
        constexpr ImVec2 InitialPosition(450.0f, 420.0f);
        constexpr ImVec2 InitialSize(620.0f, 420.0f);

        // Width of the entity list on the left; the components take the rest. The border between them can be dragged.
        constexpr float EntityListWidth = 220.0f;

        // Space between the entries of the module legend.
        constexpr float LegendSpacing = 16.0f;

        // How much a value changes per pixel of mouse movement when it is dragged in a DragFloat field.
        constexpr float PositionDragSpeed = 0.01f;   // meters
        constexpr float RotationDragSpeed = 0.5f;    // degrees
        constexpr float ScaleDragSpeed = 0.01f;
        constexpr float SmallestScale = 0.01f;       // a scale of 0 would squash the mesh to nothing

        // Header colors of the component sections, one per module (see ComponentModule below). Muted colors keep the white
        // header text readable: steel for the basics, teal for drawing, amber for game rules.
        constexpr ImVec4 CoreModuleColor(0.33f, 0.38f, 0.46f, 1.0f);
        constexpr ImVec4 RendererModuleColor(0.10f, 0.42f, 0.42f, 1.0f);
        constexpr ImVec4 GameplayModuleColor(0.55f, 0.37f, 0.10f, 1.0f);

        // How much brighter a header gets under the mouse and while it is being clicked.
        constexpr float HoveredHeaderBrightness = 1.25f;
        constexpr float ClickedHeaderBrightness = 1.45f;

        // The entity index without its version: the number people see ("#3"). entt::to_entity takes the index part out
        // of the entity value, which also holds the version (like the generation of our AssetHandle).
        std::uint32_t GetEntityNumber(entt::entity entity)
        {
            return static_cast<std::uint32_t>(entt::to_entity(entity));
        }

        // "#3 Crate", or "#3" for an entity without a Name.
        std::string FormatEntityLabel(const entt::registry& registry, entt::entity entity)
        {
            const Core::Name* name = registry.try_get<Core::Name>(entity);
            if (name == nullptr)
                return std::format("#{}", GetEntityNumber(entity));

            return std::format("#{} {}", GetEntityNumber(entity), name->value);
        }

        // The name of an asset, or a note that the handle points to nothing (the stores then give a fallback).
        const char* FormatAssetName(const std::string* name)
        {
            return name != nullptr ? name->c_str() : "(invalid handle: fallback)";
        }

        // --- Section headers, colored by the module the component belongs to ---

        // The modules components come from. Every module has its own header color, so the inspector of an entity shows at
        // a glance which parts of the engine it is made of (later Physics and AI get colors too).
        enum class ComponentModule
        {
            Core,
            Renderer,
            Gameplay,
        };

        constexpr std::array AllComponentModules{ComponentModule::Core, ComponentModule::Renderer, ComponentModule::Gameplay};

        const char* GetModuleName(ComponentModule module)
        {
            switch (module)
            {
                case ComponentModule::Core:
                    return "Core";
                case ComponentModule::Renderer:
                    return "Renderer";
                case ComponentModule::Gameplay:
                    return "Gameplay";
            }

            return "";
        }

        ImVec4 GetModuleColor(ComponentModule module)
        {
            switch (module)
            {
                case ComponentModule::Core:
                    return CoreModuleColor;
                case ComponentModule::Renderer:
                    return RendererModuleColor;
                case ComponentModule::Gameplay:
                    return GameplayModuleColor;
            }

            // Never reached: every module is handled above. The compiler still wants a return value after the switch.
            return CoreModuleColor;
        }

        // The same color, brighter by factor (for hovered and clicked headers).
        ImVec4 Brighten(ImVec4 color, float factor)
        {
            return ImVec4(std::min(color.x * factor, 1.0f), std::min(color.y * factor, 1.0f), std::min(color.z * factor, 1.0f),
                          color.w);
        }

        // A collapsing header in the color of the module, with a tooltip saying which module the component is from and what
        // it does. Returns true while the section is open.
        bool DrawComponentHeader(const char* label, ComponentModule module, const char* description)
        {
            // PushStyleColor changes a color of the ImGui style until the matching PopStyleColor, so only this header is
            // affected. A header has three colors: normal, under the mouse, and while it is being clicked.
            const ImVec4 color = GetModuleColor(module);
            ImGui::PushStyleColor(ImGuiCol_Header, color);
            ImGui::PushStyleColor(ImGuiCol_HeaderHovered, Brighten(color, HoveredHeaderBrightness));
            ImGui::PushStyleColor(ImGuiCol_HeaderActive, Brighten(color, ClickedHeaderBrightness));
            const bool isOpen = ImGui::CollapsingHeader(label, ImGuiTreeNodeFlags_DefaultOpen);
            ImGui::PopStyleColor(3);

            ImGui::SetItemTooltip("%s component: %s", GetModuleName(module), description);

            return isOpen;
        }

        // A row of colored squares with the module names, so the colors never have to be remembered.
        void DrawModuleLegend()
        {
            for (const ComponentModule module : AllComponentModules)
            {
                // ColorButton draws a small square of a color; the flags turn off its own tooltip and color picker.
                constexpr ImGuiColorEditFlags SquareFlags = ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoPicker;
                const float squareSize = ImGui::GetTextLineHeight();
                ImGui::ColorButton(GetModuleName(module), GetModuleColor(module), SquareFlags,
                                   ImVec2(squareSize, squareSize));
                ImGui::SameLine();
                ImGui::TextUnformatted(GetModuleName(module));
                ImGui::SameLine(0.0f, ScaleToUI(LegendSpacing));
            }
            ImGui::NewLine();
        }

        // --- One drawing function per component type ---

        void DrawTransform(Core::Transform& transform)
        {
            // DragFloat3 edits 3 floats in a row: drag with the mouse to change them, double-click to type a value.
            // glm::vec3 stores x, y, z next to each other, so &position.x is exactly what it needs.
            ImGui::DragFloat3("Position", &transform.position.x, PositionDragSpeed);

            // People cannot read quaternions, so the rotation is shown as three angles in degrees: around X (pitch),
            // Y (yaw) and Z (roll). They are calculated from the quaternion every frame and turned back into a quaternion
            // only when changed. Near a pitch of +-90 degrees the angles can jump to an equivalent set (gimbal lock),
            // which is fine for a debug tool.
            glm::vec3 angles = glm::degrees(glm::eulerAngles(transform.rotation));
            if (ImGui::DragFloat3("Rotation", &angles.x, RotationDragSpeed))
                transform.rotation = glm::quat(glm::radians(angles));
            ImGui::SetItemTooltip("Degrees around X, Y and Z.");

            if (ImGui::DragFloat3("Scale", &transform.scale.x, ScaleDragSpeed, SmallestScale, 100.0f))
                transform.scale = glm::max(transform.scale, glm::vec3(SmallestScale));
        }

        void DrawPreviousTransform()
        {
            ImGui::TextUnformatted("Drawn between the last two ticks (interpolated).");
        }

        void DrawMeshRenderer(const Renderer::MeshRenderer& meshRenderer, const Renderer::RenderAssets& assets)
        {
            // The component holds only handles; the stores know which asset each of them is.
            ImGui::Text("Mesh:    %s", FormatAssetName(assets.meshes.GetName(meshRenderer.mesh)));
            ImGui::Text("Texture: %s", FormatAssetName(assets.textures.GetPath(meshRenderer.texture)));
            ImGui::Text("Program: %s", FormatAssetName(assets.shaders.GetName(meshRenderer.shaderProgram)));
        }

        void DrawSpin(Gameplay::Spin& spin)
        {
            ImGui::DragFloat3("Axis", &spin.axis.x, 0.01f);
            ImGui::DragFloat("Speed", &spin.speed, 0.01f);
            ImGui::SetItemTooltip("Radians per second; negative turns the other way.");
        }

        void DrawCameraLens(Renderer::CameraLens& lens)
        {
            // Shown in degrees, stored in radians.
            float verticalFOVDegrees = glm::degrees(lens.verticalFOV);
            if (ImGui::SliderFloat("Vertical FOV", &verticalFOVDegrees, 20.0f, 120.0f, "%.0f deg"))
                lens.verticalFOV = glm::radians(verticalFOVDegrees);

            // The near plane must stay above 0 and below the far plane, or the projection breaks.
            ImGui::DragFloat("Near plane", &lens.nearPlane, 0.01f, 0.01f, lens.farPlane - 0.01f, "%.2f m");
            ImGui::DragFloat("Far plane", &lens.farPlane, 1.0f, lens.nearPlane + 0.01f, 10000.0f, "%.0f m");
        }

        void DrawLookAngles(const Gameplay::LookAngles& look)
        {
            // Read-only: the view is built from these angles, so they are changed with the mouse.
            ImGui::Text("Yaw:   %.1f deg", glm::degrees(look.yaw));
            ImGui::Text("Pitch: %.1f deg", glm::degrees(look.pitch));
        }
    }

    void EntitiesWindow::Draw(bool* isOpen, entt::registry& registry, const Renderer::RenderAssets& assets)
    {
        ImGui::SetNextWindowPos(ScaleToUI(InitialPosition), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ScaleToUI(InitialSize), ImGuiCond_FirstUseEver);

        if (!ImGui::Begin("Entities", isOpen))
        {
            ImGui::End();

            return;
        }

        // Left: the list. A child window is a scrollable region inside a window; ResizeX lets the border be dragged.
        ImGui::BeginChild("EntityList", ImVec2(ScaleToUI(EntityListWidth), 0.0f),
                          ImGuiChildFlags_Borders | ImGuiChildFlags_ResizeX);
        DrawEntityList(registry);
        ImGui::EndChild();

        ImGui::SameLine();

        // Right: the components of the selected entity.
        ImGui::BeginChild("Components");
        if (!registry.valid(m_selectedEntity))
        {
            ImGui::TextUnformatted("Select an entity in the list.");
        }
        else
        {
            // The title shows the number and the Name of the entity. The name is read-only: names come from code and,
            // later, from maps, and a name typed here would be lost when the game closes.
            const entt::entity entity = m_selectedEntity;
            ImGui::TextUnformatted(FormatEntityLabel(registry, entity).c_str());
            DrawModuleLegend();
            ImGui::Separator();

            // try_get returns nullptr for a component the entity does not have, so every section appears only for
            // the components the entity really has. The order follows how general the components are.
            if (Core::Transform* transform = registry.try_get<Core::Transform>(entity); transform != nullptr)
                if (DrawComponentHeader("Transform", ComponentModule::Core,
                                        "where the entity is, how it is turned and how big it is."))
                    DrawTransform(*transform);

            if (registry.all_of<Core::PreviousTransform>(entity))
                if (DrawComponentHeader("Previous Transform", ComponentModule::Core,
                                        "the transform before the last tick, for smooth drawing between ticks."))
                    DrawPreviousTransform();

            if (const Renderer::MeshRenderer* meshRenderer = registry.try_get<Renderer::MeshRenderer>(entity);
                meshRenderer != nullptr)
                if (DrawComponentHeader("Mesh Renderer", ComponentModule::Renderer,
                                        "which mesh, texture and program draw the entity."))
                    DrawMeshRenderer(*meshRenderer, assets);

            if (Gameplay::Spin* spin = registry.try_get<Gameplay::Spin>(entity); spin != nullptr)
                if (DrawComponentHeader("Spin", ComponentModule::Gameplay, "keeps turning the entity around an axis."))
                    DrawSpin(*spin);

            if (Renderer::CameraLens* lens = registry.try_get<Renderer::CameraLens>(entity); lens != nullptr)
                if (DrawComponentHeader("Camera Lens", ComponentModule::Renderer,
                                        "how wide, how near and how far the camera sees."))
                    DrawCameraLens(*lens);

            if (const Gameplay::LookAngles* look = registry.try_get<Gameplay::LookAngles>(entity); look != nullptr)
                if (DrawComponentHeader("Look Angles", ComponentModule::Gameplay,
                                        "yaw and pitch of the view, turned with the mouse."))
                    DrawLookAngles(*look);
        }
        ImGui::EndChild();

        ImGui::End();
    }

    void EntitiesWindow::DrawEntityList(const entt::registry& registry)
    {
        // view<entt::entity>() visits every entity that exists, whatever components it has.
        const auto allEntities = registry.view<entt::entity>();
        ImGui::Text("%zu entities", static_cast<std::size_t>(std::ranges::distance(allEntities)));
        ImGui::Separator();

        for (const entt::entity entity : allEntities)
        {
            // ImGui identifies widgets by their label. Two entities may have the same name ("Crate"), so every row gets
            // its own ID from the entity value: PushID/PopID wrap the widgets that belong to one entity.
            ImGui::PushID(static_cast<int>(entt::to_integral(entity)));
            if (ImGui::Selectable(FormatEntityLabel(registry, entity).c_str(), entity == m_selectedEntity))
                m_selectedEntity = entity;
            ImGui::PopID();
        }
    }
}
