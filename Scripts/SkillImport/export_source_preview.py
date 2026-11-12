"""Export a project-authored bone proxy on the real HKX rig for UE retarget preview.

No character art is fabricated: this deliberately contains tiny octahedra only.
The source c0000 FLVER has no mesh. Never use this proxy as a gameplay character.
"""
import json
from pathlib import Path
import bpy
from mathutils import Vector

root = Path(__file__).resolve().parents[2] / 'Saved/SkillImport/GlintstonePebble/Converted'
bpy.ops.wm.open_mainfile(filepath=str(root / 'ER_Pebble_SourceAnimations.blend'))
rig = next(o for o in bpy.context.scene.objects if o.type == 'ARMATURE')
rig.animation_data.action = bpy.data.actions['a401_045010']
bpy.context.scene.frame_set(0)
bpy.context.view_layer.update()
reference_basis = {b.name:b.matrix_basis.copy() for b in rig.pose.bones}
rig.animation_data_clear()
rig.animation_data_create()
rig.animation_data.action = bpy.data.actions.new('SourceCastFirstFrameReference')
for bone in rig.pose.bones:
    bone.matrix_basis = reference_basis[bone.name]
    bone.rotation_mode = 'QUATERNION'
    for frame in [0, 1]:
        for prop in ['location', 'rotation_quaternion', 'scale']:
            bone.keyframe_insert(prop, frame=frame)
bpy.context.scene.frame_start = 0
bpy.context.scene.frame_end = 1
bpy.context.scene.frame_set(0)
bpy.context.view_layer.update()
vertices, faces, weights = [], [], []
for bone in rig.data.bones:
    center = bone.head_local
    start = len(vertices)
    offsets = [(0.8,0,0),(-0.8,0,0),(0,0.8,0),(0,-0.8,0),(0,0,0.8),(0,0,-0.8)]
    vertices.extend([tuple(center + Vector(v)) for v in offsets])
    faces.extend([tuple(start + i for i in face) for face in
                  [(0,2,4),(2,1,4),(1,3,4),(3,0,4),(2,0,5),(1,2,5),(3,1,5),(0,3,5)]])
    weights.append((bone.name, list(range(start, start+6))))
mesh = bpy.data.meshes.new('SourceBoneProxyGeometry')
mesh.from_pydata(vertices, [], faces)
obj = bpy.data.objects.new('SourceBoneProxy_NotCharacterArt', mesh)
bpy.context.collection.objects.link(obj)
obj.parent = rig
modifier = obj.modifiers.new('SourceSkeleton', 'ARMATURE')
modifier.object = rig
for name, indices in weights:
    obj.vertex_groups.new(name=name).add(indices, 1.0, 'REPLACE')
bpy.ops.object.select_all(action='DESELECT')
rig.select_set(True)
obj.select_set(True)
bpy.context.view_layer.objects.active = rig
bpy.ops.export_scene.fbx(filepath=str(root / 'ER_SourceBoneProxyRest.fbx'), use_selection=True,
                        object_types={'ARMATURE','MESH'}, add_leaf_bones=False,
                        bake_anim=True, bake_anim_use_all_actions=False, bake_anim_use_nla_strips=False,
                        bake_anim_simplify_factor=0, apply_scale_options='FBX_SCALE_NONE',
                        axis_forward='-Y', axis_up='Z')
(root/'source-preview.json').write_text(json.dumps(dict(
    purpose='Project-authored diagnostic geometry; original HKX bone hierarchy',
    import_reference='a401_045010 frame 0; UE use_t0_as_ref_pose=True',
    head_coordinates='Original Blender rest pose, centimeters; not imported first-frame pose',
    bones=[dict(name=b.name, parent=b.parent.name if b.parent else None,
                head=list(b.head_local)) for b in rig.data.bones],
    vertices=len(vertices), faces=len(faces)), indent=2), encoding='utf-8')
print('SOURCE_PREVIEW_EXPORTED', len(rig.data.bones), len(vertices))
