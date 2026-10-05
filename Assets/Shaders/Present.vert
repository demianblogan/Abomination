#version 460 core

// One triangle that covers the whole screen, with no vertex buffer: the three corners come from the number of the
// vertex (gl_VertexID 0, 1, 2). The triangle is twice as large as the screen, so its long side passes outside the
// corner of the screen and the screen is covered without a seam (two triangles of a quad would meet along a diagonal):
//
//   (-1, 3)
//      |  \
//      |    \
//   (-1, 1)-----(1, 1)          the screen is the square from -1 to 1
//      |   screen   | \
//   (-1,-1)-----(1,-1)----(3,-1)
//
// The fragment shader reads the scene by the pixel coordinates (gl_FragCoord), so no texture coordinates are needed.

void main()
{
    // gl_VertexID 0 -> (0, 0), 1 -> (2, 0), 2 -> (0, 2); times 2 minus 1 -> (-1, -1), (3, -1), (-1, 3). Counter-clockwise,
    // so it is the front side and back-face culling keeps it.
    vec2 corner = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
    gl_Position = vec4(corner * 2.0 - 1.0, 0.0, 1.0);
}
