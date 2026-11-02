"""Create missing M4 A/X test assets through UE; preserve all existing assets."""
import unreal
import sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).parent))
from rarity_palette import style

DEST="/Game/Items/Tests/RandomM4"
tools=unreal.AssetToolsHelpers.get_asset_tools()
def make(name, cls, values):
    path=DEST+"/"+name
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        return unreal.load_asset(path)
    factory=unreal.DataAssetFactory()
    factory.set_editor_property("data_asset_class",cls)
    asset=tools.create_asset(name,DEST,cls,factory)
    for key,value in values.items():
        asset.set_editor_property(key,value)
    if not unreal.EditorAssetLibrary.save_loaded_asset(asset):
        raise RuntimeError("Save failed: "+path)
    return asset
entries=[]
for name,bucket,low,high in [("SlashA",unreal.UmbraDamageBucket.ADDITIVE,.1,.3),("SlashX",unreal.UmbraDamageBucket.MULTIPLICATIVE,1.1,1.5)]:
    bonus=unreal.UmbraDamageBonus()
    # EditDefaultsOnly nested fields are initialized as reflected struct text before assigning the asset default.
    bonus.import_text("(Bucket="+("Additive" if name=="SlashA" else "Multiplicative")+",Types=(Slashing),AttackSource=BasicAttack)")
    values=unreal.UmbraAffixRollConfig()
    for key,value in dict(affix_id=name,kind=unreal.UmbraAffixKind.DAMAGE,damage_bonus=bonus,min_value=low,max_value=high).items():
        values.set_editor_property(key,value)
    tier=unreal.UmbraAffixTier()
    tier.set_editor_property("tier_id","T1")
    tier.set_editor_property("values",values)
    affix=make("DA_Affix_"+name,unreal.UmbraAffixDefinition,dict(affix_id=name,tiers=[tier]))
    entry=unreal.UmbraAffixPoolEntry()
    entry.set_editor_property("affix",affix)
    entries.append(entry)
pool=make("DA_Pool_Damage",unreal.UmbraAffixPool,dict(entries=entries))
rarity=unreal.UmbraGenerationRarity()
rarity.set_editor_property("rarity",unreal.UmbraItemRarity.RARE)
rarity.set_editor_property("affix_count",2)
style(rarity,2)
profile=make("DA_Generation_Damage",unreal.UmbraItemGenerationProfile,dict(pool=pool,rarities=[rarity]))
path=DEST+"/DA_Item_DamageSword"
if not unreal.EditorAssetLibrary.does_asset_exist(path):
    item=unreal.EditorAssetLibrary.duplicate_asset("/Game/Items/Tests/RandomMVP/DA_Item_RandomSword",path)
    item.set_editor_property("definition_id","M4.DamageSword")
    item.set_editor_property("display_name","M4 Damage Sword")
    item.set_editor_property("generation_profile",profile)
    if not unreal.EditorAssetLibrary.save_loaded_asset(item):
        raise RuntimeError("Save item failed")
item=unreal.load_asset(path)
bp_path=DEST+"/BP_LootTest_Damage"
if not unreal.EditorAssetLibrary.does_asset_exist(bp_path):
    bp=unreal.EditorAssetLibrary.duplicate_asset("/Game/Items/Tests/RandomMVP/BP_LootTest_RandomMVP",bp_path)
    unreal.get_default_object(bp.generated_class()).set_editor_property("items",[item,item])
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    if not unreal.EditorAssetLibrary.save_loaded_asset(bp):
        raise RuntimeError("Save spawner failed")
unreal.log("RANDOM_M4_ASSETS_READY")
