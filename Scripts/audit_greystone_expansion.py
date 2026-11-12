"""Read source animation poses, FX availability and current input mappings."""
import unreal as u, json, os
root='/Game/ParagonGreystone/'
report={'animations':[],'effects':[],'inputs':[]}
for name in ['Ability_Q','Ability_E','Ability_R','Ability_Ultimate','Ability_Ultimate_FrozenPose','Deflect_InPlace']:
    a=u.load_asset(root+'Characters/Heroes/Greystone/Animations/'+name)
    assert isinstance(a,u.AnimSequence)
    row={'name':name,'length':a.get_play_length(),'skeleton':str(a.get_editor_property('skeleton')),'notifies':str(u.AnimationLibrary.get_animation_notify_events(a)),'poses':[]}
    for t in [.0,.25,.5,.75]:
        try:row['poses'].append({'t':a.get_play_length()*t,'root':str(u.AnimationLibrary.get_bone_pose_for_time(a,'root',a.get_play_length()*t,False)),
            'pelvis':str(u.AnimationLibrary.get_bone_pose_for_time(a,'pelvis',a.get_play_length()*t,False))})
        except Exception as e:row['pose_error']=str(e);break
    report['animations'].append(row)
for p in u.EditorAssetLibrary.list_assets(root+'FX/Particles/Greystone/Abilities',True,False):
    a=u.load_asset(p)
    if isinstance(a,u.ParticleSystem):report['effects'].append({'path':p,'class':a.get_class().get_name()})
imc=u.load_asset('/Game/Input/IMC_Default')
for m in imc.get_editor_property('default_key_mappings').get_editor_property('mappings'):
    report['inputs'].append({'action':str(m.get_editor_property('action')),'key':str(m.get_editor_property('key'))})
out=os.path.join(u.Paths.project_saved_dir(),'SkillPrototype','expansion-audit.json')
with open(out,'w',encoding='utf-8') as f:json.dump(report,f,ensure_ascii=False,indent=2)
u.log('GREYSTONE_EXPANSION_AUDIT_COMPLETE')
