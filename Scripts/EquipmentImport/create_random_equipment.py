"""UE-only Library -> gameplay definitions. Default audit, opt-in UMBRA_RANDOM_APPLY=1.

Never saves Library drafts. Existing outputs must match recipe AND authored properties;
manual changes produce a conflict rather than being overwritten. Reports stay in Saved.
"""
import json
import os
from pathlib import Path
import re
import sys
import unreal as u

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent))
from rarity_palette import six_rows

CONFIG = json.loads(Path(os.environ.get('UMBRA_RANDOM_CONFIG', str(HERE / 'random_equipment_v2.json'))).read_text(encoding='utf-8'))
ROOT = CONFIG['destination']
APPLY = os.environ.get('UMBRA_RANDOM_APPLY') == '1'
LIB = u.EditorAssetLibrary
TOOLS = u.AssetToolsHelpers.get_asset_tools()
REPORT = {'apply': APPLY, 'items': [], 'saved': [], 'eligible': [], 'ok': False}
TAG = 'UmbraRandomEquipment.Recipe'


def enum_name(name):
    return re.sub(r'(?<!^)(?=[A-Z])', '_', name).upper()


def put(obj, **values):
    for key, value in values.items(): obj.set_editor_property(key, value)
    return obj


def normalized(value):
    if value is None: return None
    if isinstance(value, (str, bool, int)): return value
    if isinstance(value, float): return round(value, 6)
    if isinstance(value, u.Object): return value.get_path_name()
    if hasattr(value, 'export_text'): return value.export_text()
    if isinstance(value, (list, tuple, u.Array)): return [normalized(v) for v in value]
    return str(value)


def make(name, cls, values, source=None):
    path = ROOT + '/' + name
    recipe = json.dumps({'version': CONFIG['rules_version'], 'source': source,
                         'fields': {k: normalized(v) for k, v in values.items()}}, sort_keys=True, ensure_ascii=False)
    if LIB.does_asset_exist(path):
        obj = u.load_asset(path)
        if not isinstance(obj, cls) or LIB.get_metadata_tag(obj, TAG) != recipe:
            raise ValueError('Output recipe conflict, preserved: ' + path)
        for key, value in values.items():
            if normalized(obj.get_editor_property(key)) != normalized(value):
                raise ValueError('Output edited, preserved: ' + path + ':' + key)
        return obj
    factory = put(u.DataAssetFactory(), data_asset_class=cls)
    folder, leaf = path.rsplit('/', 1)
    obj = TOOLS.create_asset(leaf, folder, cls, factory)
    if not obj: raise RuntimeError('Create failed: ' + path)
    put(obj, **values)
    LIB.set_metadata_tag(obj, TAG, recipe)
    if not LIB.save_loaded_asset(obj, only_if_is_dirty=False): raise RuntimeError('Save failed: ' + path)
    REPORT['saved'].append(path)
    return obj


def scan():
    seen_ids, seen_models = set(), set()
    selected = []
    types = {getattr(u.UmbraWeaponType, enum_name(k)): k for k in CONFIG['weapon_types']}
    for path in sorted(LIB.list_assets('/Game/EquipmentImport/Library', recursive=True, include_folder=False)):
        if '/Drafts/DA_Source_' not in path: continue
        row = {'source': path}; REPORT['items'].append(row)
        try:
            draft = u.load_asset(path)
            if not isinstance(draft, u.UmbraItemDefinition): raise ValueError('Not an item definition')
            source = json.loads(LIB.get_metadata_tag(draft, 'UmbraItemImport.Source'))['item']
            if not str(draft.display_name).strip() or not draft.icon: raise ValueError('Missing name/icon')
            identity = str(draft.definition_id)
            if identity != 'ER_' + source['table'] + '_' + str(source['row_id']): raise ValueError('Source identity mismatch')
            if source.get('definition_excluded'): raise ValueError('Excluded source record')
            mesh = str(draft.get_editor_property('world_drop_skeletal_mesh'))
            if mesh in ('None', ''): mesh = str(draft.get_editor_property('world_drop_mesh'))
            # Soft references are resolved read-only, never moved or saved.
            mesh_obj = draft.get_editor_property('world_drop_skeletal_mesh') or draft.get_editor_property('world_drop_mesh')
            if not mesh_obj: raise ValueError('Missing world mesh')
            if isinstance(mesh_obj, str): mesh_obj = u.load_asset(mesh_obj)
            if not isinstance(mesh_obj, (u.StaticMesh, u.SkeletalMesh)): raise ValueError('Mesh could not load')
            mesh = mesh_obj.get_path_name()
            if draft.item_category == u.UmbraItemCategory.WEAPON:
                kind = types.get(draft.weapon_type)
                if not kind: raise ValueError('Weapon type has no gameplay tuning in this configuration')
                category, slot = 'Weapon', CONFIG['weapon_types'][kind].get('slot','MainHand')
            elif draft.item_category == u.UmbraItemCategory.ARMOR:
                slot = source['umbra_suggestion'].get('SuggestedSlot')
                if slot not in CONFIG['armor_slots']: raise ValueError('Unsupported armor slot')
                category, kind = 'Armor', slot
            else: raise ValueError('Not a supported equipment category')
            if identity in seen_ids or mesh in seen_models: raise ValueError('Duplicate source identity/model')
            # Do not change existing requirements, fixed affixes, or combat setup while promoting a draft.
            if draft.generation_profile or draft.weapon or draft.affix_roll_configs or draft.attribute_bonuses or draft.damage_bonuses:
                raise ValueError('Draft already has gameplay configuration; explicit merge required')
            if any(getattr(draft.primary_bonuses, k) != 0 for k in ('strength','dexterity','intelligence','faith')):
                raise ValueError('Draft has fixed primary bonuses; explicit merge required')
            seen_ids.add(identity); seen_models.add(mesh)
            source_rarity = int(source['raw_params']['rarity']) if CONFIG.get('use_source_rarity') else -1
            if CONFIG.get('use_source_rarity') and source_rarity not in (0,1,2,3): raise ValueError('Invalid source rarity')
            row.update(source_rarity=source_rarity)
            row.update(status='eligible', id=identity, name=str(draft.display_name), category=category, kind=kind,
                       requirements=normalized(draft.requirements), mesh=mesh)
            selected.append((draft, row, slot))
        except Exception as exc: row.update(status='excluded', reason=str(exc))
    return selected


