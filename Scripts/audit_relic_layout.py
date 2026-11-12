import json
from pathlib import Path
import unreal as u
menu=u.load_asset('/Game/UI/CharacterMenu/EquipmentMenu/WBP_Equipment')
rows=[]
for w in u.UmbraEquipmentNetworkTestLibrary.get_blueprint_widgets(menu):
    if not isinstance(w,u.UmbraEquipmentSlotWidget):continue
    slot=w.slot
    row=dict(name=w.get_name(),type=str(w.get_slot_type()),parent=w.get_parent().get_name(),slot_class=slot.get_class().get_name(),icons={str(k):v.get_path_name() if v else None for k,v in w.get_editor_property('empty_slot_icons').items()})
    if isinstance(slot,u.CanvasPanelSlot):row.update(position=str(slot.get_position()),size=str(slot.get_size()),anchors=str(slot.get_anchors()),alignment=str(slot.get_alignment()))
    rows.append(row)
out=Path(u.Paths.project_saved_dir())/'RelicSlots';out.mkdir(exist_ok=True)
(out/'audit.json').write_text(json.dumps(rows,indent=2),encoding='utf-8')
