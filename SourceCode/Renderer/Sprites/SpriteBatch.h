#pragma once

#include "Renderer/Assets/TextureStore.h"

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <span>
#include <vector>

namespace Abomination::Renderer
{
    // How a sprite is mixed with what is behind it.
    enum class SpriteBlend
    {
        // Covers what is behind it as much as its alpha says: smoke, dust, blood, marks on walls.
        Alpha,

        // Adds its light to what is behind it, so it can only brighten: fire, sparks, the muzzle flash. The order of such
        // sprites does not matter (addition does not care about order), and black parts of the texture add nothing.
        Additive,
    };

    // One textured rectangle: its center, and the vectors from the center to the middle of its right edge and of its top
    // edge (their lengths are half the width and half the height). A billboard gets them from the camera when it is drawn,
    // so it always faces the camera; a flat quad has them fixed, lying on a wall for example.
    struct Sprite
    {
        glm::vec3 center{0.0f};

        // A billboard: halfSize (meters) and a rotation in the plane of the screen (radians) instead of right and up.
        bool isBillboard = true;
        float halfSize = 0.1f;
        float rotation = 0.0f;

        // A flat quad (isBillboard false).
        glm::vec3 right{0.1f, 0.0f, 0.0f};
        glm::vec3 up{0.0f, 0.1f, 0.0f};

        // Multiplies the texture: tints it and sets how transparent it is.
        glm::vec4 color{1.0f};

        // The part of the texture shown (texture coordinates, (0, 0) the bottom left): all of it, or one frame of a
        // flipbook, a sheet of the frames of an animation.
        glm::vec2 texCoordMinimum{0.0f};
        glm::vec2 texCoordMaximum{1.0f};

        TextureHandle texture;
        SpriteBlend blend = SpriteBlend::Alpha;
    };

    // The sprites of one frame: game code adds them, SpriteRenderer draws them, and they are cleared for the next frame
    // (like DebugLines). Needs no OpenGL.
    class SpriteBatch
    {
    public:
        // A sprite that always faces the camera.
        void AddBillboard(const glm::vec3& center, float halfSize, float rotation, const glm::vec4& color,
                          TextureHandle texture, SpriteBlend blend);

        // A flat quad spanned by right and up (see Sprite), showing all of its texture or a part of it.
        void AddQuad(const glm::vec3& center, const glm::vec3& right, const glm::vec3& up, const glm::vec4& color,
                     TextureHandle texture, SpriteBlend blend, const glm::vec2& texCoordMinimum = glm::vec2(0.0f),
                     const glm::vec2& texCoordMaximum = glm::vec2(1.0f));

        [[nodiscard]] std::span<const Sprite> GetSprites() const noexcept;

        void Clear() noexcept;

    private:
        std::vector<Sprite> m_sprites;
    };
}
