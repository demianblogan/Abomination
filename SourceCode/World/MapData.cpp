#include "World/MapData.h"

#include <algorithm>
#include <charconv>

namespace Abomination::World
{
    const std::string* FindProperty(const MapEntity& entity, const std::string& key)
    {
        const auto iterator = entity.properties.find(key);
        if (iterator == entity.properties.end())
            return nullptr;

        return &iterator->second;
    }

    double ReadNumberProperty(const MapEntity& entity, const std::string& key, double fallback)
    {
        const std::string* text = FindProperty(entity, key);
        if (text == nullptr)
            return fallback;

        double value = fallback;
        const auto [end, error] = std::from_chars(text->data(), text->data() + text->size(), value);

        return error == std::errc() ? value : fallback;
    }

    bool IsClipBrush(const MapBrush& brush)
    {
        return std::ranges::any_of(brush.faces, [](const MapFace& face) { return face.textureName == ClipTextureName; });
    }
}
