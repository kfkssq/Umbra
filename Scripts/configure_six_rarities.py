"""Explicit presentation migration + MVP six-rarity expansion. Never edits audio or maps."""
import sys,json,hashlib
from pathlib import Path
import unreal as u
sys.path.insert(0,str(Path(__file__).parent))
from rarity_palette import NAMES,HEX,color,art,style,six_rows
root=Path(u.Paths.project_content_dir())
audio=root/"Asset/Audio/Libraries/DA_Audio_Default.uasset"
before=hashlib.sha256(audio.read_bytes()).hexdigest()
report={"colors_srgb":dict(zip(NAMES,HEX)),"saved":[],"profiles":[]}
reg=u.AssetRegistryHelpers.get_asset_registry()
reg.search_all_assets(True)
for data in reg.get_assets_by_path("/Game/Items",recursive=True):
    kind=str(data.asset_class_path.asset_name)
    if kind not in ("UmbraItemDefinition","UmbraItemGenerationProfile"): continue
    asset=data.get_asset()
    if kind=="UmbraItemGenerationProfile":
        rows=list(asset.get_editor_property("rarities"))
        if str(data.package_name)=="/Game/Items/Tests/RandomMVP/DA_Generation_MVP" and len(rows)==3:
            old={str(r.rarity):r for r in rows}
            rows=six_rows()
            for r in rows:
                if str(r.rarity) in old:
                    r.set_editor_property("weight",old[str(r.rarity)].weight)
                    r.set_editor_property("affix_count",old[str(r.rarity)].affix_count)
            asset.set_editor_property("rules_version",asset.get_editor_property("rules_version")+1)
        for row in rows:
            index=next(i for i,n in enumerate(NAMES) if row.rarity==getattr(u.UmbraItemRarity,n))
            style(row,index)
        asset.set_editor_property("rarities",rows)
        report["profiles"].append(dict(path=str(data.package_name),rarities=[str(r.rarity) for r in rows]))
    else:
        # Generated definitions get styles from profile rows, preserving pipeline-owned fields.
        if asset.get_editor_property("generation_profile"): continue
        index=next(i for i,n in enumerate(NAMES) if asset.rarity==getattr(u.UmbraItemRarity,n))
        asset.set_editor_property("rarity_color",color(index))
        asset.set_editor_property("tint_rarity_backgrounds",True)
        for key,value in art().items(): asset.set_editor_property(key,value)
    if not u.EditorAssetLibrary.save_loaded_asset(asset): raise RuntimeError("Save failed")
    report["saved"].append(str(data.package_name))
report["audio_unchanged"]=before==hashlib.sha256(audio.read_bytes()).hexdigest()
if not report["audio_unchanged"]: raise RuntimeError("Audio package changed unexpectedly")
Path(u.Paths.project_saved_dir(),"SixRarityMigration.json").write_text(json.dumps(report,indent=2),encoding="utf-8")
