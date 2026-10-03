#version 460 core

// Places the vertices like the other shaders (a skinned mesh bent by its joints, as in TexturedShaded.vert) and passes
// nothing else on: the wireframe needs only positions.
// Used for every mesh when the Renderer window of the debug overlay switches to Wireframe.

layout(location = 0) in vec3 aPosition;
layout(location = 3) in uvec4 aJoints;
layout(location = 4) in vec4 aWeights;

layout(location = 0) uniform mat4 uniModel;
layout(location = 1) uniform mat4 uniView;
layout(location = 2) uniform mat4 uniProjection;
layout(location = 3) uniform bool uniIsSkinned;

layout(std430, binding = 0) readonly buffer JointMatrices
{
    mat4 uniJointMatrices[];
};

void main()
{
    mat4 skin = mat4(1.0);
    if (uniIsSkinned)
    {
        skin = aWeights.x * uniJointMatrices[aJoints.x] + aWeights.y * uniJointMatrices[aJoints.y] +
               aWeights.z * uniJointMatrices[aJoints.z] + aWeights.w * uniJointMatrices[aJoints.w];
    }

    gl_Position = uniProjection * uniView * uniModel * skin * vec4(aPosition, 1.0);
}
