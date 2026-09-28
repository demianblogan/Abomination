#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

namespace Abomination::Renderer
{
    // A block of memory owned by the graphics driver, usually in video memory: vertices, indices, shader data.
    // Two kinds:
    //   - static (the constructor): the data is uploaded once and never changes, which lets the driver place it in the
    //     fastest memory. Meshes use it;
    //   - dynamic (CreateDynamic): a fixed size, but the contents can be replaced with Update() as often as needed.
    //     Data that changes every frame (debug lines) uses it.
    // Both use immutable storage: the size never changes. The buffer is deleted in the destructor. Move-only.
    class GLBuffer
    {
    public:
        // Creates the buffer and copies the bytes into it. std::as_bytes(std::span(array)) turns any array into bytes.
        explicit GLBuffer(std::span<const std::byte> data);

        // Creates an empty dynamic buffer of byteCount bytes; fill it with Update().
        [[nodiscard]] static GLBuffer CreateDynamic(std::size_t byteCount);

        GLBuffer(const GLBuffer&) = delete;
        GLBuffer& operator=(const GLBuffer&) = delete;

        GLBuffer(GLBuffer&& other) noexcept;
        GLBuffer& operator=(GLBuffer&& other) noexcept;

        ~GLBuffer();

        // Copies data into a dynamic buffer, starting at its first byte. data must fit into the buffer. The draw calls issued
        // before still see the old contents: OpenGL takes care of the order.
        void Update(std::span<const std::byte> data);

        // The name OpenGL gave the buffer; other OpenGL wrappers (GLVertexArray) need it.
        [[nodiscard]] std::uint32_t GetID() const noexcept;

    private:
        GLBuffer(std::uint32_t bufferID, std::size_t byteCount) noexcept;

        // 0 means "no buffer" (a moved-from object).
        std::uint32_t m_bufferID = 0;

        // The size of the buffer, which never changes; Update() checks that its data fits.
        std::size_t m_byteCount = 0;
    };
}
