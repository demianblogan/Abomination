# Makes the gibs of the dog: three chunks of its body (a lump, a piece of a leg with the bone sticking out, a small
# scrap), low-poly like the monsters of Quake, textured with a 64x64 texture made from the dog's own skin: the outside
# of a chunk is its fur, the torn inside is the same patch of skin turned to raw meat, the bone a pale strip.
#
# Run from the repository root:
#   blender -b --python Tools/Blender/MakeGibs.py -- <preview.png>
# Writes Assets/Models/Enemies/Gibs/Gib1.glb - Gib3.glb (meters, centered on the origin) and a preview of the three.

import math
import os
import random
import sys

import bmesh
import bpy
import numpy as np
from mathutils import Vector, noise

ROOT = os.getcwd()
DOG = os.path.join(ROOT, "Assets", "Models", "Enemies", "Dog.glb")
OUTPUT = os.path.join(ROOT, "Assets", "Models", "Enemies", "Gibs")
PREVIEW = sys.argv[sys.argv.index("--") + 1] if "--" in sys.argv else os.path.join(ROOT, "Build", "GibsPreview.png")

TEXTURE_SIZE = 64

# The rows of the texture (V from the bottom, as in UV space): fur at the top, raw meat below it, a strip of bone at
# the very bottom.
FUR = (0.5, 1.0)
MEAT = (0.125, 0.5)
BONE = (0.0, 0.125)

random.seed(7)


def make_texture(dog_image):
    """The 64x64 texture of the gibs from a 32x64 patch of the dog's skin (fur in its atlas)."""
    width, height = dog_image.size
    pixels = np.array(dog_image.pixels[:], dtype=np.float32).reshape(height, width, 4)

    # A patch of plain fur from the middle of the atlas (rows are counted from the bottom in Blender).
    patch = pixels[120:152, 96:160, :3]

    texture = np.zeros((TEXTURE_SIZE, TEXTURE_SIZE, 4), dtype=np.float32)
    texture[..., 3] = 1.0

    # Fur: the patch as it is (top half: rows 32-63).
    texture[32:64, :, :3] = patch

    # Meat: the brightness of the same patch over soft blotches (random values on a coarse 8x8 grid, blown up and
    # blended), from dark clotted red to lighter wet red.
    brightness = patch.mean(axis=2, keepdims=True)
    rng = np.random.default_rng(3)
    coarse = rng.random((5, 9))
    rows = np.linspace(0, 4, 32)
    columns = np.linspace(0, 8, 64)
    row_low, column_low = rows.astype(int).clip(0, 3), columns.astype(int).clip(0, 7)
    row_t, column_t = (rows - row_low)[:, None], (columns - column_low)[None, :]
    blotches = (coarse[row_low][:, column_low] * (1 - row_t) * (1 - column_t) +
                coarse[row_low + 1][:, column_low] * row_t * (1 - column_t) +
                coarse[row_low][:, column_low + 1] * (1 - row_t) * column_t +
                coarse[row_low + 1][:, column_low + 1] * row_t * column_t)[..., None]
    dark = np.array([0.22, 0.025, 0.03])
    wet = np.array([0.55, 0.09, 0.08])
    meat = dark + (wet - dark) * blotches
    meat = meat * (0.75 + brightness * 0.6)
    texture[8:32, :, :3] = meat[8:32]

    # Bone: pale yellowish grey with a little of the patch's grain.
    bone = 0.62 + (brightness[:8] - brightness.mean()) * 0.5
    texture[0:8, :, :3] = bone * np.array([1.0, 0.95, 0.82])

    image = bpy.data.images.new("Gibs", TEXTURE_SIZE, TEXTURE_SIZE)
    image.pixels = np.clip(texture, 0.0, 1.0).ravel()
    image.pack()
    return image


def make_material(image):
    material = bpy.data.materials.new("Gibs")
    material.use_nodes = True
    nodes = material.node_tree.nodes
    shader = nodes["Principled BSDF"]
    shader.inputs["Roughness"].default_value = 0.85
    texture_node = nodes.new("ShaderNodeTexImage")
    texture_node.image = image
    texture_node.interpolation = "Closest"
    material.node_tree.links.new(texture_node.outputs["Color"], shader.inputs["Base Color"])
    return material


def lumpy(bm, size, stretch, roughness, seed):
    """Pushes every vertex of a round mesh out or in by noise: a chunk of torn flesh instead of a ball."""
    offset = Vector((seed * 3.1, seed * 1.7, seed * 2.3))
    for vertex in bm.verts:
        direction = vertex.co.normalized()
        bump = noise.noise(direction * 2.2 + offset)
        vertex.co = Vector((direction.x * stretch[0], direction.y * stretch[1], direction.z * stretch[2])) * size * (
            1.0 + roughness * bump)


def map_uv(bm, region_for_face, scale):
    """Projects every face onto its region of the texture, from the side it faces most."""
    uv_layer = bm.loops.layers.uv.verify()
    for face in bm.faces:
        v_low, v_high = region_for_face(face)
        normal = face.normal
        axis = max(range(3), key=lambda index: abs(normal[index]))
        u_axis, w_axis = [(1, 2), (0, 2), (0, 1)][axis]
        for loop in face.loops:
            u = (loop.vert.co[u_axis] * scale + 0.5) % 1.0
            w = (loop.vert.co[w_axis] * scale + 0.5) % 1.0
            loop[uv_layer].uv = (u, v_low + w * (v_high - v_low))


