#include "Renderer/RmlUiRendererBackend.h"

#include "Core/Files/FileSystem.h"
#include "Core/Files/Image.h"
#include "Core/Logging/Log.h"
#include "Renderer/OpenGL/GPUProfileZone.h"
#include "Renderer/OpenGL/ShaderInterface.h"

#include <glad/gl.h>
#include <glm/ext/matrix_clip_space.hpp>

#include <algorithm>
#include <cstdint>
#include <span>
#include <utility>

namespace Abomination::Renderer
{
    namespace
    {
        // The vertex array of every geometry has one vertex buffer, connected to binding slot 0.
        constexpr std::uint32_t VertexBufferBinding = 0;

        GLTexture CreateWhiteTexture()
        {
            const Core::Image white{.width = 1, .height = 1, .pixels = {255, 255, 255, 255}};
            return GLTexture::CreateFromImage(white, TextureFiltering::Pixelated, TextureEncoding::Raw);
        }

        // The texture of a handle RmlUi gives back: the address of the GLTexture (see LoadTexture), or 0 for none.
        const GLTexture* GetTexture(Rml::TextureHandle handle)
        {
            return reinterpret_cast<const GLTexture*>(handle);
        }

        // Moves a texture to the heap and gives RmlUi its address as the handle; ReleaseTexture deletes it.
        Rml::TextureHandle ToHandle(GLTexture texture)
        {
            return reinterpret_cast<Rml::TextureHandle>(new GLTexture(std::move(texture)));
        }
    }

    RmlUiRendererBackend::RmlUiRendererBackend()
        : m_whiteTexture(CreateWhiteTexture())
    {}

    RmlUiRendererBackend::~RmlUiRendererBackend() = default;

    void RmlUiRendererBackend::BeginFrame(glm::vec2 viewportSize, const GLShaderProgram& program)
    {
        m_viewportSize = viewportSize;
        m_program = &program;
        m_drawCallCount = 0;

        // Left 0, right width, bottom height, top 0: pixel (0, 0) is the top left corner and +Y goes down, the way RmlUi
        // lays out documents.
        program.Use();
        program.SetUniform(ProjectionUniform, glm::ortho(0.0f, viewportSize.x, viewportSize.y, 0.0f, -1.0f, 1.0f));
        program.SetUniform(GameUITransformUniform, glm::mat4(1.0f));

        // Over everything, in the order RmlUi draws, both sides visible (+Y down turns the corners the other way round).
        // The colors come with premultiplied alpha: the color written = new color + old color * (1 - new alpha).
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    }

    void RmlUiRendererBackend::EndFrame()
    {
        glDisable(GL_SCISSOR_TEST);
        glDisable(GL_BLEND);
        glEnable(GL_CULL_FACE);
        glEnable(GL_DEPTH_TEST);
        m_program = nullptr;
    }

    int RmlUiRendererBackend::GetDrawCallCount() const noexcept
    {
        return m_drawCallCount;
    }

    Rml::CompiledGeometryHandle RmlUiRendererBackend::CompileGeometry(Rml::Span<const Rml::Vertex> vertices,
                                                                      Rml::Span<const int> indices)
    {
        // Nothing to draw (an empty text): handle 0 tells RmlUi there is no geometry. A buffer cannot be empty.
        if (vertices.empty() || indices.empty())
            return 0;

        auto* geometry = new Geometry{
            .vertexBuffer = GLBuffer(std::as_bytes(std::span(vertices.data(), vertices.size()))),
            .indexBuffer = GLBuffer(std::as_bytes(std::span(indices.data(), indices.size()))),
            .vertexArray = GLVertexArray(),
            .vertexCount = vertices.size(),
            .indexCount = indices.size(),
        };

        // Rml::Vertex: a position (2 floats), a color (4 bytes) and texture coordinates (2 floats).
        GLVertexArray& vertexArray = geometry->vertexArray;
        vertexArray.SetVertexBuffer(VertexBufferBinding, geometry->vertexBuffer, sizeof(Rml::Vertex));
        vertexArray.SetFloatAttribute(GameUIPositionAttribute, VertexBufferBinding, 2, offsetof(Rml::Vertex, position));
        vertexArray.SetNormalizedByteAttribute(GameUIColorAttribute, VertexBufferBinding, 4, offsetof(Rml::Vertex, colour));
        vertexArray.SetFloatAttribute(GameUITexCoordAttribute, VertexBufferBinding, 2, offsetof(Rml::Vertex, tex_coord));
        vertexArray.SetIndexBuffer(geometry->indexBuffer);

        return reinterpret_cast<Rml::CompiledGeometryHandle>(geometry);
    }

