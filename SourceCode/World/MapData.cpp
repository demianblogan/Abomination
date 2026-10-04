#include "World/MapData.h"

#include <algorithm>

namespace Abomination::World
{
    const std::string* FindProperty(const MapEntity& entity, const std::string& key)
    {
        const auto iterator = entity.properties.find(key);
        if (iterator == entity.properties.end())
            return nullptr;

        return &iterator->second;
    }

    bool IsClipBrush(const MapBrush& brush)
    {
        return std::ranges::any_of(brush.faces, [](const MapFace& face) { return face.textureName == ClipTextureName; });
    }
}
