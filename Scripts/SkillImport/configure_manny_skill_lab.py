"""Persist an isolated, directly playable native skill lab through UE asset APIs."""
import json
from pathlib import Path
import unreal as u

root='/Game/SkillLab'
assets=u.EditorAssetLibrary
tools=u.AssetToolsHelpers.get_asset_tools()
out=Path(u.Paths.project_saved_dir()).resolve()/'SkillImport/NativeBaseline'
def load(path):
    asset=u.load_asset(path);assert asset,path
    return asset
def save(asset):
    assert assets.save_loaded_asset(asset,False),asset.get_path_name()
def create(name,cls,factory,folder=root):
    path=folder+'/'+name
    return load(path) if assets.does_asset_exist(path) else tools.create_asset(name,folder,cls,factory)
def bp(name,parent):
    factory=u.BlueprintFactory();factory.set_editor_property('parent_class',parent)
    result=create(name,u.Blueprint,factory)
    return result,u.get_default_object(result.generated_class())
def finish_bp(asset):
    u.BlueprintEditorLibrary.compile_blueprint(asset);save(asset)
def tag(name):
    result=u.GameplayTag();result.import_text('(TagName="'+name+'")');return result

assert u.UmbraSkillEditorLibrary.create_native_lab_anim_blueprint(),'Anim graph creation failed'
anim_bp=load(root+'/Animation/ABP_MannyLab')
animation=load('/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_ChargedAttack')
factory=u.AnimMontageFactory()
factory.set_editor_properties(dict(source_animation=animation,target_skeleton=animation.get_editor_property('skeleton')))
montage=create('AM_MannyArcCast',u.AnimMontage,factory,root+'/Animation')
assert u.UmbraSkillEditorLibrary.set_skill_montage_sequence(montage,animation,1)
save(montage)
factory=u.DataAssetFactory();factory.set_editor_property('data_asset_class',u.UmbraSweepDefinition)
definition=create('DA_MannyArcBolt',u.UmbraSweepDefinition,factory)
definition.set_editor_properties(dict(skill_id='MannyArcBolt',display_name='原生冲击弹',
    icon=load('/Game/Skills/GlintstonePebble/T_GlintstonePebble_Icon'),
    execution=u.UmbraSkillExecution.PROJECTILE,cooldown_tag=tag('Cooldown.Skill.GlintstonePebble'),
    montage=montage,strike_time=.55,play_rate=1.,resource_cost=7.,cooldown_seconds=1.8,
    cast_socket='hand_r',active_niagara=load(root+'/Fixtures/NS_TestFlight'),
    active_effect_scale=u.Vector(.6,.6,.6),strike_niagara=load(root+'/Fixtures/NS_TestBurst'),
    effect_scale=u.Vector(.25,.25,.25),projectile_niagara=load(root+'/Fixtures/NS_TestFlight'),
    impact_niagara=load(root+'/Fixtures/NS_TestBurst'),projectile_initial_speed=900.,
    projectile_max_speed=1200.,projectile_acceleration=600.,projectile_lifetime=2.,projectile_turn_rate=0.))
damage=definition.get_editor_property('damage');typed=damage.get_editor_property('typed')
typed.set_editor_properties(dict(model=u.UmbraDamageModel.EXPLICIT_CHANNELS,can_critical=False,
    channels=[u.UmbraAttackDamageChannel(type=u.UmbraWeaponDamageType.COLD,base_damage=24.,ability_power_coefficient=0.,attack_power_coefficient=0.)]))
damage.set_editor_property('typed',typed);definition.set_editor_property('damage',damage)
assets.set_metadata_tag(definition,'Umbra.Purpose','Native playable prototype; engine Niagara fixtures, not final art or ER reproduction')
save(definition)

player_bp,player=bp('BP_MannyLab',u.UmbraPlayerCharacter)
production=u.get_default_object(load('/Game/Blueprints/Player/BP_UmbraPlayerCharacter').generated_class())
action_path=root+'/IA_MannySkill1'
action=load(action_path) if assets.does_asset_exist(action_path) else tools.duplicate_asset('IA_MannySkill1',root,load('/Game/Input/IA_GlintstonePebble'))
save(action)
imc_path=root+'/IMC_MannyLab'
imc=load(imc_path) if assets.does_asset_exist(imc_path) else tools.duplicate_asset('IMC_MannyLab',root,load('/Game/Input/IMC_Default'))
# Keep movement/menu/click bindings; replace combat slots only in this lab's mapping context.
for binding in production.get_editor_property('ability_input_actions'):
    imc.unmap_all_keys_from_action(binding.input_action)
imc.unmap_all_keys_from_action(action)
key=u.Key();key.import_text('One');imc.map_key(action,key);save(imc)
source_controller=load('/Game/Blueprints/Player/BP_UmbraPlayerController')
controller_bp,controller=bp('BP_MannyLabController',source_controller.generated_class())
for property_name in ['default_mapping_contexts','mobile_excluded_mapping_contexts']:
    contexts=u.get_default_object(source_controller.generated_class()).get_editor_property(property_name)
    controller.set_editor_property(property_name,[imc if c.get_path_name().split('.')[0]=='/Game/Input/IMC_Default' else c for c in contexts])
finish_bp(controller_bp)
player.set_editor_properties(dict(move_action=production.get_editor_property('move_action'),
    ability_input_actions=[u.UmbraTaggedInputAction(input_action=action,input_tag=tag('Input.Skill.Seven'))],
    death_animation=load('/Game/Characters/Mannequins/Anims/Death/MM_Death_Back_01')))
