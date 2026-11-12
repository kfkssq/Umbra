"""Independent readback of the saved playable lab, bindings, and production isolation."""
import json
from pathlib import Path
import unreal as u
def cdo(path):return u.get_default_object(u.load_asset(path).generated_class())
root='/Game/SkillLab'
player=cdo(root+'/BP_MannyLab');mode=cdo(root+'/BP_MannyLabMode');state=cdo(root+'/BP_MannyLabState')
assert mode.get_editor_property('default_pawn_class')==player.get_class()
assert mode.get_editor_property('player_state_class')==state.get_class()
mesh=player.mesh.get_skeletal_mesh_asset()
assert mesh.get_name()=='SKM_Manny_Simple'
rotation=player.mesh.get_editor_property('relative_rotation')
assert abs(rotation.pitch)<.01 and abs(rotation.roll)<.01 and abs(rotation.yaw+90)<.01
assert player.mesh.get_editor_property('anim_class')==u.load_asset(root+'/Animation/ABP_MannyLab').generated_class()
slots=state.get_editor_property('initial_skill_slots');assert len(slots)==1
definition=slots[0].definition
assert definition.get_name()=='DA_MannyArcBolt'
montage=definition.get_editor_property('montage')
assert montage.get_editor_property('skeleton')==mesh.skeleton
assert 0<definition.get_editor_property('strike_time')<montage.get_play_length()
assert player.mesh.does_socket_exist(definition.get_editor_property('cast_socket'))
for key in ['active_niagara','strike_niagara','projectile_niagara','impact_niagara']:
    assert isinstance(definition.get_editor_property(key),u.NiagaraSystem)
assert not state.get_editor_property('initial_abilities')
controller=cdo(root+'/BP_MannyLabController')
assert mode.get_editor_property('player_controller_class')==controller.get_class()
contexts=controller.get_editor_property('default_mapping_contexts')
mappings=[]
for context in contexts:
    for m in context.get_editor_property('default_key_mappings').get_editor_property('mappings'):
        action=m.get_editor_property('action')
        if action:mappings.append(dict(context=context.get_path_name(),action=action.get_path_name(),key=str(m.get_editor_property('key').get_editor_property('key_name'))))
move=player.get_editor_property('move_action').get_path_name()
assert {'W','A','S','D'}.issubset({m['key'] for m in mappings if m['action']==move})
assert any(m['key']=='One' and m['action'].endswith('IA_MannySkill1.IA_MannySkill1') for m in mappings)
assert not any(m['key']=='Five' and 'IA_MannySkill1' in m['action'] for m in mappings)
assert definition.get_editor_property('icon')==u.load_asset('/Game/Skills/GlintstonePebble/T_GlintstonePebble_Icon')
primary=controller.get_editor_property('primary_action').get_path_name()
assert any(m['key']=='LeftMouseButton' and m['action']==primary for m in mappings)
level=u.get_editor_subsystem(u.LevelEditorSubsystem)
assert level.load_level(root+'/Maps/L_MannySkillLab')
world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
assert world.get_world_settings().get_editor_property('default_game_mode')==mode.get_class()
assert len(u.GameplayStatics.get_all_actors_of_class(world,u.PlayerStart))==1
assert len(u.GameplayStatics.get_all_actors_of_class(world,u.UmbraEnemyCharacter))==3
bounds=u.GameplayStatics.get_all_actors_of_class(world,u.NavMeshBoundsVolume)
nav_data=u.GameplayStatics.get_all_actors_of_class(world,u.RecastNavMesh)
assert len(bounds)==1 and len(nav_data)==1,'Missing lab navigation assets'
extent=bounds[0].get_actor_bounds(False)[1]
assert extent.x>=1800 and extent.y>=1800 and extent.z>=250,'Navigation bounds are empty or too small'
assert nav_data[0].get_editor_property('runtime_generation')==u.RuntimeGenerationType.DYNAMIC
production=cdo('/Game/Blueprints/Player/BP_UmbraPlayerCharacter')
assert production.mesh.get_skeletal_mesh_asset().get_path_name().startswith('/Game/ParagonGreystone/')
original=u.load_asset('/Game/Skills/GlintstonePebble/DA_GlintstonePebble')
assert not any(original.get_editor_property(k) for k in ['active_niagara','strike_niagara','projectile_niagara','impact_niagara'])
production_imc=u.load_asset('/Game/Input/IMC_Default')
assert any(str(m.get_editor_property('key').get_editor_property('key_name'))=='Five'
    and m.get_editor_property('action')==u.load_asset('/Game/Input/IA_GlintstonePebble')
    for m in production_imc.get_editor_property('default_key_mappings').get_editor_property('mappings'))
report=dict(result='passed',movement_keys=[m for m in mappings if m['action']==move],skill_key='1',slot=0,
    pawn=player.get_class().get_path_name(),montage=montage.get_path_name(),map=root+'/Maps/L_MannySkillLab',
    icon=definition.get_editor_property('icon').get_path_name(),mouse_key='LeftMouseButton',dynamic_navigation=True,
    mesh_rotation=str(rotation),target_count=3,production_mesh=production.mesh.get_skeletal_mesh_asset().get_path_name())
(Path(u.Paths.project_saved_dir())/'SkillImport/NativeBaseline/manny-audit.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
u.log('MANNY_LAB_AUDIT_PASSED')
