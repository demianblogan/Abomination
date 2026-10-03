#include "UI/Windows/AnimationWindow.h"

#include "Core/Logging/Log.h"
#include "Core/Scene/Name.h"
#include "Gameplay/Animation/Animator.h"
#include "Renderer/Assets/ModelStore.h"
#include "UI/UIScale.h"
#include "UI/Widgets.h"

#include <glm/trigonometric.hpp>
#include <imgui.h>

#include <algorithm>
#include <cstddef>
#include <format>
#include <string>
#include <vector>

namespace Abomination::UI
{
    namespace
    {
        using Core::LogCategory;
        using Core::LogLevel;

        // The window opens for the first time below the Effects window.
        constexpr ImVec2 InitialPosition(560.0f, 360.0f);

        // What the window remembers between frames: the chosen model and how long a cross-fade takes.
        std::size_t g_selectedIndex = 0;
        float g_blendDuration = 0.25f;

        // The segments as C++ code, one line each, in the form of Gameplay::FindModelSegments.
        void LogSegments(const Gameplay::Animator& animator)
        {
            std::string code;
            for (const Gameplay::AnimationSegment& segment : animator.segments)
                code += std::format("\n    {{.name = \"{}\", .clip = {}, .start = {:.2f}f, .end = {:.2f}f{}}},", segment.name,
                                    segment.clip, segment.start, segment.end, segment.isLooping ? "" : ", .isLooping = false");
            Core::Log::Write(LogCategory::UI, LogLevel::Info, "Animation segments:{}", code);
        }

        void DrawAnimator(Gameplay::Animator& animator, const Renderer::Model& model)
        {
            // The segments: clicking one cross-fades to it.
            ImGui::SeparatorText("Segments");
            DrawSlider("Switch time", g_blendDuration, 0.0f, 1.0f, "%.2f s",
                       "Used when you click another segment below: how long the old animation fades out while\n"
                       "the new one fades in. Changes nothing by itself.");
            for (std::size_t index = 0; index < animator.segments.size(); ++index)
            {
                const Gameplay::AnimationSegment& segment = animator.segments[index];
                const std::string label = std::format("{}  ({:.2f} s)##{}", segment.name,
                                                      Gameplay::CalculateSegmentDuration(segment, model), index);
                if (ImGui::Selectable(label.c_str(), animator.current.segment == index))
                    Gameplay::PlayAnimation(animator, index, g_blendDuration);
            }

            ImGui::SeparatorText("Playback");
            DrawSlider("Speed", animator.speed, 0.0f, 2.0f, "%.2f", "0 stops the animation; scrub the time line then.");
            if (ImGui::Button(animator.speed == 0.0f ? "Play" : "Pause"))
                animator.speed = animator.speed == 0.0f ? 1.0f : 0.0f;
            ImGui::SameLine();
            ImGui::Checkbox("Show skeleton", &animator.isSkeletonVisible);
            ImGui::SameLine();
            ImGui::Checkbox("Show axes", &animator.areAxesVisible);
            ImGui::SetItemTooltip("At the middle of the model: red - its right (+X), green - up (+Y),\n"
                                  "blue - where the game expects its face (-Z).");

            // The part held by a joint (the scythe), turned and moved relative to the hand until it lies in the fist.
            ImGui::SeparatorText("Held part");
            DrawCentimeterSlider("Move X", animator.heldPartOffset.x, -30.0f, 30.0f, "Along the X axis of the hand.");
            DrawCentimeterSlider("Move Y", animator.heldPartOffset.y, -30.0f, 30.0f, "Along the Y axis of the hand.");
            DrawCentimeterSlider("Move Z", animator.heldPartOffset.z, -30.0f, 30.0f, "Along the Z axis of the hand.");
            DrawDegreeSlider("Turn X", animator.heldPartAngles.x, -180.0f, 180.0f, "%.0f deg");
            DrawDegreeSlider("Turn Y", animator.heldPartAngles.y, -180.0f, 180.0f, "%.0f deg");
            DrawDegreeSlider("Turn Z", animator.heldPartAngles.z, -180.0f, 180.0f, "%.0f deg");
            if (ImGui::Button("Log held part"))
                Core::Log::Write(LogCategory::UI, LogLevel::Info,
                                 "Held part: move {:.1f} {:.1f} {:.1f} cm, turn {:.0f} {:.0f} {:.0f} deg",
                                 animator.heldPartOffset.x * 100.0f, animator.heldPartOffset.y * 100.0f,
                                 animator.heldPartOffset.z * 100.0f, glm::degrees(animator.heldPartAngles.x),
                                 glm::degrees(animator.heldPartAngles.y), glm::degrees(animator.heldPartAngles.z));

            if (animator.current.segment >= animator.segments.size())
                return;

            // The time line of the current segment; moving it shows that moment (and ends a cross-fade at once).
            Gameplay::AnimationSegment& segment = animator.segments[animator.current.segment];
            const float duration = Gameplay::CalculateSegmentDuration(segment, model);
            ImGui::SetNextItemWidth(ScaleToUI(SliderWidth));
            if (ImGui::SliderFloat("Time", &animator.current.time, 0.0f, duration, "%.2f s"))
                animator.previous.reset();
            ImGui::Text("Clip time: %.2f s (frame %d of the FBX, 30 per second)", segment.start + animator.current.time,
                        static_cast<int>((segment.start + animator.current.time) * 30.0f));

            // Where the segment lies in its clip, to cut the animations of one long clip apart by eye.
            ImGui::SeparatorText("Current segment");
            const float clipDuration = segment.clip < model.animations.size() ? model.animations[segment.clip].duration : 0.0f;
            float end = segment.end < 0.0f ? clipDuration : segment.end;
            DrawSlider("Start", segment.start, 0.0f, clipDuration, "%.2f s", "Where the segment starts in its clip.");
            if (DrawSlider("End", end, 0.0f, clipDuration, "%.2f s", "Where the segment ends in its clip."))
                segment.end = end;
            segment.start = std::min(segment.start, end);
            ImGui::Checkbox("Looping", &segment.isLooping);

            ImGui::Separator();
            if (ImGui::Button("Log segments"))
                LogSegments(animator);
            ImGui::SetItemTooltip("Writes the segments to the console as code for Gameplay::FindModelSegments.");
        }
    }

