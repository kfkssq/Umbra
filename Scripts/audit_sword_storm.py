import unreal as u,json
from pathlib import Path
out=Path(u.Paths.project_saved_dir())/'SwordStorm';out.mkdir(exist_ok=True)
bp=u.load_asset('/Game/Blueprints/Player/BP_UmbraPlayerCharacter');c=u.get_default_object(bp.generated_class());mesh=c.get_component_by_class(u.SkeletalMeshComponent)
a='/Game/ParagonGreystone/Characters/Heroes/Greystone/Animations/'
r=dict(anim_class=str(mesh.get_editor_property('anim_class')),animations=[])
for n in ['Ability_E','Spin_Jog_Fwd2Bwd_CW','Spin_Jog_Bwd2Fwd_CW','Jog_Fwd','Idle']:
 o=u.load_asset(a+n);length=o.get_editor_property('sequence_length');row=dict(name=n,length=length,poses=[])
 for t in [0,length*.25,length*.5,length*.75,length]:
  row['poses'].append(dict(t=t,**{b:str(u.AnimationLibrary.get_bone_pose_for_time(o,b,t,False)) for b in ['root','pelvis','spine_01']}))
 r['animations'].append(row)
(out/'audit.json').write_text(json.dumps(r,indent=2),encoding='utf-8')
u.log('STORM_AUDIT_DONE')
