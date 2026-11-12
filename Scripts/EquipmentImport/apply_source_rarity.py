"""Migrate only the production V2 source-rarity rules through UE Editor."""
import json
from pathlib import Path
import unreal as u

lib=u.EditorAssetLibrary
root='/Game/Items/GeneratedEquipment/V2'
tag='UmbraRandomEquipment.Recipe'
config=json.loads((Path(__file__).parent/'random_equipment_v2.json').read_text(encoding='utf-8'))
report={'items':[],'profiles':[],'saved':[]}
prepared=[]
for path in sorted(lib.list_assets(root,recursive=True,include_folder=False)):
    obj=u.load_asset(path)
    text=lib.get_metadata_tag(obj,tag)
    if not text: continue
    recipe=json.loads(text)
    source_rank=None
    if '/Items/DA_Item_' in path:
        draft=u.load_asset(recipe['source'])
        source=json.loads(lib.get_metadata_tag(draft,'UmbraItemImport.Source'))['item']
        source_rank=int(source['raw_params']['rarity'])
        assert source_rank in (0,1,2,3),path
        assert str(obj.definition_id)==config['id_prefix']+'ER_'+source['table']+'_'+str(source['row_id']),path
        report['items'].append({'path':obj.get_path_name(),'name':str(obj.display_name),'source_rarity':source_rank})
    prepared.append((obj,recipe,source_rank))
assert len(report['items'])==580
assert sum(isinstance(o,u.UmbraItemGenerationProfile) for o,_,_ in prepared)==5
for obj,recipe,rank in prepared:
    before=json.dumps(recipe,sort_keys=True,ensure_ascii=False)
    if rank is not None:
        obj.set_editor_property('source_rarity',rank)
        recipe['fields']['source_rarity']=rank
        recipe['fields'].pop('required_level',None) # Legacy field no longer exposed; requirement is ItemLevel.
    if isinstance(obj,u.UmbraItemGenerationProfile):
        rows=list(obj.rarities)
        for row in rows:
            if row.rarity in (u.UmbraItemRarity.LEGENDARY,u.UmbraItemRarity.UNIQUE):
                index=0 if row.rarity==u.UmbraItemRarity.LEGENDARY else 1
                row.set_editor_property('weight',config['high_rarity_weights'][index])
                row.set_editor_property('affix_count',config['high_rarity_affix_count'])
        obj.set_editor_property('rarities',rows)
        obj.set_editor_property('rules_version',config['rules_version'])
        obj.set_editor_property('source_two_legendary_chance',config['source_two_legendary_chance'])
        recipe['fields']['source_two_legendary_chance']=config['source_two_legendary_chance']
        recipe['fields']['rarities']=[r.export_text() for r in rows]
        recipe['fields']['rules_version']=config['rules_version']
        report['profiles'].append(obj.get_path_name())
    # Retain the already approved physical-affix source correction in its managed recipe.
    if obj.get_name() in ('DA_Affix_SlashingA','DA_Affix_BluntA','DA_Affix_PiercingA'):
        assert all(t.values.damage_bonus.attack_source==u.UmbraBonusAttackSource.ANY for t in obj.tiers)
        recipe['fields']['tiers']=[t.export_text() for t in obj.tiers]
    recipe['version']=config['rules_version']
    after=json.dumps(recipe,sort_keys=True,ensure_ascii=False)
    if before!=after:
        lib.set_metadata_tag(obj,tag,after)
        assert lib.save_loaded_asset(obj),obj.get_path_name()
        report['saved'].append(obj.get_path_name())
out=Path(u.Paths.project_saved_dir())/'SourceRarityPromotion'
out.mkdir(parents=True,exist_ok=True)
(out/'assets.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
