#version 460 core

// The texture, lighter or darker depending on where the surface faces. This is not lighting (that comes in 0.5): it
// only makes the floor, the ceiling and walls facing different ways look different, so the shape stays readable even
// where they have the same texture.

layout(location = 0) in vec2 TexCoord;
layout(location = 1) in vec3 Normal;

layout(binding = 0) uniform sampler2D uniAlbedoTexture;

layout(location = 0) out vec4 FragColor;

// A made-up direction the "light" comes from: from above, a little from the right and the front.
const vec3 LightDirection = normalize(vec3(0.4, 1.0, 0.6));

// The darkest shade, for a surface facing away from the light: no surface turns black (the textures are dark already).
const float MinimumBrightness = 0.45;

void main()
{
    // After interpolation between the vertices the normal is a little shorter than 1, so it is normalized again.
    vec3 normal = normalize(Normal);

    // dot() is 1 for a surface facing the light, -1 for one facing away. Mapping it from -1..1 to 0..1 ("half-Lambert")
    // gives every direction its own shade instead of making all surfaces facing away equally black.
    float facing = dot(normal, LightDirection) * 0.5 + 0.5;

    // mix(a, b, t) = a + (b - a) * t: facing 0 gives MinimumBrightness, facing 1 gives full brightness.
    float brightness = mix(MinimumBrightness, 1.0, facing);

    vec4 albedo = texture(uniAlbedoTexture, TexCoord);
    FragColor = vec4(albedo.rgb * brightness, albedo.a);
}
