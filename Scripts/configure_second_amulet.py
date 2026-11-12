"""Migrate the retired shoe widget to Amulet2 through the matching UE Editor."""
import json
from pathlib import Path
import unreal as u

lib = u.EditorAssetLibrary
base = '/Game/UI/CharacterMenu/EquipmentMenu/'
menu = u.load_asset(base + 'WBP_Equipment')
slot_bp = u.load_asset(base + 'WBP_EquipmentSlot')
slot_class = u.load_class(None, base + 'WBP_EquipmentSlot.WBP_EquipmentSlot_C')
assert menu and slot_bp and slot_class
def get_slots():
    return [w for w in u.UmbraEquipmentNetworkTestLibrary.get_blueprint_widgets(menu)
            if isinstance(w, u.UmbraEquipmentSlotWidget)]
slots = get_slots()
assert len(slots) == 10
cdo = u.get_default_object(slot_class)
assert u.UmbraEquipmentNetworkTestLibrary.migrate_retired_feet_widget(cdo)
icons = dict(cdo.get_editor_property('empty_slot_icons'))
assert icons.get(u.UmbraEquipmentSlot.AMULET), 'Existing amulet icon required'
u.BlueprintEditorLibrary.compile_blueprint(slot_bp)
assert lib.save_loaded_asset(slot_bp)
for w in get_slots():
    assert u.UmbraEquipmentNetworkTestLibrary.migrate_retired_feet_widget(w)
assert len(set(str(w.get_slot_type()) for w in get_slots())) == 10
u.BlueprintEditorLibrary.compile_blueprint(menu)
assert lib.save_loaded_asset(menu)
types = [str(w.get_slot_type()) for w in get_slots()]
assert len(types) == len(set(types)) == 10
assert sum(w.get_slot_type() == u.UmbraEquipmentSlot.AMULET2 for w in get_slots()) == 1
out = Path(u.Paths.project_saved_dir()) / 'SecondAmulet'
out.mkdir(parents=True, exist_ok=True)
(out / 'assets.json').write_text(json.dumps({'slots': types, 'icon': icons[u.UmbraEquipmentSlot.AMULET].get_path_name()}, indent=2), encoding='utf-8')
