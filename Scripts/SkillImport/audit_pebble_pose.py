"""Read-only diagnostic: distinguish retarget pose offsets from animation changes.

These measurements are evidence, not a visual acceptance test.
"""
import json
from pathlib import Path
import unreal as u
root='/Game/Skills/Staging/GlintstonePebble'
controller=u.IKRetargeterController.get_controller(u.load_asset(root+'/RetargetV4/RTG_ER_Pebble_Greystone'))
report={'status':'diagnostic_only','retarget_offsets':{},'samples':[]}
for bone in ['pelvis','spine_01','spine_02','spine_03','neck_01','head','clavicle_l','clavicle_r','upperarm_l','upperarm_r']:
    q=controller.get_rotation_offset_for_retarget_pose_bone(bone,u.RetargetSourceOrTarget.TARGET)
    report['retarget_offsets'][bone]=str(q.rotator())
for label,mesh_path,anim_path,bones in [
    ('source',root+'/SourceV4/SK_ER_SourceBoneProxy',root+'/SourceV4/AS_ER_Cast_a401_045010',
        ['Pelvis','Spine2','Neck','Head','L_Clavicle','R_Clavicle','L_Hand','R_Hand']),
    ('target','/Game/ParagonGreystone/Characters/Heroes/Greystone/Meshes/Greystone',
        '/Game/Skills/GlintstonePebble/A_PebbleCast',
        ['pelvis','spine_03','neck_01','head','clavicle_l','clavicle_r','hand_l','hand_r'])]:
    animation=u.load_asset(anim_path);component=u.SkeletalMeshComponent()
    component.set_skeletal_mesh_asset(u.load_asset(mesh_path))
    for time in [0,.15,.3,.5,.8,1.0,1.45]:
        cache={}
        def global_pose(bone):
            if bone not in cache:
                local=u.AnimationLibrary.get_bone_pose_for_time(animation,bone,time,False)
                parent=str(component.get_parent_bone(bone))
                cache[bone]=u.MathLibrary.compose_transforms(local,global_pose(parent)) if parent!='None' else local
            return cache[bone]
        report['samples'].append(dict(label=label,time=time,bones={bone:dict(
            position=str(global_pose(bone).translation),rotation=str(global_pose(bone).rotation.rotator()),
            local_rotation=str(u.AnimationLibrary.get_bone_pose_for_time(animation,bone,time,False).rotation.rotator())) for bone in bones}))
out=Path(u.Paths.project_saved_dir()).resolve()/'SkillImport/GlintstonePebble'
(out/'pose-diagnostic.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
u.log('PEBBLE_POSE_DIAGNOSTIC_COMPLETE')
