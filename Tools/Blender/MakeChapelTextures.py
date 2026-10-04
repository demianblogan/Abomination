# Cuts the textures of the chapel out of the concept sheet generated with ChatGPT and makes them seamless.
#
# The tiles of the sheet look seamless but are not: their opposite edges do not match, so a wall made of them shows a
# line at every repeat. Each tile is made seamless at full size before it is shrunk: a band of `blend` pixels at its
# far edge is cut off, and cross-faded into the band at its near edge, so the last row before the cut flows into the
# first row of the next repeat. (Row k of the near band becomes a mix of the cut-off row and row k, going from all
# cut-off at k = 0 to all its own at k = blend.) Trims repeat only across, so only across; the door and the window do
# not repeat at all.
#
# Run from the repository root:
#   blender -b --python Tools/Blender/MakeChapelTextures.py -- <sheet.png> <output folder> <preview.png>

import os
import sys

import bpy
import numpy as np

SIZE = 64

# The tiles: name, column and row on the sheet, and which ways the texture repeats.
TILES = [
    ("Wall_MossyBlocks", 0, 0, "both"), ("Wall_GothicNiches", 1, 0, "both"), ("Wall_CrackedPlaster", 2, 0, "both"),
    ("Wall_RottenPlanks", 3, 0, "both"), ("Floor_Mud", 1, 1, "both"), ("Floor_MossyFlagstone", 2, 1, "both"),
    ("Trim_Arches", 0, 2, "across"), ("Trim_IronBand", 1, 2, "across"), ("Door_Chapel", 2, 2, "none"),
    ("Window_Chapel", 3, 2, "none"),
]

# Where the tiles are on the sheet (1448 x 1086): the left and top of each column and row inside the dark lines
# between them, and the size cut out of each.
COLUMNS = [12, 372, 733, 1098]
ROWS = [6, 371, 729]
CROP = 340

# How much of a tile is cross-faded at its seam (pixels of the sheet, about 3 of the final texture).
BLEND = 16


def load_rgb(path):
    image = bpy.data.images.load(path)
    width, height = image.size
    # Blender keeps rows from the bottom; flipped here so row 0 is the top, like the sheet.
    pixels = np.array(image.pixels[:], dtype=np.float32).reshape(height, width, 4)[::-1, :, :3]
    return pixels


def make_seamless_rows(tile, blend):
    """Seamless from bottom to top: the last `blend` rows are cut off and faded into the first ones."""
    height = tile.shape[0]
    body = tile[: height - blend].copy()
    cut = tile[height - blend:]
    weights = (np.arange(blend, dtype=np.float32) / blend)[:, None, None]
    body[:blend] = cut * (1.0 - weights) + body[:blend] * weights
    return body


def find_period_rows(tile, blend):
    """The height at which the tile repeats itself best: the cut where the rows just after it look most like the
    first rows. Patterns (blocks, arches, rivets) repeat every so many pixels; cutting there keeps them whole, so only
    a narrow band has to be faded."""
    height = tile.shape[0]
    best, best_difference = height - blend, float("inf")
    for cut in range(int(height * 0.6), height - blend + 1):
        difference = float(np.mean((tile[cut:cut + blend] - tile[:blend]) ** 2))
        if difference < best_difference:
            best, best_difference = cut, difference
    return best


def make_seamless_rows_at_period(tile, blend):
    """Cut at the period of the pattern (plus the band faded into the start), then fade only that band."""
    cut = find_period_rows(tile, blend)
    return make_seamless_rows(tile[: cut + blend], blend)


def shrink(tile):
    """Area average down to SIZE x SIZE (blocks of pixels averaged, with fractional edges by repeating first)."""
    height, width = tile.shape[:2]
    # Repeat every pixel SIZE times, then average blocks of height x width: exact area averaging for any size.
    rows = np.repeat(tile, SIZE, axis=0).reshape(SIZE, height, width, 3).mean(axis=1)
    return np.repeat(rows, SIZE, axis=1).reshape(SIZE, SIZE, width, 3).mean(axis=2)


def save_rgb(pixels, path):
    height, width = pixels.shape[:2]
    image = bpy.data.images.new(os.path.basename(path), width, height, alpha=False)
    rgba = np.concatenate([pixels[::-1], np.ones((height, width, 1), dtype=np.float32)], axis=2)
    image.pixels = rgba.ravel()
    image.filepath_raw = path
    image.file_format = "PNG"
    image.save()


def main():
    arguments = sys.argv[sys.argv.index("--") + 1:]
    sheet_path, output, preview_path = arguments
    sheet = load_rgb(sheet_path)
    tiles = []
    for name, column, row, repeats in TILES:
        tile = sheet[ROWS[row]:ROWS[row] + CROP, COLUMNS[column]:COLUMNS[column] + CROP]
        if repeats in ("both", "across"):
            tile = make_seamless_rows_at_period(tile.transpose(1, 0, 2), BLEND).transpose(1, 0, 2)
        if repeats == "both":
            tile = make_seamless_rows_at_period(tile, BLEND)
        small = shrink(tile)
        save_rgb(small, os.path.join(output, name + ".png"))
        tiles.append((small, repeats))
        print("TEXTURE", name, repeats)

    # The preview: every texture repeated 3 x 3, pixels shown 2 x, five in a row.
    cells = []
    for small, _ in tiles:
        tiled = np.tile(small, (3, 3, 1))
        cells.append(np.repeat(np.repeat(tiled, 2, axis=0), 2, axis=1))
    rows = [np.concatenate(cells[index:index + 5], axis=1) for index in range(0, len(cells), 5)]
    save_rgb(np.concatenate(rows, axis=0), preview_path)


main()
