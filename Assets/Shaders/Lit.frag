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

// The direction the surface faces at this pixel: the normal of the vertices, bent by the normal map. The normal map
// stores a direction in the axes of the texture (x along U, y along V, z out of the surface), each from -1..1 as 0..1.
vec3 CalculateSurfaceNormal()
{
    vec3 normal = normalize(ViewNormal);

    // Interpolated between the vertices, the tangent may lean out of the surface a little: the part along the normal
    // is removed (Gram-Schmidt), so the three axes stay at right angles.
    vec3 tangent = normalize(ViewTangent.xyz - normal * dot(ViewTangent.xyz, normal));
    vec3 bitangent = cross(normal, tangent) * ViewTangent.w;

    vec3 mapped = texture(uniNormalTexture, TexCoord).xyz * 2.0 - 1.0;
    return normalize(tangent * mapped.x + bitangent * mapped.y + normal * mapped.z);
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
    vec4 baseColor = texture(uniAlbedoTexture, TexCoord) * uniBaseColorFactor;

    // Alpha testing: texels that are (almost) fully transparent are not drawn at all, as if there were no surface there.
    // Models cut shapes out of flat faces this way (glTF alphaMode "MASK"), the level its windows and grilles.
    if (baseColor.a < AlphaCutoff)
        discard;

    // Green: roughness, blue: metalness (the layout of glTF).
    vec4 metalRoughness = texture(uniMetalRoughnessTexture, TexCoord);
    float roughness = clamp(metalRoughness.g * uniRoughnessFactor, MinimumRoughness, 1.0);
    float metalness = clamp(metalRoughness.b * uniMetalnessFactor, 0.0, 1.0);

    vec3 normal = CalculateSurfaceNormal();

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

    // The camera is at the origin: the eye is in the direction opposite to the position. The halfway vector lies
    // between the directions to the eye and to the light.
    vec3 towardsView = normalize(-ViewPosition);
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
    color += texture(uniEmissiveTexture, TexCoord).rgb * uniEmissiveFactor;

    FragColor = vec4(color, baseColor.a);
}
