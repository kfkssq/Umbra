"""Create missing presentation drafts for imported Library assets; never reimport meshes or edit existing drafts."""
import json
import os
from pathlib import Path
import sys

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]


def prepare(output):
    from item_catalog import build, read_json
    from dcx_run import write
    catalog, _ = build(read_json(HERE / 'item_catalog.local.example.json'))
    lookup = {r['source_id']: r for r in catalog['items']}
    jobs = {}
    for base in ('Bulk', 'Queues'):
        for path in (ROOT / 'Saved/EquipmentImport' / base).rglob('report.json'):
            source = path.parent / 'source_plan.json'
            if not source.exists(): continue
            report, plan = read_json(path), read_json(source)
            if not report.get('apply') or plan.get('destination') != 'Library': continue
            sources = {r['item']['source_id']: r for r in plan['items']}
            for row in report.get('items', []):
                if row.get('status') != 'asset_only' or not row.get('mesh'): continue
                item = lookup[row['source_id']]
                if item.get('definition_excluded'): continue
                if not item['umbra_suggestion']: raise ValueError('Still unmapped: ' + row['source_id'])
                root = '/Game/EquipmentImport/Library/' + plan['batch']
                identity = item['table'] + '_' + str(item['row_id'])
                destination = root + '/Drafts/DA_Source_' + identity
                chosen = sources[row['source_id']]
                jobs[destination] = dict(destination=destination, root=root, item=item,
                    mesh=row['mesh'], icon=row['icon'], model_sha256=chosen['model_sha256'],
                    icon_sha256=chosen['icon_sha256'], icon_id=chosen['icon_id'], usage=chosen['usage'],
                    provenance=catalog['provenance'], source_version=catalog['source_version'])
    write(output, list(jobs.values()))
    print('BACKFILL PLAN', len(jobs), str(output))


def execute(plan_path):
    import unreal as u
    from pipeline import stable
    import re
    lib = u.EditorAssetLibrary
    tools = u.AssetToolsHelpers.get_asset_tools()
    result = {'ok': False, 'items': [], 'saved': []}
    for job in json.loads(plan_path.read_text(encoding='utf-8')):
        item = job['item']; suggestion = item['umbra_suggestion']
        row = {'source_id': item['source_id'], 'draft': job['destination']}
        result['items'].append(row)
        try:
            def enum_name(value): return re.sub(r'(?<!^)(?=[A-Z])', '_', value).upper()
            category = getattr(u.UmbraItemCategory, enum_name(suggestion['ItemCategory']))
            weapon_type = getattr(u.UmbraWeaponType, enum_name(suggestion['WeaponType'])) if 'WeaponType' in suggestion else None
            mesh, icon = u.load_asset(job['mesh']), u.load_asset(job['icon'])
            assert isinstance(mesh, (u.StaticMesh, u.SkeletalMesh)) and isinstance(icon, u.Texture2D)
            assert all(p.startswith(job['root'] + '/') for p in (job['mesh'], job['icon'], job['destination']))
            icon_recipe = stable({'version': 1, 'source_sha256': job['icon_sha256'], 'id': job['icon_id'], 'preset': 'UI_RGBA'})
            recipe = stable({'version': 1, 'item': item, 'mesh': mesh.get_path_name(), 'icon_recipe': icon_recipe,
                             'usage': job['usage'], 'model_sha256': job['model_sha256']})
            if lib.does_asset_exist(job['destination']):
                draft = u.load_asset(job['destination'])
                assert isinstance(draft, u.UmbraItemDefinition)
                assert lib.get_metadata_tag(draft, 'UmbraItemImport.Recipe') == recipe, 'Existing draft differs; preserved'
                row['status'] = 'preserved'
            else:
                factory = u.DataAssetFactory(); factory.set_editor_property('data_asset_class', u.UmbraItemDefinition)
                folder, name = job['destination'].rsplit('/', 1)
                draft = tools.create_asset(name, folder, u.UmbraItemDefinition, factory)
                draft.set_editor_property('definition_id', 'ER_' + item['table'] + '_' + str(item['row_id']))
                draft.set_editor_property('display_name', item['name']['value'])
                draft.set_editor_property('flavor_text', item['description']['value'] or '')
                draft.set_editor_property('item_category', category)
                if weapon_type is not None: draft.set_editor_property('weapon_type', weapon_type)
                draft.set_editor_property('icon', icon)
                draft.set_editor_property('world_drop_skeletal_mesh' if isinstance(mesh, u.SkeletalMesh) else 'world_drop_mesh', mesh)
                from weapon_requirements import apply as apply_requirements
                apply_requirements(draft, item, lib)
                lib.set_metadata_tag(draft, 'UmbraItemImport.Source', stable({'item': item, 'provenance': job['provenance'], 'source_version': job['source_version']}))
                lib.set_metadata_tag(draft, 'UmbraItemImport.State', 'Draft: presentation only; gameplay configuration and visual review required')
                lib.set_metadata_tag(draft, 'UmbraItemImport.Recipe', recipe)
                assert lib.save_loaded_asset(draft, only_if_is_dirty=False)
                result['saved'].append(draft.get_path_name()); row['status'] = 'created'
            assert draft.item_category == category and draft.icon == icon
            if weapon_type is not None: assert draft.weapon_type == weapon_type
            assert not draft.weapon and not draft.allowed_slots and not draft.generation_profile
            row['weapon_type'] = suggestion.get('WeaponType')
        except Exception as exc: row.update(status='failed', error=str(exc))
        u.SystemLibrary.collect_garbage()
    result['ok'] = all(r['status'] != 'failed' for r in result['items'])
    destination = plan_path.with_name('result.json')
    destination.write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding='utf-8')
    if not result['ok']: raise RuntimeError('Classification backfill incomplete: ' + str(destination))


if __name__ == '__main__':
    sys.path.insert(0, str(HERE))
    if os.environ.get('UMBRA_CLASSIFICATION_PLAN'):
        execute(Path(os.environ['UMBRA_CLASSIFICATION_PLAN']))
    else:
        import argparse
        parser = argparse.ArgumentParser(description=__doc__)
        parser.add_argument('--output', required=True)
        prepare(Path(parser.parse_args().output))
