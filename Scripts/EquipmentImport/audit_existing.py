"""Read-only editor audit; no save or export of original assets."""
import json
from pathlib import Path
import unreal as u

ROOT = Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
PATHS = [
    '/Game/Asset/Equipments/Head/head_01', '/Game/Asset/Equipments/Body/body',
    '/Game/Asset/Equipments/Arm/arm', '/Game/Asset/Equipments/Legs/legs',
    '/Game/Asset/Weapons/longsword/SM_longsword', '/Game/Asset/Weapons/MorningStar/MorningStar',
]

def inspect(path):
    obj = u.load_asset(path)
    row = {'path': path, 'class': obj.get_class().get_name() if obj else None}
    if not obj:
        return row
    try:
        row['sources'] = list(obj.get_editor_property('asset_import_data').extract_filenames())
        row['source_exists'] = [Path(p).is_file() for p in row['sources']]
    except Exception as exc:
        row['source_error'] = str(exc)
    if isinstance(obj, (u.StaticMesh, u.SkeletalMesh)):
        if isinstance(obj, u.SkeletalMesh):
            row['skeleton'] = str(obj.get_editor_property('skeleton').get_path_name())
        slots = obj.get_editor_property('materials' if isinstance(obj, u.SkeletalMesh) else 'static_materials')
        row['slots'] = []
        for slot in slots:
            mat = slot.material_interface
            record = {'name': str(slot.material_slot_name), 'material': mat.get_path_name() if mat else None}
            if isinstance(mat, u.Material):
                record['blend_mode'] = str(mat.get_editor_property('blend_mode'))
                record['textures'] = [inspect(t.get_path_name()) for t in u.MaterialEditingLibrary.get_used_textures(mat)]
            row['slots'].append(record)
    if isinstance(obj, u.Texture2D):
        row.update(srgb=obj.get_editor_property('srgb'), compression=str(obj.get_editor_property('compression_settings')))
    return row

report = ROOT / 'Saved/EquipmentImport/audit_existing.json'
report.parent.mkdir(parents=True, exist_ok=True)
report.write_text(json.dumps([inspect(p) for p in PATHS], ensure_ascii=False, indent=2), encoding='utf-8')
u.log('Equipment import audit: ' + str(report))
