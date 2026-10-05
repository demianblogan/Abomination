#version 460 core

// The color of the line, the same along its whole length.

layout(location = 0) in vec3 Color;

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
    FragColor = vec4(ConvertSRGBToLinear(Color), 1.0);
}
