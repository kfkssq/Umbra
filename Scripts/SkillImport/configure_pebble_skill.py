"""Create a playable Pebble adaptation through UE APIs; preserve other skill slots and UI layout."""
import hashlib
import json
from pathlib import Path
import unreal as u

root='/Game/Skills/GlintstonePebble'
out=Path(u.Paths.project_saved_dir())/'SkillImport/GlintstonePebble'
assets=u.EditorAssetLibrary
tools=u.AssetToolsHelpers.get_asset_tools()
report={'assets':[], 'presentation':'Rebuilt particles using extracted ER textures; project-authored sounds; not lossless FXR/audio conversion'}
def load(path):
    result=u.load_asset(path)
    assert result,path
    return result
def save(asset):
    assert assets.save_loaded_asset(asset,False),asset.get_path_name()
def tag(name):
    result=u.GameplayTag();result.import_text('(TagName="'+name+'")');return result
def create(name,cls,factory):
    path=root+'/'+name
    return load(path) if assets.does_asset_exist(path) else tools.create_asset(name,root,cls,factory)
def import_file(path,name):
    destination=root+'/'+name
    if not assets.does_asset_exist(destination):
        task=u.AssetImportTask()
        task.set_editor_properties(dict(filename=str(path),destination_path=root,destination_name=name,
                                       automated=True,replace_existing=False,save=False))
        tools.import_asset_tasks([task])
    asset=load(destination)
    digest=hashlib.sha256(path.read_bytes()).hexdigest()
    assets.set_metadata_tag(asset,'UmbraSkillImport.SourceHash',digest)
    assets.set_metadata_tag(asset,'UmbraSkillImport.Source',str(path))
    save(asset)
    report['assets'].append(dict(path=destination,source=str(path),sha256=digest))
    return asset

mask=import_file(out/'Textures/s10011_a.png','T_ER_PebbleGlow')
noise=import_file(out/'Textures/s34050_a.png','T_ER_PebbleMagic')
matlib=u.MaterialEditingLibrary
def material(name,use_noise):
    path=root+'/'+name
    if assets.does_asset_exist(path):return load(path)
    mat=create(name,u.Material,u.MaterialFactoryNew())
    mat.set_editor_properties(dict(blend_mode=u.BlendMode.BLEND_ADDITIVE,shading_model=u.MaterialShadingModel.MSM_UNLIT,two_sided=True))
    matlib.set_material_usage(mat,u.MaterialUsage.MATUSAGE_PARTICLE_SPRITES)
    def node(cls,x,y):return matlib.create_material_expression(mat,cls,x,y)
    color=node(u.MaterialExpressionParticleColor,-600,0)
    radial=node(u.MaterialExpressionTextureSample,-600,250);radial.set_editor_property('texture',mask)
    opacity=node(u.MaterialExpressionMultiply,-100,200)
    matlib.connect_material_expressions(color,'A',opacity,'A')
    matlib.connect_material_expressions(radial,'A',opacity,'B')
    matlib.connect_material_property(opacity,'',u.MaterialProperty.MP_OPACITY)
    if use_noise:
        tex=node(u.MaterialExpressionTextureSample,-600,-200);tex.set_editor_property('texture',noise)
        multiply=node(u.MaterialExpressionMultiply,-100,0)
        matlib.connect_material_expressions(color,'RGB',multiply,'A')
        matlib.connect_material_expressions(tex,'RGB',multiply,'B')
        matlib.connect_material_property(multiply,'',u.MaterialProperty.MP_EMISSIVE_COLOR)
    else:matlib.connect_material_property(color,'RGB',u.MaterialProperty.MP_EMISSIVE_COLOR)
    matlib.recompile_material(mat);save(mat)
    return mat
glow=material('M_PebbleGlow',False)
magic=material('M_PebbleMagic',True)
effects={}
for stage,name in enumerate(['Cast','Flight','Impact','Release']):
    system=create('P_Pebble_'+name,u.ParticleSystem,u.ParticleSystemFactoryNew())
    assert u.UmbraSkillEditorLibrary.configure_pebble_particles(system,glow,magic,stage)
    assets.set_metadata_tag(system,'UmbraSkillImport.Purpose','Authored reconstruction, not converted FXR')
    save(system);effects[name]=system
sounds={name:import_file(out/('Audio/S_Pebble_'+name+'.wav'),'S_Pebble_'+name) for name in ['Cast','Release','Impact']}
for sound in sounds.values():
    assets.set_metadata_tag(sound,'UmbraSkillImport.Purpose','Project-authored synthesis; not Elden Ring audio')
    save(sound)
icon=import_file(Path('C:/baidunetdiskdownload/艾尔登法环图标/艾尔登法环图标/技能/menu_knowledge_06000out.obj.png'),'T_GlintstonePebble_Icon')
icon.set_editor_properties(dict(compression_settings=u.TextureCompressionSettings.TC_EDITOR_ICON,
    lod_group=u.TextureGroup.TEXTUREGROUP_UI,mip_gen_settings=u.TextureMipGenSettings.TMGS_NO_MIPMAPS,srgb=True,never_stream=True));save(icon)

animation_path=root+'/A_PebbleCast'
animation=load(animation_path) if assets.does_asset_exist(animation_path) else assets.duplicate_asset(
    '/Game/Skills/Staging/GlintstonePebble/RetargetV4/AS_Greystone_Pebble_Cast_a401_045010',animation_path)
