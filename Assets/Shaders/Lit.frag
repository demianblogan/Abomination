#version 460 core

// Lights a surface by its material (see Renderer::Material and Documentation/ARCHITECTURE.md, section 6, materials): the
// physically based model most games use, Cook-Torrance with the GGX distribution of microfacets, metallic-roughness like
// glTF. Everything is in the coordinates of the camera (see Lit.vert) and in linear values; the result goes into the HDR
// framebuffer, and Present fits it onto the screen.
//
// The light is one made-up "sun" and a weak light from everywhere (see Renderer::SceneLighting) until the level has
// lights of its own.

layout(location = 0) in vec2 TexCoord;
layout(location = 1) in vec3 ViewPosition;
layout(location = 2) in vec3 ViewNormal;
layout(location = 3) in vec4 ViewTangent;

// The maps of the material. A map the material does not have is a built-in texture of one texel (flat, white, black).
layout(binding = 0) uniform sampler2D uniAlbedoTexture;
layout(binding = 1) uniform sampler2D uniNormalTexture;
layout(binding = 2) uniform sampler2D uniMetalRoughnessTexture;
layout(binding = 3) uniform sampler2D uniEmissiveTexture;
layout(binding = 4) uniform sampler2D uniHeightTexture;

// The factors the maps are multiplied by.
layout(location = 4) uniform vec4 uniBaseColorFactor;
layout(location = 5) uniform float uniRoughnessFactor;
layout(location = 6) uniform float uniMetalnessFactor;
layout(location = 7) uniform vec3 uniEmissiveFactor;

// The light: the direction towards the sun (length 1), its light, the light from everywhere.
layout(location = 8) uniform vec3 uniSunDirection;
layout(location = 9) uniform vec3 uniSunColor;
layout(location = 10) uniform vec3 uniAmbientColor;

// What to show: the value of Renderer::ShadingView.
layout(location = 11) uniform int uniShadingView;

// How deep the relief of parallax occlusion mapping goes, in texture coordinates; 0 for a flat surface (see FollowRelief).
layout(location = 12) uniform float uniParallaxDepth;

// Whether highlights are widened where the normal changes fast (see WidenRoughness), and how many steps parallax takes.
layout(location = 13) uniform bool uniIsSpecularAntiAliasingEnabled;
layout(location = 14) uniform int uniParallaxStepCount;

layout(location = 0) out vec4 FragColor;

const float Pi = 3.14159265;

// Texels less opaque than this are cut out (see main).
const float AlphaCutoff = 0.5;

// How much light a surface that is not a metal reflects when looked at straight on: about 4% for stone, wood, plastic,
// skin (F0, the reflectance at 0 degrees). A metal reflects its own color instead.
const vec3 DielectricReflectance = vec3(0.04);

// A roughness of exactly 0 gives an infinitely small, infinitely bright highlight (a division by almost 0 below).
const float MinimumRoughness = 0.045;

const int ShadingViewFinal = 0;
const int ShadingViewBaseColor = 1;
const int ShadingViewNormals = 2;
const int ShadingViewRoughness = 3;
const int ShadingViewMetalness = 4;
const int ShadingViewHeight = 5;

// The most steps parallax occlusion mapping may take, whatever the Renderer window asks (a loop must have a bound).
const int MaximumParallaxSteps = 64;

// Specular anti-aliasing (see WidenRoughness): how strongly the change of the normal widens the highlight, and the most
// it may add, the numbers of Filament (the renderer of Google).
const float SpecularAntiAliasingVariance = 0.25;
const float SpecularAntiAliasingLimit = 0.18;

// The change of the texture coordinates from one pixel of the screen to the next, in x and in y. Every map is read with
// them (textureGrad), not with the derivatives of the coordinates after parallax, which jump at the edges of the relief
// and would pick a far too small mipmap level there.
vec2 TexCoordStepX;
vec2 TexCoordStepY;

vec4 Sample(sampler2D map, vec2 texCoord)
{
    return textureGrad(map, texCoord, TexCoordStepX, TexCoordStepY);
}

// The axes of the texture on the surface at this pixel, in the coordinates of the camera: the columns are the tangent
// (where U grows), the bitangent (where V grows) and the normal (out of the surface).
mat3 CalculateTextureAxes()
{
    vec3 normal = normalize(ViewNormal);

    // Interpolated between the vertices, the tangent may lean out of the surface a little: the part along the normal
    // is removed (Gram-Schmidt), so the three axes stay at right angles.
    vec3 tangent = normalize(ViewTangent.xyz - normal * dot(ViewTangent.xyz, normal));
    vec3 bitangent = cross(normal, tangent) * ViewTangent.w;

    return mat3(tangent, bitangent, normal);
}

