# Makes the models TrenchBroom shows for the lights of a map (the game itself never loads them):
#   PointLight.glb  a small glowing crystal, so a point light is easy to tell from other entities;
#   SpotLight.glb   a cone opening from the light the way it shines, so its direction is seen and turns with it.
#
# TrenchBroom turns a glTF model so that its +Z (the front of glTF) is the forward of the entity, the way the
# rotate tool and the "angles" property point it. Blender exports its -Y as glTF +Z (export_yup), so the cone opens
# along Blender -Y.
#
# Run from the repository root:
#   blender -b --python Tools/Blender/MakeEditorLightModels.py -- <preview.png>
# Writes Assets/Models/Editor/PointLight.glb and SpotLight.glb (meters) and a preview of both.

import math
import os
import sys

import bmesh
import bpy

ROOT = os.getcwd()
OUTPUT = os.path.join(ROOT, "Assets", "Models", "Editor")
PREVIEW = (sys.argv[sys.argv.index("--") + 1] if "--" in sys.argv
           else os.path.join(ROOT, "Build", "EditorLightModelsPreview.png"))

# Sizes in meters: the crystal about as large as the entity box (16 units), the cone half a meter long and as wide
# as the default cone of a spot light (30 degrees from its axis).
CRYSTAL_RADIUS = 0.2
CONE_LENGTH = 0.6
CONE_ANGLE = math.radians(30.0)
CONE_SIDES = 12

# The colors of the entities in the editor (Abomination.fgd), in sRGB.
POINT_COLOR = (1.0, 0.86, 0.47)
SPOT_COLOR = (1.0, 0.7, 0.31)


def make_material(name, color):
    """A material of one flat color: a 4x4 image, because TrenchBroom draws a model by its texture."""
    image = bpy.data.images.new(name, 4, 4)
    image.pixels[:] = [*color, 1.0] * 16
    image.pack()

    material = bpy.data.materials.new(name)
    material.use_nodes = True
    nodes = material.node_tree.nodes
    texture = nodes.new("ShaderNodeTexImage")
    texture.image = image
    texture.interpolation = "Closest"
    material.node_tree.links.new(texture.outputs["Color"], nodes["Principled BSDF"].inputs["Base Color"])
    return material


def new_object(name, bm, material):
    mesh = bpy.data.meshes.new(name)
    bm.to_mesh(mesh)
    bm.free()
    mesh.materials.append(material)
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.collection.objects.link(obj)

    # Every face gets the whole one-color image.
    uv_layer = mesh.uv_layers.new()
    for loop in mesh.loops:
        uv_layer.data[loop.index].uv = (0.5, 0.5)
    return obj


def make_point_light():
    """An octahedron, stretched up a little like a flame."""
    bm = bmesh.new()
    r = CRYSTAL_RADIUS
    vertices = [bm.verts.new(position) for position in
                [(r, 0, 0), (0, r, 0), (-r, 0, 0), (0, -r, 0), (0, 0, r * 1.5), (0, 0, -r)]]
    for tip in (vertices[4], vertices[5]):
        for index in range(4):
            a, b = vertices[index], vertices[(index + 1) % 4]
            face = bm.faces.new((a, b, tip) if tip is vertices[4] else (b, a, tip))
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    return new_object("PointLight", bm, make_material("PointLight", POINT_COLOR))


def make_spot_light():
    """A cone with its tip at the light and its base CONE_LENGTH away along Blender -Y, closed at the base."""
    bm = bmesh.new()
    radius = CONE_LENGTH * math.tan(CONE_ANGLE)
    tip = bm.verts.new((0.0, 0.0, 0.0))
    rim = [bm.verts.new((radius * math.cos(angle), -CONE_LENGTH, radius * math.sin(angle)))
           for angle in (2.0 * math.pi * index / CONE_SIDES for index in range(CONE_SIDES))]
    for index in range(CONE_SIDES):
        bm.faces.new((tip, rim[(index + 1) % CONE_SIDES], rim[index]))
    bm.faces.new(rim)
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    return new_object("SpotLight", bm, make_material("SpotLight", SPOT_COLOR))


def export(obj):
    bpy.ops.object.select_all(action="DESELECT")
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    path = os.path.join(OUTPUT, obj.name + ".glb")
    bpy.ops.export_scene.gltf(filepath=path, use_selection=True, export_format="GLB", export_yup=True,
                              export_apply=True, export_animations=False)
    print(f"EDITOR MODEL {obj.name}: {len(obj.data.polygons)} faces -> {path}")


def render_preview(objects):
    scene = bpy.context.scene
    objects[0].location = (-0.45, 0.0, 0.0)
    objects[1].location = (0.05, 0.3, 0.0)
    objects[1].rotation_euler = (0.0, 0.0, math.radians(90))  # seen from the side: it opens to the right
    bpy.ops.object.camera_add(location=(0.0, -1.6, 0.9))
    camera = bpy.context.object
    camera.rotation_euler = (math.radians(62), 0.0, 0.0)
    camera.data.lens = 40
    scene.camera = camera
    bpy.ops.object.light_add(type="SUN", location=(1, -1, 2))
    bpy.context.object.data.energy = 4.0
    bpy.context.object.rotation_euler = (math.radians(40), math.radians(20), math.radians(30))
    world = bpy.data.worlds.new("World")
    world.use_nodes = True
    world.node_tree.nodes["Background"].inputs["Color"].default_value = (0.08, 0.08, 0.09, 1.0)
    scene.world = world
    scene.render.engine = "BLENDER_EEVEE"
    scene.render.resolution_x = 800
    scene.render.resolution_y = 450
    scene.render.filepath = PREVIEW
    bpy.ops.render.render(write_still=True)


def main():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    os.makedirs(OUTPUT, exist_ok=True)
    models = [make_point_light(), make_spot_light()]
    for model in models:
        export(model)
    render_preview(models)


main()
