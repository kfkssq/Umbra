"""Run in UE 5.8 after build. Creates missing test assets only; never edits source assets/maps."""
import unreal
import sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).parent))
from rarity_palette import six_rows

DEST = "/Game/Items/Tests/RandomMVP"
tools = unreal.AssetToolsHelpers.get_asset_tools()

def make(name, cls, values):
    path = DEST + "/" + name
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        return unreal.load_asset(path)
    factory = unreal.DataAssetFactory()
    factory.set_editor_property("data_asset_class", cls)
    asset = tools.create_asset(name, DEST, cls, factory)
    if not asset:
        raise RuntimeError("Cannot create " + path)
    for key, value in values.items():
        asset.set_editor_property(key, value)
    if not unreal.EditorAssetLibrary.save_loaded_asset(asset):
        raise RuntimeError("Cannot save " + path)
    return asset

sources = [unreal.load_asset("/Game/Items/Weapons/DA_Item_TestSword"),
           unreal.load_asset("/Game/Items/Armor/DA_Armor_1730_Chest")]
if not all(isinstance(x, unreal.UmbraItemDefinition) for x in sources):
    raise RuntimeError("Source templates did not load as ItemDefinition")
entries = []
for name, kind, primary, attribute, low, high, integer in [
    ("Strength", unreal.UmbraAffixKind.PRIMARY, unreal.UmbraPrimaryAttribute.STRENGTH, unreal.UmbraEquipmentAffixStat.MAX_HEALTH, 2, 12, True),
    ("Dexterity", unreal.UmbraAffixKind.PRIMARY, unreal.UmbraPrimaryAttribute.DEXTERITY, unreal.UmbraEquipmentAffixStat.MAX_HEALTH, 2, 12, True),
    ("Health", unreal.UmbraAffixKind.ATTRIBUTE, unreal.UmbraPrimaryAttribute.STRENGTH, unreal.UmbraEquipmentAffixStat.MAX_HEALTH, 10, 60, True),
    ("Armor", unreal.UmbraAffixKind.ATTRIBUTE, unreal.UmbraPrimaryAttribute.STRENGTH, unreal.UmbraEquipmentAffixStat.ARMOR, 5, 15, True),
    ("CriticalChance", unreal.UmbraAffixKind.ATTRIBUTE, unreal.UmbraPrimaryAttribute.STRENGTH, unreal.UmbraEquipmentAffixStat.CRITICAL_CHANCE, .01, .06, False),
]:
    tiers = []
    for level, multiplier in [(1, 1), (10, 2)]:
        values = unreal.UmbraAffixRollConfig()
        for key, value in dict(affix_id=name, kind=kind, primary=primary, attribute=attribute,
                               min_value=low*multiplier, max_value=high*multiplier, tier_count=6,
                               integer_values=integer).items():
            values.set_editor_property(key, value)
        tier = unreal.UmbraAffixTier()
        tier.set_editor_property("tier_id", "T1" if level == 1 else "T2")
        tier.set_editor_property("minimum_level", level)
        tier.set_editor_property("values", values)
        tiers.append(tier)
    affix = make("DA_Affix_"+name, unreal.UmbraAffixDefinition, dict(affix_id=name, tiers=tiers))
    entry = unreal.UmbraAffixPoolEntry()
    entry.set_editor_property("affix", affix)
    entries.append(entry)
pool = make("DA_Pool_MVP", unreal.UmbraAffixPool, dict(entries=entries))
profile = make("DA_Generation_MVP", unreal.UmbraItemGenerationProfile, dict(pool=pool, rarities=six_rows(), rules_version=2))
items = []
for source, name, category in zip(sources, ["Sword", "Chest"], [unreal.UmbraItemCategory.WEAPON, unreal.UmbraItemCategory.ARMOR]):
    path = DEST + "/DA_Item_Random" + name
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        item = unreal.load_asset(path)
    else:
        item = unreal.EditorAssetLibrary.duplicate_asset(source.get_path_name(), path)
        for key, value in dict(definition_id="MVP."+name, display_name="Random MVP "+name,
                               item_category=category, generation_profile=profile, affix_roll_configs=[],
                               primary_bonuses=unreal.UmbraPrimaryValues(), attribute_bonuses=[], damage_bonuses=[],
                               required_level=1, requirements=unreal.UmbraPrimaryValues()).items():
            item.set_editor_property(key, value)
        if not unreal.EditorAssetLibrary.save_loaded_asset(item):
            raise RuntimeError("Cannot save " + path)
    items.append(item)
loot_entries = []
for item in items:
    row = unreal.UmbraLootEntry()
    row.set_editor_property("item_definition", item)
    loot_entries.append(row)
make("DA_Loot_RandomMVP", unreal.UmbraLootTable, dict(entries=loot_entries))
bp_path = DEST + "/BP_LootTest_RandomMVP"
if not unreal.EditorAssetLibrary.does_asset_exist(bp_path):
    factory = unreal.BlueprintFactory()
    factory.set_editor_property("parent_class", unreal.UmbraLootTestSpawner)
    bp = tools.create_asset("BP_LootTest_RandomMVP", DEST, unreal.Blueprint, factory)
    defaults = unreal.get_default_object(bp.generated_class())
    defaults.set_editor_property("items", items * 6)
    defaults.set_editor_property("world_item_class", unreal.load_class(None, "/Game/Items/Loot/BP_WorldItem.BP_WorldItem_C"))
    context = unreal.UmbraGenerationContext()
    context.set_editor_property("source_level", 10)
    defaults.set_editor_property("generation_context", context)
    defaults.set_editor_property("generation_seed", 12345)
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    if not unreal.EditorAssetLibrary.save_loaded_asset(bp):
        raise RuntimeError("Cannot save test spawner")
unreal.log("RANDOM_MVP_ASSETS_CREATED " + DEST)
