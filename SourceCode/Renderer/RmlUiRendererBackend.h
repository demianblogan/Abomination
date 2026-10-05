#pragma once

#include "Renderer/OpenGL/GLBuffer.h"
#include "Renderer/OpenGL/GLShaderProgram.h"
#include "Renderer/OpenGL/GLTexture.h"
#include "Renderer/OpenGL/GLVertexArray.h"

#include <RmlUi/Core/RenderInterface.h>

#include <glm/vec2.hpp>

#include <cstddef>

namespace Abomination::Renderer
{
    // The OpenGL part of RmlUi, the library of the game interface (the HUD, later the menus): RmlUi lays out documents and
    // turns them into triangles, textures and clipping rectangles, and calls this class to keep and draw them.
    //
    // RmlUi refers to the geometry and the textures it gets back by numbers (handles); here a handle is the address of
    // the object that owns the OpenGL buffers or the texture. Everything is drawn in window pixels (+Y down) over the game
    // with premultiplied alpha, without a depth test. Requires a current OpenGL context. Not copyable or movable: RmlUi
    // keeps a pointer to it.
    class RmlUiRendererBackend final : public Rml::RenderInterface
    {
    public:
        RmlUiRendererBackend();

        RmlUiRendererBackend(const RmlUiRendererBackend&) = delete;
        RmlUiRendererBackend& operator=(const RmlUiRendererBackend&) = delete;

        ~RmlUiRendererBackend() override;

        // Called before the context of RmlUi is drawn: the size of the window in pixels and the program (Shaders/GameUI)
        // the frame is drawn with. Sets the OpenGL states of the interface pass.
        void BeginFrame(glm::vec2 viewportSize, const GLShaderProgram& program);

        // Called after the context is drawn: back to the states the rest of the renderer expects.
        void EndFrame();

        // How many draw calls the interface made in the last frame (for the Renderer window).
        [[nodiscard]] int GetDrawCallCount() const noexcept;

        // Rml::RenderInterface.
        Rml::CompiledGeometryHandle CompileGeometry(Rml::Span<const Rml::Vertex> vertices,
                                                    Rml::Span<const int> indices) override;
        void RenderGeometry(Rml::CompiledGeometryHandle geometry, Rml::Vector2f translation,
                            Rml::TextureHandle texture) override;
        void ReleaseGeometry(Rml::CompiledGeometryHandle geometry) override;
        Rml::TextureHandle LoadTexture(Rml::Vector2i& textureDimensions, const Rml::String& source) override;
        Rml::TextureHandle GenerateTexture(Rml::Span<const Rml::byte> source, Rml::Vector2i sourceDimensions) override;
        void ReleaseTexture(Rml::TextureHandle texture) override;
        void EnableScissorRegion(bool enable) override;
        void SetScissorRegion(Rml::Rectanglei region) override;
        void SetTransform(const Rml::Matrix4f* transform) override;

    private:
        // One piece of geometry RmlUi compiled: its vertices and indices in video memory. Declared in this order so the
        // vertex array, which only refers to the buffers, is destroyed first.
        struct Geometry
        {
            GLBuffer vertexBuffer;
            GLBuffer indexBuffer;
            GLVertexArray vertexArray;
            std::size_t vertexCount = 0;
            std::size_t indexCount = 0;
        };

        // The texture of untextured geometry: a single white texel, so the vertex colors alone show.
        GLTexture m_whiteTexture;

        // Valid only between BeginFrame and EndFrame.
        const GLShaderProgram* m_program = nullptr;
        glm::vec2 m_viewportSize{0.0f};

        int m_drawCallCount = 0;
    };
}
