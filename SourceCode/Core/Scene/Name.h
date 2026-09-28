#pragma once

#include <string>

namespace Abomination::Core
{
    // Component: a name for people, shown in the entity inspector and in log messages ("Crate", "Camera").
    // Not unique and not used to find entities: code refers to entities by their entt::entity number.
    // Later names from TrenchBroom maps ("targetname", which buttons use to find their doors) get their own component.
    struct Name
    {
        std::string value;
    };
}
