"""Independent editor readback and review scene. No production character changes."""
import json
from pathlib import Path
import unreal as u
out=Path(u.Paths.project_saved_dir()).resolve()/'SkillImport/NativeBaseline'
out.mkdir(parents=True,exist_ok=True)
registry=u.AssetRegistryHelpers.get_asset_registry();registry.scan_paths_synchronous(['/Game/Characters','/Game/SkillLab'],True)
inventory=[]
for data in registry.get_assets_by_path('/Game/Characters',recursive=True):
    inventory.append(dict(path=str(data.package_name),type=str(data.asset_class_path.asset_name)))
report={'inventory':inventory,'characters':[],'production_character_changed':False}
meshes=[]
for name in ['Manny','Quinn']:
    mesh=u.load_asset('/Game/Characters/Mannequins/Meshes/SKM_'+name+'_Simple')
    assert isinstance(mesh,u.SkeletalMesh) and mesh.skeleton
    component=u.SkeletalMeshComponent();component.set_skeletal_mesh_asset(mesh)
    required=['root','pelvis','spine_01','neck_01','head','hand_l','hand_r','foot_l','foot_r']
    assert all(component.get_bone_index(b)>=0 for b in required)
    meshes.append(mesh)
    report['characters'].append(dict(mesh=mesh.get_path_name(),skeleton=mesh.skeleton.get_path_name(),
        bone_count=component.get_num_bones(),required_bones=required))
assert meshes[0].skeleton==meshes[1].skeleton
report['same_skeleton']=True
definition=u.load_asset('/Game/Skills/GlintstonePebble/DA_GlintstonePebble')
report['production_pebble_niagara']={key:bool(definition.get_editor_property(key)) for key in
    ['active_niagara','strike_niagara','projectile_niagara','impact_niagara']}
assert not any(report['production_pebble_niagara'].values()),'Smoke fixture was persisted into production'
player_bp=u.load_asset('/Game/Blueprints/Player/BP_UmbraPlayerCharacter')
player=u.get_default_object(player_bp.generated_class())
report['production_mesh']=player.mesh.get_skeletal_mesh_asset().get_path_name()
assert report['production_mesh'].startswith('/Game/ParagonGreystone/'),'Unexpected production character replacement'
for name in ['NS_TestFlight','NS_TestBurst']:
    assert isinstance(u.load_asset('/Game/SkillLab/Fixtures/'+name),u.NiagaraSystem)
map_path='/Game/SkillLab/Maps/L_CharacterBaseline'
level=u.get_editor_subsystem(u.LevelEditorSubsystem)
actors=u.get_editor_subsystem(u.EditorActorSubsystem)
if not u.EditorAssetLibrary.does_asset_exist(map_path):
    assert level.new_level(map_path)
    for i,mesh in enumerate(meshes):
        actor=actors.spawn_actor_from_class(u.SkeletalMeshActor,u.Vector(0,i*200,0))
        actor.set_actor_label('Baseline_'+['Manny','Quinn'][i])
        actor.skeletal_mesh_component.set_skeletal_mesh_asset(mesh)
    floor=actors.spawn_actor_from_class(u.StaticMeshActor,u.Vector(0,100,-10))
    floor.static_mesh_component.set_static_mesh(u.load_asset('/Engine/BasicShapes/Cube'))
    floor.set_actor_scale3d(u.Vector(8,8,.2));floor.set_actor_label('ReviewFloor')
    light=actors.spawn_actor_from_class(u.DirectionalLight,u.Vector(0,0,400),u.Rotator(-45,-30,0))
    light.set_actor_label('ReviewLight')
    assert level.save_current_level()
report['review_map']=map_path
report['status']='structural_readback_passed_visual_acceptance_pending'
(out/'audit.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
u.log('NATIVE_BASELINE_AUDIT_PASSED')
