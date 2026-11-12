"""Read-only UE audit/export for Greystone UI selection and animation planning."""
import json
from pathlib import Path
import unreal as u

out = Path(u.Paths.project_saved_dir()) / 'GreystoneDirection'
out.mkdir(parents=True, exist_ok=True)
report = {'icons': [], 'animations': [], 'widgets': {}, 'slots': []}
for path in u.EditorAssetLibrary.list_assets('/Game/Asset/Icons', recursive=True):
    obj = u.load_asset(path)
    if not isinstance(obj, u.Texture2D):
        continue
    relative = path.split('/Icons/')[1].split('.')[0]
    destination = out / 'Icons' / (relative + '.png')
    destination.parent.mkdir(parents=True, exist_ok=True)
    task = u.AssetExportTask()
    task.set_editor_properties(dict(object=obj, filename=str(destination), automated=True, prompt=False))
    task.exporter = u.TextureExporterPNG()
    assert u.Exporter.run_asset_export_task(task), path
    report['icons'].append({'path': path, 'png': str(destination)})
for path in u.EditorAssetLibrary.list_assets('/Game/ParagonGreystone/Characters/Heroes/Greystone/Animations', recursive=True):
    obj = u.load_asset(path)
    row = {'path': path, 'class': obj.get_class().get_name()}
    if isinstance(obj, u.AnimSequenceBase):
        row['seconds'] = obj.get_play_length()
        row['skeleton'] = obj.get_editor_property('skeleton').get_path_name()
        row['notifies'] = [str(n) for n in u.AnimationLibrary.get_animation_notify_events(obj)]
    if isinstance(obj, u.AnimSequence):
        row['root_motion'] = obj.get_editor_property('enable_root_motion')
        row['additive'] = str(obj.get_editor_property('additive_anim_type'))
        poses = [u.AnimationLibrary.get_bone_pose_for_time(obj, 'root', row['seconds'] * i / 20, False).translation for i in range(21)]
        row['root_range_cm'] = {axis: [min(getattr(p, axis) for p in poses), max(getattr(p, axis) for p in poses)] for axis in ['x', 'y', 'z']}
    if isinstance(obj, u.AnimMontage):
        row['tracks'] = [str(t) for t in obj.get_editor_property('slot_anim_tracks')]
    report['animations'].append(row)
state = u.get_default_object(u.load_asset('/Game/Blueprints/Player/BP_UmbraPlayerState').generated_class())
for slot in state.get_editor_property('initial_skill_slots'):
    definition = slot.definition
    report['slots'].append({'definition': definition.get_path_name() if definition else None,
        'key': str(slot.key_label), 'tag': str(slot.input_tag),
        'id': str(definition.get_editor_property('skill_id')) if definition else None,
        'icon': str(definition.get_editor_property('icon')) if definition else None})
for path in u.EditorAssetLibrary.list_assets('/Game/UI', recursive=True):
    if not any(word in path for word in ['Combat', 'SkillBar', 'StatEntry', 'PlayerHUD']):
        continue
    bp = u.load_asset(path)
    if not isinstance(bp, u.WidgetBlueprint):
        continue
    rows = []
    for widget in u.UmbraEquipmentNetworkTestLibrary.get_blueprint_widgets(bp):
        row = {'name': widget.get_name(), 'class': widget.get_class().get_name()}
        for prop in ['stat', 'attribute', 'slot_index', 'preview_icon', 'preview_key_label']:
            try: row[prop] = str(widget.get_editor_property(prop))
            except Exception: pass
        if isinstance(widget, u.Image):
            row['image'] = str(widget.get_editor_property('brush').get_editor_property('resource_object'))
        rows.append(row)
    report['widgets'][path] = rows
(out / 'audit.json').write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
u.log('GREYSTONE_DIRECTION_AUDIT_PASSED')
