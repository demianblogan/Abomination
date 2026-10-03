# Rigs the dog made by Tripo (a mesh with a texture, no skeleton) with the skeleton and animations of the Husky from
# Quaternius' Ultimate Animated Animals (CC0), and exports it to Assets/Models/Enemies/Dog.glb.
# Run with Blender 5.2 (any folder):
#   blender -b --factory-startup --python Tools/Blender/RigDog.py -- <Dog.glb> <Husky.gltf> [--preview <folder>]
#
# What it does:
#   1. Stands the dog up: Tripo exports it lying on its side (its length along X, its back towards -Y).
#   2. Fits the Husky's skeleton to the dog: the Husky is much bigger and slimmer, so its skeleton is scaled along each
#      axis on its own until the box around the Husky's body matches the box around the dog's.
#   3. Binds the dog's skin to the skeleton: each vertex follows the bones nearest to it.
#   4. Keeps the Husky's clips (Idle, Walk, Gallop, Attack, ...), retargeting the legs: their joints were moved into the
#      dog's legs, so their turns are carried over in the space of the world.
#   5. Scales the textures down to the coarseness of the rest of the game.

import math
import os
import sys

import bpy
from mathutils import Matrix, Quaternion, Vector

arguments = sys.argv[sys.argv.index("--") + 1:]
DOG_PATH, DONOR_PATH = arguments[0], arguments[1]
PREVIEW = os.path.abspath(arguments[arguments.index("--preview") + 1]) if "--preview" in arguments else None
TARGET = os.path.join(os.path.dirname(__file__), "..", "..", "Assets", "Models", "Enemies", "Dog.glb")
TEXTURE_SIZE = 256


def world_box(obj):
    points = [obj.matrix_world @ vertex.co for vertex in obj.data.vertices]
    low = Vector([min(point[axis] for point in points) for axis in range(3)])
    high = Vector([max(point[axis] for point in points) for axis in range(3)])
    return low, high


def remove_helpers():
    # The importer shows bones as spheres (Icosphere) and leaves empty nodes of a skeleton the dog no longer has.
    for obj in list(bpy.data.objects):
        if obj.type == 'EMPTY' or obj.name.startswith("Icosphere"):
            bpy.data.objects.remove(obj, do_unlink=True)


bpy.ops.wm.read_factory_settings(use_empty=True)

# ---------- 1. the dog ----------
bpy.ops.import_scene.gltf(filepath=DOG_PATH)
dog = [obj for obj in bpy.data.objects if obj.type == 'MESH' and not obj.name.startswith("Icosphere")][0]
dog.name = "Dog"
for obj in list(bpy.data.objects):
    if obj.type == 'ARMATURE':
        world = dog.matrix_world.copy()
        dog.parent = None
        dog.matrix_world = world
        bpy.data.objects.remove(obj, do_unlink=True)
remove_helpers()

# Stand it up: its back (-Y) becomes up (+Z), then its length (X) goes along Y. Then its head must point to -Y, like
# the Husky's; the head end is found as the end whose top is higher.
dog.matrix_world = Matrix.Rotation(math.radians(-90), 4, 'X') @ dog.matrix_world
dog.matrix_world = Matrix.Rotation(math.radians(90), 4, 'Z') @ dog.matrix_world
bpy.context.view_layer.update()
low, high = world_box(dog)
points = [dog.matrix_world @ vertex.co for vertex in dog.data.vertices]
top_at_low_end = max(point.z for point in points if point.y < low.y + 0.2)
top_at_high_end = max(point.z for point in points if point.y > high.y - 0.2)
if top_at_high_end > top_at_low_end:
    dog.matrix_world = Matrix.Rotation(math.pi, 4, 'Z') @ dog.matrix_world
bpy.context.view_layer.update()

# The feet on the ground (z = 0) and the middle of the body over the origin.
low, high = world_box(dog)
dog.matrix_world = Matrix.Translation((-(low.x + high.x) / 2, -(low.y + high.y) / 2, -low.z)) @ dog.matrix_world
bpy.context.view_layer.update()
bpy.context.view_layer.objects.active = dog
dog.select_set(True)
bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
dog_low, dog_high = world_box(dog)
print("Dog box", [round(v, 3) for v in dog_low], [round(v, 3) for v in dog_high])

