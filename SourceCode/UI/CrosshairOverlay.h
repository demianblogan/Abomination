#pragma once

namespace Abomination::Gameplay
{
    struct HitMarkerSettings;
    struct Weapon;
}

namespace Abomination::UI
{
    // Draws the crosshair of the weapon in the middle of the screen and its hit markers when a shot hurts or kills.
    //
    // A temporary home: the game has no interface renderer of its own yet, so the crosshair is drawn with the lines of
    // Dear ImGui over everything. It moves to the HUD with text rendering in 0.4. It is drawn whether the debug overlay
    // is shown or hidden.
    class CrosshairOverlay
    {
    public:
        // Draws the crosshair for this frame. verticalFOV is that of the camera (radians), so the circle is exactly as
        // wide as the spread of the pellets on the screen.
        void Draw(const Gameplay::Weapon& weapon, float verticalFOV);

    private:
        // Starts a marker when the weapon counted a new hit or kill since the last frame, and moves the running ones.
        void UpdateMarkers(const Gameplay::Weapon& weapon, float deltaTime);

        // The four diagonal lines of a marker secondsSinceStart after it began, around a circle of circleRadius.
        static void DrawMarker(const Gameplay::HitMarkerSettings& settings, float secondsSinceStart, float circleRadius);

        // The counts of the weapon already seen, and how long ago each kind of marker started. Unseen at first, so no
        // marker flashes when the crosshair first appears.
        bool m_hasSeenWeapon = false;
        int m_seenHitCount = 0;
        int m_seenKillCount = 0;
        float m_secondsSinceHit = 1000.0f;
        float m_secondsSinceKill = 1000.0f;

        // The pulse of the circle after a shot: how much wider it is (a part of its radius) and how fast that changes.
        int m_seenShotCount = 0;
        float m_pulse = 0.0f;
        float m_pulseVelocity = 0.0f;
    };
}