    void RmlUiRendererBackend::RenderGeometry(Rml::CompiledGeometryHandle geometryHandle, Rml::Vector2f translation,
                                              Rml::TextureHandle textureHandle)
    {
        const auto* geometry = reinterpret_cast<const Geometry*>(geometryHandle);
        if (geometry == nullptr)
            return;

        // One GPU zone per piece of the interface; the Statistics window of the profiler adds them up.
        PROFILE_GPU_ZONE("Game interface");

        const GLTexture* texture = GetTexture(textureHandle);

        m_program->SetUniform(GameUITranslationUniform, glm::vec2(translation.x, translation.y));
        (texture != nullptr ? *texture : m_whiteTexture).Bind(AlbedoTextureUnit);
        geometry->vertexArray.Bind();

        // The indices are ints that are never negative, so they can be read as unsigned ones.
        // The range of the indices is given, so the driver does not read them all on the CPU to find it (see Mesh::Draw).
        glDrawRangeElements(GL_TRIANGLES, 0, static_cast<GLuint>(geometry->vertexCount - 1),
                            static_cast<GLsizei>(geometry->indexCount), GL_UNSIGNED_INT, nullptr);
        ++m_drawCallCount;
    }

    void RmlUiRendererBackend::ReleaseGeometry(Rml::CompiledGeometryHandle geometry)
    {
        delete reinterpret_cast<Geometry*>(geometry);
    }

    Rml::TextureHandle RmlUiRendererBackend::LoadTexture(Rml::Vector2i& textureDimensions, const Rml::String& source)
    {
        // source is the path of an image used by a document (an icon), already joined with the folder of the document.
        std::expected<Core::Image, std::string> image =
            Core::LoadImageFile(std::filesystem::path(reinterpret_cast<const char8_t*>(source.c_str())));
        if (!image.has_value())
        {
            Core::Log::Write(Core::LogCategory::UI, Core::LogLevel::Warning, "Interface image not loaded: {}", image.error());
            return 0;
        }

        // RmlUi expects the top row first (texture coordinate 0 at the top) and premultiplied alpha, while Core::Image
        // keeps the bottom row first and straight alpha.
        Core::Image& pixels = *image;
        const std::size_t rowSize = static_cast<std::size_t>(pixels.width) * Core::ImageChannelCount;
        std::uint8_t* data = pixels.pixels.data();
        for (std::size_t row = 0; row < static_cast<std::size_t>(pixels.height) / 2; ++row)
        {
            std::uint8_t* top = data + row * rowSize;
            std::uint8_t* bottom = data + (static_cast<std::size_t>(pixels.height) - 1 - row) * rowSize;
            std::swap_ranges(top, top + rowSize, bottom);
        }

        for (std::size_t index = 0; index < pixels.pixels.size(); index += Core::ImageChannelCount)
        {
            const unsigned int alpha = pixels.pixels[index + 3];
            for (std::size_t channel = 0; channel < 3; ++channel)
                pixels.pixels[index + channel] = static_cast<std::uint8_t>(pixels.pixels[index + channel] * alpha / 255);
        }

        textureDimensions = {pixels.width, pixels.height};
        return ToHandle(GLTexture::CreateFromImage(pixels, TextureFiltering::Smooth, TextureEncoding::Raw));
    }

    Rml::TextureHandle RmlUiRendererBackend::GenerateTexture(Rml::Span<const Rml::byte> source,
                                                             Rml::Vector2i sourceDimensions)
    {
        // A texture RmlUi made itself, for example the letters of a font at one size: RGBA, top row first, premultiplied.
        // It is uploaded as it is, so its first row is at texture coordinate 0, where RmlUi expects it.
        Core::Image image{.width = sourceDimensions.x, .height = sourceDimensions.y};
        image.pixels.assign(reinterpret_cast<const std::uint8_t*>(source.data()),
                            reinterpret_cast<const std::uint8_t*>(source.data()) + source.size());
        return ToHandle(GLTexture::CreateFromImage(image, TextureFiltering::Smooth, TextureEncoding::Raw));
    }

    void RmlUiRendererBackend::ReleaseTexture(Rml::TextureHandle texture)
    {
        delete reinterpret_cast<GLTexture*>(texture);
    }

    void RmlUiRendererBackend::EnableScissorRegion(bool enable)
    {
        if (enable)
            glEnable(GL_SCISSOR_TEST);
        else
            glDisable(GL_SCISSOR_TEST);
    }

    void RmlUiRendererBackend::SetTransform(const Rml::Matrix4f* transform)
    {
        // RmlUi keeps its matrices by columns, like OpenGL and glm, so the 16 numbers are copied as they are. nullptr means
        // no transform. A transformed element is still clipped by the scissor of its parent, which is enough for the HUD.
        glm::mat4 matrix(1.0f);
        if (transform != nullptr)
            std::copy_n(transform->data(), 16, &matrix[0][0]);
        m_program->SetUniform(GameUITransformUniform, matrix);
    }

    void RmlUiRendererBackend::SetScissorRegion(Rml::Rectanglei region)
    {
        // Only pixels inside the rectangle are drawn (a scrolled list cuts off what is outside it). OpenGL counts rows
        // from the bottom of the window, RmlUi from the top, so the rectangle is turned upside down.
        const int bottom = static_cast<int>(m_viewportSize.y) - region.Bottom();
        glScissor(region.Left(), bottom, region.Width(), region.Height());
    }
}
