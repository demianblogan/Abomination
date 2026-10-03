# Converts the Straw Reaper by imaginais from the FBX of its archive to Assets/Models/Enemies/StrawReaper.glb.
# Run with Blender 5.2 (any folder):
#   blender -b --factory-startup --python Tools/Blender/ExportStrawReaper.py -- "<path to THE STRAW REAPER.fbx>"
#
# The FBX imports with two problems that the game would show as a broken body:
#   - the author's camera, light and an empty mesh came with the scene;
#   - the vertices of the body are already in the space of the skeleton, but the importer hangs the body from it through
#     a parent-inverse matrix (scale 0.01) and its own transform (the inverse of the skeleton's): the body ends up 100
#     times too small and a meter below its bones, and every animated pose tears it apart.
# All its animations stay one clip ("Scarecrow anim"); the game cuts it into named segments (see Gameplay::Animator).

import os
import sys

import bpy

SOURCE = sys.argv[sys.argv.index("--") + 1]
TARGET = os.path.join(os.path.dirname(__file__), "..", "..", "Assets", "Models", "Enemies", "StrawReaper.glb")

bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.fbx(filepath=SOURCE)

for name in ["Camera", "Light", "Cube.002"]:
    obj = bpy.data.objects.get(name)
    if obj is not None:
        bpy.data.objects.remove(obj, do_unlink=True)

body = bpy.data.objects["Cube.010"]
body.matrix_parent_inverse.identity()
body.matrix_basis.identity()
bpy.context.view_layer.update()

bpy.ops.export_scene.gltf(
    filepath=os.path.abspath(TARGET),
    export_format='GLB',
    export_animations=True,
    export_animation_mode='ACTIONS',
    export_skins=True,
    export_yup=True,
)
print("Exported", os.path.abspath(TARGET))
