#pragma once

#include "Renderer/OpenGL/GLBuffer.h"

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

#include <entt/entt.hpp>

#include <cstddef>
#include <span>
#include <vector>

namespace Abomination::Renderer
{
    // One light as Lit.frag reads it from the light buffer (struct Light there), in the coordinates of the camera. Four
    // vec4, because the std430 layout of a storage buffer aligns a vec3 like a vec4: packing a number into every w keeps
    // the C++ struct and the GLSL struct the same, byte for byte.
    struct ShaderLight
    {
        // xyz: where the light is; w: its range (meters).
        glm::vec4 positionAndRange{0.0f};

        // xyz: its color times its intensity (linear); w: its type (the value of LightType: 0 point, 1 spot).
        glm::vec4 colorAndType{0.0f};

        // xyz: the direction a spot shines (length 1); w: the cosine of its outer cone angle.
        glm::vec4 directionAndOuterCosine{0.0f};

        // x: the cosine of the inner cone angle of a spot; yzw unused.
        glm::vec4 innerCosine{0.0f};
    };
    static_assert(sizeof(ShaderLight) == 64, "ShaderLight must match struct Light of Lit.frag");

    // Collects every light of the registry (Core::Transform + Renderer::Light) as the shader reads it, seen through
    // viewMatrix, into lights (cleared first; its memory is reused, so a frame allocates nothing). A light with a
    // Core::PreviousTransform is placed between its last two ticks like every moving entity (interpolationFactor).
    void GatherShaderLights(const entt::registry& registry, const glm::mat4& viewMatrix, float interpolationFactor,
                            std::vector<ShaderLight>& lights);

    // The lights of the frame in video memory: a shader storage buffer Lit.frag reads (see LightsStorageBinding). Filled
    // once per frame, before the world is drawn; the weapon in the hands reads the same lights, since it is lit in the
    // coordinates of the same camera. Requires a current OpenGL context. Move-only.
    class LightBuffer
    {
    public:
        // Lit.frag goes through every light for every pixel, so their number is the cost of lighting; far more than a
        // room needs. Beyond that, lights would have to be sorted into tiles of the screen first (Forward+).
        static constexpr std::size_t MaximumLightCount = 256;

        LightBuffer();

        // Gathers the lights (see GatherShaderLights), copies them into the buffer and connects it to the binding the
        // shader reads. Lights beyond MaximumLightCount are left out. Returns how many lights the buffer holds.
        int Upload(const entt::registry& registry, const glm::mat4& viewMatrix, float interpolationFactor);

    private:
        GLBuffer m_buffer;
        std::vector<ShaderLight> m_lights;
    };
}