# ---------- 2. the Husky's skeleton ----------
bpy.ops.import_scene.gltf(filepath=DONOR_PATH)
remove_helpers()
donor = [obj for obj in bpy.data.objects if obj.type == 'ARMATURE'][0]
donor_mesh = [obj for obj in bpy.data.objects if obj.type == 'MESH' and obj.parent == donor][0]
donor_low, donor_high = world_box(donor_mesh)

# Scale the skeleton along each axis so the Husky's body box becomes the dog's, and put them in the same place.
scale = Vector([(dog_high[axis] - dog_low[axis]) / (donor_high[axis] - donor_low[axis]) for axis in range(3)])
print("Skeleton scale", [round(v, 3) for v in scale])
bpy.data.objects.remove(donor_mesh, do_unlink=True)
donor.matrix_world = Matrix.Diagonal((*scale, 1.0)) @ donor.matrix_world
bpy.context.view_layer.update()
donor_low_scaled = Vector([donor_low[axis] * scale[axis] for axis in range(3)])
donor.matrix_world = Matrix.Translation(dog_low - donor_low_scaled) @ donor.matrix_world
bpy.context.view_layer.update()

# Bake the scale into the bones. The clips keep rotations, which do not care about scale; the few translations they
# have (the body bobbing up and down) would stay at the Husky's size, so they are scaled too.
bpy.ops.object.select_all(action='DESELECT')
donor.select_set(True)
bpy.context.view_layer.objects.active = donor
bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
average_scale = (scale.x + scale.y + scale.z) / 3.0


def fcurves_of(action):
    curves = list(getattr(action, "fcurves", []) or [])
    for layer in getattr(action, "layers", []):
        for strip in layer.strips:
            for bag in strip.channelbags:
                curves.extend(bag.fcurves)
    return curves


for action in bpy.data.actions:
    for curve in fcurves_of(action):
        if curve.data_path.endswith(".location"):
            for point in curve.keyframe_points:
                point.co.y *= average_scale
                point.handle_left.y *= average_scale
                point.handle_right.y *= average_scale

# A copy of the skeleton as the Husky has it (only scaled), kept to retarget the clips of the moved leg bones from
# (see step 4).
source = donor.copy()
source.data = donor.data.copy()
source.name = "HuskySkeleton"
bpy.context.scene.collection.objects.link(source)

# The legs of the Husky do not stand where the dog's do once the skeleton is scaled: its hind legs reach far back and
# its hocks end up 20 cm behind the dog's legs, its front legs 10 cm behind. Bones turning around points outside a leg
# crush it. So the joints of the legs are moved into the dog's legs (measured on the mesh, meters, the head at -Y),
# keeping the zigzag of a dog's leg: the hind knee forward, the hock back. Bones only change where they are, not how
# they are turned relative to their parents, so the clips still bend them the same way.
LEG_JOINTS = {
    # bone: (head, tail) for the left side; the right one is mirrored.
    "FrontUpperLeg": ((0.13, -0.20, 0.30), (0.13, -0.22, 0.20)),
    "FrontLowerLeg": ((0.13, -0.22, 0.20), (0.13, -0.24, 0.07)),
    "BackLeg": ((0.14, 0.24, 0.36), (0.14, 0.18, 0.24)),
    "BackUpperLeg": ((0.14, 0.18, 0.24), (0.14, 0.27, 0.11)),
    "BackLowerLeg": ((0.14, 0.27, 0.11), (0.14, 0.25, 0.02)),
}
bpy.ops.object.mode_set(mode='EDIT')
for name, (head, tail) in LEG_JOINTS.items():
    for side, sign in (("L", 1.0), ("R", -1.0)):
        bone = donor.data.edit_bones[f"{name}.{side}"]
        # A bone turns around its own axes, which follow its direction: pointing it elsewhere also turns the axis a
        # knee bends around, and the leg would swing at a slant instead of forward. So after the move the bone is
        # rolled around its length until its X axis (the bending axis) points where it did before, sideways.
        bending_axis = bone.x_axis.copy()
        bone.head = (head[0] * sign, head[1], head[2])
        bone.tail = (tail[0] * sign, tail[1], tail[2])
        bone.align_roll(bending_axis.cross(bone.y_axis))
        if bone.x_axis.dot(bending_axis) < 0.0:
            bone.roll += math.pi
        print(f"Leg {bone.name}: bending axis off by {math.degrees(bone.x_axis.angle(bending_axis)):.0f} deg")
