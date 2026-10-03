#version 460 core

// Places the vertices of a mesh and passes on the texture coordinates and the direction each surface faces.
// Used for the level and the models: their textures are shaded by the direction of the surface until real lighting
// arrives (0.5). A skinned mesh (a character) is first bent by the joints of its skeleton.

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec2 aTexCoord;
layout(location = 2) in vec3 aNormal;

// Only for a skinned mesh: the four joints that pull the vertex and how much each (the weights add up to 1).
layout(location = 3) in uvec4 aJoints;
layout(location = 4) in vec4 aWeights;

layout(location = 0) uniform mat4 uniModel;
layout(location = 1) uniform mat4 uniView;
layout(location = 2) uniform mat4 uniProjection;
layout(location = 3) uniform bool uniIsSkinned;

// The skinning matrix of every joint of the skeleton: how far it has moved the space around it since the skin was bound
// (see Renderer::CalculateSkinningMatrices). std430 lays the matrices out one after another, as in C++.
layout(std430, binding = 0) readonly buffer JointMatrices
{
    mat4 uniJointMatrices[];
};

layout(location = 0) out vec2 TexCoord;
layout(location = 1) out vec3 Normal;

void main()
{
    // Skinning: the vertex is moved by each of its joints, as much as that joint's weight. A vertex of an elbow is
    // half moved by the upper arm and half by the forearm, so the skin bends smoothly between them. A rigid mesh is not
    // moved (the identity).
    mat4 skin = mat4(1.0);
    if (uniIsSkinned)
    {
        skin = aWeights.x * uniJointMatrices[aJoints.x] + aWeights.y * uniJointMatrices[aJoints.y] +
               aWeights.z * uniJointMatrices[aJoints.z] + aWeights.w * uniJointMatrices[aJoints.w];
    }
    mat4 model = uniModel * skin;

    TexCoord = aTexCoord;

    // The normal is turned with the model (mat3 keeps the rotation and drops the translation of the model matrix).
    // This is exact for rotations and uniform scaling, which is all the game uses; the fragment shader normalizes it.
    Normal = mat3(model) * aNormal;

    gl_Position = uniProjection * uniView * model * vec4(aPosition, 1.0);
}
