#version 460 core

// The color of the line, the same along its whole length.

layout(location = 0) in vec3 Color;

layout(location = 0) out vec4 FragColor;

void main()
{
    FragColor = vec4(Color, 1.0);
}
