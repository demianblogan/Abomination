#include "UI/AudioWindow.h"

#include "Audio/SoundEvent.h"
#include "Core/Assets/AssetLifetime.h"
#include "UI/UIScale.h"

#include <glm/geometric.hpp>
#include <imgui.h>

#include <format>
#include <string>

namespace Abomination::UI
{
    namespace
    {
        // The window opens for the first time at the left edge, below the Movement window.
        constexpr ImVec2 InitialPosition(10.0f, 560.0f);

        // The test sound is placed this far in front of the camera (meters).
        constexpr float TestSoundDistance = 3.0f;

        const std::string TestSoundPath = "Sounds/Debug/TestKnock.ogg";

        // Where a point is for the listener, in words: "3.2 m, in front, left".
        std::string DescribeDirection(const glm::vec3& listenerPosition, const glm::vec3& listenerForward,
                                      const glm::vec3& point)
        {
            const glm::vec3 toPoint = point - listenerPosition;
            const float distance = glm::length(toPoint);
            if (distance < 0.1f)
                return "at the listener";

            // The right of the listener is perpendicular to where they look and to the world up (+Y). The dot product
            // with a direction tells how much of toPoint goes that way: positive in front / to the right.
            const glm::vec3 forward = glm::normalize(listenerForward);
            const glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f)));
            const float ahead = glm::dot(toPoint, forward) / distance;
            const float side = glm::dot(toPoint, right) / distance;

            // Within about 25 degrees of a direction it counts as straight that way (sin 25° is about 0.42).
            constexpr float Straight = 0.42f;
            std::string description = std::format("{:.1f} m", distance);
            if (ahead > Straight)
                description += ", in front";
            else if (ahead < -Straight)
                description += ", behind";
            if (side > Straight)
                description += ", right";
            else if (side < -Straight)
                description += ", left";

            return description;
        }
    }

    void AudioWindow::Draw(bool* isOpen, Audio::AudioEngine& audio)
    {
        ImGui::SetNextWindowPos(ScaleToUI(InitialPosition), ImGuiCond_FirstUseEver);
        if (!ImGui::Begin("Audio", isOpen, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::End();
            return;
        }

        ImGui::Text("Sound card:     %s", audio.HasDevice() ? "yes" : "NONE (the game is silent)");
        ImGui::Text("Playing voices: %zu / %zu", audio.GetPlayingVoiceCount(), Audio::AudioEngine::VoiceCount);
        ImGui::SetItemTooltip("A voice is one sound playing. When all are busy, the oldest sound is cut off.");

        float volume = audio.GetMasterVolume();
        ImGui::SetNextItemWidth(ScaleToUI(180.0f));
        if (ImGui::SliderFloat("Master volume", &volume, 0.0f, 1.0f, "%.2f"))
            audio.SetMasterVolume(volume);

        DrawTestSound(audio);
        DrawSoundList(audio);

        ImGui::End();
    }

    void AudioWindow::DrawTestSound(Audio::AudioEngine& audio)
    {
        ImGui::SeparatorText("3D test sound");

        if (ImGui::Button(m_isTestSoundPlaced ? "Move here" : "Place in front of the camera"))
        {
            audio.Stop(m_testVoice);

            // A 3D sound must not change its pitch at random while it is compared with itself, so no variation.
            const Audio::SoundEvent knock{
                .variants = {audio.LoadSound(TestSoundPath, Core::AssetLifetime::Global)},
                .pitchVariation = 0.0f,
            };
            m_testPosition = audio.GetListenerPosition() + glm::normalize(audio.GetListenerForward()) * TestSoundDistance;
            m_testVoice = audio.Play(knock, m_testPosition, true);
            m_isTestSoundPlaced = true;
        }
        ImGui::SetItemTooltip("A knock that repeats at one point in the world. Walk around it, turn away,\n"
                              "go farther: it moves between the ears and gets quieter. Headphones help.");

        if (!m_isTestSoundPlaced)
            return;

        ImGui::SameLine();
        if (ImGui::Button("Stop"))
        {
            audio.Stop(m_testVoice);
            m_isTestSoundPlaced = false;
            return;
        }

        const std::string where = DescribeDirection(audio.GetListenerPosition(), audio.GetListenerForward(), m_testPosition);
        ImGui::Text("The sound is %s", where.c_str());
    }

    void AudioWindow::DrawSoundList(Audio::AudioEngine& audio)
    {
        ImGui::SeparatorText("Loaded sounds");

        // The sounds are played after the list is drawn: playing does not change the list, but it keeps the visitor
        // free of side effects on the engine it reads.
        Audio::SoundHandle soundToPlay;
        bool isPlayRequested = false;

        audio.GetSounds().VisitSounds([&](const std::string& path, Audio::SoundHandle handle, const Audio::SoundClip& clip,
                                          bool isFallback, Core::AssetLifetime lifetime)
        {
            // PushID makes the "Play" buttons of different rows different for ImGui, although their labels are equal.
            ImGui::PushID(path.c_str());
            if (ImGui::SmallButton("Play"))
            {
                soundToPlay = handle;
                isPlayRequested = true;
            }
            ImGui::PopID();

            ImGui::SameLine();
            const double seconds = static_cast<double>(clip.GetFrameCount()) / clip.sampleRate;
            ImGui::Text("%s  %.2f s  %s%s", path.c_str(), seconds, Core::GetAssetLifetimeName(lifetime).data(),
                        isFallback ? "  MISSING (beep)" : "");
        });

        if (isPlayRequested)
            audio.Play(Audio::SoundEvent{.variants = {soundToPlay}, .pitchVariation = 0.0f});
    }
}
