#pragma once

#include <cstdint>

// The numbers the C++ code and the shaders agree on. A shader gives every input, uniform and texture a fixed number
// (layout(location = N), layout(binding = N)), and the C++ code uses the same number instead of asking OpenGL for it
// by name at run time. All of them are here, so a change is made in one place, together with the shaders listed.
namespace Abomination::Renderer
{
    // --- Meshes: Lit, Wireframe and the fallback program of ShaderStore ---

    // Vertex inputs: layout(location = N) in ... . They follow the layout of MeshVertex (see Mesh::Create); a shader may
    // leave out what it does not need (Wireframe reads only the position).
    inline constexpr std::uint32_t MeshPositionAttribute = 0;
    inline constexpr std::uint32_t MeshTexCoordAttribute = 1;
    inline constexpr std::uint32_t MeshNormalAttribute = 2;

    // A skinned mesh has two more inputs, from its second vertex buffer (see VertexSkin): the joints that pull a vertex
    // (uvec4) and how much (vec4). A rigid mesh leaves them disabled, and the shader does not read them.
    inline constexpr std::uint32_t MeshJointsAttribute = 3;
    inline constexpr std::uint32_t MeshWeightsAttribute = 4;

    // The tangent of a vertex (vec4, see MeshVertex::tangent), for normal maps. Every mesh has it.
    inline constexpr std::uint32_t MeshTangentAttribute = 5;

    // The uniform that tells Lit and Wireframe whether the mesh is skinned: layout(location = 3) uniform bool.
    inline constexpr std::uint32_t IsSkinnedUniform = 3;

    // The matrices of the joints of a skinned mesh (see CalculateSkinningMatrices), read by the vertex shader from a
    // shader storage buffer: layout(std430, binding = 0) readonly buffer. Unlike a uniform array, it has no small size
    // limit, and the whole array is uploaded with one call.
    inline constexpr std::uint32_t JointMatricesStorageBinding = 0;

    // The texture unit of uniAlbedoTexture: layout(binding = 0) uniform sampler2D. Also the color of a Lit material.
    inline constexpr std::uint32_t AlbedoTextureUnit = 0;

    // --- Lit: the maps and numbers of a material (see Renderer::Material) and the light of the scene ---

    // The other maps of the material: layout(binding = N) uniform sampler2D.
    inline constexpr std::uint32_t NormalTextureUnit = 1;
    inline constexpr std::uint32_t MetalRoughnessTextureUnit = 2;
    inline constexpr std::uint32_t EmissiveTextureUnit = 3;

    // The factors of the material: vec4 color, float roughness, float metalness, vec3 emission.
    inline constexpr std::uint32_t BaseColorFactorUniform = 4;
    inline constexpr std::uint32_t RoughnessFactorUniform = 5;
    inline constexpr std::uint32_t MetalnessFactorUniform = 6;
    inline constexpr std::uint32_t EmissiveFactorUniform = 7;

    // The light of the scene (see Renderer::SceneLighting), in the coordinates of the camera: vec3 direction towards the
    // sun, vec3 its light, vec3 the light from everywhere.
    inline constexpr std::uint32_t SunDirectionUniform = 8;
    inline constexpr std::uint32_t SunColorUniform = 9;
    inline constexpr std::uint32_t AmbientColorUniform = 10;

    // What the fragment shader shows: the value of Renderer::ShadingView (int).
    inline constexpr std::uint32_t ShadingViewUniform = 11;

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

    // --- GameUI (see RmlUiRendererBackend) ---

    // Vertex inputs: the corner in window pixels, its color (4 bytes, alpha premultiplied) and texture coordinates.
    inline constexpr std::uint32_t GameUIPositionAttribute = 0;
    inline constexpr std::uint32_t GameUIColorAttribute = 1;
    inline constexpr std::uint32_t GameUITexCoordAttribute = 2;

    // Uniforms besides the projection (ProjectionUniform): how far the geometry is moved, in pixels. The texture is
    // uniAlbedoTexture (AlbedoTextureUnit).
    inline constexpr std::uint32_t GameUITranslationUniform = 3;

    // The transform of the element (a rotation of a hit marker line, CSS "transform"), applied after the translation.
    inline constexpr std::uint32_t GameUITransformUniform = 4;

    // --- Present (see SceneFramebuffer) ---

    // The texture unit of the HDR picture of the scene: layout(binding = 0) uniform sampler2D uniSceneColor. No vertex
    // inputs: the corners of the triangle over the screen come from gl_VertexID.
    inline constexpr std::uint32_t SceneColorTextureUnit = 0;

    // Uniforms: the exposure as a multiplier (float) and the tone mapping (int, the value of Renderer::ToneMapping).
    inline constexpr std::uint32_t PresentExposureUniform = 0;
    inline constexpr std::uint32_t PresentToneMappingUniform = 1;
}