for side, sign in (("L", 1.0), ("R", -1.0)):
    # The shoulders end where the upper legs start.
    donor.data.edit_bones[f"FrontShoulder.{side}"].tail = donor.data.edit_bones[f"FrontUpperLeg.{side}"].head
    donor.data.edit_bones[f"BackShoulder.{side}"].tail = donor.data.edit_bones[f"BackLeg.{side}"].head
bpy.ops.object.mode_set(mode='OBJECT')

# Only the bones of the body bend the skin. The IK handles and pole targets of the Husky's rig do not; neither do the
# four bones of each of its long ears, which would twist the dog's small ears and the skin around them: the ears follow
# the head.
for bone in donor.data.bones:
    bone.use_deform = not bone.name.startswith(("IK", "FF", "Pole", "Ear"))

# ---------- 3. the skin ----------
# Blender's automatic weights fail on this mesh (generated meshes are seldom closed), so the weights are computed here:
# every vertex follows the bones nearest to it, the nearer the more (1 / distance^4), at most MAXIMUM_INFLUENCES of them.
# A bone counts by its whole length (the distance to the segment from its head to its tail), so a vertex in the middle
# of a thigh follows the thigh, and one at the knee is shared by the thigh and the shin.
MAXIMUM_INFLUENCES = 3


def distance_to_segment(point, start, end):
    segment = end - start
    t = 0.0 if segment.length_squared == 0 else max(0.0, min(1.0, (point - start).dot(segment) / segment.length_squared))
    return (point - (start + segment * t)).length


deform_bones = [bone for bone in donor.data.bones if bone.use_deform]
segments = [(bone.name, donor.matrix_world @ bone.head_local, donor.matrix_world @ bone.tail_local) for bone in deform_bones]
groups = {name: dog.vertex_groups.new(name=name) for name, _, _ in segments}
for vertex in dog.data.vertices:
    point = dog.matrix_world @ vertex.co
    nearest = sorted((distance_to_segment(point, start, end), name) for name, start, end in segments)[:MAXIMUM_INFLUENCES]
    strengths = [(1.0 / max(distance, 0.005) ** 4, name) for distance, name in nearest]
    total = sum(strength for strength, _ in strengths)
    for strength, name in strengths:
        groups[name].add([vertex.index], strength / total, 'REPLACE')

dog.parent = donor
modifier = dog.modifiers.new("Skeleton", 'ARMATURE')
modifier.object = donor
print("Skin: every vertex follows its", MAXIMUM_INFLUENCES, "nearest of", len(segments), "bones")

# ---------- 4. the clips of the moved legs ----------
# A clip turns every bone relative to its own direction at rest. The leg bones now point elsewhere than the Husky's (a
# shin straight down instead of down and back), so the same turns would swing the dog's legs at a slant. They are
# retargeted in the space of the world instead: in every frame, every leg bone of the dog is turned in space exactly as
# far from its rest pose as the Husky's bone is from its own, so the paws go forward and back as the Husky's do.
RETARGETED = [f"{name}.{side}" for name in ("FrontShoulder", "FrontUpperLeg", "FrontLowerLeg", "BackShoulder", "BackLeg",
                                            "BackUpperLeg", "BackLowerLeg") for side in ("L", "R")]


# How much of the Husky's leg swing the dog keeps: 1 the whole, 0.6 three fifths of every turn away from the rest pose.
LEG_SWING = 0.6


def rest_rotation(armature, name):
    return (armature.matrix_world @ armature.data.bones[name].matrix_local).to_quaternion()


def use_action(armature, action):
    armature.animation_data_create()
    armature.animation_data.action = action
    if action.slots:
        armature.animation_data.action_slot = action.slots[0]


source_rest = {name: rest_rotation(source, name) for name in RETARGETED}
target_rest = {name: rest_rotation(donor, name) for name in RETARGETED}
scene = bpy.context.scene
for action in list(bpy.data.actions):
    fitted = action.copy()
    fitted.name = action.name + "#fitted"
    for layer in getattr(fitted, "layers", []):
        for strip in layer.strips:
            for bag in strip.channelbags:
                for curve in list(bag.fcurves):
                    if any(f'"{name}"' in curve.data_path for name in RETARGETED) and "rotation" in curve.data_path:
                        bag.fcurves.remove(curve)
    use_action(source, action)
    use_action(donor, fitted)
    start, end = int(math.floor(action.frame_range[0])), int(math.ceil(action.frame_range[1]))
    for frame in range(start, end + 1):
        scene.frame_set(frame)
        # Parents first (the list goes from the shoulders down), so every bone is set on its already turned parent.
        for name in RETARGETED:
            turn = (source.matrix_world @ source.pose.bones[name].matrix).to_quaternion() @ source_rest[name].inverted()
            # The Husky is long-legged and strides wide; the fat dog's legs swing only part of the way.
            turn = Quaternion().slerp(turn, LEG_SWING)
            wanted = donor.matrix_world.to_quaternion().inverted() @ (turn @ target_rest[name])
            bone = donor.pose.bones[name]
            bone.rotation_mode = 'QUATERNION'
            bone.matrix = Matrix.Translation(bone.matrix.to_translation()) @ wanted.to_matrix().to_4x4()
            bpy.context.view_layer.update()
            bone.keyframe_insert("rotation_quaternion", frame=frame)
    print(f"Retargeted {action.name}: frames {start}-{end}")

