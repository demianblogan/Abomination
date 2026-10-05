#version 460 core

// Writes the scene onto the screen. The scene is drawn into an HDR framebuffer in linear values (proportional to the
// amount of light); the screen expects sRGB numbers, so every pixel is converted here (see Renderer::SceneFramebuffer).

layout(binding = 0) uniform sampler2D uniSceneColor;

layout(location = 0) out vec4 FragColor;

// A linear value (0..1) as an sRGB number, by the exact formula of the standard (the same as
// Renderer::ConvertLinearToSRGB): a short straight piece near black, then a power of 1/2.4.
vec3 ConvertLinearToSRGB(vec3 color)
{
    vec3 straight = color * 12.92;
    vec3 curved = 1.055 * pow(color, vec3(1.0 / 2.4)) - 0.055;
    return mix(curved, straight, lessThanEqual(color, vec3(0.0031308)));
}

void main()
{
    // The pixel of the scene under this pixel of the screen: both have the same size, so it is read 1:1 by its
    // coordinates (texelFetch: no filtering, mipmap level 0).
    vec3 color = texelFetch(uniSceneColor, ivec2(gl_FragCoord.xy), 0).rgb;

    // The screen shows 0..1: anything brighter is cut off for now (tone mapping comes next).
    FragColor = vec4(ConvertLinearToSRGB(clamp(color, 0.0, 1.0)), 1.0);
}
