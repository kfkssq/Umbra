"""UE migration/readback for the nineteen fixed-grade stones. Preserves provenance."""
import json
import os
import sys
from pathlib import Path
import unreal as u

sys.path.insert(0, str(Path(__file__).resolve().parent))
from stone_grades import SMITHING_IDS, SOMBER_IDS, presentation_fields

lib = u.EditorAssetLibrary
readback = os.environ.get('UMBRA_STONE_READBACK') == '1'
tag = 'UmbraNativeAccessories.Recipe'
root = '/Game/Items/NativeAccessoriesV1'
profile = u.load_asset(root + '/DA_Generation_Material')
assert isinstance(profile, u.UmbraItemGenerationProfile) and profile.material_rarity_only
items = [u.load_asset(root + '/Items/DA_Item_EquipParamGoods_' + str(i)) for i in SMITHING_IDS + SOMBER_IDS]
assert len(items) == 19 and all(isinstance(a, u.UmbraItemDefinition) for a in items)

def norm(value):
    if isinstance(value, (str, bool, int, float)) or value is None: return value
    if isinstance(value, u.Object): return value.get_path_name()
    if hasattr(value, 'export_text'): return value.export_text()
    return str(value)

def update(asset, fields):
    recipe = json.loads(lib.get_metadata_tag(asset, tag))
    if readback:
        for key, value in fields.items():
            assert norm(asset.get_editor_property(key)) == norm(value), asset.get_path_name() + ':' + key
            assert recipe['fields'][key] == norm(value), 'Recipe mismatch: ' + key
        return
    for key, value in fields.items(): asset.set_editor_property(key, value)
    recipe['fields'].update({k: norm(v) for k, v in fields.items()})
    lib.set_metadata_tag(asset, tag, json.dumps(recipe, ensure_ascii=False, sort_keys=True))
    assert lib.save_loaded_asset(asset, False), asset.get_path_name()

update(profile, dict(rules_version=8, rarity_from_affix_count=False))
report = []
for row_id, asset in zip(SMITHING_IDS + SOMBER_IDS, items):
    assert asset.generation_profile == profile and asset.item_category == u.UmbraItemCategory.MATERIAL
    update(asset, presentation_fields(row_id, profile))
    report.append(dict(path=asset.get_path_name(), name=str(asset.display_name), rarity=str(asset.rarity)))
out = Path(u.Paths.project_saved_dir()) / 'StoneGrades'
out.mkdir(exist_ok=True)
(out / ('readback.json' if readback else 'migration.json')).write_text(
    json.dumps(dict(ok=True, items=report), ensure_ascii=False, indent=2), encoding='utf-8')
