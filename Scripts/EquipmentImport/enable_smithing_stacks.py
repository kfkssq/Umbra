"""Explicit UE migration of the nineteen imported stones and their importer recipes."""
import json
from pathlib import Path
import unreal as u

root='/Game/Items/NativeAccessoriesV1'
tag='UmbraNativeAccessories.Recipe'
config=json.loads((Path(__file__).parent/'native_accessories.json').read_text(encoding='utf-8'))
old='锻造材料。当前可掉落和拾取，强化与堆叠功能暂未实现；品质不改变石头种类。'
new='锻造材料。同种、同品质可堆叠；强化功能暂未实现，品质不改变石头种类。'
pending=[]
for identity in config['stone_ids']:
    item=u.load_asset(root+'/Items/DA_Item_EquipParamGoods_'+str(identity))
    assert item and item.item_category==u.UmbraItemCategory.MATERIAL and not item.equipping_enabled and not item.allowed_slots
    recipe=json.loads(u.EditorAssetLibrary.get_metadata_tag(item,tag))
    assert recipe['source']['item']['row_id']==identity
    assert str(item.flavor_text) in (old,new) and recipe['fields']['flavor_text'] in (old,new)
    pending.append((item,recipe))
saved=[]
for item,recipe in pending:
    if item.get_editor_property('stackable_material') and str(item.flavor_text)==new and recipe['fields'].get('stackable_material') is True: continue
    item.set_editor_property('stackable_material',True)
    item.set_editor_property('flavor_text',new)
    recipe['fields'].update(stackable_material=True,flavor_text=new)
    u.EditorAssetLibrary.set_metadata_tag(item,tag,json.dumps(recipe,sort_keys=True,ensure_ascii=False))
    assert u.EditorAssetLibrary.save_loaded_asset(item,False)
    saved.append(item.get_path_name())
out=Path(u.Paths.project_saved_dir())/'SmithingStacks';out.mkdir(exist_ok=True)
(out/'migration.json').write_text(json.dumps(dict(saved=saved,checked=len(pending)),indent=2),encoding='utf-8')