def rules():
    # Reuse the established MVP and M4 candidate gradients rather than copying source-game stats.
    common, defense, offense = [], [], []
    for name in ('Strength', 'Dexterity', 'Health', 'Armor', 'CriticalChance'):
        old = u.load_asset('/Game/Items/Tests/RandomMVP/DA_Affix_' + name)
        if not isinstance(old, u.UmbraAffixDefinition): raise ValueError('Missing gradient: ' + name)
        category = [u.UmbraItemCategory.ARMOR] if name == 'Armor' else [u.UmbraItemCategory.WEAPON] if name == 'CriticalChance' else []
        affix = make('Affixes/DA_Affix_' + name, u.UmbraAffixDefinition,
                     dict(affix_id=old.affix_id, tiers=list(old.tiers), maximum_level=old.maximum_level, categories=category))
        (defense if name == 'Armor' else offense if name == 'CriticalChance' else common).append(affix)
    # Intelligence/Faith share the existing primary gradient; no imported scaling coefficients.
    primary_source = u.load_asset('/Game/Items/Tests/RandomMVP/DA_Affix_Strength')
    for name in ('Intelligence','Faith'):
        tiers = []
        for old_tier in primary_source.tiers:
            values = u.UmbraAffixRollConfig(); values.import_text(old_tier.values.export_text())
            put(values, affix_id=name, primary=getattr(u.UmbraPrimaryAttribute,name.upper()))
            tiers.append(put(u.UmbraAffixTier(), tier_id=old_tier.tier_id, minimum_level=old_tier.minimum_level, values=values))
        common.append(make('Affixes/DA_Affix_' + name,u.UmbraAffixDefinition,dict(affix_id=name,tiers=tiers)))
    damage = u.load_asset('/Game/Items/Tests/RandomM4/DA_Affix_SlashA')
    if not damage: raise ValueError('Missing existing damage gradient')
    damage_types = ('Slashing','Blunt','Piercing') if CONFIG['rules_version']<4 else sorted({v['damage_type'] for v in CONFIG['weapon_types'].values() if 'damage_type' in v})
    for damage_type in damage_types:
        name = damage_type + 'A'
        tiers = []
        for old_tier in damage.tiers:
            values = u.UmbraAffixRollConfig(); values.import_text(old_tier.values.export_text())
            source = 'Any' if damage_type in ('Slashing','Blunt','Piercing') else 'BasicAttack'
            bonus = u.UmbraDamageBonus(); bonus.import_text('(Bucket=Additive,Types=(' + damage_type + '),AttackSource=' + source + ')')
            put(values, affix_id=name, damage_bonus=bonus)
            tiers.append(put(u.UmbraAffixTier(),tier_id=old_tier.tier_id,minimum_level=old_tier.minimum_level,values=values))
        allowed = [getattr(u.UmbraWeaponType,enum_name(k)) for k,v in CONFIG['weapon_types'].items() if v.get('damage_type')==damage_type]
        offense.append(make('Affixes/DA_Affix_' + name,u.UmbraAffixDefinition,
                            dict(affix_id=name,tiers=tiers,categories=[u.UmbraItemCategory.WEAPON],weapon_types=allowed)))
    families = {v.get('family','Melee') for v in CONFIG['weapon_types'].values()}
    pools = [('Weapon',offense),('Armor',defense)]
    if 'Shield' in families:
        shield_types = [getattr(u.UmbraWeaponType,enum_name(k)) for k,v in CONFIG['weapon_types'].items() if v.get('family')=='Shield']
        shield_affixes=[]
        armor_source=u.load_asset('/Game/Items/Tests/RandomMVP/DA_Affix_Armor')
        for name,stat in [('ShieldArmor',u.UmbraEquipmentAffixStat.ARMOR),('ShieldMagicResistance',u.UmbraEquipmentAffixStat.MAGIC_RESISTANCE)]:
            tiers=[]
            for old_tier in armor_source.tiers:
                values=u.UmbraAffixRollConfig();values.import_text(old_tier.values.export_text())
                put(values,affix_id=name,attribute=stat)
                tiers.append(put(u.UmbraAffixTier(),tier_id=old_tier.tier_id,minimum_level=old_tier.minimum_level,values=values))
            shield_affixes.append(make('Affixes/DA_Affix_'+name,u.UmbraAffixDefinition,
                dict(affix_id=name,tiers=tiers,categories=[u.UmbraItemCategory.WEAPON],weapon_types=shield_types)))
        pools.append(('Shield',shield_affixes))
    if 'Ranged' in families: pools.append(('Ranged',offense))
    if 'Catalyst' in families:
        name='SpellA';tiers=[]
        for old_tier in damage.tiers:
            values=u.UmbraAffixRollConfig();values.import_text(old_tier.values.export_text())
            bonus=u.UmbraDamageBonus();bonus.import_text('(Bucket=Additive,Types=(Fire,Lightning,Cold,Radiant,Poison,Shadow),AttackSource=Skill)')
            put(values,affix_id=name,damage_bonus=bonus)
            tiers.append(put(u.UmbraAffixTier(),tier_id=old_tier.tier_id,minimum_level=old_tier.minimum_level,values=values))
        allowed=[getattr(u.UmbraWeaponType,enum_name(k)) for k,v in CONFIG['weapon_types'].items() if v.get('family')=='Catalyst']
        spell=make('Affixes/DA_Affix_'+name,u.UmbraAffixDefinition,
                   dict(affix_id=name,tiers=tiers,categories=[u.UmbraItemCategory.WEAPON],weapon_types=allowed))
        pools.append(('Catalyst',[spell]))
    profiles = {}
    for category, extras in pools:
        pool = make('DA_Pool_' + category,u.UmbraAffixPool,
                    dict(entries=[put(u.UmbraAffixPoolEntry(),affix=a,weight=1.0) for a in common+extras]))
        rows = six_rows()
        for row in rows: put(row,weight=0.0,affix_count=0)
        if CONFIG.get('use_source_rarity'):
            for i in (4,5): put(rows[i],weight=CONFIG['high_rarity_weights'][i-4],affix_count=CONFIG['high_rarity_affix_count'])
        profiles[category] = make('DA_Generation_' + category,u.UmbraItemGenerationProfile,
                                  dict(pool=pool,rarities=rows,rarity_from_affix_count=True,
                                       affix_count_weights=CONFIG['affix_count_weights'],rules_version=CONFIG['rules_version'],
                                       **({'source_two_legendary_chance':CONFIG['source_two_legendary_chance']} if CONFIG.get('use_source_rarity') else {})))
    return profiles


