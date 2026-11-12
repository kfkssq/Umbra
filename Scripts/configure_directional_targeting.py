"""UE editor only. Add independent directional prototypes to empty slots 9/10; preserve slots 1..8."""
import unreal as u,json,datetime,shutil
from pathlib import Path
root=Path(u.Paths.project_dir()).resolve();out=root/'Saved/DirectionalTargeting';out.mkdir(exist_ok=True)
backup=out/'Backup'/datetime.datetime.now().strftime('%Y%m%d_%H%M%S');p='/Game/Skills/TargetingPrototype'
def load(path):
 o=u.load_asset(path);assert o,path;return o
def save(o):
 src=root/'Content'/(o.get_path_name().split('.')[0].removeprefix('/Game/')+'.uasset')
 if src.exists():
  dst=backup/src.relative_to(root/'Content');dst.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(src,dst)
 assert u.EditorAssetLibrary.save_loaded_asset(o,False)
def tag(s):
 t=u.GameplayTag();t.import_text('(TagName="'+s+'")');return t
def create(n,c,f):return load(p+'/'+n) if u.EditorAssetLibrary.does_asset_exist(p+'/'+n) else u.AssetToolsHelpers.get_asset_tools().create_asset(n,p,c,f)
state_bp=load('/Game/Blueprints/Player/BP_UmbraPlayerState');state=u.get_default_object(state_bp.generated_class());slots=list(state.initial_skill_slots)
assert len(slots)<=10
for index,identity in [(8,'TargetSectorPrototype'),(9,'TargetWavePrototype')]:
 assert len(slots)<=index or not slots[index].definition or str(slots[index].definition.skill_id)==identity,'Occupied slot '+str(index+1)
before=[str(s.definition) for s in slots[:8]]
mat=create('M_TargetDirection',u.Material,u.MaterialFactoryNew());edit=u.MaterialEditingLibrary
edit.delete_all_material_expressions(mat);mat.set_editor_properties(dict(material_domain=u.MaterialDomain.MD_DEFERRED_DECAL,blend_mode=u.BlendMode.BLEND_TRANSLUCENT))
custom=edit.create_material_expression(mat,u.MaterialExpressionCustom,0,0);inputs=[]
for n in ['Position','ShapeOrigin','ShapeForward','Radius','HalfAngle','Length','HalfWidth','Shape','Fill','Wave','Front']:
 i=u.CustomInput();i.set_editor_property('input_name',n);inputs.append(i)
custom.set_editor_properties(dict(inputs=inputs,output_type=u.CustomMaterialOutputType.CMOT_FLOAT1,code='''float2 d=Position.xy-ShapeOrigin.xy; float2 f=normalize(ShapeForward.xy+float2(.000001,0)); float x=dot(d,f); float y=dot(d,float2(-f.y,f.x)); float r=length(d); float a=abs(atan2(y,x)); float inside; float edge; if(Shape>.5){inside=step(0,x)*step(x,Length)*step(abs(y),HalfWidth); edge=1-step(3,min(min(x,Length-x),HalfWidth-abs(y)));}else{inside=step(r,Radius)*step(a,HalfAngle); edge=1-step(3,min(Radius-r,(HalfAngle-a)*r));} if(Wave>.5) return inside*step(x,Front)*step(max(0,Front-30),x)*.8; return inside*saturate(Fill+edge*.85);'''))
position=edit.create_material_expression(mat,u.MaterialExpressionWorldPosition,-600,0);edit.connect_material_expressions(position,'',custom,'Position')
for j,(n,v) in enumerate([('Radius',300.),('HalfAngle',1.04719755),('Length',800.),('HalfWidth',100.),('Shape',0.),('Fill',.06),('Wave',0.),('Front',0.)]):
 x=edit.create_material_expression(mat,u.MaterialExpressionScalarParameter,-600,100+j*90);x.set_editor_properties(dict(parameter_name=n,default_value=v));edit.connect_material_expressions(x,'',custom,n)
for j,(n,v) in enumerate([('ShapeOrigin',u.LinearColor(0,0,0,0)),('ShapeForward',u.LinearColor(1,0,0,0)),('Tint',u.LinearColor(.1,1,.65,1))]):
 x=edit.create_material_expression(mat,u.MaterialExpressionVectorParameter,-900,100+j*90);x.set_editor_properties(dict(parameter_name=n,default_value=v))
 if n=='Tint':edit.connect_material_property(x,'',u.MaterialProperty.MP_EMISSIVE_COLOR)
 else:edit.connect_material_expressions(x,'',custom,n)
