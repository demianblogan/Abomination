# Prepares the first-person hands WRAD ARMS by wriks (CC0) for the game and exports them to
# Build/Incoming/Models/HandsPrepared.glb, the hands HandsPoseScene.py builds the posing scene with (the game's
# Assets/Models/Weapons/Hands.glb is exported from that scene by ExportHandsPoses.py). Run with Blender 5.2 (any folder):
#   blender -b --factory-startup --python Tools/Blender/PrepareHands.py -- <arms.glb> [--preview <folder>]
#
# What it does:
#   1. Scales the arms from the author's units to meters (a tenth): the shoulders 38 cm apart. They already reach
#      forward along +Y and down in Blender, which the exporter turns into the -Z the game looks along.
#   2. Leaves the author's IK handles out of the skin (they do not bend it; the game places the hands itself).
#   3. Makes the clean, pink skin of the texture fit the game: darker, less pink, with grime and soot, scaled down to
#      256 x 256. The texture is processed, not drawn anew, so the artist's work (veins, knuckles, nails) stays.

import math
import os
import random
import sys

import bpy
import numpy
from mathutils import Matrix

arguments = sys.argv[sys.argv.index("--") + 1:]
SOURCE = arguments[0]
PREVIEW = os.path.abspath(arguments[arguments.index("--preview") + 1]) if "--preview" in arguments else None
TARGET = os.path.join(os.path.dirname(__file__), "..", "..", "Build", "Incoming", "Models", "HandsPrepared.glb")
TEXTURE_SIZE = 256

# How the skin is made dirtier: how much darker it gets (0.92: 92% of its brightness, a pale skin stays pale), how much
# of its color is taken away (0.25: a quarter of the way to grey), the tint of the dirt it is pulled towards, and how much of it is covered by
# dark patches of grime.
DARKENING = 0.92
DESATURATION = 0.25
DIRT_TINT = numpy.array([0.42, 0.36, 0.28])
DIRT_TINT_AMOUNT = 0.08
GRIME_COVER = 0.25

random.seed(2207)

bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=SOURCE)
for obj in list(bpy.data.objects):
    if obj.name.startswith("Icosphere"):
        bpy.data.objects.remove(obj, do_unlink=True)
armature = [obj for obj in bpy.data.objects if obj.type == 'ARMATURE'][0]
armature.name = "Hands"

# ---------- 1. meters ----------
armature.matrix_world = Matrix.Diagonal((0.1, 0.1, 0.1, 1.0)) @ armature.matrix_world
bpy.context.view_layer.update()
bpy.ops.object.select_all(action='DESELECT')
for obj in bpy.data.objects:
    obj.select_set(True)
bpy.context.view_layer.objects.active = armature
bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)

# ---------- 2. the skin follows only the bones of the arms ----------
for bone in armature.data.bones:
    bone.use_deform = not bone.name.startswith(("wrist_ik", "arm_target", "head", "root"))


# ---------- 3. the texture ----------
def value_noise(size, cells, seed):
    """Smooth noise from 0 to 1 over a size x size image: random values on a cells x cells grid, blended between."""
    rng = numpy.random.default_rng(seed)
    grid = rng.random((cells + 1, cells + 1))
    coordinates = numpy.linspace(0.0, cells, size, endpoint=False)
    cell = coordinates.astype(int)
    fraction = coordinates - cell
    fraction = fraction * fraction * (3.0 - 2.0 * fraction)
    rows = grid[cell][:, cell] * (1 - fraction)[None, :] + grid[cell][:, cell + 1] * fraction[None, :]
    rows_next = grid[cell + 1][:, cell] * (1 - fraction)[None, :] + grid[cell + 1][:, cell + 1] * fraction[None, :]
    return rows * (1 - fraction)[:, None] + rows_next * fraction[:, None]


for image in bpy.data.images:
    width, height = image.size
    if width == 0:
        continue
    pixels = numpy.empty(width * height * 4, dtype=numpy.float32)
    image.pixels.foreach_get(pixels)
    pixels = pixels.reshape(height, width, 4)
    color = pixels[:, :, :3]

    # Less pink, darker, pulled towards the color of dirt.
    grey = color.mean(axis=2, keepdims=True)
    color = color + (grey - color) * DESATURATION
    color = color * DARKENING
    color = color + (DIRT_TINT * DARKENING - color) * DIRT_TINT_AMOUNT

    # Patches of grime: large soft blotches of dark brown-black, and fine soot over everything.
    grime = value_noise(width, 6, 11) * 0.6 + value_noise(width, 18, 12) * 0.4
    patches = numpy.clip((grime - (1.0 - GRIME_COVER)) / GRIME_COVER, 0.0, 1.0)[:, :, None]
    color = color * (1.0 - patches * 0.45)
    soot = value_noise(width, 64, 13)[:, :, None]
    color = color * (0.92 + soot * 0.08)

    pixels[:, :, :3] = numpy.clip(color, 0.0, 1.0)
    image.pixels.foreach_set(pixels.ravel())
    if width > TEXTURE_SIZE:
        image.scale(TEXTURE_SIZE, TEXTURE_SIZE)
    image.pack()
    print("Texture", image.name, "processed and scaled to", image.size[:])

# ---------- previews ----------
if PREVIEW is not None:
    os.makedirs(PREVIEW, exist_ok=True)
    for image in bpy.data.images:
        if image.size[0] > 0:
            copy = image.copy()
            copy.filepath_raw = os.path.join(PREVIEW, "HandsTexture.png")
            copy.file_format = 'PNG'
            copy.save()
    scene = bpy.context.scene
    scene.render.engine = 'BLENDER_WORKBENCH'
    scene.display.shading.color_type = 'TEXTURE'
    world = bpy.data.worlds.new("Preview")
    world.color = (0.17, 0.165, 0.12)
    scene.world = world
    scene.display.shading.background_type = 'WORLD'
    scene.render.resolution_x = 480
    scene.render.resolution_y = 300
    camera_data = bpy.data.cameras.new("Camera")
    camera_data.type = 'ORTHO'
    camera = bpy.data.objects.new("Camera", camera_data)
    scene.collection.objects.link(camera)
    scene.camera = camera
    # Close to the right hand: from its outer side and from above, and the whole right arm from the side.
    for name, location, rotation, size in (("HandSide", (1.0, 0.25, -0.31), (math.radians(90), 0.0, math.radians(90)), 0.3),
                                           ("HandAbove", (0.48, 0.27, 0.5), (0.0, 0.0, 0.0), 0.3),
                                           ("ArmSide", (1.2, 0.12, -0.15), (math.radians(90), 0.0, math.radians(90)), 0.7)):
        camera.location = location
        camera.rotation_euler = rotation
        camera_data.ortho_scale = size
        scene.render.filepath = os.path.join(PREVIEW, name + ".png")
        bpy.ops.render.render(write_still=True)

bpy.ops.export_scene.gltf(filepath=os.path.abspath(TARGET), export_format='GLB', export_animations=False,
                          export_yup=True)
print("Exported", os.path.abspath(TARGET))
