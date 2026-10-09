# Makes the models of the light sources of Episode 1: a wall torch, a brazier, a group of candles and an oil lantern.
# Low-poly like the props of Quake, all four textured from one pixel atlas of 64x64 texels made here: sixteen tiles of
# 16x16, one per material (iron, rust, wood, charred wood, pitch-soaked cloth, wax, glowing coals, horn panes, wick).
# Every face takes the whole of its tile. The atlas has every map of a material (see Renderer::Material): color, normal
# (from a height made with the color), roughness and metalness, and emission for the parts that glow (coals, the ember
# tips of the wicks, the horn panes of the lantern lit from inside). The flames themselves are sprites of the game.
#
# The front of a model (towards the room) is glTF +Z, which Blender exports from its -Y: a torch and a lantern stand
# against the wall at Blender y = 0 and reach out along -Y. TrenchBroom turns the front the way the entity faces, and so
# does the game (see World::Level). Every model is moved to have the middle of its box at its origin.
#
# Run from the repository root:
#   blender -b --python Tools/Blender/MakeLightSources.py -- [preview.png] [--install]
# Writes Torch.glb, Brazier.glb, Candles.glb, Lantern.glb (meters) to Build/LightSources, to be looked at first, or
# with --install to Assets/Models/Props, each also with its fire out (TorchOut.glb, ...); and previews of both.

import math
import os
import random
import sys

import bmesh
import bpy
import numpy as np
from mathutils import Matrix, Vector

ROOT = os.getcwd()
ARGUMENTS = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
OUTPUT = (os.path.join(ROOT, "Assets", "Models", "Props") if "--install" in ARGUMENTS
          else os.path.join(ROOT, "Build", "LightSources"))
PREVIEW = (next((argument for argument in ARGUMENTS if argument.endswith(".png")), None)
           or os.path.join(ROOT, "Build", "LightSourcesPreview.png"))

ATLAS_SIZE = 64
TILE_SIZE = 16

# Where every material lies in the atlas: (column, row) of its tile, rows counted from the top of the image.
IRON = (0, 0)
RUST = (1, 0)
WOOD = (2, 0)
CHARRED = (3, 0)
CLOTH = (0, 1)
WAX = (1, 1)
COALS = (2, 1)
HORN = (3, 1)
WICK = (0, 2)
ASH = (1, 2)
HORN_DARK = (2, 2)
WICK_OUT = (3, 2)

# Per tile: base color (sRGB 0..1), how much the brightness varies, roughness, metalness, glow (sRGB color times
# strength; None for none), and how deep its relief is.
TILES = {
    IRON: dict(color=(0.24, 0.23, 0.22), noise=0.18, roughness=0.5, metal=1.0, glow=None, relief=1.0),
    RUST: dict(color=(0.36, 0.2, 0.11), noise=0.3, roughness=0.85, metal=0.3, glow=None, relief=2.0),
    WOOD: dict(color=(0.33, 0.22, 0.13), noise=0.25, roughness=0.8, metal=0.0, glow=None, relief=2.0),
    CHARRED: dict(color=(0.09, 0.07, 0.06), noise=0.4, roughness=0.95, metal=0.0, glow=None, relief=2.5),
    CLOTH: dict(color=(0.2, 0.15, 0.1), noise=0.35, roughness=0.9, metal=0.0, glow=None, relief=2.0),
    WAX: dict(color=(0.78, 0.72, 0.56), noise=0.08, roughness=0.45, metal=0.0, glow=None, relief=0.6),
    COALS: dict(color=(0.12, 0.07, 0.05), noise=0.4, roughness=0.9, metal=0.0, glow=(1.0, 0.36, 0.08), relief=3.0),
    HORN: dict(color=(0.62, 0.45, 0.2), noise=0.12, roughness=0.6, metal=0.0, glow=(0.9, 0.55, 0.2), relief=0.5),
    WICK: dict(color=(0.05, 0.04, 0.04), noise=0.2, roughness=0.9, metal=0.0, glow=(1.0, 0.45, 0.12), relief=0.5),
    # What they are when the fire is out: gray ash, dull dark horn, a black wick.
    ASH: dict(color=(0.16, 0.15, 0.14), noise=0.4, roughness=0.95, metal=0.0, glow=None, relief=3.0),
    HORN_DARK: dict(color=(0.3, 0.22, 0.12), noise=0.12, roughness=0.6, metal=0.0, glow=None, relief=0.5),
    WICK_OUT: dict(color=(0.04, 0.035, 0.035), noise=0.2, roughness=0.9, metal=0.0, glow=None, relief=0.5),
}