edit.connect_material_property(custom,'',u.MaterialProperty.MP_OPACITY);edit.recompile_material(mat);save(mat)
player_bp=load('/Game/Blueprints/Player/BP_UmbraPlayerCharacter');player=u.get_default_object(player_bp.generated_class());bindings=list(player.get_editor_property('ability_input_actions'));imc=load('/Game/Input/IMC_Default')
configs=[(8,'9','Nine','Sector','扇形斩击测试',u.UmbraSkillExecution.SECTOR),(9,'0','Zero','Wave','条形发波测试',u.UmbraSkillExecution.LINEAR_WAVE)]
while len(slots)<10:slots.append(u.UmbraSkillSlot())
for index,label,keyname,kind,name,execution in configs:
 path=p+'/DA_Target'+kind
 d=load(path) if u.EditorAssetLibrary.does_asset_exist(path) else u.EditorAssetLibrary.duplicate_asset('/Game/Skills/Greystone/DA_BreachSlash',path)
 # Independent montage copy; never change the original skill or montage.
 montage_path=p+'/AM_Target'+kind
 montage=load(montage_path) if u.EditorAssetLibrary.does_asset_exist(montage_path) else u.EditorAssetLibrary.duplicate_asset(d.montage.get_path_name().split('.')[0],montage_path)
 save(montage)
 d.set_editor_properties(dict(skill_id='Target'+kind+'Prototype',display_name=name,description='鼠标选择方向，左键确认，右键取消。独立零消耗判定测试。',execution=execution,montage=montage,
 targeted_leap=False,targeted_direction=True,target_indicator_material=mat,target_leap_curve=None,radius=300.,arc_degrees=120.,target_line_length=800.,target_line_half_width=100.,target_wave_speed=1000.,
 resource_cost=0.,cooldown_seconds=1.,cooldown_tag=tag('Cooldown.Skill.Target'+kind),strike_time=.25,play_rate=1.,windup_end_time=0.,windup_duration=0.,
 active_effect=None,active_niagara=None,strike_effect=None,strike_niagara=None,cast_sound=None,cast_voice=None,resolve_voice=None,strike_sound=None,active_sound=None))
 save(d)
 actionpath='/Game/Input/IA_Skill'+str(index+1)
 action=load(actionpath) if u.EditorAssetLibrary.does_asset_exist(actionpath) else u.AssetToolsHelpers.get_asset_tools().create_asset('IA_Skill'+str(index+1),'/Game/Input',u.InputAction,u.InputAction_Factory());save(action)
 mappings=imc.get_editor_property('default_key_mappings').get_editor_property('mappings')
 assert not any(str(x.get_editor_property('key').get_editor_property('key_name'))==keyname and x.action!=action for x in mappings),'Occupied key '+keyname
 imc.unmap_all_keys_from_action(action);key=u.Key();key.import_text(keyname);imc.map_key(action,key)
 bindings=[b for b in bindings if b.input_action!=action];bindings.append(u.UmbraTaggedInputAction(input_action=action,input_tag=tag('Input.Skill.'+keyname)))
 slots[index]=u.UmbraSkillSlot(definition=d,input_tag=tag('Input.Skill.'+keyname),key_label=label)
save(imc);player.set_editor_property('ability_input_actions',bindings);state.set_editor_property('initial_skill_slots',slots)
for bp in [player_bp,state_bp]:u.BlueprintEditorLibrary.compile_blueprint(bp);save(bp)
bar=load('/Game/UI/Skills/WBP_SkillBar')
for w in u.UmbraEquipmentNetworkTestLibrary.get_blueprint_widgets(bar):
 if isinstance(w,u.UmbraSkillSlotWidget):
  index=w.get_editor_property('slot_index')
  if index in [8,9]:w.set_editor_properties(dict(preview_icon=slots[index].definition.icon,preview_key_label=slots[index].key_label,preview_resource_cost=0.))
u.BlueprintEditorLibrary.compile_blueprint(bar);save(bar)
assert before==[str(s.definition) for s in list(state.initial_skill_slots)[:8]]
(out/'configuration.json').write_text(json.dumps(dict(previous_slots=before,backup=str(backup)),ensure_ascii=False,indent=2),encoding='utf-8');u.log('DIRECTIONAL_CONFIGURED')
