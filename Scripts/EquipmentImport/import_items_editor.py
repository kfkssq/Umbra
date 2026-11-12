"""UE-only bridge. Creates isolated drafts; existing managed assets are never rewritten."""
import json
import os
from pathlib import Path
import re
import sys
import unreal as u

sys.path.insert(0, str(Path(__file__).resolve().parent))
from pipeline import run as import_meshes, stable, sha
from item_catalog import resource_gate, display_name

TAG = 'UmbraItemImport.Recipe'
lib = u.EditorAssetLibrary
tools = u.AssetToolsHelpers.get_asset_tools()


def existing(path, recipe, cls):
    if not lib.does_asset_exist(path):
        return None
    obj = u.load_asset(path)
    if not isinstance(obj, cls) or lib.get_metadata_tag(obj, TAG) != recipe:
        raise ValueError('Unmanaged or changed recipe; preserve existing asset, use a new batch: ' + path)
    return obj


def save(obj, recipe, saved):
    lib.set_metadata_tag(obj, TAG, recipe)
    if not lib.save_loaded_asset(obj, only_if_is_dirty=False):
        raise ValueError('Cannot save ' + obj.get_path_name())
    saved.append(obj.get_path_name())


def enum_name(value):
    return re.sub(r'(?<!^)(?=[A-Z])', '_', value).upper()


