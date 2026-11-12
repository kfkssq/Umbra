"""Explicit UE-only import/configuration of four Greystone skills. Preserves existing HUD layout and Q/E tuning."""
import unreal as u
import os, json, hashlib

assets=u.EditorAssetLibrary
tools=u.AssetToolsHelpers.get_asset_tools()
root='/Game/Skills/Greystone'
animroot='/Game/ParagonGreystone/Characters/Heroes/Greystone/Animations/'
fxroot='/Game/ParagonGreystone/FX/Particles/Greystone/Abilities/'
iconroot='C:/baidunetdiskdownload/艾尔登法环图标/艾尔登法环图标/其它'
report={'skills':[], 'icon_source':'User-local Elden Ring icon dump; version and redistribution license unverified; chosen by visible content, not asserted original skill names.'}

def load(path):
    obj=u.load_asset(path)
    assert obj,path
    return obj
def save(obj):assert assets.save_loaded_asset(obj,False),obj.get_path_name()
def tag(name):
    value=u.GameplayTag();value.import_text('(TagName="'+name+'")');return value
def create(name,cls):
    path=root+'/'+name
    if assets.does_asset_exist(path):return load(path)
    factory=u.DataAssetFactory();factory.set_editor_property('data_asset_class',cls)
    return tools.create_asset(name,root,cls,factory)

specs=[
 dict(name='SwordStorm',title='剑刃风暴',key='One',label='1',tag='Three',anim='Ability_E',icon='08303',mode=u.UmbraSkillExecution.SWORD_STORM,
      cost=30.,cooldown=10.,radius=300.,mult=.45,strike=.48,rate=1.2,
      active='ClearAPath/FX/P_Greystone_SwordSpinning_01',effect='ClearAPath/FX/P_Greystone_ClearAPath_Impact'),
 dict(name='LeapSlam',title='跃击震地',key='Two',label='2',tag='Four',anim='Ability_R',icon='08400',mode=u.UmbraSkillExecution.LEAP_SLAM,
      cost=25.,cooldown=8.,radius=300.,mult=2.,strike=.12,rate=1.,
      active=None,effect='LeapAOE/FX/P_Greystone_LeapAOE_GroundSmash'),
 dict(name='Guard',title='坚盾防御',key='Three',label='3',tag='Five',anim='Deflect_InPlace',icon='08361',mode=u.UmbraSkillExecution.GUARD,
      cost=20.,cooldown=9.,radius=100.,mult=0.,strike=.1,rate=1.133333/2.5,
      active='Deflect/FX/P_Greystone_Deflect_Looping',effect=None),
 dict(name='StoneRecovery',title='石化复苏',key='Four',label='4',tag='Six',anim='Ability_Ultimate',icon='08421',mode=u.UmbraSkillExecution.STONE_RECOVERY,
      cost=40.,cooldown=20.,radius=350.,mult=1.25,strike=3.333333,rate=1.,
      active='Ultimate/FX/P_Greystone_Ultimate_Stone',effect='Ultimate/FX/P_Greystone_HToKill_Resurrect')]

# Check every input conflict before changing assets.
imc=load('/Game/Input/IMC_Default')
mappings=list(imc.get_editor_property('default_key_mappings').get_editor_property('mappings'))
report['existing_inputs']=[{'key':str(m.get_editor_property('key').get_editor_property('key_name')),
                          'action':m.get_editor_property('action').get_path_name()} for m in mappings]
for index,s in enumerate(specs,3):
    path='/Game/Input/IA_Skill'+str(index)
    for m in mappings:
        if str(m.get_editor_property('key').get_editor_property('key_name'))==s['key']:
            assert m.get_editor_property('action').get_path_name().split('.')[0]==path,'Occupied input: '+s['key']

