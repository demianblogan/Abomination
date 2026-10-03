#version 460 core

// A corner of the game interface (see Renderer::RmlUiRendererBackend): its position in window pixels from the top left
// corner (+Y down), moved by the translation of the element it belongs to and by its transform (CSS "transform", the
// identity for most elements); the orthographic projection maps it to the screen.

layout(location = 0) in vec2 aPosition;
layout(location = 1) in vec4 aColor;
layout(location = 2) in vec2 aTexCoord;

layout(location = 2) uniform mat4 uniProjection;
layout(location = 3) uniform vec2 uniTranslation;
layout(location = 4) uniform mat4 uniTransform;

layout(location = 0) out vec4 Color;
layout(location = 1) out vec2 TexCoord;

void main()
{
    Color = aColor;
    TexCoord = aTexCoord;
    gl_Position = uniProjection * uniTransform * vec4(aPosition + uniTranslation, 0.0, 1.0);
}
