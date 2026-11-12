"""UE-only, idempotent import. Creates isolated items, icons, profiles and a mixed loot table.
UMBRA_NATIVE_MODE=readback never saves. UMBRA_NATIVE_LIMIT supports an initial smoke batch.
"""
import hashlib
import json
import os
import sys
from pathlib import Path
import unreal as u

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
from item_catalog import display_name
from stone_grades import presentation_fields, DESCRIPTION
OUT = Path(u.Paths.project_saved_dir())/'NativeAccessories'
PLAN = json.loads((OUT/'plan.json').read_text(encoding='utf-8'))
CONFIG = PLAN['config']
ROOT = CONFIG['destination']
READBACK = os.environ.get('UMBRA_NATIVE_MODE') == 'readback'
LIMIT = int(os.environ.get('UMBRA_NATIVE_LIMIT','0'))
LIB = u.EditorAssetLibrary
TOOLS = u.AssetToolsHelpers.get_asset_tools()
TAG = 'UmbraNativeAccessories.Recipe'
REPORT = {'ok':False,'mode':'readback' if READBACK else 'apply','saved':[],'items':[]}

def norm(value):
    if value is None or isinstance(value,(str,bool,int)): return value
    if isinstance(value,float): return round(value,6)
    if isinstance(value,u.Object): return value.get_path_name()
    if hasattr(value,'export_text'): return value.export_text()
    if isinstance(value,(list,tuple,u.Array)): return [norm(v) for v in value]
    return str(value)

def put(obj, **fields):
    for key,value in fields.items(): obj.set_editor_property(key,value)
    return obj

def save(obj):
    if not LIB.save_loaded_asset(obj,only_if_is_dirty=False): raise RuntimeError('Save failed: '+obj.get_path_name())
    REPORT['saved'].append(obj.get_path_name())

def make(name, cls, fields, source=None):
    path=ROOT+'/'+name
    recipe=json.dumps({'version':1,'source':source,'fields':{k:norm(v) for k,v in fields.items()}},sort_keys=True,ensure_ascii=False)
    if LIB.does_asset_exist(path):
        obj=u.load_asset(path)
        if not isinstance(obj,cls) or LIB.get_metadata_tag(obj,TAG)!=recipe: raise ValueError('Recipe conflict: '+path)
        for key,value in fields.items():
            if norm(obj.get_editor_property(key))!=norm(value): raise ValueError('Hand edited output preserved: '+path+':'+key)
        return obj
    if READBACK: raise ValueError('Missing saved asset: '+path)
    factory=put(u.DataAssetFactory(),data_asset_class=cls)
    folder,leaf=path.rsplit('/',1)
    obj=TOOLS.create_asset(leaf,folder,cls,factory)
    assert obj
    put(obj,**fields)
    LIB.set_metadata_tag(obj,TAG,recipe)
    if source:
        LIB.set_metadata_tag(obj,'UmbraItemImport.Source',json.dumps(source,ensure_ascii=False,sort_keys=True))
    save(obj)
    return obj

def icon(row):
    path=ROOT+'/Icons/T_Icon_'+str(row['icon_id'])
    recipe=json.dumps({'sha256':row['icon_sha256'],'preset':'UI_RGBA'},sort_keys=True)
    if not LIB.does_asset_exist(path):
        if READBACK: raise ValueError('Missing icon '+path)
        task=put(u.AssetImportTask(),filename=row['icon_path'],destination_path=ROOT+'/Icons',
                 destination_name='T_Icon_'+str(row['icon_id']),automated=True,replace_existing=False,save=False)
        TOOLS.import_asset_tasks([task])
        texture=u.load_asset(path)
        assert isinstance(texture,u.Texture2D)
        put(texture,srgb=True,compression_settings=u.TextureCompressionSettings.TC_EDITOR_ICON,
            lod_group=u.TextureGroup.TEXTUREGROUP_UI,mip_gen_settings=u.TextureMipGenSettings.TMGS_NO_MIPMAPS)
        LIB.set_metadata_tag(texture,TAG,recipe)
        save(texture)
    texture=u.load_asset(path)
    assert isinstance(texture,u.Texture2D) and LIB.get_metadata_tag(texture,TAG)==recipe
    assert texture.get_editor_property('srgb') and texture.get_editor_property('compression_settings')==u.TextureCompressionSettings.TC_EDITOR_ICON
    assert texture.get_editor_property('lod_group')==u.TextureGroup.TEXTUREGROUP_UI
    assert texture.get_editor_property('mip_gen_settings')==u.TextureMipGenSettings.TMGS_NO_MIPMAPS
    return texture

