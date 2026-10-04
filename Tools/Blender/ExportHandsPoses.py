# Exports the first-person hands posed on the shotgun in Tools/Blender/HandsPose.blend (made by
# Tools/Blender/HandsPoseScene.py) to Assets/Models/Weapons/Hands.glb, with their poses as clips:
#   Hold         - the hands holding the shotgun;
#   HoldPumpBack - the same, the left hand on the pump pulled back.
# Run with Blender 5.2 (any folder):
#   blender -b Tools/Blender/HandsPose.blend --python Tools/Blender/ExportHandsPoses.py
#
# The hands are exported in the coordinates of the shotgun (its middle at the origin, as the game centers every model):
# the game draws them with the shotgun's matrix, so they follow everything it does (the recoil, the turn to the chest,
# the sway), and blends from Hold to HoldPumpBack as the pump goes back (see Gameplay::CalculateWeaponHandsPose).

import os

import bpy

TARGET = os.path.join(os.path.dirname(__file__), "..", "..", "Assets", "Models", "Weapons", "Hands.glb")
POSES = ("Hold", "HoldPumpBack")

hands = bpy.data.objects["Hands"]
shotgun = bpy.data.objects["Shotgun"]

# Where the hands are relative to the shotgun; once the shotgun is gone, that is where they are in the file.
hands_in_shotgun = shotgun.matrix_world.inverted() @ hands.matrix_world
for obj in list(bpy.data.objects):
    if obj != hands and obj.parent != hands:
        bpy.data.objects.remove(obj, do_unlink=True)
hands.matrix_world = hands_in_shotgun

# Only the two poses go into the file.
for action in list(bpy.data.actions):
    if action.name not in POSES:
        bpy.data.actions.remove(action)
missing = [name for name in POSES if name not in bpy.data.actions]
if missing:
    raise RuntimeError(f"The scene has no pose {missing}: make it in the Action Editor (see HandsPoseScene.py)")
for name in POSES:
    bpy.data.actions[name].use_fake_user = True

# The arms may be posed with IK handles (see AddHandsIK.py): the poses keep only where the handles are, and the bones
# follow them through constraints, which a glTF file cannot hold. So every pose is baked: each bone gets a key where the
# constraints put it, then the constraints are removed.
bpy.context.view_layer.objects.active = hands
bpy.ops.object.mode_set(mode='POSE')
for name in POSES:
    action = bpy.data.actions[name]
    hands.animation_data.action = action
    if action.slots:
        hands.animation_data.action_slot = action.slots[0]
    bpy.context.scene.frame_set(1)
    bpy.ops.nla.bake(frame_start=1, frame_end=1, only_selected=False, visual_keying=True, clear_constraints=False,
                     use_current_action=True, bake_types={'POSE'})
for bone in hands.pose.bones:
    for constraint in list(bone.constraints):
        bone.constraints.remove(constraint)
bpy.ops.object.mode_set(mode='OBJECT')

bpy.ops.export_scene.gltf(filepath=os.path.abspath(TARGET), export_format='GLB', export_animations=True,
                          export_animation_mode='ACTIONS', export_yup=True)
print("Exported", os.path.abspath(TARGET))
