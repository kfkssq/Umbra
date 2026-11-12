"""Read-only independent load and pose checks for the candidate source/target clip."""
import json
from pathlib import Path
import unreal as u

root = '/Game/Skills/Staging/GlintstonePebble'
report_path = Path(u.Paths.project_saved_dir())/'SkillImport/GlintstonePebble/ue-retarget-readback.json'
report_path.write_text(json.dumps({'result':'running', 'candidate':'SourceV4/RetargetV4'}), encoding='utf-8')
source_mesh = u.load_asset(root+'/SourceV4/SK_ER_SourceBoneProxy')
source = u.load_asset(root+'/SourceV4/AS_ER_Cast_a401_045010')
target = u.load_asset(root+'/RetargetV4/AS_Greystone_Pebble_Cast_a401_045010')
greystone = u.load_asset('/Game/ParagonGreystone/Characters/Heroes/Greystone/Meshes/Greystone')
assert source_mesh.skeleton and source.get_editor_property('skeleton') == source_mesh.skeleton
assert target.get_editor_property('skeleton') == greystone.skeleton
assert abs(source.get_play_length()-1.5) < 0.001 and abs(target.get_play_length()-1.5) < 0.001
report = {'source':source.get_path_name(), 'target':target.get_path_name(), 'duration':1.5}
for label, path, bones in [('source','IK_ER_Pebble_Source',['Master','RootPos','Pelvis','Head','L_Foot']),
                           ('target','IK_Greystone_Pebble',['root','pelvis','head','foot_l'])]:
    rig = u.load_asset(root+'/RetargetV4/'+path)
    controller = u.IKRigController.get_controller(rig)
    report[label+'_reference'] = {bone:str(controller.get_ref_pose_transform_of_bone(bone)) for bone in bones}
for label, animation, bones in [('source',source,['ER_c0000_SourceRig','Master','RootPos','Pelvis','L_UpperArm','R_UpperArm','Spine','L_Foot']),
                                ('target',target,['root','pelvis','upperarm_l','upperarm_r','spine_01','foot_l'])]:
    report[label+'_poses'] = {bone:[str(u.AnimationLibrary.get_bone_pose_for_time(animation,bone,t,False))
                                   for t in [0,0.5,1.0,1.45]] for bone in bones}
    assert any(len(set(values))>1 for values in report[label+'_poses'].values()), label+' has no pose variation'
report['result'] = 'checking'
report['global_heights'] = []
for label, mesh, animation, head, pelvis, feet in [
    ('source', source_mesh, source, 'Head', 'Pelvis', ['L_Foot','R_Foot']),
    ('target', greystone, target, 'head', 'pelvis', ['foot_l','foot_r'])]:
    component = u.SkeletalMeshComponent()
    component.set_skeletal_mesh_asset(mesh)
    for time in [0, 0.5, 1.0, 1.45]:
        cache = {}
        def global_pose(bone):
            if bone not in cache:
                local = u.AnimationLibrary.get_bone_pose_for_time(animation, bone, time, False)
                parent = str(component.get_parent_bone(bone))
                cache[bone] = u.MathLibrary.compose_transforms(local, global_pose(parent)) if parent != 'None' else local
            return cache[bone]
        heights = {bone:global_pose(bone).translation.z for bone in [head,pelvis]+feet}
        report['global_heights'].append(dict(label=label,time=time,heights=heights))
        report_path.write_text(json.dumps(report,indent=2), encoding='utf-8')
        assert heights[head]-heights[pelvis] > 30, (label,time,'head/pelvis orientation',heights)
        assert heights[pelvis]-max(heights[bone] for bone in feet) > 50, (label,time,'pelvis/feet orientation',heights)
report['result'] = 'passed'
report_path.write_text(
    json.dumps(report,indent=2),encoding='utf-8')
u.log('PEBBLE_RETARGET_READBACK_PASSED')
