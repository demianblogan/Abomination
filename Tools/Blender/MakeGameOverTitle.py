# Renders the GAME OVER title of the death screen: letters of Oswald (blocky, like the titles of Quake) as plates of rusty,
# dented iron with dried blood run down from their edges, lit from below and to the side like by a dying torch, on
# transparency. In Quake titles were pictures of metal and stone, not text; this is one in the same spirit.
#
# Run from the repository root:
#   blender -b --python Tools/Blender/MakeGameOverTitle.py -- <output.png>
# Writes the picture (1600 x 400, transparent) to the given path, by default Assets/UI/Images/GameOver.png.

import math
import os
import sys

import bpy

ROOT = os.getcwd()
FONT = os.path.join(ROOT, "Assets", "Fonts", "OswaldBold.ttf")
OUTPUT = sys.argv[sys.argv.index("--") + 1] if "--" in sys.argv else os.path.join(ROOT, "Assets", "UI", "Images",
                                                                                   "GameOver.png")


def node(nodes, kind, location, **inputs):
    created = nodes.new(kind)
    created.location = location
    for name, value in inputs.items():
        created.inputs[name].default_value = value
    return created


def make_rusty_iron():
    """Dark iron with patches of orange-brown rust, scratched and pitted, with dried blood running down."""
    material = bpy.data.materials.new("RustyIron")
    material.use_nodes = True
    nodes = material.node_tree.nodes
    links = material.node_tree.links
    shader = nodes["Principled BSDF"]
    coordinates = node(nodes, "ShaderNodeTexCoord", (-1200, 0))

    # Rust: large blotches of noise; where it is high the iron is rust, rough and orange-brown.
    rust_noise = node(nodes, "ShaderNodeTexNoise", (-900, 200), Scale=6.0, Detail=12.0, Roughness=0.65)
    links.new(coordinates.outputs["Object"], rust_noise.inputs["Vector"])
    rust_mask = node(nodes, "ShaderNodeValToRGB", (-650, 200))
    rust_mask.color_ramp.elements[0].position = 0.5
    rust_mask.color_ramp.elements[1].position = 0.62
    links.new(rust_noise.outputs["Fac"], rust_mask.inputs["Fac"])

    rust_color = node(nodes, "ShaderNodeValToRGB", (-650, 450))
    rust_color.color_ramp.elements[0].color = (0.10, 0.035, 0.015, 1.0)
    rust_color.color_ramp.elements[1].color = (0.24, 0.085, 0.03, 1.0)
    links.new(rust_noise.outputs["Fac"], rust_color.inputs["Fac"])

    # Dried blood: long drops running down (noise stretched along the height), dark brown-red, only in places.
    stretch = node(nodes, "ShaderNodeMapping", (-900, -250))
    stretch.inputs["Scale"].default_value = (14.0, 1.6, 14.0)
    links.new(coordinates.outputs["Object"], stretch.inputs["Vector"])
    blood_noise = node(nodes, "ShaderNodeTexNoise", (-650, -250), Scale=3.0, Detail=6.0)
    links.new(stretch.outputs["Vector"], blood_noise.inputs["Vector"])
    blood_mask = node(nodes, "ShaderNodeValToRGB", (-400, -250))
    blood_mask.color_ramp.elements[0].position = 0.52
    blood_mask.color_ramp.elements[1].position = 0.58
    links.new(blood_noise.outputs["Fac"], blood_mask.inputs["Fac"])

    iron = (0.07, 0.065, 0.06, 1.0)
    blood = (0.16, 0.012, 0.008, 1.0)
    iron_or_rust = node(nodes, "ShaderNodeMix", (-350, 300))
    iron_or_rust.data_type = "RGBA"
    iron_or_rust.inputs[7].default_value = iron
    links.new(rust_mask.outputs["Color"], iron_or_rust.inputs[0])
    links.new(rust_color.outputs["Color"], iron_or_rust.inputs[7])
    iron_or_rust.inputs[6].default_value = iron
    with_blood = node(nodes, "ShaderNodeMix", (-150, 100))
    with_blood.data_type = "RGBA"
    with_blood.inputs[7].default_value = blood
    links.new(blood_mask.outputs["Color"], with_blood.inputs[0])
    links.new(iron_or_rust.outputs[2], with_blood.inputs[6])
    links.new(with_blood.outputs[2], shader.inputs["Base Color"])

    # Bare iron shines a little; rust and blood are matte.
    metallic = node(nodes, "ShaderNodeMapRange", (-150, -100), **{"To Min": 0.85, "To Max": 0.1})
    links.new(rust_mask.outputs["Color"], metallic.inputs["Value"])
    links.new(metallic.outputs["Result"], shader.inputs["Metallic"])
    roughness = node(nodes, "ShaderNodeMapRange", (-150, -300), **{"To Min": 0.45, "To Max": 0.9})
    links.new(rust_mask.outputs["Color"], roughness.inputs["Value"])
    links.new(roughness.outputs["Result"], shader.inputs["Roughness"])

    # Pits and dents: fine noise as bumps.
    pits = node(nodes, "ShaderNodeTexNoise", (-650, -550), Scale=40.0, Detail=8.0)
    links.new(coordinates.outputs["Object"], pits.inputs["Vector"])
    bump = node(nodes, "ShaderNodeBump", (-150, -500), Strength=0.35)
    links.new(pits.outputs["Fac"], bump.inputs["Height"])
    links.new(bump.outputs["Normal"], shader.inputs["Normal"])
    return material