    void DrawAnimationWindow(bool* isOpen, entt::registry& registry, const Renderer::ModelStore& models)
    {
        ImGui::SetNextWindowPos(ScaleToUI(InitialPosition), ImGuiCond_FirstUseEver);
        if (!ImGui::Begin("Animation", isOpen))
        {
            ImGui::End();
            return;
        }

        std::vector<entt::entity> entities;
        for (const entt::entity entity : registry.view<Gameplay::Animator>())
            entities.push_back(entity);

        if (entities.empty())
        {
            ImGui::TextUnformatted("No animated models in the level.");
            ImGui::End();
            return;
        }

        // The animated models by name; the first one is chosen until another is.
        g_selectedIndex = std::min(g_selectedIndex, entities.size() - 1);
        const auto nameOf = [&](entt::entity entity)
        {
            const Core::Name* name = registry.try_get<Core::Name>(entity);
            return name != nullptr ? name->value : std::format("Entity {}", static_cast<std::uint32_t>(entity));
        };
        ImGui::SetNextItemWidth(ScaleToUI(SliderWidth * 1.6f));
        if (ImGui::BeginCombo("Model", nameOf(entities[g_selectedIndex]).c_str()))
        {
            for (std::size_t index = 0; index < entities.size(); ++index)
                if (ImGui::Selectable(std::format("{}##{}", nameOf(entities[index]), index).c_str(), index == g_selectedIndex))
                    g_selectedIndex = index;
            ImGui::EndCombo();
        }

        Gameplay::Animator& animator = registry.get<Gameplay::Animator>(entities[g_selectedIndex]);
        DrawAnimator(animator, models.Get(animator.model));

        ImGui::End();
    }
}
