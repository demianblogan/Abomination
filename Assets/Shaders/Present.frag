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
const int ToneMappingHillACES = 2;
const int ToneMappingAgX = 3;

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

// ACES as fitted by Stephen Hill (BakingLab, MIT license), closer to the real ACES than the curve above: the colors go into
// the working space of ACES (a matrix), through a fit of its two transforms (the "look" and the one for an sRGB screen),
// and back. It is applied to the mix of the channels, not to each alone: bright colors keep their hue better and go
// less yellow; the whole picture is a little darker. GLSL fills a mat3 by columns, so the rows of the published
// matrices are written here as columns.
vec3 ApplyHillACES(vec3 color)
{
    const mat3 InputMatrix = mat3(0.59719, 0.07600, 0.02840,
                                  0.35458, 0.90834, 0.13383,
                                  0.04823, 0.01566, 0.83777);
    const mat3 OutputMatrix = mat3(1.60475, -0.10208, -0.00327,
                                   -0.53108, 1.10813, -0.07276,
                                   -0.07367, -0.00605, 1.07602);
    vec3 v = InputMatrix * color;
    vec3 numerator = v * (v + 0.0245786) - 0.000090537;
    vec3 denominator = v * (0.983729 * v + 0.4329510) + 0.238081;
    return clamp(OutputMatrix * (numerator / denominator), 0.0, 1.0);
}

// AgX (Troy Sobotka, the default of Blender 4; this short form by Benjamin Wrensch): the colors are first mixed a little
// towards each other (a matrix), so a very bright color goes to white instead of staying saturated: a flame burns
// white-yellow at its core, like in a photograph. Then the brightness becomes stops (log2) between -12.5 and +4,
// fitted into 0..1, and an S-curve (a polynomial fit) gives the contrast. Finally the mix is undone and the result
// is turned back into linear values (the power 2.2).
vec3 ApplyAgX(vec3 color)
{
    const mat3 InsetMatrix = mat3(0.842479062253094, 0.0423282422610123, 0.0423756549057051,
                                  0.0784335999999992, 0.878468636469772, 0.0784336,
                                  0.0792237451477643, 0.0791661274605434, 0.879142973793104);
    const mat3 OutsetMatrix = mat3(1.19687900512017, -0.0528968517574562, -0.0529716355144438,
                                   -0.0980208811401368, 1.15190312990417, -0.0980434501171241,
                                   -0.0990297440797205, -0.0989611768448433, 1.15107367264116);
    const float MinimumStops = -12.47393;
    const float MaximumStops = 4.026069;

    vec3 v = InsetMatrix * color;
    v = clamp(log2(max(v, vec3(1e-10))), MinimumStops, MaximumStops);
    v = (v - MinimumStops) / (MaximumStops - MinimumStops);

    vec3 v2 = v * v;
    vec3 v4 = v2 * v2;
    v = 15.5 * v4 * v2 - 40.14 * v4 * v + 31.96 * v4 - 6.868 * v2 * v + 0.4298 * v2 + 0.1191 * v - 0.00232;

    return pow(clamp(OutsetMatrix * v, 0.0, 1.0), vec3(2.2));
}

void main()
{
    // The pixel of the scene under this pixel of the screen: both have the same size, so it is read 1:1 by its
    // coordinates (texelFetch: no filtering, mipmap level 0).
    vec3 color = texelFetch(uniSceneColor, ivec2(gl_FragCoord.xy), 0).rgb;

    // Exposure first, like the shutter of a camera, then the scene is fitted into the 0..1 the screen shows.
    color *= uniExposure;
    if (uniToneMapping == ToneMappingACES)
        color = ApplyACES(color);
    else if (uniToneMapping == ToneMappingHillACES)
        color = ApplyHillACES(color);
    else if (uniToneMapping == ToneMappingAgX)
        color = ApplyAgX(color);
    else
        color = clamp(color, 0.0, 1.0);

    FragColor = vec4(ConvertLinearToSRGB(color), 1.0);
}
