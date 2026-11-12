"""UE editor-only import of a diagnostic source rig and one candidate cast clip.

Creates new Staging assets only; existing assets must have matching source hashes.
Does not assign skills, change characters, or overwrite third-party assets.
"""
import hashlib
import json
from pathlib import Path
import unreal as u

out = Path(u.Paths.project_saved_dir()) / 'SkillImport/GlintstonePebble'
source = out / 'Converted'
destination = '/Game/Skills/Staging/GlintstonePebble/SourceV4'
tools = u.AssetToolsHelpers.get_asset_tools()
report = {'engine': u.SystemLibrary.get_engine_version(), 'assets': []}

def import_asset(filename, name, skeleton=None):
    path = destination + '/' + name
    digest = hashlib.sha256(filename.read_bytes()).hexdigest()
    repair_missing_skeleton = False
    if u.EditorAssetLibrary.does_asset_exist(path):
        asset = u.load_asset(path)
        assert u.EditorAssetLibrary.get_metadata_tag(asset, 'UmbraSkillImport.SourceHash') == digest, path
        repair_missing_skeleton = isinstance(asset, (u.SkeletalMesh, u.AnimSequence)) and not asset.get_editor_property('skeleton')
        if not repair_missing_skeleton:
            return asset
    settings = u.FbxImportUI()
    settings.set_editor_properties(dict(automated_import_should_detect_type=False,
        import_as_skeletal=True, import_mesh=skeleton is None, import_animations=skeleton is not None,
        import_materials=False, import_textures=False, create_physics_asset=False,
        mesh_type_to_import=u.FBXImportType.FBXIT_ANIMATION if skeleton else u.FBXImportType.FBXIT_SKELETAL_MESH))
    if skeleton:
        settings.set_editor_property('skeleton', skeleton)
        settings.anim_sequence_import_data.set_editor_property('animation_length', u.FBXAnimationLengthImportType.FBXALIT_EXPORTED_TIME)
        settings.anim_sequence_import_data.set_editor_property('import_uniform_scale', 1.0)
    else:
        settings.skeletal_mesh_import_data.set_editor_property('use_t0_as_ref_pose', True)
        settings.skeletal_mesh_import_data.set_editor_property('import_uniform_scale', 1.0)
    task = u.AssetImportTask()
    task.set_editor_properties(dict(filename=str(filename), destination_path=destination,
        destination_name=name, automated=True, replace_existing=repair_missing_skeleton, save=False,
        factory=u.FbxFactory(), options=settings))
    tools.import_asset_tasks([task])
    objects = list(task.get_objects())
    expected = u.AnimSequence if skeleton else u.SkeletalMesh
    asset = next((a for a in objects if isinstance(a, expected)), None)
    assert asset, [(a.get_path_name(), a.get_class().get_name()) for a in objects]
    if isinstance(asset, u.SkeletalMesh):
        assert asset.skeleton
        assert u.EditorAssetLibrary.save_loaded_asset(asset.skeleton, False)
    else:
        assert asset.get_editor_property('skeleton') == skeleton, 'Importer did not use the supplied Skeleton'
    u.EditorAssetLibrary.set_metadata_tag(asset, 'UmbraSkillImport.SourceHash', digest)
    u.EditorAssetLibrary.set_metadata_tag(asset, 'UmbraSkillImport.Purpose', 'Source-rig staging; not gameplay-ready')
    for obj in objects:
        assert u.EditorAssetLibrary.save_loaded_asset(obj, False), obj.get_path_name()
    report['assets'].append({'path': asset.get_path_name(), 'source': str(filename), 'sha256': digest})
    return asset

mesh = import_asset(source / 'ER_SourceBoneProxyRest.fbx', 'SK_ER_SourceBoneProxy')
assert isinstance(mesh, u.SkeletalMesh)
animation = import_asset(source / 'a401_045010.fbx', 'AS_ER_Cast_a401_045010', mesh.skeleton)
assert abs(animation.get_play_length() - 1.5) < 1 / 30
target = u.load_asset('/Game/ParagonGreystone/Characters/Heroes/Greystone/Meshes/Greystone')
for label, asset in [('source', mesh), ('target', target)]:
    component = u.SkeletalMeshComponent()
    component.set_skeletal_mesh_asset(asset)
    report[label] = dict(mesh=asset.get_path_name(), skeleton=asset.skeleton.get_path_name(),
                         bones=[str(component.get_bone_name(i)) for i in range(component.get_num_bones())])
report['animation'] = dict(path=animation.get_path_name(), duration=animation.get_play_length())
report['api'] = {name: getattr(u, name).__doc__ for name in ['IKRetargetBatchOperationInputs']}
(out / 'ue-source-import.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
u.log('PEBBLE_SOURCE_IMPORT_PASSED')
