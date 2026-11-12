"""Create isolated FK/IK retarget assets for the first source cast candidate."""
import json
from pathlib import Path
import unreal as u

root = '/Game/Skills/Staging/GlintstonePebble'
out = Path(u.Paths.project_saved_dir()) / 'SkillImport/GlintstonePebble'
source = u.load_asset(root + '/SourceV4/SK_ER_SourceBoneProxy')
target = u.load_asset('/Game/ParagonGreystone/Characters/Heroes/Greystone/Meshes/Greystone')
animation = u.load_asset(root + '/SourceV4/AS_ER_Cast_a401_045010')
assert source and target and animation and source.skeleton and animation.get_editor_property('skeleton')
asset_tools = u.AssetToolsHelpers.get_asset_tools()
created = []

def create(name, cls, factory):
    path = root + '/RetargetV4/' + name
    if u.EditorAssetLibrary.does_asset_exist(path):
        asset = u.load_asset(path)
        assert isinstance(asset, cls)
        assert u.EditorAssetLibrary.get_metadata_tag(asset, 'UmbraSkillImport.Purpose') == 'Candidate retarget; visual validation required'
        return asset
    asset = asset_tools.create_asset(name, root + '/RetargetV4', cls, factory)
    assert asset
    created.append(asset)
    return asset

chains = [
    ('Spine', 'Spine', 'Spine2', 'spine_01', 'spine_03'),
    ('NeckHead', 'Neck', 'Head', 'neck_01', 'head'),
]
for side, suffix in [('L','l'), ('R','r')]:
    for label, a, b, c, d in [
        ('Clavicle','Clavicle','Clavicle','clavicle','clavicle'),
        ('Arm','UpperArm','Hand','upperarm','hand'),
        ('Leg','Thigh','Foot','thigh','foot'),
        ('Toe','Toe0','Toe0','ball','ball'),
    ]:
        chains.append((side+label, side+'_'+a, side+'_'+b, c+'_'+suffix, d+'_'+suffix))
    for index, finger in enumerate(['thumb','index','middle','ring','pinky']):
        chains.append((side+finger, f'{side}_Finger{index}', f'{side}_Finger{index}2',
                       f'{finger}_01_{suffix}', f'{finger}_03_{suffix}'))

source_rig = create('IK_ER_Pebble_Source', u.IKRigDefinition, u.IKRigDefinitionFactory())
target_rig = create('IK_Greystone_Pebble', u.IKRigDefinition, u.IKRigDefinitionFactory())
for rig, mesh, is_source in [(source_rig, source, True), (target_rig, target, False)]:
    controller = u.IKRigController.get_controller(rig)
    if rig not in created:
        continue
    assert controller.set_skeletal_mesh(mesh)
    assert controller.set_retarget_root('Pelvis' if is_source else 'pelvis')
    for name, start_s, end_s, start_t, end_t in chains:
        actual = controller.add_retarget_chain(name, start_s if is_source else start_t,
                                                end_s if is_source else end_t, 'None')
        assert str(actual) == name, (name, actual)

retargeter = create('RTG_ER_Pebble_Greystone', u.IKRetargeter, u.IKRetargetFactory())
controller = u.IKRetargeterController.get_controller(retargeter)
controller.set_ik_rig(u.RetargetSourceOrTarget.SOURCE, source_rig)
controller.set_ik_rig(u.RetargetSourceOrTarget.TARGET, target_rig)
controller.set_preview_mesh(u.RetargetSourceOrTarget.SOURCE, source)
controller.set_preview_mesh(u.RetargetSourceOrTarget.TARGET, target)
if retargeter in created:
    controller.add_default_ops()
    controller.auto_map_chains(u.AutoMapChainType.EXACT, True)
    controller.auto_align_all_bones(u.RetargetSourceOrTarget.TARGET, u.RetargetAutoAlignMethod.CHAIN_TO_CHAIN)
for asset in created:
    u.EditorAssetLibrary.set_metadata_tag(asset, 'UmbraSkillImport.Purpose', 'Candidate retarget; visual validation required')
    assert u.EditorAssetLibrary.save_loaded_asset(asset, False)

inputs = u.IKRetargetBatchOperationInputs()
assert not u.EditorAssetLibrary.does_asset_exist(root+'/RetargetV4/AS_Greystone_Pebble_Cast_a401_045010'), 'Output exists; use readback_pebble_retarget.py for validation'
inputs.set_editor_properties(dict(assets_to_retarget=[u.EditorAssetLibrary.find_asset_data(animation.get_path_name())],
    source_mesh=source, target_mesh=target, ik_retarget_asset=retargeter,
    search='AS_ER_', replace='AS_Greystone_Pebble_', target_path=root+'/RetargetV4',
    use_source_path=False, include_referenced_assets=False, overwrite_existing_files=False))
results = u.IKRetargetBatchOperation.run_batch_retarget(inputs)
assert len(results) == 1
result = results[0].get_asset()
assert isinstance(result, u.AnimSequence) and result.get_editor_property('skeleton') == target.skeleton
assert abs(result.get_play_length() - animation.get_play_length()) < 0.001
assert u.EditorAssetLibrary.save_loaded_asset(result, False)
(out/'ue-retarget.json').write_text(json.dumps(dict(
    result=result.get_path_name(), skeleton=result.get_editor_property('skeleton').get_path_name(), duration=result.get_play_length(),
    chains=chains, ops=controller.get_num_retarget_ops(), assets=[a.get_path_name() for a in created],
    status='retarget_exported_not_visually_validated'), indent=2), encoding='utf-8')
u.log('PEBBLE_RETARGET_EXPORTED')
