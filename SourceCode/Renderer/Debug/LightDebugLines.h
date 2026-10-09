#pragma once

#include "Renderer/Debug/DebugLines.h"

#include <entt/entt.hpp>

namespace Abomination::Renderer
{
    // Shows every light (Core::Transform + Renderer::Light) in its own color: a small star at the light, always visible,
    // and where its light ends, hidden behind walls: three circles of its range for a point light, the cone for a spot
    // light, with an arrow along its axis.
    void AddLightDebugLines(const entt::registry& registry, DebugLines& lines);
}