// Parallax occlusion mapping: where the eye really meets the relief. The surface is drawn flat, but its height map says
// it has bumps; seen from the side, a stone in front hides the joint behind it. Starting at the flat surface (depth 0,
// the white of the height map), the ray of the eye goes into the relief in equal steps of depth, moving across the
// texture as it goes (the steeper the angle, the farther), until it is below the relief: there it has hit it. The
// texture coordinates of that point are returned, and every map is read there. viewInTexture is the direction towards
// the eye in the axes of the texture (z out of the surface, above 0).
vec2 FollowRelief(vec2 texCoord, vec3 viewInTexture)
{
    // The same number of steps at every angle: a number that changed with the angle would move the edges of the stones
    // from step to step while the camera turns, and they would ripple.
    int stepCount = clamp(uniParallaxStepCount, 1, MaximumParallaxSteps);
    float stepDepth = 1.0 / float(stepCount);

    // Across the whole depth the eye moves this far over the texture: x and y over z (similar triangles), times the depth
    // of the relief. Away from the eye, so it is subtracted. z is kept above 0.05: at a grazing angle the shift grows
    // without bound.
    vec2 shiftPerStep = viewInTexture.xy / max(viewInTexture.z, 0.05) * uniParallaxDepth / float(stepCount);

    vec2 current = texCoord;
    float rayDepth = 0.0;
    float reliefDepth = 1.0 - Sample(uniHeightTexture, current).r;
    for (int stepIndex = 0; stepIndex < stepCount && rayDepth < reliefDepth; ++stepIndex)
    {
        current -= shiftPerStep;
        rayDepth += stepDepth;
        reliefDepth = 1.0 - Sample(uniHeightTexture, current).r;
    }

    // The ray crossed the relief between the last two steps: how far below it is now and how far above it was a step
    // ago give the point of the crossing between them (the two lines meet where their differences cancel).
    vec2 previous = current + shiftPerStep;
    float belowNow = reliefDepth - rayDepth;
    float aboveBefore = (1.0 - Sample(uniHeightTexture, previous).r) - (rayDepth - stepDepth);
    float weight = belowNow / (belowNow - aboveBefore);
    return mix(current, previous, clamp(weight, 0.0, 1.0));
}

// The direction the surface faces at this pixel: the normal of the vertices, bent by the normal map. The normal map
// stores a direction in the axes of the texture (x along U, y along V, z out of the surface), each from -1..1 as 0..1;
// the axes turn it into the coordinates of the camera.
vec3 CalculateSurfaceNormal(mat3 textureAxes, vec2 texCoord)
{
    vec3 mapped = Sample(uniNormalTexture, texCoord).xyz * 2.0 - 1.0;
    return normalize(textureAxes * mapped);
}

// Specular anti-aliasing ("geometric", Kaplanyan and Hoffman 2016, as in Filament): a highlight smaller than a pixel
// flickers when the camera moves, because each pixel samples one point of it, now on it, now off it. Where the normal
// changes a lot from one pixel to the next (bumps of a normal map seen from afar, edges of the relief), the pixel
// covers many directions at once; adding that spread to the roughness widens the highlight to what the pixel really
// sees. The squares of the changes of the normal along x and y estimate the spread; it is added to roughness^2 (alpha,
// what the highlight uses) and limited, so a smooth wet floor does not turn matte far away.
float WidenRoughness(float roughness, vec3 normal)
{
    vec3 changeX = dFdx(normal);
    vec3 changeY = dFdy(normal);
    float variance = SpecularAntiAliasingVariance * (dot(changeX, changeX) + dot(changeY, changeY));
    float widening = min(2.0 * variance, SpecularAntiAliasingLimit);
    return sqrt(clamp(roughness * roughness + widening, 0.0, 1.0));
}

// GGX ("Trowbridge-Reitz"): how many of the microfacets face exactly halfway between the light and the eye, the ones
// that reflect the light into the eye. alpha = roughness^2 (squared, so the slider feels even). Smooth: almost all
// microfacets face the same way, a small bright highlight; rough: spread, a wide dim one.
float DistributeMicrofacets(float normalDotHalf, float alpha)
{
    float alpha2 = alpha * alpha;
    float denominator = normalDotHalf * normalDotHalf * (alpha2 - 1.0) + 1.0;
    return alpha2 / (Pi * denominator * denominator);
}

// Smith with Schlick's approximation: how many microfacets are neither in the shadow of others (towards the light) nor
// hidden behind others (towards the eye). k as in the lighting of Unreal Engine 4: (roughness + 1)^2 / 8.
float ShadowMicrofacets(float normalDotView, float normalDotLight, float roughness)
{
    float k = (roughness + 1.0) * (roughness + 1.0) / 8.0;
    float towardsView = normalDotView / (normalDotView * (1.0 - k) + k);
    float towardsLight = normalDotLight / (normalDotLight * (1.0 - k) + k);
    return towardsView * towardsLight;
}

// Fresnel with Schlick's approximation: any surface reflects more at a grazing angle (a wet floor far ahead is a mirror,
// under the feet it is not). reflectance is F0, the reflection straight on.
vec3 Reflect(float viewDotHalf, vec3 reflectance)
{
    return reflectance + (1.0 - reflectance) * pow(1.0 - viewDotHalf, 5.0);
}

