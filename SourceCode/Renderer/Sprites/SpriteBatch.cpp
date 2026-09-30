#include "Renderer/Sprites/SpriteBatch.h"

namespace Abomination::Renderer
{
    void SpriteBatch::AddBillboard(const glm::vec3& center, float halfSize, float rotation, const glm::vec4& color,
                                   TextureHandle texture, SpriteBlend blend)
    {
        m_sprites.push_back(Sprite{
            .center = center,
            .isBillboard = true,
            .halfSize = halfSize,
            .rotation = rotation,
            .color = color,
            .texture = texture,
            .blend = blend,
        });
    }

    void SpriteBatch::AddQuad(const glm::vec3& center, const glm::vec3& right, const glm::vec3& up, const glm::vec4& color,
                              TextureHandle texture, SpriteBlend blend)
    {
        m_sprites.push_back(Sprite{
            .center = center,
            .isBillboard = false,
            .right = right,
            .up = up,
            .color = color,
            .texture = texture,
            .blend = blend,
        });
    }

    std::span<const Sprite> SpriteBatch::GetSprites() const noexcept
    {
        return m_sprites;
    }

    void SpriteBatch::Clear() noexcept
    {
        m_sprites.clear();
    }
}
