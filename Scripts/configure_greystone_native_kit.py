"""Explicit UE migration to the native Greystone kit; never edits vendor animations."""
import json
from pathlib import Path
import unreal as u
out=Path(u.Paths.project_saved_dir())/'GreystoneDirection'
out.mkdir(parents=True,exist_ok=True)
root='/Game/Skills/Greystone'
anim='/Game/ParagonGreystone/Characters/Heroes/Greystone/Animations/'
def load(p):
    o=u.load_asset(p);assert o,p;return o
def save(o):assert u.EditorAssetLibrary.save_loaded_asset(o,False),o.get_path_name()
def tag(s):
    t=u.GameplayTag();t.import_text('(TagName="'+s+'")');return t
def create(name,cls,factory):
    p=root+'/'+name
    return load(p) if u.EditorAssetLibrary.does_asset_exist(p) else u.AssetToolsHelpers.get_asset_tools().create_asset(name,root,cls,factory)
# Slot one was manually redesigned; this historical bulk migration must not overwrite it.
existing=u.load_asset(root+'/DA_BreachSlash')
assert not existing or str(existing.display_name) != '斩切波', 'Use configure_slash_wave.py; native-kit migration predates the slot-one redesign'
names=['BreachSlash','SwordStorm','LeapSlam','Guard','StoneRecovery','BattleFocus']
sources=['Ability_Q','Ability_E','Ability_R','Deflect_InPlace',None,'Cast']
numbers=['12','11','08','13','26','23']
keys=['One','Two','Three','Four','Five','Six']
actions=[load('/Game/Input/IA_Skill'+str(i)) for i in range(1,7)]
imc=load('/Game/Input/IMC_Default')
mappings=list(imc.get_editor_property('default_key_mappings').get_editor_property('mappings'))
pebble=load('/Game/Input/IA_GlintstonePebble')
for m in mappings:
    assert str(m.get_editor_property('key').get_editor_property('key_name')) not in keys or m.get_editor_property('action') in actions+[pebble], 'Number key occupied by unrelated action'
factory=u.DataAssetFactory();factory.set_editor_property('data_asset_class',u.UmbraSweepDefinition)
breach=create('DA_BreachSlash',u.UmbraSweepDefinition,factory)
base=load('/Game/Skills/Prototype/DA_Sweep')
for prop in ['damage','strike_effect','strike_sound','effect_offset','effect_rotation','effect_scale']:
    breach.set_editor_property(prop,base.get_editor_property(prop))
breach.set_editor_properties(dict(skill_id='BreachSlash',display_name='破阵斩',execution=u.UmbraSkillExecution.SECTOR,
    cooldown_tag=tag('Cooldown.Skill.Sweep'),strike_time=.4,play_rate=1.,radius=250.,arc_degrees=120.,resource_cost=15.,cooldown_seconds=4.))
focus=create('DA_BattleFocus',u.UmbraSweepDefinition,factory)
focus.set_editor_properties(dict(skill_id='BattleFocus',display_name='战意',execution=u.UmbraSkillExecution.BATTLE_FOCUS,
    cooldown_tag=tag('Cooldown.Skill.BattleFocus'),strike_time=.55,play_rate=1.,resource_cost=20.,cooldown_seconds=12.,
    focus_duration=6.,focus_critical_chance=.2))
definitions=[]
for name,source,number in zip(names,sources,numbers):
    d=load(root+'/DA_'+name)
    if source:
        a=load(anim+source);f=u.AnimMontageFactory();f.set_editor_properties(dict(source_animation=a,target_skeleton=a.get_editor_property('skeleton')))
        montage=create('AM_'+name,u.AnimMontage,f)
        if name=='Guard':
            assert u.UmbraSkillEditorLibrary.configure_guard_montage(montage,a,2.5)
            d.set_editor_property('play_rate',1.)
        else:assert u.UmbraSkillEditorLibrary.set_skill_montage_sequence(montage,a,3 if name=='SwordStorm' else 1)
        save(montage);d.set_editor_property('montage',montage)
    d.set_editor_property('icon',load('/Game/Asset/Icons/flat_skills_icons_no_color/warrior/warrior_'+number))
    # Animation iteration phase: no resource gate or deduction for the active kit.
    d.set_editor_property('resource_cost',0.)
    save(d);definitions.append(d)
for action in actions+[pebble]:imc.unmap_all_keys_from_action(action)
for action,key in zip(actions,keys):
    k=u.Key();k.import_text(key);imc.map_key(action,k)
save(imc)
player_bp=load('/Game/Blueprints/Player/BP_UmbraPlayerCharacter');player=u.get_default_object(player_bp.generated_class())
bindings=[b for b in player.get_editor_property('ability_input_actions') if b.input_action not in actions+[pebble]]
bindings += [u.UmbraTaggedInputAction(input_action=a,input_tag=tag('Input.Skill.'+key)) for a,key in zip(actions,keys)]
player.set_editor_property('ability_input_actions',bindings)
state_bp=load('/Game/Blueprints/Player/BP_UmbraPlayerState');state=u.get_default_object(state_bp.generated_class())
state.set_editor_property('initial_skill_slots',[u.UmbraSkillSlot(definition=d,input_tag=tag('Input.Skill.'+key),key_label=str(i+1)) for i,(d,key) in enumerate(zip(definitions,keys))])
for bp in [player_bp,state_bp]:u.BlueprintEditorLibrary.compile_blueprint(bp);save(bp)
bar=load('/Game/UI/Skills/WBP_SkillBar')
for w in u.UmbraEquipmentNetworkTestLibrary.get_blueprint_widgets(bar):
    if not isinstance(w,u.UmbraSkillSlotWidget):continue
    i=w.get_editor_property('slot_index')
    w.set_editor_properties(dict(preview_icon=definitions[i].icon if i<6 else None,
        preview_key_label=str(i+1) if i<6 else '',preview_resource_cost=definitions[i].resource_cost if i<6 else 0.,preview_cooldown_fraction=0.))
u.BlueprintEditorLibrary.compile_blueprint(bar);save(bar)
report=[dict(key=str(i+1),definition=d.get_path_name(),icon=d.icon.get_path_name(),montage=d.montage.get_path_name(),tracks=[str(t) for t in d.montage.get_editor_property('slot_anim_tracks')]) for i,d in enumerate(definitions)]
(out/'native-configuration.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
u.log('NATIVE_GREYSTONE_KIT_CONFIGURED')
