# Adds IK to the arms in Tools/Blender/HandsPose.blend, so a hand is posed by moving one handle: the bones
# wrist_ik.l / wrist_ik.r that WRAD ARMS already has. Moving a handle moves the hand (its turn is kept: the hand copies
# the handle's rotation), and the elbow and the shoulder follow; arm_target.l / arm_target.r show where the elbows point.
# The handles are placed where the hands are in every pose, so nothing moves when the IK is switched on.
# Run with Blender 5.2, with the scene closed in Blender (any folder):
#   blender -b Tools/Blender/HandsPose.blend --python Tools/Blender/AddHandsIK.py
# ExportHandsPoses.py bakes the poses of all the bones before it exports them.

import math

import bpy
from mathutils import Matrix

hands = bpy.data.objects["Hands"]
SIDES = ("l", "r")


def use_action(action):
    hands.animation_data.action = action
    if action.slots:
        hands.animation_data.action_slot = action.slots[0]
    bpy.context.view_layer.update()


def place_handles():
    """Puts each wrist handle on its wrist and each elbow target in front of its elbow, as the pose has them now."""
    for side in SIDES:
        wrist = hands.pose.bones[f"wrist.{side}"]
        forearm = hands.pose.bones[f"forearm.{side}"]
        bicep = hands.pose.bones[f"bicep.{side}"]
        handle = hands.pose.bones[f"wrist_ik.{side}"]
        target = hands.pose.bones[f"arm_target.{side}"]
        handle.matrix = wrist.matrix.copy()
        bpy.context.view_layer.update()

        # The elbow target: out from the line shoulder - wrist through the elbow, a forearm's length away.
        middle = (bicep.head + wrist.head) / 2.0
        outward = (forearm.head - middle).normalized()
        target.matrix = Matrix.Translation(forearm.head + outward * (wrist.head - forearm.head).length)
        bpy.context.view_layer.update()


# The handles must hang from the root, not from the arms, or moving the arm would move its own handle.
bpy.context.view_layer.objects.active = hands
bpy.ops.object.mode_set(mode='EDIT')
for side in SIDES:
    for name in (f"wrist_ik.{side}", f"arm_target.{side}"):
        bone = hands.data.edit_bones[name]
        bone.parent = hands.data.edit_bones["root"]
        bone.use_connect = False
bpy.ops.object.mode_set(mode='POSE')

# Every pose gets its handles where its hands are.
for action in bpy.data.actions:
    use_action(action)
    place_handles()
    for side in SIDES:
        for name in (f"wrist_ik.{side}", f"arm_target.{side}"):
            bone = hands.pose.bones[name]
            bone.keyframe_insert("location", frame=1)
            bone.keyframe_insert("rotation_quaternion", frame=1)
            bone.keyframe_insert("scale", frame=1)

# The constraints: the forearm and the upper arm reach the handle (two bones), the elbow pointing at its target; the
# hand turns as the handle does.
for side in SIDES:
    forearm = hands.pose.bones[f"forearm.{side}"]
    for constraint in list(forearm.constraints):
        forearm.constraints.remove(constraint)
    ik = forearm.constraints.new('IK')
    ik.target = hands
    ik.subtarget = f"wrist_ik.{side}"
    ik.pole_target = hands
    ik.pole_subtarget = f"arm_target.{side}"
    ik.chain_count = 2

    wrist = hands.pose.bones[f"wrist.{side}"]
    for constraint in list(wrist.constraints):
        wrist.constraints.remove(constraint)
    rotation = wrist.constraints.new('COPY_ROTATION')
    rotation.target = hands
    rotation.subtarget = f"wrist_ik.{side}"

# The angle of the elbow around the line to the target that keeps every pose as it was: the one with the smallest
# difference between the elbows with and without the IK, of a few tried.
use_action(bpy.data.actions["Hold"])
forearms = {side: hands.pose.bones[f"forearm.{side}"] for side in SIDES}


def elbow_error(side):
    bpy.context.view_layer.update()
    return (forearms[side].head - expected[side]).length


for side in SIDES:
    forearms[side].constraints["IK"].mute = True
bpy.context.view_layer.update()
expected = {side: forearms[side].head.copy() for side in SIDES}
for side in SIDES:
    forearms[side].constraints["IK"].mute = False

# Each arm on its own: the left one is mirrored, so its angle differs.
for side in SIDES:
    ik = forearms[side].constraints["IK"]
    best = None
    for degrees in range(-180, 180, 1):
        ik.pole_angle = math.radians(degrees)
        error = elbow_error(side)
        if best is None or error < best[1]:
            best = (degrees, error)
    ik.pole_angle = math.radians(best[0])
    print(f"Arm {side}: pole angle {best[0]} deg, elbow off by {best[1] * 100:.2f} cm")

# The handles are drawn larger and in front, so they are easy to grab.
hands.show_in_front = True
for side in SIDES:
    hands.pose.bones[f"wrist_ik.{side}"].custom_shape = None

use_action(bpy.data.actions["HoldPumpBack"])
bpy.ops.object.mode_set(mode='OBJECT')
bpy.ops.wm.save_mainfile()
print("Saved with IK")
