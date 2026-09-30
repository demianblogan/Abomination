#version 460 core

// The texture of a sprite tinted by its color. The blending (see Renderer::SpriteBlend) mixes it with what is behind.

layout(location = 0) in vec2 TexCoord;
layout(location = 1) in vec4 Color;

layout(binding = 0) uniform sampler2D uniAlbedoTexture;

layout(location = 0) out vec4 FragColor;

void main()
{
    vec4 color = texture(uniAlbedoTexture, TexCoord) * Color;

    // Fully transparent pixels change nothing on the screen; throwing them away saves the work of blending them.
    if (color.a < 0.01)
        discard;

    FragColor = color;
}