def promote(selected):
    profiles = rules()
    weapon_profiles = {}
    for kind, tuning in CONFIG['weapon_types'].items():
        if not tuning.get('equipping_enabled',True): continue
        channel = put(u.UmbraWeaponDamageChannel(),type=getattr(u.UmbraWeaponDamageType,tuning['damage_type'].upper()),
                      use_damage_range=True,min_base_damage=tuning['min'],max_base_damage=tuning['max'])
        weapon_profiles[kind] = make('Weapons/DA_Weapon_' + kind,u.UmbraWeaponProfile,
                                     dict(damage=put(u.UmbraWeaponDamageProfile(),channels=[channel])))
    groups = {}
    for draft, row, slot in selected:
        category, kind = row['category'], row['kind']
        tuning = CONFIG['weapon_types' if category=='Weapon' else 'armor_slots'][kind]
        fields = {k:draft.get_editor_property(k) for k in ('display_name','flavor_text','icon','item_category','weapon_type',
                  'requirements','world_drop_mesh','world_drop_skeletal_mesh','world_drop_rotation','world_drop_offset','world_drop_scale')}
        family=tuning.get('family','Melee') if category=='Weapon' else 'Armor'
        enabled=tuning.get('equipping_enabled',True)
        profile_key=family if family in ('Shield','Ranged','Catalyst') else category
        fields.update(definition_id=CONFIG.get('id_prefix','RandomV1.')+row['id'],allowed_slots=[getattr(u.UmbraEquipmentSlot,enum_name(slot))],
                      generation_profile=profiles[profile_key],weight=tuning['load'],
                      armor=tuning.get('armor',0),magic_resistance=tuning.get('magic_resistance',0))
        if CONFIG.get('use_source_rarity'): fields['source_rarity']=row['source_rarity']
        if CONFIG['rules_version']>=4:
            reason={'Shield':'盾牌格挡尚未开放：可拾取保留，暂不可穿戴。',
                    'Ranged':'远程攻击尚未开放：可拾取保留，暂不可穿戴。',
                    'Catalyst':'施法器使用尚未开放：可拾取保留，暂不可穿戴。'}
            fields.update(equipping_enabled=enabled,equipping_unavailable_reason=reason.get(family,'') if not enabled else '')
        if category=='Weapon' and enabled: fields['weapon']=weapon_profiles[kind]
        # Fit a transient preview, then include its result in the managed fields before saving.
        preview = put(u.new_object(u.UmbraItemDefinition),**fields)
        if not preview.try_fit_world_drop_physics():
            row.update(status='excluded',reason='Drop physics bounds do not fit supported limits'); continue
        for key in ('world_drop_physics_extent','world_drop_physics_center','world_drop_physics_rotation'):
            fields[key]=preview.get_editor_property(key)
        item = make('Items/DA_Item_'+row['id'],u.UmbraItemDefinition,fields,source=row['source'])
        assert normalized(item.requirements)==row['requirements'], 'Requirements changed'
        row.update(status='ready',asset=item.get_path_name(),family=family,equipping_enabled=enabled)
        groups.setdefault((category,kind),[]).append(item)
        REPORT['eligible'].append(item.get_path_name())
    categories=[]
    for category in ('Weapon','Armor'):
        types=[]
        for kind,tuning in CONFIG['weapon_types' if category=='Weapon' else 'armor_slots'].items():
            items=groups.get((category,kind),[])
            if not items: raise ValueError('Enabled type has no valid assets: '+kind)
            group=put(u.UmbraLootTypeGroup(),group_id=kind,weight=tuning['weight'],
                      templates=[put(u.UmbraLootTemplate(),item_definition=i,weight=1.0) for i in items])
            put(group,**({'weapon_type':getattr(u.UmbraWeaponType,enum_name(kind))} if category=='Weapon'
                         else {'armor_slot':getattr(u.UmbraEquipmentSlot,enum_name(kind))}))
            types.append(group)
        categories.append(put(u.UmbraLootCategoryGroup(),category=getattr(u.UmbraItemCategory,category.upper()),
                              weight=CONFIG['category_weights'][category],types=types))
    table=make('DA_Loot_Equipment',u.UmbraLootTable,dict(use_type_groups=True,categories=categories,
               equipment_drop_chance=CONFIG['drop_chance'],min_equipment_count=1,max_equipment_count=1))
    REPORT['loot_table']=table.get_path_name()


