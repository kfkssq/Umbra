"""Blender-only source-rig conversion probe; does not retarget or edit UE assets.

Uses the official Soulstruct 3.2.1 release extracted under Saved/SkillImport.
Original HKX, compendium and skeleton are read-only. Source root motion is
reported separately; these intermediate FBXs must not be treated as final clips.
"""
import json
import sys
from pathlib import Path
from unittest.mock import patch

import bpy
from mathutils import Matrix

project = Path(__file__).resolve().parents[2]
work = project / 'Saved/SkillImport'
sys.path.insert(0, str(work / 'SoulstructDependencies'))
# Keep the third-party library's log/config in this task's isolated workspace.
original_expanduser = Path.expanduser
with patch.object(Path, 'expanduser', lambda self: work / 'SoulstructUserData'
                  if str(self).replace('\\', '/') == '~/AppData/Roaming/soulstruct'
                  else original_expanduser(self)):
    import soulstruct.config
from soulstruct.havok.core import HKX
from soulstruct.havok.fromsoft.eldenring import AnimationHKX, SkeletonHKX

source = work / 'GlintstonePebble'
output = source / 'Converted'
output.mkdir(exist_ok=True)
compendium = HKX.from_path(source / 'a9x.compendium')
skeleton = SkeletonHKX.from_path(source / 'Skeleton.hkx').skeleton
# FromSoft Y-up, left-handed -> Blender Z-up, right-handed.
axes = Matrix(((1, 0, 0, 0), (0, 0, 1, 0), (0, 1, 0, 0), (0, 0, 0, 1)))
def matrix(trs):
    result = axes @ Matrix(trs.to_matrix4().data.tolist()) @ axes
    result.translation *= 100.0  # bake meters to centimeters; no FBX node-scale workaround
    return result

bpy.ops.object.select_all(action='SELECT')
bpy.ops.object.delete(use_global=False)
data = bpy.data.armatures.new('ER_c0000_SourceSkeleton')
rig = bpy.data.objects.new('ER_c0000_SourceRig', data)
bpy.context.collection.objects.link(rig)
bpy.context.view_layer.objects.active = rig
rig.select_set(True)
bpy.ops.object.mode_set(mode='EDIT')
rest = skeleton.get_arma_space_reference_poses()
for bone in skeleton.bones:
    edit = data.edit_bones.new(bone.name)
    edit.matrix = matrix(rest[bone.name])
    edit.length = 3.0
for bone in skeleton.bones:
    if bone.parent:
        data.edit_bones[bone.name].parent = data.edit_bones[bone.parent.name]
bpy.ops.object.mode_set(mode='OBJECT')
rig.animation_data_create()
bpy.context.scene.render.fps = 30
bpy.context.scene.unit_settings.system = 'METRIC'
bpy.context.scene.unit_settings.scale_length = 0.01
reports = []
for path in sorted(source.glob('a401_*.hkx')):
    animation = AnimationHKX.from_path(path, compendium=compendium).to_interleaved_hkx()
    container = animation.animation_container
    frames = container.interleaved_data
    indices = container.get_track_bone_indices()
    duration = container.hkx_animation.duration
    action = bpy.data.actions.new(path.stem)
    action.use_fake_user = True
    rig.animation_data.action = action
    max_error = 0.0
    for frame_index, frame in enumerate(frames):
        # Preserve source duration instead of assuming an exact source frame rate.
        time = frame_index * duration * 30 / max(1, len(frames) - 1)
        bpy.context.scene.frame_set(int(time), subframe=time % 1)
        locals_by_index = dict(zip(indices, frame))
        worlds = {}
        for bone in skeleton.bones:
            local = matrix(locals_by_index.get(bone.index, bone.get_reference_pose()))
            parent_world = worlds[bone.parent.index] if bone.parent else Matrix.Identity(4)
            world = worlds[bone.index] = parent_world @ local
            pose = rig.pose.bones[bone.name]
            rest_world = data.bones[bone.name].matrix_local
            parent_rest = data.bones[bone.parent.name].matrix_local if bone.parent else Matrix.Identity(4)
            pose.rotation_mode = 'QUATERNION'
            pose.matrix_basis = rest_world.inverted() @ parent_rest @ local
            for prop in ('location', 'rotation_quaternion', 'scale'):
                pose.keyframe_insert(prop, frame=time, group=bone.name)
        bpy.context.view_layer.update()
        for bone in skeleton.bones:
            actual, expected = rig.pose.bones[bone.name].matrix, worlds[bone.index]
            max_error = max(max_error, max(abs(actual[r][c] - expected[r][c])
                                          for r in range(4) for c in range(4)))
    assert max_error < 0.001, (path.name, max_error)
    bpy.context.scene.frame_start = 0
    bpy.context.scene.frame_end = round(duration * 30)
    bpy.ops.export_scene.fbx(filepath=str(output / (path.stem + '.fbx')),
                            use_selection=True, object_types={'ARMATURE'},
                            add_leaf_bones=False, bake_anim=True,
                            bake_anim_use_all_actions=False, bake_anim_use_nla_strips=False,
                            bake_anim_simplify_factor=0, apply_scale_options='FBX_SCALE_NONE',
                            axis_forward='-Y', axis_up='Z')
    try:
        root_motion = container.get_reference_frame_samples().tolist()
    except ValueError as error:
        if 'No `extractedMotion`' not in str(error):
            raise
        root_motion = []
    reports.append(dict(source=path.name, frames=len(frames), tracks=len(indices),
                        duration_seconds=duration, max_matrix_error=max_error,
                        root_motion_samples=root_motion))
    print('PEBBLE_ANIMATION_EXPORTED', reports[-1], flush=True)
bpy.ops.wm.save_as_mainfile(filepath=str(output / 'ER_Pebble_SourceAnimations.blend'))
(output / 'animation-report.json').write_text(json.dumps(dict(
    status='source_rig_intermediate_only', bones=len(skeleton.bones), animations=reports,
    limitations=['Root-motion samples not applied to object trajectory',
                 'Not retargeted to Greystone', 'No UE import or visual validation']), indent=2), encoding='utf-8')
