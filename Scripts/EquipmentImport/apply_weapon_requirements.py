"""UE-only guarded migration of imported Library weapon requirements. Default read-only."""
import json
import os
from pathlib import Path
import sys
import unreal as u

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
from weapon_requirements import values, decision, apply, FIELDS, TAG, SCALING_TAG, scaling_reference

lib = u.EditorAssetLibrary
write = os.environ.get('UMBRA_REQUIREMENTS_APPLY') == '1'
result = {'apply': write, 'items': [], 'saved': []}
for path in lib.list_assets('/Game/EquipmentImport/Library', recursive=True, include_folder=False):
    if '/Drafts/DA_Source_EquipParamWeapon_' not in path: continue
    row = {'asset': path}; result['items'].append(row)
    try:
        draft = u.load_asset(path)
        assert isinstance(draft, u.UmbraItemDefinition)
        source = lib.get_metadata_tag(draft, 'UmbraItemImport.Source')
        assert source and lib.get_metadata_tag(draft, 'UmbraItemImport.Recipe'), 'Not a managed import'
        item = json.loads(source)['item']
        assert str(draft.definition_id) == 'ER_' + item['table'] + '_' + str(item['row_id'])
        expected = values(item)
        if expected is None: row['status'] = 'excluded'; continue
        current = {f: float(draft.requirements.get_editor_property(f)) for f in FIELDS}
        baseline = lib.get_metadata_tag(draft, TAG)
        row.update(before=current, requirements=expected, source_id=item['source_id'],
                   scaling_reference=scaling_reference(item),
                   ignored_source_luck=item['raw_params'].get('properLuck'),
                   status=decision(current, expected, json.loads(baseline) if baseline else None))
        if write:
            change = apply(draft, item, lib)
            if change['changed']:
                assert lib.save_loaded_asset(draft, only_if_is_dirty=False)
                result['saved'].append(path)
            assert {f: float(draft.requirements.get_editor_property(f)) for f in FIELDS} == expected
            assert json.loads(lib.get_metadata_tag(draft, SCALING_TAG)) == scaling_reference(item)
    except Exception as exc: row.update(status='conflict', error=str(exc))
    u.SystemLibrary.collect_garbage()
result['ok'] = not any(r['status'] == 'conflict' for r in result['items'])
output = HERE.parents[1] / 'Saved/EquipmentImport/WeaponRequirements'
output.mkdir(parents=True, exist_ok=True)
name = os.environ.get('UMBRA_REQUIREMENTS_REPORT', 'apply.json' if write else 'preview.json')
assert Path(name).name == name
(output / name).write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding='utf-8')
if not result['ok']: raise RuntimeError('Requirement conflicts preserved; inspect report')
