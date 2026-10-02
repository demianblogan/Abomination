#pragma once

namespace Abomination::Gameplay
{
    struct LandingDip;
}

namespace Abomination::Physics
{
    struct CharacterBody;
    struct MovementSettings;
    struct PhysicsSettings;
}

namespace Abomination::UI
{
    // The Movement window of the debug overlay (Movement in the menu bar): a speedometer and the state of the player, and
    // sliders for every movement setting and for the dip of the view after a landing, to find values that feel right
    // while playing. The values are not saved between
    // runs yet (that comes with the JSON configuration files, 0.6); "Reset" brings back the default values.
    //
    // Draws the window while *isOpen is true; its close button sets *isOpen to false.
    void DrawMovementWindow(bool* isOpen, Physics::PhysicsSettings& physicsSettings,
                            Physics::MovementSettings& movementSettings, Gameplay::LandingDip& landingDip,
                            const Physics::CharacterBody& playerBody);
}