# The models whose fire is out ("TorchOut.glb", ...) are the same, with these tiles instead of the glowing ones.
UNLIT_TILES = {COALS: ASH, HORN: HORN_DARK, WICK: WICK_OUT}

# How much larger than built every model is exported: the size of a prop is set here, the editor cannot scale models.
MODEL_SCALES = {"Brazier": 1.35}

random.seed(5)
rng = np.random.default_rng(5)


# --- The atlas ---------------------------------------------------------------------------------------------------------

def tile_pattern(key, size):
    """The brightness pattern of a tile (1 around its color): grain for wood, blotches for rust, lumps for coals."""
    y, x = np.mgrid[0:size, 0:size]
    pattern = rng.normal(0.0, 1.0, (size, size))
    if key in (WOOD, CHARRED):
        # Grain along the length (the columns): one random value per column, smeared down.
        pattern = 0.4 * pattern + np.repeat(rng.normal(0.0, 1.0, (1, size)), size, axis=0)
    elif key in (RUST, COALS, CLOTH, ASH):
        # Blotches: a coarse 4x4 noise blown up, plus the fine noise.
        coarse = np.kron(rng.normal(0.0, 1.0, (size // 4, size // 4)), np.ones((4, 4)))
        pattern = 0.5 * pattern + coarse
    elif key in (HORN, HORN_DARK):
        # Panes darker towards their edges, like horn thinner in the middle.
        edge = np.minimum(np.minimum(x, size - 1 - x), np.minimum(y, size - 1 - y)) / (size / 2)
        pattern = 0.3 * pattern + (edge - 0.5) * 2.0
    elif key == WICK:
        # The tip of a wick glows: the top quarter.
        pattern = 0.3 * pattern + np.where(y < size // 4, 1.5, -0.5)
    return pattern / max(np.abs(pattern).max(), 1e-6)


def make_atlas():
    """Color, normal, metal-roughness and emissive images of the atlas, as float RGBA arrays with the top row first."""
    color = np.zeros((ATLAS_SIZE, ATLAS_SIZE, 4), np.float32)
    normal = np.zeros_like(color)
    metal_roughness = np.zeros_like(color)
    emissive = np.zeros_like(color)
    color[..., 3] = normal[..., 3] = metal_roughness[..., 3] = emissive[..., 3] = 1.0
    normal[..., :3] = (0.5, 0.5, 1.0)

    for key, tile in TILES.items():
        column, row = key
        rows = slice(row * TILE_SIZE, (row + 1) * TILE_SIZE)
        columns = slice(column * TILE_SIZE, (column + 1) * TILE_SIZE)
        pattern = tile_pattern(key, TILE_SIZE)

        brightness = 1.0 + tile["noise"] * pattern
        color[rows, columns, :3] = np.clip(np.array(tile["color"]) * brightness[..., None], 0.0, 1.0)

        # The relief: bright is high. Its slopes (wrapping around inside the tile) tilt the normal: x to the right, y up
        # the image (the rows go down, so the difference of rows is negated).
        height = pattern * tile["relief"] * 0.15
        slope_x = (np.roll(height, -1, axis=1) - np.roll(height, 1, axis=1)) * 0.5
        slope_y = -(np.roll(height, -1, axis=0) - np.roll(height, 1, axis=0)) * 0.5
        n = np.stack([-slope_x, -slope_y, np.ones_like(height)], axis=-1)
        n /= np.linalg.norm(n, axis=-1, keepdims=True)
        normal[rows, columns, :3] = n * 0.5 + 0.5

        # glTF: green roughness, blue metalness. Rough where it is dark (dirt in the dents).
        metal_roughness[rows, columns, 1] = np.clip(tile["roughness"] - 0.1 * pattern, 0.0, 1.0)
        metal_roughness[rows, columns, 2] = tile["metal"]

        if tile["glow"] is not None:
            # The glow follows the pattern: the brighter lumps of the coals, the tip of the wick, the middle of a pane.
            glow = np.clip(pattern * 0.8 + 0.4, 0.0, 1.0)
            if key == WICK:
                glow = np.where(np.arange(TILE_SIZE)[:, None] < TILE_SIZE // 4, 1.0, 0.0) * np.ones((1, TILE_SIZE))
            emissive[rows, columns, :3] = np.array(tile["glow"]) * glow[..., None]

    return color, normal, metal_roughness, emissive


def make_image(name, pixels, is_data):
    # The color space first: changing it later makes Blender generate the image again, black.
    image = bpy.data.images.new(name, ATLAS_SIZE, ATLAS_SIZE, alpha=False, is_data=is_data)
    # Blender stores pixels from the bottom row up.
    image.pixels.foreach_set(np.ascontiguousarray(np.flipud(pixels), dtype=np.float32).ravel())
    image.update()
    image.pack()
    return image


def make_material():
    color, normal, metal_roughness, emissive = make_atlas()
    material = bpy.data.materials.new("LightSources")
    material.use_nodes = True
    nodes = material.node_tree.nodes
    links = material.node_tree.links
    shader = nodes["Principled BSDF"]

    def texture(image):
        node = nodes.new("ShaderNodeTexImage")
        node.image = image
        node.interpolation = "Closest"
        return node

    links.new(texture(make_image("LightSources", color, False)).outputs["Color"], shader.inputs["Base Color"])

    # One image for both, as glTF stores them: the exporter keeps it as it is.
    separate = nodes.new("ShaderNodeSeparateColor")
    links.new(texture(make_image("LightSources_MetalRough", metal_roughness, True)).outputs["Color"],
              separate.inputs["Color"])
    links.new(separate.outputs["Green"], shader.inputs["Roughness"])
    links.new(separate.outputs["Blue"], shader.inputs["Metallic"])

    normal_map = nodes.new("ShaderNodeNormalMap")
    links.new(texture(make_image("LightSources_Normal", normal, True)).outputs["Color"], normal_map.inputs["Color"])
    links.new(normal_map.outputs["Normal"], shader.inputs["Normal"])

    links.new(texture(make_image("LightSources_Emissive", emissive, False)).outputs["Color"],
              shader.inputs["Emission Color"])
    shader.inputs["Emission Strength"].default_value = 1.0
    return material


# --- Geometry ------------------------------------------------------------------------------------------------------------

def map_tile_uvs(obj, tile):
    """Every face of obj takes the whole tile: its corners are laid flat along the two axes its normal leans least on,
    and stretched to the tile, half a texel inside its edges so neighbouring tiles never bleed in."""
    column, row = tile
    inset = 0.5 / ATLAS_SIZE
    u0 = column * TILE_SIZE / ATLAS_SIZE + inset
    u1 = (column + 1) * TILE_SIZE / ATLAS_SIZE - inset
    # UV v grows up the image; row 0 is the top of the image.
    v1 = 1.0 - row * TILE_SIZE / ATLAS_SIZE - inset
    v0 = 1.0 - (row + 1) * TILE_SIZE / ATLAS_SIZE + inset

    mesh = obj.data
    uv_layer = mesh.uv_layers.new(name="UVMap")
    for polygon in mesh.polygons:
        axis = max(range(3), key=lambda index: abs(polygon.normal[index]))
        a, b = [index for index in range(3) if index != axis]
        points = [(mesh.vertices[mesh.loops[loop].vertex_index].co[a],
                   mesh.vertices[mesh.loops[loop].vertex_index].co[b]) for loop in polygon.loop_indices]
        low = [min(p[i] for p in points) for i in range(2)]
        high = [max(p[i] for p in points) for i in range(2)]
        for loop, point in zip(polygon.loop_indices, points):
            s = [(point[i] - low[i]) / max(high[i] - low[i], 1e-6) for i in range(2)]
            uv_layer.data[loop].uv = (u0 + s[0] * (u1 - u0), v0 + s[1] * (v1 - v0))


def part(bm, tile, material):
    mesh = bpy.data.meshes.new("Part")
    bm.to_mesh(mesh)
    bm.free()
    mesh.materials.append(material)
    obj = bpy.data.objects.new("Part", mesh)
    bpy.context.collection.objects.link(obj)
    map_tile_uvs(obj, UNLIT_TILES.get(tile, tile) if is_building_unlit else tile)
    return obj


# Whether the models being built now are the ones with the fire out (see UNLIT_TILES).
is_building_unlit = False


def box(center, size, tile, material, rotation=Matrix.Identity(4)):
    bm = bmesh.new()
    bmesh.ops.create_cube(bm, size=1.0)
    bmesh.ops.transform(bm, matrix=Matrix.Translation(center) @ rotation @ Matrix.Diagonal((*size, 1.0)),
                        verts=bm.verts)
    return part(bm, tile, material)


def cylinder_between(start, end, radius_start, radius_end, segments, tile, material):
    """A cylinder (or a cone) from start to end, closed at both ends."""
    start, end = Vector(start), Vector(end)
    axis = end - start
    bm = bmesh.new()
    bmesh.ops.create_cone(bm, cap_ends=True, segments=segments, radius1=radius_start, radius2=radius_end,
                          depth=axis.length)
    # create_cone makes it along +Z, centered: turned onto the axis and moved to its middle.
    rotation = Vector((0.0, 0.0, 1.0)).rotation_difference(axis.normalized()).to_matrix().to_4x4()
    bmesh.ops.transform(bm, matrix=Matrix.Translation((start + end) / 2.0) @ rotation, verts=bm.verts)
    return part(bm, tile, material)


def to_game(vector):
    """A position in Blender (Z up, -Y the front) in the coordinates of glTF and the game (Y up, +Z the front)."""
    return Vector((vector.x, vector.z, -vector.y))


def join(name, parts, fire, flames=()):
    bpy.ops.object.select_all(action="DESELECT")
    for obj in parts:
        obj.select_set(True)
    bpy.context.view_layer.objects.active = parts[0]
    bpy.ops.object.join()
    obj = bpy.context.object
    obj.name = name
    obj.data.name = name

    # The game places a model by the middle of its box (see Renderer::ModelStore), TrenchBroom by its origin: with the
    # middle of the box at the origin, both show it at the same place. Then it is scaled to its size.
    corners = [vertex.co for vertex in obj.data.vertices]
    low = Vector([min(corner[axis] for corner in corners) for axis in range(3)])
    high = Vector([max(corner[axis] for corner in corners) for axis in range(3)])
    middle = (low + high) / 2.0
    scale = MODEL_SCALES.get(name.removesuffix("Out"), 1.0)
    obj.data.transform(Matrix.Diagonal((scale, scale, scale, 1.0)) @ Matrix.Translation(-middle))

    # What the game and the editor need to know of the model (World/LightSources.cpp, Abomination.fgd): where its fire
    # is, in the coordinates of the game from the middle of the model, and half the size of its box in map units.
    fire_in_game = to_game((Vector(fire) - middle) * scale)
    half_size = (high - low) / 2.0 * scale * 32.0
    print(f"FIRE {name}: ({fire_in_game.x:.3f}, {fire_in_game.y:.3f}, {fire_in_game.z:.3f}) m;"
          f" half box {half_size.x:.1f} x {half_size.y:.1f} x {half_size.z:.1f} units (Blender X, Y, Z)")
    # Where the flames of the game stand (their bottom), when the model has more than its one fire (the wicks of
    # the candles).
    for flame in flames:
        point = to_game((Vector(flame) - middle) * scale)
        print(f"FLAME {name}: ({point.x:.3f}, {point.y:.3f}, {point.z:.3f}) m")
    return obj


def make_torch(material, name):
    """An iron bracket on the wall (y = 0) holding a wooden stick that leans out of it, its head wrapped in cloth."""
    parts = [box((0.0, -0.01, 0.0), (0.08, 0.02, 0.18), IRON, material),
             box((0.0, -0.07, -0.03), (0.025, 0.12, 0.025), IRON, material)]
    ring = Vector((0.0, -0.14, -0.02))
    parts.append(cylinder_between(ring + Vector((0, 0, -0.02)), ring + Vector((0, 0, 0.02)), 0.035, 0.035, 8,
                                  RUST, material))

    # The stick leans 20 degrees away from the wall, through the ring.
    lean = Vector((0.0, -math.sin(math.radians(20)), math.cos(math.radians(20))))
    bottom, top = ring - lean * 0.22, ring + lean * 0.24
    parts.append(cylinder_between(bottom, top, 0.018, 0.022, 6, WOOD, material))
    head_top = top + lean * 0.12
    parts.append(cylinder_between(top, head_top, 0.04, 0.036, 8, CLOTH, material))
    parts.append(cylinder_between(head_top, head_top + lean * 0.012, 0.034, 0.024, 8, COALS, material))
    # The fire burns on the head, a little above it.
    return join(name, parts, head_top + lean * 0.04)


def make_brazier(material, name):
    """An iron bowl on three splayed legs, full of glowing coals."""
    random.seed(11)  # the same lumps of coal for the lit and the burnt-out brazier
    parts = [cylinder_between((0, 0, 0.62), (0, 0, 0.8), 0.17, 0.3, 8, RUST, material),
             cylinder_between((0, 0, 0.79), (0, 0, 0.82), 0.31, 0.31, 8, IRON, material)]
    for index in range(3):
        angle = 2.0 * math.pi * index / 3.0
        direction = Vector((math.cos(angle), math.sin(angle), 0.0))
        parts.append(cylinder_between(direction * 0.3, direction * 0.16 + Vector((0, 0, 0.68)), 0.02, 0.02, 4,
                                      IRON, material))
        parts.append(box(direction * 0.3 + Vector((0, 0, 0.01)), (0.06, 0.06, 0.02), IRON, material))

    # The coals: a low lumpy dome inside the rim.
    bm = bmesh.new()
    bmesh.ops.create_cone(bm, cap_ends=True, segments=10, radius1=0.28, radius2=0.1, depth=0.08)
    for vertex in bm.verts:
        vertex.co.z += 0.82 + random.uniform(-0.012, 0.012)
    parts.append(part(bm, COALS, material))
    return join(name, parts, (0.0, 0.0, 0.9))


# The candles: where each stands, its radius and its height (meters).
CANDLES = [(0.0, 0.0, 0.03, 0.24), (0.07, 0.03, 0.025, 0.16), (-0.055, 0.045, 0.022, 0.11), (0.02, -0.07, 0.027, 0.19)]


def make_candles(material, name):
    """Four candles of different heights in a puddle of old wax, each with a glowing wick."""
    wicks = []
    parts = [cylinder_between((0, 0, 0), (0, 0, 0.012), 0.13, 0.11, 10, WAX, material)]
    for x, y, radius, height in CANDLES:
        parts.append(cylinder_between((x, y, 0.01), (x, y, height), radius, radius * 0.95, 8, WAX, material))
        parts.append(box((x, y, height + 0.008), (0.006, 0.006, 0.016), WICK, material))
        wicks.append((x, y, height + 0.016))
    # One light for the four flames, above the middle of the group.
    return join(name, parts, (0.01, 0.0, 0.25), wicks)


def make_lantern(material, name):
    """A six-sided iron lantern with horn panes lit from inside, a pointed roof and a ring to hang it by."""
    parts = [cylinder_between((0, 0, 0), (0, 0, 0.03), 0.095, 0.095, 6, IRON, material),
             cylinder_between((0, 0, 0.03), (0, 0, 0.21), 0.08, 0.08, 6, HORN, material),
             cylinder_between((0, 0, 0.21), (0, 0, 0.29), 0.105, 0.025, 6, RUST, material)]
    for index in range(6):
        angle = 2.0 * math.pi * index / 6.0
        corner = Vector((math.cos(angle), math.sin(angle), 0.0)) * 0.085
        parts.append(cylinder_between(corner + Vector((0, 0, 0.03)), corner + Vector((0, 0, 0.21)), 0.008, 0.008, 4,
                                      IRON, material))

    # The ring: eight short bars around a circle standing upright above the roof.
    center, radius = Vector((0.0, 0.0, 0.33)), 0.035
    for index in range(8):
        a0, a1 = 2.0 * math.pi * index / 8.0, 2.0 * math.pi * (index + 1) / 8.0
        p0 = center + Vector((math.cos(a0), 0.0, math.sin(a0))) * radius
        p1 = center + Vector((math.cos(a1), 0.0, math.sin(a1))) * radius
        parts.append(cylinder_between(p0, p1, 0.007, 0.007, 4, IRON, material))
    return join(name, parts, (0.0, 0.0, 0.12))


# --- Export and preview ---------------------------------------------------------------------------------------------

def export(obj):
    bpy.ops.object.select_all(action="DESELECT")
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    path = os.path.join(OUTPUT, obj.name + ".glb")
    bpy.ops.export_scene.gltf(filepath=path, use_selection=True, export_format="GLB", export_yup=True,
                              export_apply=True, export_animations=False)
    triangles = sum(len(polygon.vertices) - 2 for polygon in obj.data.polygons)
    print(f"LIGHT SOURCE {obj.name}: {triangles} triangles -> {path}")


def render_preview(lit_models, unlit_models):
    """Two pictures side by side in one scene setting: the four lit, and the four with their fire out."""
    scene = bpy.context.scene
    bpy.ops.object.camera_add(location=(0.0, -4.3, 0.1))
    camera = bpy.context.object
    camera.rotation_euler = (math.radians(88), 0.0, 0.0)
    camera.data.lens = 42
    scene.camera = camera
    bpy.ops.object.light_add(type="SUN", location=(1, -1, 2))
    bpy.context.object.data.energy = 2.5
    bpy.context.object.rotation_euler = (math.radians(45), math.radians(15), math.radians(-20))
    world = bpy.data.worlds.new("World")
    world.use_nodes = True
    world.node_tree.nodes["Background"].inputs["Color"].default_value = (0.05, 0.05, 0.055, 1.0)
    scene.world = world
    scene.render.engine = "BLENDER_EEVEE"
    scene.render.resolution_x = 1400
    scene.render.resolution_y = 600

    # Side by side, turned so their fronts (-Y) are seen from the side, at about the heights they stand at.
    places = [(-1.2, 0.0, 0.25), (-0.35, 0.0, 0.0), (0.5, 0.0, -0.3), (1.15, 0.0, 0.25)]
    for shown, hidden, path in [(lit_models, unlit_models, PREVIEW),
                                (unlit_models, lit_models, PREVIEW.removesuffix(".png") + "Out.png")]:
        for obj, place in zip(shown, places):
            obj.location = place
            obj.rotation_euler = (0.0, 0.0, math.radians(-50))
            obj.hide_render = False
        for obj in hidden:
            obj.hide_render = True
        scene.render.filepath = path
        bpy.ops.render.render(write_still=True)


def main():
    global is_building_unlit
    bpy.ops.wm.read_factory_settings(use_empty=True)
    os.makedirs(OUTPUT, exist_ok=True)
    material = make_material()
    builders = [make_torch, make_brazier, make_candles, make_lantern]
    names = ["Torch", "Brazier", "Candles", "Lantern"]
    lit_models = [build(material, name) for build, name in zip(builders, names)]
    is_building_unlit = True
    unlit_models = [build(material, name + "Out") for build, name in zip(builders, names)]
    for model in lit_models + unlit_models:
        export(model)
    render_preview(lit_models, unlit_models)


main()
