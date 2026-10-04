# Makes the pose HoldPumpBack in Tools/Blender/HandsPose.blend from Hold: the left hand moved back along the
# barrel by exactly the travel of the pump in the game (PumpActionSettings::travel), with its elbow target, through the
# IK handles added by AddHandsIK.py. The barrel lies along +Y of the scene, so back is -Y.
# Run with Blender 5.2, with the scene closed in Blender (any folder):
#   blender -b Tools/Blender/HandsPose.blend --python Tools/Blender/PumpBackPose.py

import bpy
from mathutils import Matrix, Vector

PUMP_TRAVEL = 0.099

hands = bpy.data.objects["Hands"]
hold = bpy.data.actions["Hold"]
pump_back = bpy.data.actions["HoldPumpBack"]


def use_action(action):
    hands.animation_data.action = action
    if action.slots:
        hands.animation_data.action_slot = action.slots[0]
    bpy.context.scene.frame_set(1)
    bpy.context.view_layer.update()


# Where the handles of the left hand are in Hold (in the space of the armature).
use_action(hold)
names = ("wrist_ik.l", "arm_target.l")
held = {name: hands.pose.bones[name].matrix.copy() for name in names}

# Back along the barrel in the world is this much in the space of the armature (which is scaled and may be moved).
offset = hands.matrix_world.inverted().to_3x3() @ Vector((0.0, -PUMP_TRAVEL, 0.0))

use_action(pump_back)
bpy.context.view_layer.objects.active = hands
bpy.ops.object.mode_set(mode='POSE')
for name in names:
    bone = hands.pose.bones[name]
    bone.matrix = Matrix.Translation(offset) @ held[name]
    bpy.context.view_layer.update()
    bone.keyframe_insert("location", frame=1)
    bone.keyframe_insert("rotation_quaternion", frame=1)
    bone.keyframe_insert("scale", frame=1)
bpy.ops.object.mode_set(mode='OBJECT')

moved = (hands.matrix_world @ hands.pose.bones["wrist.l"].head)
use_action(hold)
start = (hands.matrix_world @ hands.pose.bones["wrist.l"].head)
print(f"Left wrist moved by {(moved - start) * 100} cm")
use_action(pump_back)
bpy.ops.wm.save_mainfile()
print("Saved")