# Only the fitted clips go into the file, under the Husky's names; the copy of its skeleton is not needed any more.
for action in list(bpy.data.actions):
    if not action.name.endswith("#fitted"):
        bpy.data.actions.remove(action)
for action in bpy.data.actions:
    action.name = action.name.removesuffix("#fitted")
bpy.data.objects.remove(source, do_unlink=True)
use_action(donor, bpy.data.actions["Idle"])

# ---------- 5. the textures ----------
for image in bpy.data.images:
    if image.size[0] > TEXTURE_SIZE:
        image.scale(TEXTURE_SIZE, TEXTURE_SIZE)
        image.pack()
print("Actions", [action.name for action in bpy.data.actions])


# ---------- previews ----------
def render(name, location, rotation, frame=None, action=None):
    scene = bpy.context.scene
    if action is not None:
        donor.animation_data_create()
        donor.animation_data.action = bpy.data.actions[action]
        # Blender 5 keeps the channels of an action in slots (one per animated object); the slot must be chosen too.
        if bpy.data.actions[action].slots:
            donor.animation_data.action_slot = bpy.data.actions[action].slots[0]
    if frame is not None:
        scene.frame_set(frame)
    camera.location = location
    camera.rotation_euler = rotation
    scene.render.filepath = os.path.join(PREVIEW, name)
    bpy.ops.render.render(write_still=True)


if PREVIEW is not None:
    os.makedirs(PREVIEW, exist_ok=True)
    scene = bpy.context.scene
    scene.render.engine = 'BLENDER_WORKBENCH'
    scene.display.shading.color_type = 'TEXTURE'
    world = bpy.data.worlds.new("Preview")
    world.color = (0.17, 0.165, 0.12)
    scene.world = world
    scene.display.shading.background_type = 'WORLD'
    scene.render.resolution_x = 400
    scene.render.resolution_y = 320
    camera_data = bpy.data.cameras.new("Camera")
    camera_data.type = 'ORTHO'
    camera_data.ortho_scale = 1.4
    camera = bpy.data.objects.new("Camera", camera_data)
    scene.collection.objects.link(camera)
    scene.camera = camera
    # The body at rest from the side, then frames of a few clips: a bad skin shows as stretched or torn parts.
    donor.data.pose_position = 'REST'
    render("RigRest.png", (2.0, 0.0, 0.35), (math.radians(90), 0, math.radians(90)))
    donor.data.pose_position = 'POSE'
    for action, frames in (("Walk", (0, 8, 16)), ("Gallop", (0, 5, 10)), ("Attack", (8, 16, 24)), ("Idle_2_HeadLow", (40,)),
                           ("Death", (25,))):
        for frame in frames:
            render(f"Rig{action}{frame}.png", (2.0, -0.6, 0.45), (math.radians(80), 0, math.radians(73)), frame, action)

# The Husky looks along -Y in Blender, which the exporter turns into +Z; the game takes -Z as the front of everything
# (Core::LocalForward). The skeleton, with the dog hanging from it, is turned half around.
# The dog is made 10% bigger than Tripo made it, to look right next to the player.
donor.matrix_world = Matrix.Diagonal((1.1, 1.1, 1.1, 1.0)) @ donor.matrix_world
donor.matrix_world = Matrix.Rotation(math.pi, 4, 'Z') @ donor.matrix_world
bpy.context.view_layer.update()

bpy.ops.export_scene.gltf(filepath=os.path.abspath(TARGET), export_format='GLB', export_animations=True,
                          export_animation_mode='ACTIONS', export_yup=True)
print("Exported", os.path.abspath(TARGET))
