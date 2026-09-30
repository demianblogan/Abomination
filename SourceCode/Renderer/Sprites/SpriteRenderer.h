#pragma once

#include "Renderer/Assets/TextureStore.h"
#include "Renderer/OpenGL/GLBuffer.h"
#include "Renderer/OpenGL/GLShaderProgram.h"
#include "Renderer/OpenGL/GLVertexArray.h"
#include "Renderer/Sprites/SpriteBatch.h"

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <cstddef>
#include <vector>

namespace Abomination::Renderer
{
    // Draws the sprites of a SpriteBatch: particles, the muzzle flash, marks on walls.
    //
    // Sprites are drawn after the solid world with the depth test on, so walls in front of them hide them, but without
    // writing depth, so a sprite never hides another sprite (they are see-through). Alpha sprites are drawn first, sorted
    // from the farthest to the nearest (a see-through surface must be drawn over what is behind it, never under it); then
    // additive sprites, in any order. Consecutive sprites with the same texture and blend are drawn with one call.
    //
    // Every sprite becomes 2 triangles in one dynamic vertex buffer reused every frame, which grows when a frame has more
    // sprites than fit. Requires a current OpenGL context. Move-only.
    class SpriteRenderer
    {
    public:
        SpriteRenderer();

        // Draws the sprites with the program (Shaders/Sprite), seen through the view and projection matrices. Billboards
        // are turned to face the camera of viewMatrix. Returns the number of draw calls.
        int Draw(const SpriteBatch& batch, const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix,
                 const TextureStore& textures, const GLShaderProgram& program);

    private:
        struct SpriteVertex
        {
            glm::vec3 position{0.0f};
            glm::vec2 texCoord{0.0f};
            glm::vec4 color{1.0f};
        };

        // Creates the buffer for vertexCapacity vertices and connects it to the vertex array.
        void CreateBuffer(std::size_t vertexCapacity);

        // Kept between frames so their memory is reused.
        std::vector<const Sprite*> m_order;
        std::vector<SpriteVertex> m_vertices;
        std::size_t m_vertexCapacity = 0;

        // The vertex array only refers to the buffer, so it is declared after it and destroyed before it.
        GLBuffer m_vertexBuffer;
        GLVertexArray m_vertexArray;
    };
}
