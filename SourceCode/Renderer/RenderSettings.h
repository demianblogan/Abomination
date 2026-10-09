#pragma once

namespace Abomination::Renderer
{
    // How the HDR scene (linear values from 0 to far above 1) is fitted into the 0..1 the screen can show, in the Present
    // pass (see Present.frag). The numbers are the ones the shader compares uniToneMapping with.
    enum class ToneMapping
    {
        // Everything above 1 is cut off: a flame and a white wall look the same. For comparison.
        None = 0,

        // The ACES filmic curve (as approximated by Krzysztof Narkowicz): dark values almost unchanged, bright ones
        // pressed together more and more, never cut off. Used by films and many games.
        ACES = 1,
    };

    // What the Lit shader shows: the lit picture, or one property of the surfaces, to check a material. The numbers are the
    // ones Lit.frag compares uniShadingView with.
    enum class ShadingView
    {
        Final = 0,

        // The color of the material (its map times its factor), without light.
        BaseColor = 1,

        // The direction every pixel faces, after the normal map, in the coordinates of the camera: red = right,
        // green = up, blue = towards the camera, each from -1..1 shown as 0..1.
        Normals = 2,

        // Roughness and metalness as gray: black 0, white 1.
        Roughness = 3,
        Metalness = 4,

        // The height of parallax occlusion mapping as gray: white high, black low (flat white without a height map).
        Height = 5,
    };

    // How the scene is drawn. Changed from the debug overlay (Renderer in the menu bar); later also by the options menu.
    struct RenderSettings
    {
        // Draw only the edges of triangles instead of filled surfaces: shows how surfaces are cut into triangles,
        // and reveals holes, extra triangles or faces turned the wrong way.
        bool isWireframeEnabled = false;

        // Draw the axes of the world at its origin as arrows over everything: X red, Y (up) green, Z blue, 1 meter long.
        // Shows where the origin is and which way the axes point.
        bool areWorldAxesVisible = false;

        // Show every light as debug lines: a star where it is and where its light ends (see AddLightDebugLines).
        bool areLightsVisible = false;

        // The brightness of the scene before tone mapping, in stops like a camera: every +1 doubles it, every -1 halves
        // it, 0 leaves it as it is. A dark crypt will want more, a sunny street less.
        float exposureStops = 0.0f;

        ToneMapping toneMapping = ToneMapping::ACES;

        ShadingView shadingView = ShadingView::Final;

        // The light until the level has lights of its own (feat/dynamic-lights, feat/lightmaps): a made-up "sun" from
        // above (the direction the old shading took), as strong as sunIntensity, and ambientIntensity of light from
        // everywhere, so a surface facing away from the sun is not black. About pi for the sun makes a white surface
        // facing it white: the diffuse light of a surface is its color / pi times the light.
        float sunIntensity = 3.0f;
        float ambientIntensity = 0.35f;

        // Parallax occlusion mapping of the materials that have a height map (see Renderer::Material::height), and a
        // factor for their depth, to see how deep looks right.
        bool isParallaxEnabled = true;
        float parallaxDepthScale = 1.0f;

        // How many steps parallax takes into the relief: more is smoother at a grazing angle and costs more.
        int parallaxStepCount = 16;

        // Highlights widened where the normal changes fast between pixels, so they do not flicker when the camera moves
        // (see Lit.frag).
        bool isSpecularAntiAliasingEnabled = true;
    };
}