player.mesh.set_skeletal_mesh_asset(load('/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple'))
player.mesh.set_editor_properties(dict(relative_location=u.Vector(0,0,-90),relative_rotation=u.Rotator(pitch=0,yaw=-90,roll=0)))
player.mesh.set_anim_instance_class(anim_bp.generated_class())
finish_bp(player_bp)

state_bp,state=bp('BP_MannyLabState',u.UmbraPlayerState)
state.set_editor_properties(dict(initial_skill_slots=[u.UmbraSkillSlot(definition=definition,input_tag=tag('Input.Skill.Seven'),key_label='1')],
    initial_abilities=[],initial_inventory_items=[],use_debug_initial_attributes=True,
    debug_initial_attributes=u.UmbraDebugInitialAttributes(max_health=100.,max_resource=100.,resource_regen=10.,move_speed=450.)))
finish_bp(state_bp)
mode_bp,mode=bp('BP_MannyLabMode',u.UmbraGameMode)
mode.set_editor_properties(dict(default_pawn_class=player_bp.generated_class(),player_state_class=state_bp.generated_class(),
    player_controller_class=controller_bp.generated_class()))
finish_bp(mode_bp)
target_bp,target=bp('BP_LabTarget',u.UmbraEnemyCharacter)
target.set_editor_properties(dict(auto_possess_ai=u.AutoPossessAI.DISABLED,ai_controller_class=None,
    use_debug_initial_attributes=True,debug_initial_attributes=u.UmbraDebugInitialAttributes(max_health=10000.,health_regen=0.,move_speed=0.)))
target.mesh.set_skeletal_mesh_asset(load('/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple'))
target.mesh.set_editor_properties(dict(relative_location=u.Vector(0,0,-90),relative_rotation=u.Rotator(pitch=0,yaw=-90,roll=0)))
target.mesh.set_anim_instance_class(anim_bp.generated_class());finish_bp(target_bp)

level=u.get_editor_subsystem(u.LevelEditorSubsystem)
actors=u.get_editor_subsystem(u.EditorActorSubsystem)
editor=u.get_editor_subsystem(u.UnrealEditorSubsystem)
map_path=root+'/Maps/L_MannySkillLab'
if not assets.does_asset_exist(map_path):
    assert level.new_level(map_path)
    editor.get_editor_world().get_world_settings().set_editor_property('default_game_mode',mode_bp.generated_class())
    floor=actors.spawn_actor_from_class(u.StaticMeshActor,u.Vector(0,0,-25))
    floor.static_mesh_component.set_static_mesh(load('/Engine/BasicShapes/Cube'))
    floor.set_actor_scale3d(u.Vector(40,40,.5));floor.set_actor_label('LabFloor')
    actors.spawn_actor_from_class(u.PlayerStart,u.Vector(0,0,100),u.Rotator(pitch=0,yaw=90,roll=0))
    for i,y in enumerate([-450,0,450]):
        dummy=actors.spawn_actor_from_class(target_bp.generated_class(),u.Vector(650,y,95),u.Rotator(pitch=0,yaw=180,roll=0))
        dummy.set_actor_label('Target_'+str(i+1))
    wall=actors.spawn_actor_from_class(u.StaticMeshActor,u.Vector(100,800,100))
    wall.static_mesh_component.set_static_mesh(load('/Engine/BasicShapes/Cube'))
    wall.set_actor_scale3d(u.Vector(6,.3,2));wall.set_actor_label('ProjectileBlockWall')
    actors.spawn_actor_from_class(u.DirectionalLight,u.Vector(0,0,500),u.Rotator(pitch=-55,yaw=-30,roll=0))
    sky=actors.spawn_actor_from_class(u.SkyLight,u.Vector(0,0,300))
    sky.light_component.set_editor_property('intensity',1.5)
    assert level.save_current_level()
else:
    assert level.load_level(map_path)
# Use named rotation fields: UE Python's struct positional order is not a gameplay contract.
floor_mat=create('M_LabFloor',u.Material,u.MaterialFactoryNew())
if not assets.get_metadata_tag(floor_mat,'Umbra.Configured'):
    color=u.MaterialEditingLibrary.create_material_expression(floor_mat,u.MaterialExpressionConstant3Vector,-250,0)
    color.set_editor_property('constant',u.LinearColor(.025,.035,.045,1.))
    u.MaterialEditingLibrary.connect_material_property(color,'',u.MaterialProperty.MP_BASE_COLOR)
    u.MaterialEditingLibrary.recompile_material(floor_mat)
    assets.set_metadata_tag(floor_mat,'Umbra.Configured','1');save(floor_mat)
for actor in actors.get_all_level_actors():
    if isinstance(actor,u.PlayerStart):actor.set_actor_rotation(u.Rotator(pitch=0,yaw=0,roll=0),False)
    elif isinstance(actor,u.UmbraEnemyCharacter):actor.set_actor_rotation(u.Rotator(pitch=0,yaw=180,roll=0),False)
    elif isinstance(actor,u.DirectionalLight):actor.set_actor_rotation(u.Rotator(pitch=-55,yaw=-30,roll=0),False)
    elif actor.get_actor_label()=='LabFloor':actor.static_mesh_component.set_material(0,floor_mat)
assert u.UmbraSkillEditorLibrary.configure_lab_navigation(editor.get_editor_world()),'Navigation setup failed'
assert level.save_current_level()
(out/'playable-configuration.json').write_text(json.dumps(dict(map=map_path,pawn=player_bp.get_path_name(),
    skill=definition.get_path_name(),montage_length=montage.get_play_length(),release_seconds=.55,
    key='1',movement='WASD and left click ground navigation',production_changed=False),indent=2),encoding='utf-8')
u.log('MANNY_SKILL_LAB_CONFIGURED')