assets.make_directory(root)
definitions=[]
for s in specs:
    name=s['name'];path=root+'/AM_'+name
    animation=load(animroot+s['anim'])
    if name=='StoneRecovery':
        inplace=root+'/A_StoneRecovery_Slam'
        animation=load(inplace) if assets.does_asset_exist(inplace) else assets.duplicate_asset(animroot+s['anim'],inplace)
        # Preserve aerial descent, reducing the source's forty-metre visual root arc to 220 cm.
        assert u.UmbraSkillEditorLibrary.scale_skill_root_height(load(animroot+s['anim']),animation,220.)
        save(animation)
    if assets.does_asset_exist(path):montage=load(path)
    else:
        factory=u.AnimMontageFactory()
        factory.set_editor_properties(dict(source_animation=animation,target_skeleton=animation.get_editor_property('skeleton')))
        montage=tools.create_asset('AM_'+name,root,u.AnimMontage,factory)
    tracks=list(montage.get_editor_property('slot_anim_tracks'))
    tracks[0].set_editor_property('slot_name','UpperBody')
    track=tracks[0].get_editor_property('anim_track')
    segments=list(track.get_editor_property('anim_segments'))
    segments[0].set_editor_properties(dict(anim_reference=animation,anim_start_time=0.,anim_end_time=animation.get_play_length(),
                                          looping_count=3 if name=='SwordStorm' else 1,anim_play_rate=1.))
    track.set_editor_property('anim_segments',segments)
    tracks[0].set_editor_property('anim_track',track)
    montage.set_editor_property('slot_anim_tracks',tracks)
    assert u.UmbraSkillEditorLibrary.set_skill_montage_sequence(montage,animation,3 if name=='SwordStorm' else 1)
    save(montage)
    filename=os.path.join(iconroot,'menu_knowledge_'+s['icon']+'out.obj.png')
    iconname='T_Skill_'+name+'_ER'+s['icon']
    iconpath='/Game/UI/Icons/EldenRing/'+iconname
    if not assets.does_asset_exist(iconpath):
        task=u.AssetImportTask()
        task.set_editor_properties(dict(filename=filename,destination_path='/Game/UI/Icons/EldenRing',destination_name=iconname,
                                       automated=True,replace_existing=False,save=False))
        tools.import_asset_tasks([task])
        assert task.get_editor_property('imported_object_paths')
    icon=load(iconpath)
    icon.set_editor_properties(dict(compression_settings=u.TextureCompressionSettings.TC_EDITOR_ICON,
        lod_group=u.TextureGroup.TEXTUREGROUP_UI,mip_gen_settings=u.TextureMipGenSettings.TMGS_NO_MIPMAPS,srgb=True,never_stream=True))
    save(icon)
    definition=create('DA_'+name,u.UmbraSweepDefinition)
    definition.set_editor_properties(dict(skill_id=name,display_name=s['title'],execution=s['mode'],icon=icon,
        cooldown_tag=tag('Cooldown.Skill.'+name),montage=montage,strike_time=s['strike'],play_rate=s['rate'],
        radius=s['radius'],arc_degrees=360.,height_tolerance=120.,resource_cost=s['cost'],cooldown_seconds=s['cooldown'],
        pulse_count=6,pulse_interval=.5,guard_duration=2.5,guard_armor=100.,guard_physical_resistance=50.,
        leap_distance=450.,leap_height=160.,leap_duration=.65,heal_max_health_fraction=.3,
        active_effect=load(fxroot+s['active']) if s['active'] else None,
        active_effect_offset=u.Vector(0,0,-90),active_effect_scale=u.Vector(1,1,1),
        strike_effect=load(fxroot+s['effect']) if s['effect'] else None,
        effect_offset=u.Vector(0,0,-90),effect_scale=u.Vector(1,1,1)))
    damage=definition.get_editor_property('damage');typed=damage.get_editor_property('typed')
    typed.set_editor_properties(dict(model=u.UmbraDamageModel.WEAPON_CHANNELS,weapon_multiplier=s['mult'],use_damage_buckets=True))
    damage.set_editor_property('typed',typed);definition.set_editor_property('damage',damage)
    save(definition);definitions.append(definition)
    with open(filename,'rb') as f:sha=hashlib.sha256(f.read()).hexdigest()
    report['skills'].append(dict(skill=name,key=s['label'],asset=definition.get_path_name(),animation=animroot+s['anim'],icon_source=filename,sha256=sha))

player_bp=load('/Game/Blueprints/Player/BP_UmbraPlayerCharacter')
player=u.get_default_object(player_bp.generated_class());bindings=list(player.get_editor_property('ability_input_actions'))
state_bp=load('/Game/Blueprints/Player/BP_UmbraPlayerState')
state=u.get_default_object(state_bp.generated_class());slots=list(state.get_editor_property('initial_skill_slots'))
for index,(s,definition) in enumerate(zip(specs,definitions),3):
    name='IA_Skill'+str(index);path='/Game/Input/'+name
    if assets.does_asset_exist(path):action=load(path)
    else:
        factory=u.DataAssetFactory();factory.set_editor_property('data_asset_class',u.InputAction)
        action=tools.create_asset(name,'/Game/Input',u.InputAction,factory)
    save(action)
    key=u.Key();key.import_text(s['key'])
    if not any(m.get_editor_property('action')==action and str(m.get_editor_property('key').get_editor_property('key_name'))==s['key'] for m in mappings):
        imc.map_key(action,key)
    input_tag=tag('Input.Skill.'+s['tag'])
    if not any(b.get_editor_property('input_action')==action for b in bindings):
        bindings.append(u.UmbraTaggedInputAction(input_action=action,input_tag=input_tag))
    if not any(slot.get_editor_property('definition')==definition for slot in slots):
        slots.append(u.UmbraSkillSlot(definition=definition,input_tag=input_tag,key_label=s['label']))
player.set_editor_property('ability_input_actions',bindings)
state.set_editor_property('initial_skill_slots',slots)
for bp in [player_bp,state_bp]:u.BlueprintEditorLibrary.compile_blueprint(bp);save(bp)
save(imc)

bar=load('/Game/UI/Skills/WBP_SkillBar')
# Modify only instance previews, never reconstruct the user's layout or template children.
for widget in u.UmbraEquipmentNetworkTestLibrary.get_blueprint_widgets(bar):
    if isinstance(widget,u.UmbraSkillSlotWidget):
        index=widget.get_editor_property('slot_index')
        if 2<=index<=5:
            definition=definitions[index-2];s=specs[index-2]
            widget.set_editor_properties(dict(preview_icon=definition.get_editor_property('icon'),preview_key_label=s['label'],preview_resource_cost=s['cost']))
u.BlueprintEditorLibrary.compile_blueprint(bar);save(bar)
with open(os.path.join(u.Paths.project_saved_dir(),'SkillPrototype','expansion-configuration.json'),'w',encoding='utf-8') as f:json.dump(report,f,ensure_ascii=False,indent=2)
u.log('GREYSTONE_EXPANSION_CONFIGURATION_PASSED')
