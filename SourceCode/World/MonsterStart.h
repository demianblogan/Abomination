#pragma once

#include <glm/vec3.hpp>

#include <string>

// A monster as the map knows it: a point entity whose class name starts with "monster_" ("monster_dog"). The World module
// only reads where it stands; the Gameplay module creates the monster (see Gameplay/Enemies/Monsters.h).
//
// The origin of a monster is at the bottom of its box, between its feet, as in its entity definition for TrenchBroom
// (Tools/TrenchBroom/Abomination/Abomination.fgd) and in its model: a monster placed on the floor stands on it.
namespace Abomination::World
{
    struct MonsterStart
    {
        // "monster_dog".
        std::string className;

        // The bottom of its box in game coordinates, and the way it faces (yaw, radians).
        glm::vec3 origin{0.0f};
        float yaw = 0.0f;
    };
}
