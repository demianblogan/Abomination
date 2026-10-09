#include "UI/Windows/EntitiesWindow.h"

#include "Core/Scene/Name.h"
#include "Core/Scene/Transform.h"
#include "Core/Scene/TransformInterpolation.h"
#include "Gameplay/Camera/MouseLook.h"
#include "Gameplay/Player/Player.h"
#include "Gameplay/Spin.h"
#include "Physics/CharacterBody.h"
#include "Renderer/Assets/RenderAssets.h"
#include "Renderer/Camera/CameraLens.h"
#include "Renderer/MeshRenderer.h"
#include "Renderer/ModelRenderer.h"
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
        constexpr float SpinDragSpeed = 0.01f;       // axis components and radians per second
        constexpr float NearPlaneDragSpeed = 0.01f;  // meters
        constexpr float FarPlaneDragSpeed = 1.0f;    // meters

        // Limits of the editable values.
        constexpr float SmallestScale = 0.01f;       // a scale of 0 would squash the mesh to nothing
        constexpr float LargestScale = 100.0f;
        constexpr float SmallestVerticalFOVDegrees = 20.0f;
        constexpr float LargestVerticalFOVDegrees = 120.0f;
        constexpr float SmallestPlaneGap = 0.01f;    // meters: the near plane stays above 0 and below the far plane
        constexpr float LargestFarPlane = 10000.0f;  // meters

        // Header colors of the component sections, one per module (see ComponentModule below). Muted colors keep the white
        // header text readable: steel for the basics, teal for drawing, violet for physics, amber for game rules.
        constexpr ImVec4 CoreModuleColor(0.33f, 0.38f, 0.46f, 1.0f);
        constexpr ImVec4 RendererModuleColor(0.10f, 0.42f, 0.42f, 1.0f);
        constexpr ImVec4 PhysicsModuleColor(0.40f, 0.25f, 0.52f, 1.0f);
        constexpr ImVec4 GameplayModuleColor(0.55f, 0.37f, 0.10f, 1.0f);

        // How much the velocity of a character changes per pixel of mouse movement when it is dragged, in m/s.
        constexpr float VelocityDragSpeed = 0.05f;

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
        // a glance which parts of the engine it is made of (later AI gets a color too). In the order of the dependencies
        // of the modules, from the bottom up.
        enum class ComponentModule
        {
            Core,
            Renderer,
            Physics,
            Gameplay,
        };

        constexpr std::array AllComponentModules{ComponentModule::Core, ComponentModule::Renderer, ComponentModule::Physics,
                                                 ComponentModule::Gameplay};

        const char* GetModuleName(ComponentModule module)
        {
            switch (module)
            {
                case ComponentModule::Core:
                    return "Core";
                case ComponentModule::Renderer:
                    return "Renderer";
                case ComponentModule::Physics:
                    return "Physics";
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
                case ComponentModule::Physics:
                    return PhysicsModuleColor;
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

            if (ImGui::DragFloat3("Scale", &transform.scale.x, ScaleDragSpeed, SmallestScale, LargestScale))
                transform.scale = glm::max(transform.scale, glm::vec3(SmallestScale));
        }

        void DrawPreviousTransform()
        {
            ImGui::TextUnformatted("Drawn between the last two ticks (interpolated).");
        }

        // The name of a map of a material, or "none" for a map it does not have (an invalid handle: a built-in texture is
        // read instead, see Renderer::Material).
        const char* FormatMapName(Renderer::TextureHandle map, const Renderer::RenderAssets& assets)
        {
            const std::string* path = assets.textures.GetPath(map);
            return path != nullptr ? path->c_str() : "none";
        }

        void DrawMaterial(const Renderer::Material& material, const Renderer::RenderAssets& assets)
        {
            ImGui::Text("Color:     %s", FormatAssetName(assets.textures.GetPath(material.baseColor)));
            ImGui::Text("Normal:    %s", FormatMapName(material.normal, assets));
            ImGui::Text("Roughness: %s x %.2f, metalness x %.2f", FormatMapName(material.metalRoughness, assets),
                        material.roughnessFactor, material.metalnessFactor);
            ImGui::Text("Emissive:  %s", FormatMapName(material.emissive, assets));
            ImGui::Text("Height:    %s, parallax depth %.3f", FormatMapName(material.height, assets), material.parallaxDepth);
        }

        void DrawMeshRenderer(const Renderer::MeshRenderer& meshRenderer, const Renderer::RenderAssets& assets)
        {
            // The component holds only handles; the stores know which asset each of them is.
            ImGui::Text("Mesh:    %s", FormatAssetName(assets.meshes.GetName(meshRenderer.mesh)));
            ImGui::Text("Program: %s", FormatAssetName(assets.shaders.GetName(meshRenderer.shaderProgram)));
            DrawMaterial(meshRenderer.material, assets);
        }

        void DrawModelRenderer(const Renderer::ModelRenderer& modelRenderer, const Renderer::RenderAssets& assets)
        {
            const Renderer::Model& model = assets.models.Get(modelRenderer.model);
            ImGui::Text("Model:   %s", FormatAssetName(assets.models.GetPath(modelRenderer.model)));
            ImGui::Text("Program: %s", FormatAssetName(assets.shaders.GetName(modelRenderer.shaderProgram)));
            ImGui::Text("Parts:   %zu", model.parts.size());

            // Every part folds open to its material, so the list stays short for a model of many parts.
            for (const Renderer::ModelPart& part : model.parts)
            {
                if (ImGui::TreeNode(part.name.c_str()))
                {
                    DrawMaterial(part.material, assets);
                    ImGui::TreePop();
                }
            }
        }

        void DrawSpin(Gameplay::Spin& spin)
        {
            ImGui::DragFloat3("Axis", &spin.axis.x, SpinDragSpeed);
            ImGui::DragFloat("Speed", &spin.speed, SpinDragSpeed);
            ImGui::SetItemTooltip("Radians per second; negative turns the other way.");
        }

        void DrawCameraLens(Renderer::CameraLens& lens)
        {
            // Shown in degrees, stored in radians.
            float verticalFOVDegrees = glm::degrees(lens.verticalFOV);
            if (ImGui::SliderFloat("Vertical FOV", &verticalFOVDegrees, SmallestVerticalFOVDegrees, LargestVerticalFOVDegrees,
                                   "%.0f deg"))
                lens.verticalFOV = glm::radians(verticalFOVDegrees);

            // The near plane must stay above 0 and below the far plane, or the projection breaks.
            ImGui::DragFloat("Near plane", &lens.nearPlane, NearPlaneDragSpeed, SmallestPlaneGap,
                             lens.farPlane - SmallestPlaneGap, "%.2f m");
            ImGui::DragFloat("Far plane", &lens.farPlane, FarPlaneDragSpeed, lens.nearPlane + SmallestPlaneGap,
                             LargestFarPlane, "%.0f m");
        }

        void DrawLookAngles(const Gameplay::LookAngles& look)
        {
            // Read-only: the view is built from these angles, so they are changed with the mouse.
            ImGui::Text("Yaw:   %.1f deg", glm::degrees(look.yaw));
            ImGui::Text("Pitch: %.1f deg", glm::degrees(look.pitch));
        }

        void DrawCharacterBody(Physics::CharacterBody& body)
        {
            // The size is read-only: a box grown here could end up inside a wall, and the movement code would then have to
            // push it out. The velocity can be changed, to throw the character around and watch how the movement reacts.
            ImGui::Text("Box:     %.3f x %.3f x %.3f m", 2.0 * body.halfExtents.x, 2.0 * body.halfExtents.y,
                        2.0 * body.halfExtents.z);
            ImGui::DragFloat3("Velocity", &body.velocity.x, VelocityDragSpeed, 0.0f, 0.0f, "%.2f m/s");
            ImGui::Text("On ground: %s", body.isOnGround ? "yes" : "no");
            ImGui::Text("Stepped up last tick: %.3f m", body.steppedUpHeight);
            ImGui::Text("In solid: %s", body.isInSolid ? "YES (see the log)" : "no");
        }

        void DrawStepSmoothing(const Gameplay::StepSmoothing& smoothing)
        {
            // Read-only: changed every tick. Negative while the eyes are still catching up with the body after a step.
            ImGui::Text("Eyes behind the body: %.3f m (last tick %.3f m)", smoothing.offset, smoothing.previousOffset);
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

            if (const Renderer::ModelRenderer* modelRenderer = registry.try_get<Renderer::ModelRenderer>(entity);
                modelRenderer != nullptr)
                if (DrawComponentHeader("Model Renderer", ComponentModule::Renderer,
                                        "which model (its parts, meshes and textures) and program draw the entity."))
                    DrawModelRenderer(*modelRenderer, assets);

            if (Physics::CharacterBody* body = registry.try_get<Physics::CharacterBody>(entity); body != nullptr)
                if (DrawComponentHeader("Character Body", ComponentModule::Physics,
                                        "the box the character walks through the level with, and its velocity."))
                    DrawCharacterBody(*body);

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

            if (const auto* smoothing = registry.try_get<Gameplay::StepSmoothing>(entity); smoothing != nullptr)
                if (DrawComponentHeader("Step Smoothing", ComponentModule::Gameplay,
                                        "lets the eyes or the model glide up stairs after the body."))
                    DrawStepSmoothing(*smoothing);
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
