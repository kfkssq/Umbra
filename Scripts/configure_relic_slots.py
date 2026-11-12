"""Reorder existing UMG cells and migrate accessory slot recipes through UE only."""
import json,os
from pathlib import Path
import unreal as u

readback=os.environ.get('UMBRA_RELIC_READBACK')=='1'
lib=u.EditorAssetLibrary
menu=u.load_asset('/Game/UI/CharacterMenu/EquipmentMenu/WBP_Equipment')
widgets=u.UmbraEquipmentNetworkTestLibrary.get_blueprint_widgets(menu)
cells={w.get_name():w for w in widgets if isinstance(w,u.UmbraEquipmentSlotWidget)}
assert len(cells)==10
left=cells['Head'].get_parent();right=cells['OffHand'].get_parent()
assert isinstance(left,u.VerticalBox) and isinstance(right,u.VerticalBox) and left!=right
orders=[(left,['Head','Chest','Hands','Legs','MainHand']),(right,['Amulet','Feet','Ring1','Ring2','OffHand'])]
types={'Head':'HEAD','Chest':'CHEST','Hands':'HANDS','Legs':'LEGS','MainHand':'MAIN_HAND','Amulet':'AMULET','Feet':'AMULET2','Ring1':'RING1','Ring2':'RING2','OffHand':'OFF_HAND'}
props={name:{key:w.slot.get_editor_property(key) for key in ('padding','size','horizontal_alignment','vertical_alignment')} for name,w in cells.items()}
if not readback:
    # Reparent existing widgets, preserving names/bindings and authored spacing.
    for panel,_ in orders:panel.clear_children()
    for panel,names in orders:
        for name in names:
            slot=panel.add_child_to_vertical_box(cells[name])
            for key,value in props[name].items():slot.set_editor_property(key,value)
            # Templates have no constructed child images; do not run runtime visual events here.
            cells[name].set_editor_property('slot_type',getattr(u.UmbraEquipmentSlot,types[name]))
    u.BlueprintEditorLibrary.compile_blueprint(menu)
    assert lib.save_loaded_asset(menu,False)
widgets=u.UmbraEquipmentNetworkTestLibrary.get_blueprint_widgets(menu)
cells={w.get_name():w for w in widgets if isinstance(w,u.UmbraEquipmentSlotWidget)}
for anchor,names in [('Head',orders[0][1]),('OffHand',orders[1][1])]:
    panel=cells[anchor].get_parent()
    assert [w.get_name() for w in panel.get_all_children()]==names
    for name in names:
        w=cells[name];kind=getattr(u.UmbraEquipmentSlot,types[name])
        assert w.get_slot_type()==kind
        icon=w.get_editor_property('empty_slot_icons').get(kind)
        expected={'HEAD':'head','CHEST':'chest','HANDS':'gloves','LEGS':'pants','AMULET':'neck','AMULET2':'neck','RING1':'ring','RING2':'ring','MAIN_HAND':'melee','OFF_HAND':'shield'}[types[name]]+'_background'
        assert icon and icon.get_name()==expected,(name,str(icon))
table=u.load_asset('/Game/Items/NativeAccessoriesV1/DA_Loot_Prototype')
relic_slots=[u.UmbraEquipmentSlot.AMULET,u.UmbraEquipmentSlot.AMULET2,u.UmbraEquipmentSlot.RING1,u.UmbraEquipmentSlot.RING2]
items=[t.item_definition for c in table.categories if c.category==u.UmbraItemCategory.ACCESSORY for g in c.types for t in g.templates]
assert len(items)==116
saved=[]
for item in items:
    recipe=json.loads(lib.get_metadata_tag(item,'UmbraNativeAccessories.Recipe'))
    expected=[str(v) for v in relic_slots]
    if readback:
        assert list(item.allowed_slots)==relic_slots and recipe['fields']['allowed_slots']==expected
    elif list(item.allowed_slots)!=relic_slots or recipe['fields']['allowed_slots']!=expected:
        assert all(s in relic_slots for s in item.allowed_slots),'Unexpected hand-edited slots'
        item.set_editor_property('allowed_slots',relic_slots)
        recipe['fields']['allowed_slots']=expected
        lib.set_metadata_tag(item,'UmbraNativeAccessories.Recipe',json.dumps(recipe,ensure_ascii=False,sort_keys=True))
        assert lib.save_loaded_asset(item,False);saved.append(item.get_path_name())
    assert list(item.get_allowed_equipment_slots())==relic_slots
out=Path(u.Paths.project_saved_dir())/'RelicSlots';out.mkdir(exist_ok=True)
(out/('readback.json' if readback else 'configuration.json')).write_text(json.dumps(dict(left=orders[0][1],right=orders[1][1],relics=len(items),saved=saved,readback=readback),indent=2),encoding='utf-8')