def main():
    plan = json.loads(Path(os.environ['UMBRA_ITEM_PLAN']).read_text(encoding='utf-8'))
    folder = Path(plan['output_dir'])
    report = {'ok': False, 'saved': [], 'items': []}
    try:
        if not re.fullmatch(r'[A-Za-z][A-Za-z0-9_]*', plan['batch']):
            raise ValueError('Invalid batch')
        destination = plan.get('destination', 'Staging')
        if destination not in ('Staging', 'Library'):
            raise ValueError('Invalid destination')
        root = '/Game/EquipmentImport/' + destination + '/' + plan['batch']
        if json.loads(Path(plan['mesh_manifest']).read_text(encoding='utf-8'))['output_root'] != root:
            raise ValueError('Mesh and item destinations differ')
        for chosen in plan['items']:
            if resource_gate(chosen['item']):
                raise ValueError('Resource gate failed before UE import')
            if sha(chosen['icon_path']) != chosen['icon_sha256'] or sha(chosen['item']['models'][0]['path']) != chosen['model_sha256']:
                raise ValueError('Source hash changed')
        u.SystemLibrary.execute_console_command(None, 'Interchange.FeatureFlags.Import.FBX 0')
        meshes = import_meshes(plan['mesh_manifest'], folder / 'meshes_report.json', apply=True)
        report['mesh_saved'] = meshes['saved']
        if not meshes['ok']:
            report['mesh_failure'] = 'Some meshes failed; only successful model rows can create drafts'
        mesh_rows = {r['id']: r for r in meshes['items']}
        for chosen in plan['items']:
            item = chosen['item']
            row = {'source_id': item['source_id'], 'status': 'failed'}
            report['items'].append(row)
            try:
                if mesh_rows.get(chosen['mesh_row_id'],{}).get('status') == 'failed':
                    raise ValueError('Selected mesh failed; draft skipped')
                outputs = mesh_rows[chosen['mesh_row_id']]['outputs']
                if not outputs:
                    raise ValueError('No mesh output')
                mesh = u.load_asset(outputs[0]['asset'])
                if not isinstance(mesh, (u.StaticMesh, u.SkeletalMesh)):
                    raise ValueError('Mesh reference missing')
                material_checks = []
                for output in outputs:
                    if not isinstance(u.load_asset(output['asset']), (u.StaticMesh,u.SkeletalMesh)):
                        raise ValueError('Missing component mesh: '+output['asset'])
                for path in {p for output in outputs for p in output['slots'].values()}:
                    mi = u.load_asset(path)
                    if not isinstance(mi, u.MaterialInstanceConstant):
                        raise ValueError('Material instance missing')
                    errors = list(u.MaterialEditingLibrary.recompile_material(mi.parent))
                    if errors:
                        raise ValueError(str(errors))
                    stats = u.MaterialEditingLibrary.get_statistics(mi)
                    if stats.num_samplers <= 0:
                        raise ValueError('No compiled shader statistics')
                    material_checks.append({'asset': path, 'compile_errors': errors, 'samplers': stats.num_samplers})
                icon_path = root + '/Icons/T_Icon_' + str(chosen['icon_id'])
                icon_recipe = stable({'version': 1, 'source_sha256': chosen['icon_sha256'], 'id': chosen['icon_id'], 'preset': 'UI_RGBA'})
                icon = existing(icon_path, icon_recipe, u.Texture2D)
                if icon is None:
                    task = u.AssetImportTask()
                    task.filename = chosen['icon_path']
                    task.destination_path, task.destination_name = icon_path.rsplit('/', 1)
                    task.automated = True
                    task.replace_existing = False
                    task.save = False
                    tools.import_asset_tasks([task])
                    icon = u.load_asset(icon_path)
                    if not isinstance(icon, u.Texture2D):
                        raise ValueError('Icon import failed')
                    icon.set_editor_property('srgb', True)
                    icon.set_editor_property('compression_settings', u.TextureCompressionSettings.TC_EDITOR_ICON)
                    icon.set_editor_property('lod_group', u.TextureGroup.TEXTUREGROUP_UI)
                    icon.set_editor_property('mip_gen_settings', u.TextureMipGenSettings.TMGS_NO_MIPMAPS)
                    save(icon, icon_recipe, report['saved'])
                if not item['umbra_suggestion'] or len(outputs)!=1:
                    if not item['umbra_suggestion'] and not plan.get('allow_unmapped_asset_only'):
                        raise ValueError('Unmapped category; draft creation forbidden')
                    reasons=[]
                    if not item['umbra_suggestion']:reasons.append('Category unmapped: no ItemDefinition created')
                    if len(outputs)!=1:reasons.append('Multiple mesh components preserved: assembly required before ItemDefinition')
                    row.update(status='asset_only',icon=icon.get_path_name(),meshes=[o['asset'] for o in outputs],
                               material_checks=material_checks,source_category=item['source_category'],
                               warnings=mesh_rows[chosen['mesh_row_id']]['warnings']+reasons,
                               source_info_retained='plan.json',gameplay_ready=False)
                    if len(outputs)==1:row['mesh']=mesh.get_path_name()
                    u.SystemLibrary.collect_garbage()
                    continue
                identity = item['table'] + '_' + str(item['row_id'])
                draft_path = root + '/Drafts/DA_Source_' + identity
                recipe = stable({'version': 1, 'item': item, 'mesh': mesh.get_path_name(), 'icon_recipe': icon_recipe,
                                 'usage': chosen['usage'], 'model_sha256': chosen['model_sha256']})
                draft = existing(draft_path, recipe, u.UmbraItemDefinition)
                created = draft is None
                if created:
                    factory = u.DataAssetFactory()
                    factory.set_editor_property('data_asset_class', u.UmbraItemDefinition)
                    dest, name = draft_path.rsplit('/', 1)
                    draft = tools.create_asset(name, dest, u.UmbraItemDefinition, factory)
                    draft.set_editor_property('definition_id', 'ER_' + identity)
                    draft.set_editor_property('display_name', display_name(item['name']['value']))
                    draft.set_editor_property('flavor_text', item['description']['value'] or '')
                    draft.set_editor_property('icon', icon)
                    suggestion = item['umbra_suggestion']
                    draft.set_editor_property('item_category', getattr(u.UmbraItemCategory, enum_name(suggestion['ItemCategory'])))
                    if 'WeaponType' in suggestion:
                        draft.set_editor_property('weapon_type', getattr(u.UmbraWeaponType, enum_name(suggestion['WeaponType'])))
                    draft.set_editor_property('world_drop_skeletal_mesh' if isinstance(mesh, u.SkeletalMesh) else 'world_drop_mesh', mesh)
                    from weapon_requirements import apply as apply_requirements
                    apply_requirements(draft, item, lib)
                    # Source provenance travels with the draft; runtime combat fields stay at project defaults.
                    lib.set_metadata_tag(draft, 'UmbraItemImport.Source', stable({'item': item, 'provenance': plan['provenance'], 'source_version': plan['source_version']}))
                    lib.set_metadata_tag(draft, 'UmbraItemImport.State', 'Draft: presentation only; gameplay configuration and visual review required')
                    save(draft, recipe, report['saved'])
                if not draft.get_editor_property('icon'):
                    raise ValueError('Draft icon reference missing')
                if created:
                    assert str(draft.get_editor_property('display_name')) == display_name(item['name']['value'])
                    assert draft.get_editor_property('icon') == icon
                    assert not draft.get_editor_property('weapon')
                    assert not draft.get_editor_property('generation_profile')
                    assert not draft.get_editor_property('allowed_slots')
                row.update(status='created' if created else 'preserved', draft=draft.get_path_name(),
                           icon=icon.get_path_name(), mesh=mesh.get_path_name(), material_checks=material_checks,
                           warnings=mesh_rows[chosen['mesh_row_id']]['warnings'],
                           source_info_retained=bool(lib.get_metadata_tag(draft, 'UmbraItemImport.Source')),
                           gameplay_ready=False)
            except Exception as exc:
                row['error'] = str(exc)
            u.SystemLibrary.collect_garbage()
        report['ok'] = meshes['ok'] and all(r['status'] != 'failed' for r in report['items'])
    except Exception as exc:
        report['fatal'] = str(exc)
    finally:
        (folder / 'editor.json').write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
    if not report['ok']:
        raise RuntimeError('Item bridge incomplete; inspect editor.json')


main()