def fur_or_meat(face):
    # Only the top keeps a patch of skin; everything else is torn meat (a gib reads as flesh, not as a stone).
    return FUR if face.normal.z > 0.8 and face.index % 2 == 0 else MEAT


def new_object(name, bm, material):
    mesh = bpy.data.meshes.new(name)
    bm.to_mesh(mesh)
    bm.free()
    mesh.materials.append(material)
    for polygon in mesh.polygons:
        polygon.use_smooth = False
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.scene.collection.objects.link(obj)
    return obj


def make_lump(material):
    """Gib1: a fist-sized lump of the body, about 14 cm."""
    bm = bmesh.new()
    bmesh.ops.create_icosphere(bm, subdivisions=2, radius=1.0)
    lumpy(bm, 0.07, (1.15, 0.85, 0.75), 0.55, 1)
    bm.normal_update()
    map_uv(bm, fur_or_meat, 6.0)
    return new_object("Gib1", bm, material)


def make_leg(material):
    """Gib2: a torn piece of a leg, about 20 cm, the bone sticking out of the meat at one end."""
    bm = bmesh.new()
    bmesh.ops.create_icosphere(bm, subdivisions=2, radius=1.0)
    lumpy(bm, 0.05, (2.0, 0.8, 0.8), 0.25, 2)
    bm.normal_update()
    map_uv(bm, fur_or_meat, 6.0)

    # The bone: a thin six-sided stick out of the +X end, with a knob at its tip.
    bone = bmesh.new()
    bmesh.ops.create_cone(bone, cap_ends=True, segments=6, radius1=0.012, radius2=0.016, depth=0.07)
    bmesh.ops.rotate(bone, verts=bone.verts, cent=(0, 0, 0), matrix=__import__("mathutils").Matrix.Rotation(
        math.radians(90.0), 3, "Y"))
    bmesh.ops.translate(bone, verts=bone.verts, vec=(0.12, 0.0, 0.01))
    bone.normal_update()
    map_uv(bone, lambda face: BONE, 10.0)
    temporary = bpy.data.meshes.new("Bone")
    bone.to_mesh(temporary)
    bone.free()
    bm.from_mesh(temporary)
    bpy.data.meshes.remove(temporary)
    return new_object("Gib2", bm, material)


def make_scrap(material):
    """Gib3: a small flat scrap of skin and meat, about 8 cm."""
    bm = bmesh.new()
    bmesh.ops.create_icosphere(bm, subdivisions=1, radius=1.0)
    lumpy(bm, 0.045, (1.1, 1.0, 0.45), 0.6, 3)
    bm.normal_update()
    map_uv(bm, fur_or_meat, 8.0)
    return new_object("Gib3", bm, material)


def export(obj):
    bpy.ops.object.select_all(action="DESELECT")
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    path = os.path.join(OUTPUT, obj.name + ".glb")
    bpy.ops.export_scene.gltf(filepath=path, use_selection=True, export_format="GLB", export_yup=True,
                              export_apply=True, export_animations=False)
    triangles = sum(len(polygon.vertices) - 2 for polygon in obj.data.polygons)
    print(f"GIB {obj.name}: {triangles} triangles -> {path}")


def render_preview(objects):
    scene = bpy.context.scene
    for index, obj in enumerate(objects):
        obj.location = ((index - 1) * 0.24, 0.0, 0.0)
        obj.rotation_euler = (math.radians(20), 0.0, math.radians(30))
    bpy.ops.object.camera_add(location=(0.0, -0.75, 0.38))
    camera = bpy.context.object
    camera.rotation_euler = (math.radians(65), 0.0, 0.0)
    camera.data.lens = 38
    scene.camera = camera
    bpy.ops.object.light_add(type="SUN", location=(1, -1, 2))
    bpy.context.object.data.energy = 4.0
    bpy.context.object.rotation_euler = (math.radians(40), math.radians(20), math.radians(30))
    world = bpy.data.worlds.new("World")
    world.use_nodes = True
    world.node_tree.nodes["Background"].inputs["Color"].default_value = (0.08, 0.08, 0.09, 1.0)
    world.node_tree.nodes["Background"].inputs["Strength"].default_value = 1.5
    scene.world = world
    scene.render.engine = "BLENDER_EEVEE"
    scene.render.resolution_x = 900
    scene.render.resolution_y = 420
    scene.render.filepath = PREVIEW
    bpy.ops.render.render(write_still=True)


def main():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.gltf(filepath=DOG)
    dog_image = next(image for image in bpy.data.images if "basecolor" in image.name.lower())
    texture = make_texture(dog_image)
    for obj in list(bpy.data.objects):
        bpy.data.objects.remove(obj)

    material = make_material(texture)
    os.makedirs(OUTPUT, exist_ok=True)
    gibs = [make_lump(material), make_leg(material), make_scrap(material)]
    for gib in gibs:
        export(gib)
    render_preview(gibs)


main()
