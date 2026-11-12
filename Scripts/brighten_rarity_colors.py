"""Brighten Epic/Unique text colors in production profiles without changing their artwork."""
import json,sys,importlib
from pathlib import Path
import unreal as u
sys.path.insert(0,str(Path(__file__).parent))
import rarity_palette
palette=importlib.reload(rarity_palette)
report=[]
for family in ('Armor','Weapon','Shield','Ranged','Catalyst'):
    obj=u.load_asset('/Game/Items/GeneratedEquipment/V2/DA_Generation_'+family)
    assert isinstance(obj,u.UmbraItemGenerationProfile)
    rows=list(obj.rarities)
    changes=[]
    for row in rows:
        if row.rarity not in (u.UmbraItemRarity.EPIC,u.UmbraItemRarity.UNIQUE): continue
        index=3 if row.rarity==u.UmbraItemRarity.EPIC else 5
        before=row.color.export_text()
        row.set_editor_property('color',palette.color(index))
        changes.append(dict(rarity=str(row.rarity),before=before,after=row.color.export_text()))
    assert len(changes)==2
    obj.set_editor_property('rarities',rows)
    tag='UmbraRandomEquipment.Recipe'
    recipe=json.loads(u.EditorAssetLibrary.get_metadata_tag(obj,tag))
    recipe['fields']['rarities']=[r.export_text() for r in rows]
    u.EditorAssetLibrary.set_metadata_tag(obj,tag,json.dumps(recipe,sort_keys=True,ensure_ascii=False))
    assert u.EditorAssetLibrary.save_loaded_asset(obj)
    report.append(dict(path=obj.get_path_name(),changes=changes))
out=Path(u.Paths.project_saved_dir())/'RarityBrightness'
out.mkdir(parents=True,exist_ok=True)
(out/'assets.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
