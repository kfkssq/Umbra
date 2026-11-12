"""Separate weapon breakdown typography from the larger defense rows in UE Editor."""
import json
from pathlib import Path
import unreal as u

tip = u.load_asset('/Game/UI/Tooltip/WBP_ItemTooltip')
tip_class = u.load_class(None, '/Game/UI/Tooltip/WBP_ItemTooltip.WBP_ItemTooltip_C')
cdo = u.get_default_object(tip_class)
base_class = cdo.get_editor_property('key_value_entry_class')
assert base_class
path = '/Game/UI/Tooltip/WBP_TooltipDamageEntry'
row = u.load_asset(path) if u.EditorAssetLibrary.does_asset_exist(path) else u.EditorAssetLibrary.duplicate_asset(
    base_class.get_path_name().split('.')[0], path)
assert row
widgets = u.UmbraEquipmentNetworkTestLibrary.get_blueprint_widgets(row)
texts = [w for w in widgets if isinstance(w, u.TextBlock) and w.get_name() in ('LabelText', 'ValueText')]
assert len(texts) == 2
for text in texts:
    font = text.get_editor_property('font')
    font.set_editor_property('size', 24)
    text.set_editor_property('font', font)
u.BlueprintEditorLibrary.compile_blueprint(row)
assert u.EditorAssetLibrary.save_loaded_asset(row)
damage_class = u.load_class(None, path + '.WBP_TooltipDamageEntry_C')
cdo.set_editor_property('damage_key_value_entry_class', damage_class)
u.BlueprintEditorLibrary.compile_blueprint(tip)
assert u.EditorAssetLibrary.save_loaded_asset(tip)
out = Path(u.Paths.project_saved_dir()) / 'DamageRowFont'
out.mkdir(parents=True, exist_ok=True)
(out / 'assets.json').write_text(json.dumps({'damage_row_class': damage_class.get_path_name(),
    'damage_font_size': 24, 'defense_row_class': base_class.get_path_name()}, indent=2), encoding='utf-8')