def main():
    for path,sha in PLAN['inputs'].items():
        assert hashlib.sha256(Path(path).read_bytes()).hexdigest()==sha, 'Source changed: '+path
    for row in PLAN['items']:
        assert hashlib.sha256(Path(row['icon_path']).read_bytes()).hexdigest()==row['icon_sha256']
    # Clone only eligible common/defense gradients; never edit shared V2 affixes.
    old_profile=u.load_asset('/Game/Items/GeneratedEquipment/V2/DA_Generation_Armor')
    assert isinstance(old_profile,u.UmbraItemGenerationProfile)
    entries=[]
    for entry in old_profile.pool.entries:
        affix=entry.affix
        fields={k:affix.get_editor_property(k) for k in ('affix_id','exclusive_group','rarities','maximum_level','tiers','required_equipment_tags','blocked_equipment_tags','required_source_tags')}
        fields.update(categories=[u.UmbraItemCategory.ACCESSORY],weapon_types=[],slots=[])
        clone=make('Affixes/'+affix.get_name(),u.UmbraAffixDefinition,fields,{'gradient':affix.get_path_name()})
        entries.append(put(u.UmbraAffixPoolEntry(),affix=clone,weight=entry.weight))
    assert len(entries)>=4
    pool=make('DA_Pool_Amulet',u.UmbraAffixPool,dict(entries=entries))
    profiles={}
    for category in ('Accessory','Material'):
        fields={k:old_profile.get_editor_property(k) for k in ('rarity_from_affix_count','affix_count_weights','source_two_legendary_chance','minimum_level','maximum_level','level_offset','rarities')}
        fields.update(rules_version=8 if category=='Material' else 7,material_rarity_only=category=='Material',pool=pool if category=='Accessory' else None)
        if category=='Material': fields['rarity_from_affix_count']=False
        profiles[category]=make('DA_Generation_'+category,u.UmbraItemGenerationProfile,fields)
    mesh=u.load_asset(CONFIG['placeholder_mesh'])
    assert isinstance(mesh,u.StaticMesh)
    selected=PLAN['items']
    if LIMIT:
        # Ensure the small batch covers both source tables.
        selected=[r for r in selected if r['category']=='Accessory'][:max(1,LIMIT//2)]+[r for r in selected if r['category']=='Material'][:max(1,LIMIT//2)]
    groups={'Accessory':[],'Material':[]}
    scale=CONFIG['placeholder_scale']
    bounds=mesh.get_bounding_box()
    extent=u.Vector((bounds.max.x-bounds.min.x)/2,(bounds.max.y-bounds.min.y)/2,(bounds.max.z-bounds.min.z)/2)
    center=u.Vector((bounds.max.x+bounds.min.x)/2,(bounds.max.y+bounds.min.y)/2,(bounds.max.z+bounds.min.z)/2)
    for row in selected:
        accessory=row['category']=='Accessory'
        # Retain authentic descriptions in source metadata, not as unimplemented gameplay claims.
        description='护符。属性以本物品的随机词条为准；原作专属效果暂未实现。' if accessory else DESCRIPTION
        fields=dict(definition_id='NativeV1.'+row['table']+'_'+str(row['row_id']),display_name=display_name(row['name']['value']),
            flavor_text=description,item_category=getattr(u.UmbraItemCategory,row['category'].upper()),
            icon=icon(row),source_rarity=row['source_rarity'],generation_profile=profiles[row['category']],
            allowed_slots=[u.UmbraEquipmentSlot.AMULET,u.UmbraEquipmentSlot.AMULET2,u.UmbraEquipmentSlot.RING1,u.UmbraEquipmentSlot.RING2] if accessory else [],
            equipping_enabled=accessory,equipping_unavailable_reason='' if accessory else '强化材料不可装备。',
            weight=float(row['raw_params'].get('weight','0')),world_drop_mesh=mesh,world_drop_skeletal_mesh=None,
            world_drop_scale=u.Vector(*scale),world_drop_offset=u.Vector(0,0,0),
            world_drop_physics_extent=u.Vector(max(1,extent.x*scale[0]),max(1,extent.y*scale[1]),max(1,extent.z*scale[2])),
            world_drop_physics_center=u.Vector(center.x*scale[0],center.y*scale[1],center.z*scale[2]))
        if not accessory: fields.update(presentation_fields(row['row_id'], profiles['Material']))
        source=dict(item=row,source_version=CONFIG['source_version'],inputs=PLAN['inputs'],
                    provenance='User-local Elden Ring files; local prototype provenance retained; shared Engine primitive placeholder')
        obj=make('Items/DA_Item_'+row['table']+'_'+str(row['row_id']),u.UmbraItemDefinition,fields,source)
        assert obj.icon and obj.world_drop_mesh and obj.generation_profile
        groups[row['category']].append(put(u.UmbraLootTemplate(),item_definition=obj,weight=1.0))
        REPORT['items'].append({'asset':obj.get_path_name(),'source_rarity':obj.source_rarity,'category':row['category']})
    if not LIMIT:
        old=u.load_asset('/Game/Items/GeneratedEquipment/V2/DA_Loot_Equipment')
        assert isinstance(old,u.UmbraLootTable)
        categories=[]
        for old_category in old.categories:
            c=u.UmbraLootCategoryGroup(); c.import_text(old_category.export_text())
            key='Weapon' if c.category==u.UmbraItemCategory.WEAPON else 'Armor'
            c.set_editor_property('weight',CONFIG['category_weights'][key]); categories.append(c)
        for category,templates in groups.items():
            group=put(u.UmbraLootTypeGroup(),group_id='Amulet' if category=='Accessory' else 'SmithingStone',weight=1.0,templates=templates)
            categories.append(put(u.UmbraLootCategoryGroup(),category=getattr(u.UmbraItemCategory,category.upper()),weight=CONFIG['category_weights'][category],types=[group]))
        make('DA_Loot_Prototype',u.UmbraLootTable,dict(use_type_groups=True,categories=categories,
             equipment_drop_chance=old.equipment_drop_chance,min_equipment_count=old.min_equipment_count,max_equipment_count=old.max_equipment_count))
    REPORT.update(ok=True,counts={k:len(v) for k,v in groups.items()},excluded=len(PLAN['excluded']))

try:
    main()
except Exception as error:
    REPORT['error']=str(error)
    raise
finally:
    (OUT/('readback.json' if READBACK else 'import_smoke.json' if LIMIT else 'import.json')).write_text(json.dumps(REPORT,ensure_ascii=False,indent=2),encoding='utf-8')
