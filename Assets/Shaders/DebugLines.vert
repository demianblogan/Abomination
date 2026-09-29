#version 460 core

// A corner of the strip that draws a debug line lineWidth pixels wide (see Renderer::DebugLineRenderer).
// Both ends of the line go through the camera; on the screen the corner is moved across the line by half the width.
// No model matrix: debug lines are given in world coordinates already.

layout(location = 0) in vec3 aLineStart;
layout(location = 1) in vec3 aLineEnd;
layout(location = 2) in vec3 aColor;
layout(location = 3) in vec2 aCorner; // x: 0 = at the start, 1 = at the end; y: -1 or 1 = the side of the line

layout(location = 1) uniform mat4 uniView;
layout(location = 2) uniform mat4 uniProjection;
layout(location = 3) uniform vec2 uniViewportSize; // pixels
layout(location = 4) uniform float uniLineWidth;   // pixels

layout(location = 0) out vec3 Color;

// A point is behind the camera when w (its depth after the projection) is 0 or less; dividing by such a w would throw it
// to the wrong side of the screen. An end behind the camera is moved along the line until it is just in front.
const float MinimumW = 0.001;

void main()
{
    Color = aColor;

    vec4 start = uniProjection * uniView * vec4(aLineStart, 1.0);
    vec4 end = uniProjection * uniView * vec4(aLineEnd, 1.0);

    // The whole line behind the camera: nothing to draw. A position beyond the far plane (z > w) is clipped away.
    if (start.w < MinimumW && end.w < MinimumW)
    {
        gl_Position = vec4(0.0, 0.0, 2.0, 1.0);
        return;
    }

    // Along the line, w changes evenly (linearly) from start.w to end.w, so the point where it reaches MinimumW is the part
    // (MinimumW - start.w) / (end.w - start.w) of the way. For example, start.w = -2 and end.w = 6: w grows by 8 over the
    // line, and it needs about 2 of them to get from -2 to 0.001, so the start moves about 2 / 8 = 25% of the way to the end.
    if (start.w < MinimumW)
        start = mix(start, end, (MinimumW - start.w) / (end.w - start.w));
    if (end.w < MinimumW)
        end = mix(end, start, (MinimumW - end.w) / (start.w - end.w));

    // Both ends on the screen, in pixels: dividing by w gives -1..1 (normalized device coordinates), half the viewport
    // size turns that into pixels from the center of the screen.
    vec2 halfViewport = uniViewportSize * 0.5;
    vec2 startPixels = start.xy / start.w * halfViewport;
    vec2 endPixels = end.xy / end.w * halfViewport;

    // The direction of the line on the screen, and the direction across it (turned by 90 degrees). A line pointing
    // straight at the camera has no direction on the screen; any direction does for it.
    vec2 along = endPixels - startPixels;
    along = length(along) > 0.0001 ? normalize(along) : vec2(1.0, 0.0);
    vec2 across = vec2(-along.y, along.x);

    // Move the corner by half the width, in pixels, then back to clip space: divide by half the viewport and multiply by
    // w, because the GPU will divide by w again.
    vec4 position = aCorner.x < 0.5 ? start : end;
    vec2 offsetPixels = across * aCorner.y * uniLineWidth * 0.5;
    position.xy += offsetPixels / halfViewport * position.w;

    gl_Position = position;
}
