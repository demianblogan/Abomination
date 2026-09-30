#pragma once

#include <cstdint>

// The numbers the C++ code and the shaders agree on. A shader gives every input, uniform and texture a fixed number
// (layout(location = N), layout(binding = N)), and the C++ code uses the same number instead of asking OpenGL for it
// by name at run time. All of them are here, so a change is made in one place, together with the shaders listed.
namespace Abomination::Renderer
{
    // --- Meshes: TexturedShaded, Wireframe and the fallback program of ShaderStore ---

    // Vertex inputs: layout(location = N) in ... . They follow the layout of MeshVertex (see Mesh::Create); a shader may
    // leave out what it does not need (Wireframe reads only the position).
    inline constexpr std::uint32_t MeshPositionAttribute = 0;
    inline constexpr std::uint32_t MeshTexCoordAttribute = 1;
    inline constexpr std::uint32_t MeshNormalAttribute = 2;

    // The texture unit of uniAlbedoTexture: layout(binding = 0) uniform sampler2D.
    inline constexpr std::uint32_t AlbedoTextureUnit = 0;

    // --- Every shader that places vertices in the world (all mesh shaders and DebugLines) ---

    // Uniforms: layout(location = N) uniform mat4 ... . DebugLines has no model matrix: its lines are in world coordinates
    // already, so it starts at the view matrix.
    inline constexpr std::uint32_t ModelUniform = 0;
    inline constexpr std::uint32_t ViewUniform = 1;
    inline constexpr std::uint32_t ProjectionUniform = 2;

    // --- DebugLines (see DebugLineRenderer) ---

    // Vertex inputs: both ends of the line, its color and which corner of the strip the vertex is.
    inline constexpr std::uint32_t DebugLineStartAttribute = 0;
    inline constexpr std::uint32_t DebugLineEndAttribute = 1;
    inline constexpr std::uint32_t DebugLineColorAttribute = 2;
    inline constexpr std::uint32_t DebugLineCornerAttribute = 3;

    // Uniforms after the view and projection matrices: the size of the frame and the width of the lines, in pixels.
    inline constexpr std::uint32_t DebugLineViewportSizeUniform = 3;
    inline constexpr std::uint32_t DebugLineWidthUniform = 4;

    // --- Sprite (see SpriteRenderer) ---

    // Vertex inputs: the corner in world coordinates (no model matrix, like DebugLines), its texture coordinates and the
    // color that tints the texture. The texture is uniAlbedoTexture (AlbedoTextureUnit).
    inline constexpr std::uint32_t SpritePositionAttribute = 0;
    inline constexpr std::uint32_t SpriteTexCoordAttribute = 1;
    inline constexpr std::uint32_t SpriteColorAttribute = 2;
}