save(animation)
factory=u.AnimMontageFactory()
factory.set_editor_properties(dict(source_animation=animation,target_skeleton=animation.get_editor_property('skeleton')))
montage=create('AM_PebbleCast',u.AnimMontage,factory)
assert u.UmbraSkillEditorLibrary.set_skill_montage_sequence(montage,animation,1)
save(montage)
factory=u.DataAssetFactory();factory.set_editor_property('data_asset_class',u.UmbraSweepDefinition)
definition=create('DA_GlintstonePebble',u.UmbraSweepDefinition,factory)
definition.set_editor_properties(dict(skill_id='GlintstonePebble',display_name='辉石魔砾',icon=icon,
    execution=u.UmbraSkillExecution.PROJECTILE,cooldown_tag=tag('Cooldown.Skill.GlintstonePebble'),
    montage=montage,strike_time=.35,play_rate=1.,resource_cost=7.,cooldown_seconds=1.8,
    cast_socket='hand_r',active_effect=effects['Cast'],active_effect_offset=u.Vector(0,0,0),
    active_effect_scale=u.Vector(1,1,1),cast_sound=sounds['Cast'],strike_effect=None, # Original 523001 has audio only.
    effect_offset=u.Vector(0,0,0),effect_scale=u.Vector(1,1,1),strike_sound=sounds['Release'],
    projectile_initial_speed=1300.,projectile_max_speed=2500.,projectile_acceleration=4000.,
    projectile_acceleration_delay=.32,projectile_lifetime=1.12,projectile_radius=10.,projectile_turn_rate=35.,
    projectile_effect=effects['Flight'],impact_effect=effects['Impact'],impact_sound=sounds['Impact']))
# Preserve verified original audio when rerunning the initial skill bootstrap.
if assets.does_asset_exist(root+'/SC_ER_Pebble_Impact'):
    definition.set_editor_properties(dict(cast_sound=None,
        strike_sound=load(root+'/S_ER_Pebble_Release_524617251'),
        projectile_sound=load(root+'/S_ER_Pebble_Flight_1040010023'),
        impact_sound=load(root+'/SC_ER_Pebble_Impact')))
damage=definition.get_editor_property('damage');typed=damage.get_editor_property('typed')
typed.set_editor_properties(dict(model=u.UmbraDamageModel.EXPLICIT_CHANNELS,can_critical=True,use_damage_buckets=True,
    channels=[u.UmbraAttackDamageChannel(type=u.UmbraWeaponDamageType.COLD,base_damage=24.,ability_power_coefficient=.7,attack_power_coefficient=0.)]))
damage.set_editor_property('typed',typed);definition.set_editor_property('damage',damage)
assets.set_metadata_tag(definition,'UmbraSkillImport.Adaptation','Cold damage channel; authored 0.35s release and 1.8s cooldown; hand-cast without staff requirement')
save(definition)

imc=load('/Game/Input/IMC_Default')
action_path='/Game/Input/IA_GlintstonePebble'
if assets.does_asset_exist(action_path):action=load(action_path)
else:
    factory=u.DataAssetFactory();factory.set_editor_property('data_asset_class',u.InputAction)
    action=tools.create_asset('IA_GlintstonePebble','/Game/Input',u.InputAction,factory)
save(action)
key=u.Key();key.import_text('Five')
mappings=list(imc.get_editor_property('default_key_mappings').get_editor_property('mappings'))
matching=[m for m in mappings if str(m.get_editor_property('key').get_editor_property('key_name'))=='Five']
assert not any(m.get_editor_property('action')!=action for m in matching),'Key 5 occupied'
if not matching:imc.map_key(action,key)
save(imc)
player_bp=load('/Game/Blueprints/Player/BP_UmbraPlayerCharacter')
player=u.get_default_object(player_bp.generated_class())
bindings=list(player.get_editor_property('ability_input_actions'))
if not any(b.input_action==action for b in bindings):
    bindings.append(u.UmbraTaggedInputAction(input_action=action,input_tag=tag('Input.Skill.Seven')))
player.set_editor_property('ability_input_actions',bindings)
state_bp=load('/Game/Blueprints/Player/BP_UmbraPlayerState');state=u.get_default_object(state_bp.generated_class())
slots=list(state.get_editor_property('initial_skill_slots'))
indices=[i for i,s in enumerate(slots) if s.definition==definition]
assert indices==[6] or (not indices and len(slots)==6), 'Unexpected slot layout; preserve existing skills'
if not indices:slots.append(u.UmbraSkillSlot(definition=definition,input_tag=tag('Input.Skill.Seven'),key_label='5'))
state.set_editor_property('initial_skill_slots',slots)
for bp in [player_bp,state_bp]:u.BlueprintEditorLibrary.compile_blueprint(bp);save(bp)
bar=load('/Game/UI/Skills/WBP_SkillBar')
for widget in u.UmbraEquipmentNetworkTestLibrary.get_blueprint_widgets(bar):
    if isinstance(widget,u.UmbraSkillSlotWidget) and widget.get_editor_property('slot_index')==6:
        widget.set_editor_properties(dict(preview_icon=icon,preview_key_label='5',preview_resource_cost=7.))
u.BlueprintEditorLibrary.compile_blueprint(bar);save(bar)
report.update(definition=definition.get_path_name(),key='5',slot_index=6,source_icon='EquipParamGoods 4000 -> icon 6000; local catalog match',
              adaptations=['Hand-cast, existing weapon unchanged','Cold damage: 24 + 0.7 AP through GAS','Release .35s and cooldown 1.8s are authored, not decoded TAE'])
(out/'skill-configuration.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
u.log('PEBBLE_SKILL_CONFIGURED')
