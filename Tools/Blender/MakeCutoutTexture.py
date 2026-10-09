# Turns a picture drawn on flat magenta (#FF00FF) into a cutout texture: the magenta becomes transparent, and the
# picture is shrunk to the given size. The level shader throws transparent texels away (Lit.frag), so the
# wall behind the brush shows there: an arched door or window on a rectangular brush.
#
# Shrinking averages the colors of the opaque pixels only (weighted by how opaque they are), so no magenta bleeds into
# the edges, and every texel ends fully opaque or fully transparent (more than half opaque: opaque).
#
# Run from the repository root:
#   blender -b --python Tools/Blender/MakeCutoutTexture.py -- <picture.png> <width> <height> <texture.png>

import sys

import bpy
import numpy as np


def main():
    source, width, height, target = sys.argv[sys.argv.index("--") + 1:]
    width, height = int(width), int(height)

    image = bpy.data.images.load(source)
    source_width, source_height = image.size
    pixels = np.array(image.pixels[:], dtype=np.float32).reshape(source_height, source_width, 4)

    # Magenta: red and blue high, green low. A little tolerance for the soft edge pixels of the drawing.
    red, green, blue = pixels[..., 0], pixels[..., 1], pixels[..., 2]
    opacity = np.where((red > 0.75) & (blue > 0.75) & (green < 0.35), 0.0, 1.0).astype(np.float32)

    # Area averaging, by repeating every pixel `width` (or `height`) times and averaging blocks of the source size:
    # exact for any ratio of sizes.
    def shrink(channel):
        rows = np.repeat(channel, height, axis=0).reshape(height, source_height, source_width).mean(axis=1)
        return np.repeat(rows, width, axis=1).reshape(height, width, source_width).mean(axis=2)

    alpha = shrink(opacity)
    color = np.stack([shrink(pixels[..., channel] * opacity) for channel in range(3)], axis=2)
    color /= np.maximum(alpha, 1e-6)[..., None]
    result = np.concatenate([color, (alpha > 0.5).astype(np.float32)[..., None]], axis=2)

    output = bpy.data.images.new("Cutout", width, height, alpha=True)
    output.pixels = np.clip(result, 0.0, 1.0).ravel()
    output.filepath_raw = target
    output.file_format = "PNG"
    output.save()
    print("CUTOUT", target, width, height)


main()
