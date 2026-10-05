#version 460 core

// Paints every line of the wireframe in one color. Textures and shading are left out on purpose: a line one pixel
// wide that takes its color from a texture shows whatever texel it happens to cross, so the lines of textured meshes
// would be patchy and hard to see.

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

// Light gray, clearly visible on the dark background.
const vec3 LineColor = vec3(0.85, 0.85, 0.85);

void main()
{
    FragColor = vec4(ConvertSRGBToLinear(LineColor), 1.0);
}
