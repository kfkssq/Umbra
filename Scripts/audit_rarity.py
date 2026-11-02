"""Read existing authored rarity art/audio without changing assets."""
import json
from pathlib import Path
import unreal as u
out={"items":[],"audio":[]}
lib=u.load_asset("/Game/Asset/Audio/Libraries/DA_Audio_Default")
if lib:
    for e in lib.get_editor_property("events"):
        if "LOOT" in str(e.event):
            out["audio"].append(dict(event=str(e.event),rarity=str(e.match.rarity),has_rarity=e.match.has_rarity,
                sounds=[s.get_path_name() for s in e.sounds if s],volume=e.volume,probability=e.probability,global_cooldown=e.global_cooldown))
reg=u.AssetRegistryHelpers.get_asset_registry()
reg.search_all_assets(True)
for data in reg.get_assets_by_path("/Game/Items",recursive=True):
    if str(data.asset_class_path.asset_name)!="UmbraItemDefinition": continue
    item=data.get_asset()
    row={"path":str(data.package_name)}
    for key in ("rarity","rarity_color","slot_background_texture","rarity_frame_texture","tooltip_title_background_texture","tooltip_icon_background_texture"):
        value=item.get_editor_property(key)
        row[key]=str(value)
    out["items"].append(row)
Path(u.Paths.project_saved_dir(),"RarityAudit.json").write_text(json.dumps(out,indent=2,ensure_ascii=False),encoding="utf-8")
