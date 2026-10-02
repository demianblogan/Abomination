#version 460 core

// The texture tinted by the color of the vertex. RmlUi gives colors and textures with premultiplied alpha (the color
// already multiplied by its alpha), so they are blended with GL_ONE, GL_ONE_MINUS_SRC_ALPHA. Untextured geometry gets a
// white texture, so the color alone shows.

layout(location = 0) in vec4 Color;
layout(location = 1) in vec2 TexCoord;

layout(binding = 0) uniform sampler2D uniAlbedoTexture;

layout(location = 0) out vec4 FragColor;

void main()
{
    FragColor = texture(uniAlbedoTexture, TexCoord) * Color;
}