def main():
    try:
        selected=scan()
        if CONFIG.get('expected_definition_count') and len(selected)!=CONFIG['expected_definition_count']:
            raise ValueError('Expected complete catalog of %d definitions; got %d'%(CONFIG['expected_definition_count'],len(selected)))
        REPORT['eligible_count']=len(selected)
        REPORT['groups']={}
        for _,row,_ in selected:
            key=row['category']+'/'+row['kind']; REPORT['groups'][key]=REPORT['groups'].get(key,0)+1
        if APPLY: promote(selected)
        if APPLY and CONFIG.get('expected_definition_count') and len(REPORT['eligible'])!=CONFIG['expected_definition_count']:
            raise ValueError('Some definitions failed promotion; catalog is not complete')
        REPORT['ok']=True
    except Exception as exc:
        REPORT['error']=str(exc)
        u.log_error(str(exc))
    finally:
        output=HERE.parents[1]/'Saved'/CONFIG.get('report_directory','RandomEquipment')
        output.mkdir(parents=True,exist_ok=True)
        name=os.environ.get('UMBRA_RANDOM_REPORT','apply.json' if APPLY else 'audit.json')
        assert Path(name).name==name
        (output/name).write_text(json.dumps(REPORT,ensure_ascii=False,indent=2),encoding='utf-8')
    if not REPORT['ok']: raise RuntimeError('Random equipment preparation failed; inspect report')


main()
