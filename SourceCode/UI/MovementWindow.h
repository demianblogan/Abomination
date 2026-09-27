#pragma once

namespace Abomination::Physics
{
    struct CharacterBody;
    struct MovementSettings;
    struct PhysicsSettings;
}

namespace Abomination::UI
{
    // The Movement window of the debug overlay (View > Movement): a speedometer and the state of the player, and sliders
    // for every movement setting, to find values that feel right while playing. The values are not saved between runs
    // yet (that comes with the JSON configuration files, 0.6); "Reset" brings back the default values.
    //
    // Draws the window while *isOpen is true; its close button sets *isOpen to false.
    void DrawMovementWindow(bool* isOpen, Physics::PhysicsSettings& physicsSettings,
                            Physics::MovementSettings& movementSettings, const Physics::CharacterBody& playerBody);
}
