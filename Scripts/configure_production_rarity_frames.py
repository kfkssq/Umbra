"""Repair only default grey production frames; preserve manually assigned artwork.

Run with the matching Unreal Editor Python commandlet. No gameplay tuning changes.
"""
import json
import sys
from pathlib import Path
import unreal as u

sys.path.insert(0, str(Path(__file__).parent))
from rarity_palette import NAMES, frame

root = "/Game/Items/GeneratedEquipment/V2"
grey = frame(0)
frames = {getattr(u.UmbraItemRarity, name): frame(i) for i, name in enumerate(NAMES[:4])}
report = []
for path in u.EditorAssetLibrary.list_assets(root, recursive=True):
    if "/DA_Generation_" not in path:
        continue
    profile = u.load_asset(path)
    if not isinstance(profile, u.UmbraItemGenerationProfile):
        continue
    rows = list(profile.get_editor_property("rarities"))
    changed = False
    for row in rows:
        old = row.rarity_frame_texture
        target = frames.get(row.rarity)
        if target and old == grey and old != target:
            row.set_editor_property("rarity_frame_texture", target)
            changed = True
        report.append(dict(profile=path, rarity=str(row.rarity),
                           before=old.get_path_name() if old else None,
                           after=row.rarity_frame_texture.get_path_name() if row.rarity_frame_texture else None,
                           slot_background=str(row.slot_background_texture),
                           tooltip_icon_background=str(row.tooltip_icon_background_texture),
                           tooltip_title_background=str(row.tooltip_title_background_texture),
                           tint_backgrounds=row.get_editor_property("tint_backgrounds")))
    if changed:
        profile.set_editor_property("rarities", rows)
        if not u.EditorAssetLibrary.save_loaded_asset(profile):
            raise RuntimeError("Could not save " + path)
    # Verify persisted references through the editor API.
    for row in profile.get_editor_property("rarities"):
        if row.rarity in frames and row.rarity != u.UmbraItemRarity.COMMON:
            if row.rarity_frame_texture == grey:
                raise RuntimeError("Grey frame remains: " + path)
if not report:
    raise RuntimeError("No production profiles found")
output = Path(u.Paths.project_saved_dir()) / "TooltipDefenseFix" / "frames.json"
output.parent.mkdir(parents=True, exist_ok=True)
output.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding="utf-8")
