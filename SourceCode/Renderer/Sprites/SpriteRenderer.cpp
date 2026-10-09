#include "Renderer/Sprites/SpriteRenderer.h"

#include "Core/Profiling/ProfileZone.h"
#include "Renderer/OpenGL/GPUProfileZone.h"
#include "Renderer/OpenGL/ShaderInterface.h"

#include <glad/gl.h>
#include <glm/geometric.hpp>
#include <glm/mat3x3.hpp>
#include <glm/matrix.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <span>

namespace Abomination::Renderer
{
    namespace
    {
        // The vertex array has only one vertex buffer, connected to binding slot 0.
        constexpr std::uint32_t VertexBufferBinding = 0;

        // Every sprite is 2 triangles: 6 corners.
        constexpr std::size_t VerticesPerSprite = 6;

        // Enough for about 700 sprites; a frame with more makes the buffer grow.
        constexpr std::size_t InitialVertexCapacity = 4096;

        void SetBlend(SpriteBlend blend)
        {
            // The color written = new color * new alpha + old color * (1 - new alpha) for Alpha, or + old color * 1 for
            // Additive: the old color is kept whole and the new light is added to it.
            glBlendFunc(GL_SRC_ALPHA, blend == SpriteBlend::Additive ? GL_ONE : GL_ONE_MINUS_SRC_ALPHA);
        }
    }

    SpriteRenderer::SpriteRenderer()
        : m_vertexBuffer(GLBuffer::CreateDynamic(InitialVertexCapacity * sizeof(SpriteVertex)))
    {
        m_vertexCapacity = InitialVertexCapacity;
        m_vertexArray.SetVertexBuffer(VertexBufferBinding, m_vertexBuffer, sizeof(SpriteVertex));
        m_vertexArray.SetFloatAttribute(SpritePositionAttribute, VertexBufferBinding, 3, offsetof(SpriteVertex, position));
        m_vertexArray.SetFloatAttribute(SpriteTexCoordAttribute, VertexBufferBinding, 2, offsetof(SpriteVertex, texCoord));
        m_vertexArray.SetFloatAttribute(SpriteColorAttribute, VertexBufferBinding, 4, offsetof(SpriteVertex, color));
    }

