#version 460 core

// A corner of a sprite (see Renderer::SpriteRenderer): already in world coordinates, so no model matrix, like DebugLines.

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec2 aTexCoord;
layout(location = 2) in vec4 aColor;

layout(location = 1) uniform mat4 uniView;
layout(location = 2) uniform mat4 uniProjection;

layout(location = 0) out vec2 TexCoord;
layout(location = 1) out vec4 Color;

void main()
{
    TexCoord = aTexCoord;
    Color = aColor;
    gl_Position = uniProjection * uniView * vec4(aPosition, 1.0);
}
