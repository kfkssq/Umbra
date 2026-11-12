"""Apply user-selected six-color glow assets through the current Unreal Editor."""
import importlib
import json
import sys
from pathlib import Path
import unreal as u

sys.path.insert(0, str(Path(__file__).parent))
import rarity_palette
palette = importlib.reload(rarity_palette)
root = "/Game/Items/GeneratedEquipment/V2"
families = ("Armor", "Weapon", "Shield", "Ranged", "Catalyst")
styles = {getattr(u.UmbraItemRarity, name): palette.backgrounds(i)
          for i, name in enumerate(palette.NAMES)}
profiles = [u.load_asset(root + "/DA_Generation_" + family) for family in families]
assert all(isinstance(p, u.UmbraItemGenerationProfile) for p in profiles)
assert all(len(p.rarities) == 6 and {r.rarity for r in p.rarities} == set(styles) for p in profiles)
report = []
for profile in profiles:
    rows = list(profile.rarities)
    before = [r.export_text() for r in rows]
    for row in rows:
        for key, texture in styles[row.rarity].items():
            row.set_editor_property(key, texture)
        row.set_editor_property("tint_backgrounds", False)
    profile.set_editor_property("rarities", rows)
    assert u.EditorAssetLibrary.save_loaded_asset(profile), profile.get_path_name()
    for row in profile.rarities:
        assert not row.get_editor_property("tint_backgrounds")
        for key, texture in styles[row.rarity].items():
            assert row.get_editor_property(key) == texture
    report.append(dict(profile=profile.get_path_name(), before=before,
                       after=[r.export_text() for r in profile.rarities]))
out = Path(u.Paths.project_saved_dir()) / "TooltipDefenseFix" / "backgrounds.json"
out.parent.mkdir(parents=True, exist_ok=True)
out.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding="utf-8")
u.log("Umbra: saved and verified six rarity backgrounds in all five production profiles")
