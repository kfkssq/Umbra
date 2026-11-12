"""UE migration: clean display names only, preserving original source metadata.

Set UMBRA_NAME_FIX_READBACK=1 for a fresh-process, zero-save verification.
"""
import json
import os
import sys
from pathlib import Path
import unreal as u

sys.path.insert(0, str(Path(__file__).resolve().parent))
from item_catalog import display_name

lib = u.EditorAssetLibrary
registry = u.AssetRegistryHelpers.get_asset_registry()
registry.search_all_assets(True)
readback = os.environ.get('UMBRA_NAME_FIX_READBACK') == '1'
report = {'checked': 0, 'changes': [], 'readback': readback}
pending = []
for data in registry.get_assets_by_class(u.TopLevelAssetPath('/Script/Umbra', 'UmbraItemDefinition'), True):
    asset = data.get_asset()
    report['checked'] += 1
    before = str(asset.display_name)
    if 'error' not in before.casefold():
        continue
    assert not readback, 'Uncorrected display name: ' + asset.get_path_name()
    pending.append((asset, before, display_name(before)))

for asset, before, after in pending:
    asset.set_editor_property('display_name', after)
    for tag in ('UmbraNativeAccessories.Recipe', 'UmbraRandomEquipment.Recipe'):
        raw = lib.get_metadata_tag(asset, tag)
        if raw:
            recipe = json.loads(raw)
            assert recipe['fields']['display_name'] == before
            recipe['fields']['display_name'] = after
            lib.set_metadata_tag(asset, tag, json.dumps(recipe, ensure_ascii=False, sort_keys=True))
    assert lib.save_loaded_asset(asset), asset.get_path_name()
    report['changes'].append(dict(path=asset.get_path_name(), before=before, after=after))

report['ok'] = True
out = Path(u.Paths.project_saved_dir()) / 'ItemNameFix'
out.mkdir(exist_ok=True)
(out / ('readback.json' if readback else 'apply.json')).write_text(
    json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
