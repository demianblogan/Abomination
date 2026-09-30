#version 460 core

// Places the vertices of a mesh and passes on the texture coordinates and the direction each surface faces.
// Used for the level: its textures are shaded by the direction of the surface until real lighting arrives (0.5).

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec2 aTexCoord;
layout(location = 2) in vec3 aNormal;

layout(location = 0) uniform mat4 uniModel;
layout(location = 1) uniform mat4 uniView;
layout(location = 2) uniform mat4 uniProjection;

layout(location = 0) out vec2 TexCoord;
layout(location = 1) out vec3 Normal;

void main()
{
    TexCoord = aTexCoord;

    // The normal is turned with the model (mat3 keeps the rotation and drops the translation of the model matrix).
    // This is exact for rotations and uniform scaling, which is all the game uses.
    Normal = mat3(uniModel) * aNormal;

    gl_Position = uniProjection * uniView * uniModel * vec4(aPosition, 1.0);
}
