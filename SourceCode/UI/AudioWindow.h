#pragma once

#include "Audio/AudioEngine.h"

#include <glm/vec3.hpp>

namespace Abomination::UI
{
    // The Audio window of the debug overlay (View > Engine > Audio): the sound card, the master volume, how many voices
    // play, the loaded sounds (each can be played), and a test sound in 3D.
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
        static void DrawSoundList(Audio::AudioEngine& audio);

        // The repeating test sound, if one is placed, and where it is.
        Audio::VoiceId m_testVoice;
        glm::vec3 m_testPosition{0.0f};
        bool m_isTestSoundPlaced = false;
    };
}
