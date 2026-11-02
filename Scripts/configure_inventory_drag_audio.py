"""Run with UE 5.8 Python: fill missing generic drag feedback from existing project sounds."""
import json
import pathlib
import unreal

library = unreal.load_asset('/Game/Asset/Audio/Libraries/DA_Audio_Default')
assert library
rows = list(library.get_editor_property('events'))
changed = False
report = {}
for event, name in (
    (unreal.UmbraAudioEvent.INVENTORY_TRANSFER, 'put_item'),
    (unreal.UmbraAudioEvent.EQUIP_SUCCESS, 'equip'),
    (unreal.UmbraAudioEvent.UNEQUIP_SUCCESS, 'put_item'),
    (unreal.UmbraAudioEvent.EQUIP_FAILED, 'UIFailed'),
):
    sound = unreal.load_asset('/Game/Asset/Audio/Asset/UI/' + name)
    assert isinstance(sound, unreal.SoundBase), name
    generic = []
    for row in rows:
        context = row.get_editor_property('match')
        if (row.get_editor_property('event') == event
                and context.get_editor_property('weapon') == unreal.UmbraWeaponType.UNKNOWN
                and not context.get_editor_property('has_rarity')
                and str(context.get_editor_property('character_type')) == 'None'
                and context.get_editor_property('surface') == unreal.PhysicalSurface.SURFACE_TYPE_DEFAULT):
            generic.append(row)
    assert len(generic) <= 1, 'Ambiguous generic audio rows: ' + str(event)
    if generic:
        row = generic[0]
    else:
        row = unreal.UmbraAudioEntry()
        row.set_editor_property('event', event)
        row.set_editor_property('category', unreal.UmbraAudioCategory.EQUIPMENT)
        rows.append(row)
    if not row.get_editor_property('sounds'):
        row.set_editor_property('sounds', [sound])
        row.set_editor_property('spatial', False)
        row.set_editor_property('volume', 0.35 if event == unreal.UmbraAudioEvent.INVENTORY_TRANSFER else 0.7)
        changed = True
    report[str(event)] = [s.get_path_name() for s in row.get_editor_property('sounds') if s]
if changed:
    library.set_editor_property('events', rows)
    assert unreal.EditorAssetLibrary.save_loaded_asset(library)
report['changed'] = changed
path = pathlib.Path(unreal.Paths.project_saved_dir()) / 'InventoryDragAudio.json'
path.write_text(json.dumps(report, indent=2), encoding='utf-8')
unreal.log('INVENTORY_DRAG_AUDIO ' + json.dumps(report))
