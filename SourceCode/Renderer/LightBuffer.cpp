#include "Renderer/LightBuffer.h"

#include "Core/Profiling/ProfileZone.h"
#include "Core/Scene/Transform.h"
#include "Core/Scene/TransformInterpolation.h"
#include "Renderer/Light.h"
#include "Renderer/OpenGL/ShaderInterface.h"

#include <glad/gl.h>
#include <glm/geometric.hpp>

#include <algorithm>
#include <cmath>

namespace Abomination::Renderer
{
    void GatherShaderLights(const entt::registry& registry, const glm::mat4& viewMatrix, float interpolationFactor,
                            std::vector<ShaderLight>& lights)
    {
        lights.clear();
        const auto view = registry.view<const Core::Transform, const Light>();
        for (const entt::entity entity : view)
        {
            const Light& light = view.get<const Light>(entity);
            const Core::Transform transform = Core::CalculateDrawnTransform(registry, entity, interpolationFactor);

            // A position is moved by the view matrix (w = 1), a direction only turned (w = 0), as in
            // CalculateSceneLighting.
            const glm::vec3 position = glm::vec3(viewMatrix * glm::vec4(transform.position, 1.0f));
            const glm::vec3 direction =
                glm::normalize(glm::vec3(viewMatrix * glm::vec4(transform.rotation * Core::LocalForward, 0.0f)));

            // The shader compares cosines, not angles: the cosine of the angle between two directions is their dot
            // product, so no angle has to be worked out per pixel.
            lights.push_back(ShaderLight{
                .positionAndRange = {position, light.range},
                .colorAndType = {light.color * light.intensity, static_cast<float>(light.type)},
                .directionAndOuterCosine = {direction, std::cos(light.outerConeAngle)},
                .innerCosine = {std::cos(light.innerConeAngle), 0.0f, 0.0f, 0.0f},
            });
        }
    }

    LightBuffer::LightBuffer()
        : m_buffer(GLBuffer::CreateDynamic(MaximumLightCount * sizeof(ShaderLight)))
    {}

    int LightBuffer::Upload(const entt::registry& registry, const glm::mat4& viewMatrix, float interpolationFactor)
    {
        PROFILE_ZONE();

        GatherShaderLights(registry, viewMatrix, interpolationFactor, m_lights);
        const std::size_t count = std::min(m_lights.size(), MaximumLightCount);
        if (count > 0)
            m_buffer.Update(std::as_bytes(std::span<const ShaderLight>(m_lights).first(count)));

        // Connected even without lights: the shader declares the buffer and must find one at its binding.
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, LightsStorageBinding, m_buffer.GetID());

        return static_cast<int>(count);
    }
}
