#version 460 core

// Writes the scene onto the screen. The scene is drawn into an HDR framebuffer in linear values (proportional to the
// amount of light); the screen expects sRGB numbers, so every pixel is converted here (see Renderer::SceneFramebuffer).

layout(binding = 0) uniform sampler2D uniSceneColor;

// The brightness multiplier (2 to the power of the exposure stops of the Renderer window) and the tone mapping: the value
// of Renderer::ToneMapping.
layout(location = 0) uniform float uniExposure;
layout(location = 1) uniform int uniToneMapping;

const int ToneMappingNone = 0;
const int ToneMappingACES = 1;

layout(location = 0) out vec4 FragColor;

// A linear value (0..1) as an sRGB number, by the exact formula of the standard (the same as
// Renderer::ConvertLinearToSRGB): a short straight piece near black, then a power of 1/2.4.
vec3 ConvertLinearToSRGB(vec3 color)
{
    vec3 straight = color * 12.92;
    vec3 curved = 1.055 * pow(color, vec3(1.0 / 2.4)) - 0.055;
    return mix(curved, straight, lessThanEqual(color, vec3(0.0031308)));
}

// The ACES filmic curve as approximated by Krzysztof Narkowicz (2015): fits any brightness into 0..1. Bright values are
// pressed together more and more and never reach a hard edge (1.0 -> 0.80, 2.0 -> 0.91, 4.0 -> 0.97); the middle is lifted
// (0.18 -> 0.27) and the deepest shadows pushed down (0.02 -> 0.01): an S-shaped curve, more contrast. It is applied to
// red, green and blue apart, so a color whose channels differ gets them pulled further apart: colors look more
// saturated, and their hue moves a little (the olive of the moss towards yellow). That is the look of this curve.
vec3 ApplyACES(vec3 color)
{
    vec3 numerator = color * (2.51 * color + 0.03);
    vec3 denominator = color * (2.43 * color + 0.59) + 0.14;
    return clamp(numerator / denominator, 0.0, 1.0);
}

void main()
{
    // The pixel of the scene under this pixel of the screen: both have the same size, so it is read 1:1 by its
    // coordinates (texelFetch: no filtering, mipmap level 0).
    vec3 color = texelFetch(uniSceneColor, ivec2(gl_FragCoord.xy), 0).rgb;

    // Exposure first, like the shutter of a camera, then the scene is fitted into the 0..1 the screen shows.
    color *= uniExposure;
    color = uniToneMapping == ToneMappingACES ? ApplyACES(color) : clamp(color, 0.0, 1.0);

    FragColor = vec4(ConvertLinearToSRGB(color), 1.0);
}