// A value 0..1 shown as gray on the screen as it is: Present turns linear values into sRGB, so data is turned the other
// way first (the formula of Renderer::ConvertSRGBToLinear).
vec3 ShowData(vec3 value)
{
    vec3 straight = value / 12.92;
    vec3 curved = pow((value + 0.055) / 1.055, vec3(2.4));
    return mix(curved, straight, lessThanEqual(value, vec3(0.04045)));
}

void main()
{
    // Before anything can branch: derivatives are only defined while every pixel of a 2 x 2 block runs the same code.
    TexCoordStepX = dFdx(TexCoord);
    TexCoordStepY = dFdy(TexCoord);

    mat3 textureAxes = CalculateTextureAxes();

    // The camera is at the origin: the eye is in the direction opposite to the position.
    vec3 towardsView = normalize(-ViewPosition);

    // Where the eye meets the relief, for a material with a height map. The direction to the eye goes into the axes of
    // the texture: the transpose of the axes turns the other way (they are at right angles, so it is their inverse).
    vec2 texCoord = TexCoord;
    if (uniParallaxDepth > 0.0)
    {
        vec3 viewInTexture = transpose(textureAxes) * towardsView;
        if (viewInTexture.z > 0.0)
            texCoord = FollowRelief(TexCoord, viewInTexture);
    }

    vec4 baseColor = Sample(uniAlbedoTexture, texCoord) * uniBaseColorFactor;

    // Green: roughness, blue: metalness (the layout of glTF).
    vec4 metalRoughness = Sample(uniMetalRoughnessTexture, texCoord);
    float roughness = clamp(metalRoughness.g * uniRoughnessFactor, MinimumRoughness, 1.0);
    float metalness = clamp(metalRoughness.b * uniMetalnessFactor, 0.0, 1.0);

    vec3 normal = CalculateSurfaceNormal(textureAxes, texCoord);

    // Still before discard: WidenRoughness takes the derivatives of the normal, which need the neighbouring pixels.
    if (uniIsSpecularAntiAliasingEnabled)
        roughness = WidenRoughness(roughness, normal);

    // Alpha testing: texels that are (almost) fully transparent are not drawn at all, as if there were no surface there.
    // Models cut shapes out of flat faces this way (glTF alphaMode "MASK"), the level its windows and grilles.
    if (baseColor.a < AlphaCutoff)
        discard;

    if (uniShadingView == ShadingViewBaseColor)
    {
        FragColor = vec4(baseColor.rgb, 1.0);
        return;
    }
    if (uniShadingView == ShadingViewNormals)
    {
        FragColor = vec4(ShowData(normal * 0.5 + 0.5), 1.0);
        return;
    }
    if (uniShadingView == ShadingViewRoughness || uniShadingView == ShadingViewMetalness)
    {
        float value = uniShadingView == ShadingViewRoughness ? roughness : metalness;
        FragColor = vec4(ShowData(vec3(value)), 1.0);
        return;
    }
    if (uniShadingView == ShadingViewHeight)
    {
        FragColor = vec4(ShowData(vec3(Sample(uniHeightTexture, texCoord).r)), 1.0);
        return;
    }

    // The halfway vector lies between the directions to the eye and to the light.
    vec3 towardsLight = uniSunDirection;
    vec3 halfway = normalize(towardsView + towardsLight);

    float normalDotLight = max(dot(normal, towardsLight), 0.0);
    float normalDotView = max(dot(normal, towardsView), 1e-4);
    float normalDotHalf = max(dot(normal, halfway), 0.0);
    float viewDotHalf = max(dot(towardsView, halfway), 0.0);

    // A metal reflects its own color and scatters nothing; anything else reflects 4% and scatters the rest in its color.
    vec3 reflectance = mix(DielectricReflectance, baseColor.rgb, metalness);
    vec3 fresnel = Reflect(viewDotHalf, reflectance);

    // The highlight: the light reflected by the microfacets towards the eye (Cook-Torrance).
    float alpha = roughness * roughness;
    vec3 specular = DistributeMicrofacets(normalDotHalf, alpha) * ShadowMicrofacets(normalDotView, normalDotLight, roughness) *
                    fresnel / (4.0 * normalDotView * max(normalDotLight, 1e-4));

    // The scattered light: what is not reflected goes into the surface and comes out evenly in all directions, colored by
    // it (Lambert: its color / pi). A metal has none.
    vec3 diffuse = (1.0 - fresnel) * (1.0 - metalness) * baseColor.rgb / Pi;

    // The sun lights the surface as much as it faces it (normalDotLight).
    vec3 color = (diffuse + specular) * uniSunColor * normalDotLight;

    // The light from everywhere, so the side away from the sun is not black: the scattered color, and for a metal the
    // color of its reflection (it would reflect its surroundings, which are about that bright).
    color += uniAmbientColor * (baseColor.rgb * (1.0 - metalness) + reflectance * metalness);

    // Light the surface gives off itself, added on top: no light makes it brighter or darker.
    color += Sample(uniEmissiveTexture, texCoord).rgb * uniEmissiveFactor;

    FragColor = vec4(color, baseColor.a);
}
