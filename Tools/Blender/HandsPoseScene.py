# Builds the Blender scene for posing the first-person hands on the shotgun, Tools/Blender/HandsPose.blend: the shotgun
# and the hands (unposed, from PrepareHands.py) stand where the game draws them relative to the eyes, and the camera looks from the eyes with
# the field of view of the weapon in the hands. It was run once; the scene, posed since, is the source of the poses and
# is kept in the repository (a scene rebuilt from Hands.glb would not do: Blender's glTF importer turns the bones, and
# the poses would move). The script refuses to overwrite it.
#
# The poses of the hands are changed in that scene:
#   1. open Tools/Blender/HandsPose.blend, move the IK handles wrist_ik.* / arm_target.* (added by AddHandsIK.py) and any
#      bones, key them with I in the pose Hold or HoldPumpBack (Action Editor), save;
#      PumpBackPose.py makes HoldPumpBack from Hold by moving the left hand back by the travel of the pump;
#   2. blender -b Tools/Blender/HandsPose.blend --python Tools/Blender/ExportHandsPoses.py   (to Hands.glb; the scene
#      file itself is not changed by the export, as long as it is not saved after it).
#
# The game's eye space (+X right, +Y up, -Z forward) is Blender's +X right, +Z up, +Y forward here.

import math
import os

import bpy
from mathutils import Matrix, Vector

ROOT = os.path.join(os.path.dirname(__file__), "..", "..")
SHOTGUN = os.path.join(ROOT, "Assets", "Models", "Weapons", "Shotgun.glb")
HANDS = os.path.join(ROOT, "Build", "Incoming", "Models", "HandsPrepared.glb")
TARGET = os.path.join(os.path.dirname(__file__), "HandsPose.blend")
if os.path.exists(TARGET):
    raise RuntimeError(f"{TARGET} exists and holds the posed hands; delete it first to start over")

# Where the game holds the middle of the shotgun relative to the eyes (WeaponViewModel::offset), in Blender's axes, and
# the vertical field of view it is drawn with (WeaponViewModel::verticalFOV).
WEAPON_OFFSET = Vector((0.082, 0.355, -0.176))
WEAPON_FOV = math.radians(34.0)

# Where the hands start: the middle between the shoulders below the eyes, a little bigger than real arms.
HANDS_OFFSET = Vector((0.0, -0.02, -0.24))
HANDS_SCALE = 1.15

bpy.ops.wm.read_factory_settings(use_empty=True)
scene = bpy.context.scene


def imported(path):
    before = set(bpy.data.objects)
    bpy.ops.import_scene.gltf(filepath=os.path.abspath(path))
    objects = [obj for obj in bpy.data.objects if obj not in before]
    for obj in list(objects):
        if obj.name.startswith("Icosphere"):
            bpy.data.objects.remove(obj, do_unlink=True)
            objects.remove(obj)
    return objects


# ---------- the shotgun ----------
# The game moves every model so the middle of its box is its origin; the same is done here with an empty the parts hang
# from, so "the middle of the shotgun" means the same in Blender and in the game.
shotgun_objects = imported(SHOTGUN)
meshes = [obj for obj in shotgun_objects if obj.type == 'MESH']
points = [obj.matrix_world @ Vector(corner) for obj in meshes for corner in obj.bound_box]
low = Vector([min(point[axis] for point in points) for axis in range(3)])
high = Vector([max(point[axis] for point in points) for axis in range(3)])
center = (low + high) / 2.0

shotgun = bpy.data.objects.new("Shotgun", None)
scene.collection.objects.link(shotgun)
shotgun.empty_display_type = 'ARROWS'
shotgun.empty_display_size = 0.05
for obj in shotgun_objects:
    if obj.parent is None:
        world = obj.matrix_world.copy()
        obj.parent = shotgun
        obj.matrix_world = Matrix.Translation(-center) @ world
shotgun.location = WEAPON_OFFSET
for obj in shotgun_objects:
    obj.hide_select = obj.name != "Pump_low_Shotgun_0"

# ---------- the hands ----------
hands_objects = imported(HANDS)
hands = [obj for obj in hands_objects if obj.type == 'ARMATURE'][0]
hands.name = "Hands"
hands.location = HANDS_OFFSET
hands.scale = (HANDS_SCALE,) * 3
hands.data.display_type = 'OCTAHEDRAL'
hands.show_in_front = True
for bone in hands.pose.bones:
    bone.custom_shape = None
    bone.rotation_mode = 'QUATERNION'

# ---------- the camera at the eyes ----------
camera_data = bpy.data.cameras.new("Eyes")
camera_data.sensor_fit = 'VERTICAL'
camera_data.angle_y = WEAPON_FOV
camera_data.clip_start = 0.01
camera = bpy.data.objects.new("Eyes", camera_data)
scene.collection.objects.link(camera)
camera.location = (0.0, 0.0, 0.0)
camera.rotation_euler = (math.radians(90), 0.0, 0.0)
scene.camera = camera
scene.render.resolution_x = 1920
scene.render.resolution_y = 1080

bpy.ops.wm.save_as_mainfile(filepath=os.path.abspath(TARGET))
print("Saved", os.path.abspath(TARGET))
