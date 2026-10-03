#pragma once

#include "Audio/AudioEngine.h"

#include <glm/vec3.hpp>

namespace Abomination::UI
{
    // The Audio window of the debug overlay (Audio in the menu bar): the sound card, how many voices play, the master
    // volume and the volume of every group, every sound event with its own volume and pitch variation (to balance the
    // sounds while playing, without rebuilding the game), the loaded files (each can be played), and a test sound in 3D.
    //
    // The test sound shows what 3D sound does: it is placed in front of the camera and repeats; walk around it, turn away
    // from it, go farther: it moves between the ears and gets quieter with distance.
    class AudioWindow
    {
    public:
        // Draws the window while *isOpen is true; its close button sets *isOpen to false.
        void Draw(bool* isOpen, Audio::AudioEngine& audio);

    private:
        void DrawTestSound(Audio::AudioEngine& audio);
        static void DrawVolumes(Audio::AudioEngine& audio);
        static void DrawSoundEvents(Audio::AudioEngine& audio);
        static void DrawSoundList(Audio::AudioEngine& audio);

        // The repeating test sound, if one is placed, and where it is.
        Audio::VoiceId m_testVoice;
        glm::vec3 m_testPosition{0.0f};
        bool m_isTestSoundPlaced = false;
    };
}
