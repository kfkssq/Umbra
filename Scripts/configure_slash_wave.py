"""Targeted slot-one migration. Run in UE; preserve all other skills and vendor assets."""
import unreal as u,json,datetime,shutil
from pathlib import Path
root=Path(u.Paths.project_dir()).resolve();out=root/'Saved/SlashWave';out.mkdir(exist_ok=True)
backup=out/'Backup'/datetime.datetime.now().strftime('%Y%m%d_%H%M%S')
def load(p):
 o=u.load_asset(p);assert o,p;return o
def save(o):
 p=o.get_path_name().split('.')[0];src=root/'Content'/(p.removeprefix('/Game/')+'.uasset')
 if src.exists():
  dst=backup/src.relative_to(root/'Content');dst.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(src,dst)
 assert u.EditorAssetLibrary.save_loaded_asset(o,False)
p='/Game/Skills/Greystone';d=load(p+'/DA_BreachSlash')
a=load('/Game/ParagonGreystone/Characters/Heroes/Greystone/Animations/Attack_C_Slow')
assert not u.AnimationLibrary.get_animation_notify_events(a),'Inspect source notifies before using it as a skill'
f=u.AnimMontageFactory();f.set_editor_properties(dict(source_animation=a,target_skeleton=a.get_editor_property('skeleton')))
m=load(p+'/AM_SlashWave') if u.EditorAssetLibrary.does_asset_exist(p+'/AM_SlashWave') else u.AssetToolsHelpers.get_asset_tools().create_asset('AM_SlashWave',p,u.AnimMontage,f)
assert u.UmbraSkillEditorLibrary.set_skill_montage_sequence(m,a,1);save(m)
mp=p+'/M_SlashWave'
mat=load(mp) if u.EditorAssetLibrary.does_asset_exist(mp) else u.AssetToolsHelpers.get_asset_tools().create_asset('M_SlashWave',p,u.Material,u.MaterialFactoryNew())
edit=u.MaterialEditingLibrary;edit.delete_all_material_expressions(mat)
mat.set_editor_properties(dict(blend_mode=u.BlendMode.BLEND_ADDITIVE,two_sided=True));mat.set_editor_property('shading_model',u.MaterialShadingModel.MSM_UNLIT)
uv=edit.create_material_expression(mat,u.MaterialExpressionTextureCoordinate,-600,0)
custom=edit.create_material_expression(mat,u.MaterialExpressionCustom,-350,0)
inp=u.CustomInput();inp.set_editor_property('input_name','UV');custom.set_editor_property('inputs',[inp])
custom.set_editor_properties(dict(output_type=u.CustomMaterialOutputType.CMOT_FLOAT1,code='float2 p=(UV-0.5)*2; float bow=0.25*(1-p.y*p.y); float band=1-smoothstep(0.025,0.075,abs(p.x-bow)); return band*(1-smoothstep(0.65,0.95,abs(p.y)));'))
edit.connect_material_expressions(uv,'',custom,'UV')
color=edit.create_material_expression(mat,u.MaterialExpressionVectorParameter,-350,200);color.set_editor_properties(dict(parameter_name='WaveColor',default_value=u.LinearColor(.8,2.5,4.,1.)))
edit.connect_material_property(custom,'',u.MaterialProperty.MP_OPACITY);edit.connect_material_property(color,'',u.MaterialProperty.MP_EMISSIVE_COLOR)
edit.recompile_material(mat);save(mat)
d.set_editor_properties(dict(display_name='斩切波',execution=u.UmbraSkillExecution.PROJECTILE,montage=m,play_rate=.65,windup_end_time=.1,windup_duration=.65,strike_time=.175,resource_cost=0.,
 cast_socket='',active_effect=None,active_niagara=None,cast_sound=None,active_sound=None,resolve_voice=None,
 strike_effect=None,strike_niagara=None,strike_sound=load('/Game/Asset/Audio/Asset/Attack/swordswing_slow'),
 projectile_effect=None,projectile_niagara=None,projectile_mesh=load('/Engine/BasicShapes/Plane'),projectile_material=mat,projectile_mesh_scale=u.Vector(3,3,1),
 projectile_initial_speed=1000.,projectile_max_speed=1000.,projectile_acceleration=0.,projectile_turn_rate=0.,projectile_lifetime=.9,projectile_radius=45.,projectile_half_width=135.,
 projectile_sound=None,impact_sound=None,impact_niagara=None,impact_effect=load('/Game/ParagonGreystone/FX/Particles/Greystone/Abilities/ClearAPath/FX/P_Greystone_ClearAPath_Impact')))
save(d)
(out/'configuration.json').write_text(json.dumps(dict(name=str(d.display_name),id=str(d.skill_id),source=a.get_path_name(),length=a.get_editor_property('sequence_length'),strike_time=d.strike_time,backup=str(backup)),ensure_ascii=False,indent=2),encoding='utf-8')
u.log('SLASH_WAVE_CONFIGURED')
