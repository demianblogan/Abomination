#pragma once

#include <string>

namespace Abomination::Core
{
    // Component: a name for people, shown in the entity inspector ("Player", "Camera",
    // "World geometry: Episode1/Wall_MossyBrick"). A struct and not a plain std::string, because EnTT tells components
    // apart by their type: a second text component (like the targetname below) would otherwise be the same component.
    // Not unique and not used to find entities: code refers to entities by their entt::entity number.
    // Later names from TrenchBroom maps ("targetname", which buttons use to find their doors) get their own component.
    struct Name
    {
        std::string value;
    };
}
