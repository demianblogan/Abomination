#version 460 core

// The texture of a sprite tinted by its color. The blending (see Renderer::SpriteBlend) mixes it with what is behind.

layout(location = 0) in vec2 TexCoord;
layout(location = 1) in vec4 Color;

layout(binding = 0) uniform sampler2D uniAlbedoTexture;

layout(location = 0) out vec4 FragColor;

// An sRGB number (0..1) as a linear value, by the exact formula of the standard (the same as
// Renderer::ConvertSRGBToLinear): the colors given in the code are sRGB, the way they were picked, but the scene is drawn
// in linear values.
vec3 ConvertSRGBToLinear(vec3 color)
{
    vec3 straight = color / 12.92;
    vec3 curved = pow((color + 0.055) / 1.055, vec3(2.4));
    return mix(curved, straight, lessThanEqual(color, vec3(0.04045)));
}

void main()
{
    // The texture is sRGB and read as linear values (see Renderer::TextureEncoding); the tint is converted here.
    vec4 color = texture(uniAlbedoTexture, TexCoord) * vec4(ConvertSRGBToLinear(Color.rgb), Color.a);

    // Fully transparent pixels change nothing on the screen; throwing them away saves the work of blending them.
    if (color.a < 0.01)
        discard;

    FragColor = color;
}