    int SpriteRenderer::Draw(const SpriteBatch& batch, const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix,
                             const TextureStore& textures, const GLShaderProgram& program)
    {
        PROFILE_ZONE();
        PROFILE_GPU_ZONE("Sprites");

        const std::span<const Sprite> sprites = batch.GetSprites();
        if (sprites.empty())
            return 0;

        // The right and up directions of the camera are the first two rows of the rotation part of the view matrix (the
        // view matrix turns the world by the inverse of the camera's rotation; the inverse of a rotation is its transpose,
        // so its rows are the camera's axes). glm stores matrices by columns: viewMatrix[column][row].
        const glm::vec3 cameraRight(viewMatrix[0][0], viewMatrix[1][0], viewMatrix[2][0]);
        const glm::vec3 cameraUp(viewMatrix[0][1], viewMatrix[1][1], viewMatrix[2][1]);

        // Where the camera is: the view matrix moves the camera position to the origin, so undoing its rotation on its
        // translation gives the position back, negated.
        const glm::vec3 cameraPosition = -(glm::transpose(glm::mat3(viewMatrix)) * glm::vec3(viewMatrix[3]));

        // The order of drawing: alpha sprites first, from the farthest to the nearest, then additive ones grouped by
        // texture. stable_sort keeps the order the game added equal sprites in.
        m_order.clear();
        for (const Sprite& sprite : sprites)
            m_order.push_back(&sprite);
        std::ranges::stable_sort(m_order, [&](const Sprite* a, const Sprite* b)
        {
            if (a->blend != b->blend)
                return a->blend == SpriteBlend::Alpha;
            if (a->blend == SpriteBlend::Alpha)
            {
                const glm::vec3 toA = a->center - cameraPosition;
                const glm::vec3 toB = b->center - cameraPosition;
                return glm::dot(toA, toA) > glm::dot(toB, toB);
            }
            return a->texture.index < b->texture.index;
        });

        // The corners of every sprite. Texture coordinates: (0, 0) is the bottom left of the texture (see Core::Image).
        m_vertices.clear();
        for (const Sprite* sprite : m_order)
        {
            glm::vec3 right = sprite->right;
            glm::vec3 up = sprite->up;
            if (sprite->isBillboard)
            {
                // The camera's right and up, turned by the rotation of the sprite in the plane of the screen.
                const float cosine = std::cos(sprite->rotation);
                const float sine = std::sin(sprite->rotation);
                right = (cameraRight * cosine + cameraUp * sine) * sprite->halfSize;
                up = (cameraUp * cosine - cameraRight * sine) * sprite->halfSize;
            }

            // The part of the texture the sprite shows: all of it, or one frame of a flipbook.
            const glm::vec3& center = sprite->center;
            const glm::vec4& color = sprite->color;
            const glm::vec2& low = sprite->texCoordMinimum;
            const glm::vec2& high = sprite->texCoordMaximum;
            const SpriteVertex bottomLeft{.position = center - right - up, .texCoord = low, .color = color};
            const SpriteVertex bottomRight{.position = center + right - up, .texCoord = {high.x, low.y}, .color = color};
            const SpriteVertex topRight{.position = center + right + up, .texCoord = high, .color = color};
            const SpriteVertex topLeft{.position = center - right + up, .texCoord = {low.x, high.y}, .color = color};
            m_vertices.insert(m_vertices.end(), {bottomLeft, bottomRight, topRight, bottomLeft, topRight, topLeft});
        }

        if (m_vertices.size() > m_vertexCapacity)
            CreateBuffer(m_vertices.size() * 2);
        m_vertexBuffer.Update(std::as_bytes(std::span(m_vertices)));

        program.Use();
        program.SetUniform(ViewUniform, viewMatrix);
        program.SetUniform(ProjectionUniform, projectionMatrix);
        m_vertexArray.Bind();

        // See-through: tested against the depth of the world, but not written into it. Both sides visible. A small
        // polygon offset pulls the sprites a hair towards the camera, so marks lying on a wall do not flicker with it.
        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE);
        glDisable(GL_CULL_FACE);
        glEnable(GL_BLEND);
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(-1.0f, -1.0f);

        // One draw call for every run of sprites with the same texture and blend.
        int drawCallCount = 0;
        std::size_t runStart = 0;
        while (runStart < m_order.size())
        {
            const Sprite* first = m_order[runStart];
            std::size_t runEnd = runStart + 1;
            const auto isSameRun = [first](const Sprite* sprite)
            {
                return sprite->texture == first->texture && sprite->blend == first->blend;
            };
            while (runEnd < m_order.size() && isSameRun(m_order[runEnd]))
                ++runEnd;

            SetBlend(first->blend);
            textures.Get(first->texture).Bind(AlbedoTextureUnit);
            glDrawArrays(GL_TRIANGLES, static_cast<GLint>(runStart * VerticesPerSprite),
                         static_cast<GLsizei>((runEnd - runStart) * VerticesPerSprite));
            ++drawCallCount;
            runStart = runEnd;
        }

        // Back to the states the rest of the renderer expects.
        glDisable(GL_POLYGON_OFFSET_FILL);
        glDisable(GL_BLEND);
        glDepthMask(GL_TRUE);
        glEnable(GL_CULL_FACE);

        return drawCallCount;
    }

    void SpriteRenderer::CreateBuffer(std::size_t vertexCapacity)
    {
        // The size of a buffer cannot change, so a bigger one replaces it; the vertex array must point to the new one.
        m_vertexBuffer = GLBuffer::CreateDynamic(vertexCapacity * sizeof(SpriteVertex));
        m_vertexArray.SetVertexBuffer(VertexBufferBinding, m_vertexBuffer, sizeof(SpriteVertex));
        m_vertexCapacity = vertexCapacity;
    }
}