def make_title(material):
    bpy.ops.object.text_add(location=(0, 0, 0))
    title = bpy.context.object
    title.data.body = "GAME OVER"
    title.data.font = bpy.data.fonts.load(FONT)
    title.data.align_x = "CENTER"
    title.data.align_y = "CENTER"
    title.data.space_character = 1.05
    title.data.extrude = 0.06
    title.data.bevel_depth = 0.018
    title.data.bevel_resolution = 2
    title.data.materials.append(material)
    title.rotation_euler = (math.radians(90), 0, 0)
    return title


def light_and_render():
    scene = bpy.context.scene
    bpy.ops.object.camera_add(location=(0, -9.0, 0.0), rotation=(math.radians(90), 0, 0))
    camera = bpy.context.object
    camera.data.type = "ORTHO"
    camera.data.ortho_scale = 3.9
    scene.camera = camera

    # A warm torch below and to the left, a cold dim fill from above right, a red rim from behind.
    bpy.ops.object.light_add(type="AREA", location=(-2.5, -3.0, -2.2))
    torch = bpy.context.object
    torch.data.energy = 700
    torch.data.color = (1.0, 0.55, 0.25)
    torch.data.size = 2.0
    torch.rotation_euler = (math.radians(60), math.radians(-25), 0)
    bpy.ops.object.light_add(type="AREA", location=(3.0, -3.0, 2.5))
    fill = bpy.context.object
    fill.data.energy = 120
    fill.data.color = (0.6, 0.7, 1.0)
    fill.data.size = 3.0
    fill.rotation_euler = (math.radians(-50), math.radians(30), 0)
    bpy.ops.object.light_add(type="AREA", location=(0.0, 2.0, 0.5))
    rim = bpy.context.object
    rim.data.energy = 400
    rim.data.color = (1.0, 0.15, 0.08)
    rim.data.size = 6.0
    rim.rotation_euler = (math.radians(-90), 0, 0)

    world = bpy.data.worlds.new("World")
    world.use_nodes = True
    world.node_tree.nodes["Background"].inputs["Strength"].default_value = 0.0
    scene.world = world

    scene.render.engine = "CYCLES"
    scene.cycles.samples = 128
    scene.cycles.use_denoising = True
    scene.render.film_transparent = True
    scene.render.resolution_x = 1600
    scene.render.resolution_y = 400
    scene.render.image_settings.file_format = "PNG"
    scene.render.image_settings.color_mode = "RGBA"
    scene.view_settings.view_transform = "Standard"
    scene.render.filepath = OUTPUT
    bpy.ops.render.render(write_still=True)
    print("TITLE", OUTPUT)


def main():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    make_title(make_rusty_iron())
    light_and_render()


main()
