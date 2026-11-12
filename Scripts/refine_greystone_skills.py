"""UE-only targeted motion and radial cooldown repair; preserves user HUD geometry and other tuning."""
import unreal as u, os, json
assets=u.EditorAssetLibrary;tools=u.AssetToolsHelpers.get_asset_tools();matlib=u.MaterialEditingLibrary
root='/Game/Skills/Greystone';animroot='/Game/ParagonGreystone/Characters/Heroes/Greystone/Animations/'
def load(path):
    obj=u.load_asset(path);assert obj,path;return obj
def save(obj):assert assets.save_loaded_asset(obj,False)
report={}
material_path='/Game/UI/Skills/M_UI_SkillCooldownClock'
if assets.does_asset_exist(material_path):material=load(material_path)
else:material=tools.create_asset('M_UI_SkillCooldownClock','/Game/UI/Skills',u.Material,u.MaterialFactoryNew())
material.set_editor_properties(dict(material_domain=u.MaterialDomain.MD_UI,blend_mode=u.BlendMode.BLEND_TRANSLUCENT))
matlib.delete_all_material_expressions(material)
uv=matlib.create_material_expression(material,u.MaterialExpressionTextureCoordinate,-600,-80)
remaining=matlib.create_material_expression(material,u.MaterialExpressionScalarParameter,-600,160)
remaining.set_editor_properties(dict(parameter_name='Remaining',default_value=.75))
custom=matlib.create_material_expression(material,u.MaterialExpressionCustom,-340,0)
inputs=[]
for name in ['UV','Remaining']:
    entry=u.CustomInput();entry.set_editor_property('input_name',name);inputs.append(entry)
custom.set_editor_properties(dict(output_type=u.CustomMaterialOutputType.CMOT_FLOAT4,description='Clockwise from twelve; dark remaining sector and gold hand',
    inputs=inputs,code='''
float2 p = UV - float2(0.5,0.5);
float angle = frac(atan2(p.x,-p.y) / 6.28318530718 + 1.0);
float elapsed = 1.0 - saturate(Remaining);
float visible = step(0.0001,Remaining);
float dark = step(elapsed,angle) * visible;
float distance = abs(angle-elapsed);
distance = min(distance,1.0-distance);
float hand = (1.0-smoothstep(0.003,0.008,distance)) * step(0.025,length(p)) * visible;
return float4(lerp(float3(0.015,0.02,0.03),float3(0.85,0.66,0.32),hand), max(dark*0.66,hand*0.9));
'''))
matlib.connect_material_expressions(uv,'',custom,'UV')
matlib.connect_material_expressions(remaining,'',custom,'Remaining')
rgb=matlib.create_material_expression(material,u.MaterialExpressionComponentMask,0,0);rgb.set_editor_properties(dict(r=True,g=True,b=True,a=False))
alpha=matlib.create_material_expression(material,u.MaterialExpressionComponentMask,0,150);alpha.set_editor_properties(dict(r=False,g=False,b=False,a=True))
matlib.connect_material_expressions(custom,'',rgb,'');matlib.connect_material_expressions(custom,'',alpha,'')
matlib.connect_material_property(rgb,'',u.MaterialProperty.MP_EMISSIVE_COLOR)
matlib.connect_material_property(alpha,'',u.MaterialProperty.MP_OPACITY)
matlib.recompile_material(material);save(material)
assert u.UmbraSkillEditorLibrary.install_radial_cooldown(material)
bar=load('/Game/UI/Skills/WBP_SkillBar');u.BlueprintEditorLibrary.compile_blueprint(bar);save(bar)

def set_montage(name,animation,loops):
    montage=load(root+'/AM_'+name)
    tracks=list(montage.get_editor_property('slot_anim_tracks'));tracks[0].set_editor_property('slot_name','UpperBody')
    track=tracks[0].get_editor_property('anim_track');segments=list(track.get_editor_property('anim_segments'))
    segments[0].set_editor_properties(dict(anim_reference=animation,anim_start_time=0.,anim_end_time=animation.get_play_length(),looping_count=loops,anim_play_rate=1.))
    track.set_editor_property('anim_segments',segments);tracks[0].set_editor_property('anim_track',track)
    montage.set_editor_property('slot_anim_tracks',tracks)
    assert u.UmbraSkillEditorLibrary.set_skill_montage_sequence(montage,animation,loops)
    save(montage)
    assert montage.get_play_length()>3.7 if name=='SwordStorm' else montage.get_play_length()>4.
    report[name]={'animation':animation.get_path_name(),'montage_length':montage.get_play_length(),'loops':loops}
    return montage

storm=load(root+'/DA_SwordStorm')
set_montage('SwordStorm',load(animroot+'Ability_E'),3)
storm.set_editor_properties(dict(play_rate=1.2,strike_time=.48));save(storm)
source=load(animroot+'Ability_Ultimate');path=root+'/A_StoneRecovery_Slam'
slam=load(path) if assets.does_asset_exist(path) else assets.duplicate_asset(animroot+'Ability_Ultimate',path)
assert u.UmbraSkillEditorLibrary.scale_skill_root_height(source,slam,220.)
save(slam);set_montage('StoneRecovery',slam,1)
recovery=load(root+'/DA_StoneRecovery');recovery.set_editor_property('strike_time',3.333333);save(recovery)
report['root_samples']=[{'t':t,'root':str(u.AnimationLibrary.get_bone_pose_for_time(slam,'root',t,False))} for t in [1.1,2.3,3.1,3.333333,3.6]]
report['cooldown_material']=material.get_path_name()
with open(os.path.join(u.Paths.project_saved_dir(),'SkillPrototype','refinement-assets.json'),'w',encoding='utf-8') as f:json.dump(report,f,ensure_ascii=False,indent=2)
u.log('GREYSTONE_REFINEMENT_ASSETS_PASSED')
