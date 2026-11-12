"""Set Legendary and Unique to the same authored red frame in UE Editor."""
import json
from pathlib import Path
import unreal as u
lib=u.EditorAssetLibrary
texture=u.load_asset('/Game/Asset/classic-rpg-ui/Assets/ClassicRPGUI/ClassicUI/SpecialFrames/Frame_red')
assert isinstance(texture,u.Texture2D)
report=[]
for family in ('Armor','Weapon','Shield','Ranged','Catalyst'):
    obj=u.load_asset('/Game/Items/GeneratedEquipment/V2/DA_Generation_'+family)
    assert isinstance(obj,u.UmbraItemGenerationProfile)
    rows=list(obj.rarities)
    changes=[]
    for row in rows:
        if row.rarity not in (u.UmbraItemRarity.LEGENDARY,u.UmbraItemRarity.UNIQUE): continue
        changes.append(dict(rarity=str(row.rarity),before=str(row.rarity_frame_texture),after=texture.get_path_name()))
        row.set_editor_property('rarity_frame_texture',texture)
    assert len(changes)==2
    obj.set_editor_property('rarities',rows)
    tag='UmbraRandomEquipment.Recipe'
    recipe=json.loads(lib.get_metadata_tag(obj,tag))
    recipe['fields']['rarities']=[r.export_text() for r in rows]
    lib.set_metadata_tag(obj,tag,json.dumps(recipe,sort_keys=True,ensure_ascii=False))
    assert lib.save_loaded_asset(obj)
    report.append(dict(path=obj.get_path_name(),changes=changes))
out=Path(u.Paths.project_saved_dir())/'HighRarityFrames'
out.mkdir(parents=True,exist_ok=True)
(out/'assets.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
