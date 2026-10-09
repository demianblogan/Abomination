#version 460 core

// Places the vertices of a mesh and passes on what the lighting of Lit.frag needs: the position, the normal and the
// tangent of every vertex in the coordinates of the camera ("view space": the camera at the origin, looking along -Z),
// and the texture coordinates. Used for the level and the models. A skinned mesh (a character) is first bent by the
// joints of its skeleton.
//
// Why the coordinates of the camera and not of the world: the weapon in the hands is drawn relative to the eyes, with
// no world around it (see Renderer::DrawWeaponViewModel). In the coordinates of the camera the world and the weapon
// meet, and the light is given there too (see Renderer::CalculateSceneLighting).

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec2 aTexCoord;
layout(location = 2) in vec3 aNormal;

// Only for a skinned mesh: the four joints that pull the vertex and how much each (the weights add up to 1).
layout(location = 3) in uvec4 aJoints;
layout(location = 4) in vec4 aWeights;

// The axes of the texture: xyz where U grows, w the sign of the bitangent (see Renderer::MeshVertex::tangent).
layout(location = 5) in vec4 aTangent;

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
layout(location = 1) out vec3 ViewPosition;
layout(location = 2) out vec3 ViewNormal;
layout(location = 3) out vec4 ViewTangent;

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

    // From the mesh straight into the coordinates of the camera: model, then view (applied from right to left).
    mat4 modelView = uniView * uniModel * skin;
    vec4 viewPosition = modelView * vec4(aPosition, 1.0);

    TexCoord = aTexCoord;
    ViewPosition = viewPosition.xyz;

    // Directions are turned with the model and the camera (mat3 keeps the rotation and drops the translation). This is
    // exact for rotations and uniform scaling, which is all the game uses; the fragment shader normalizes them. The sign
    // of the bitangent is not a direction, but a mirroring matrix (a negative determinant: a part mirrored in its model
    // file) turns cross(normal, tangent) the other way, so the sign is turned with it.
    mat3 turn = mat3(modelView);
    ViewNormal = turn * aNormal;
    ViewTangent = vec4(turn * aTangent.xyz, aTangent.w * sign(determinant(turn)));

    gl_Position = uniProjection * viewPosition;
}
