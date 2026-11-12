"""Create/configure a development-only six-rarity startup fixture through UE Editor."""
import json
from pathlib import Path
import unreal as u

lib=u.EditorAssetLibrary
path='/Game/Items/Tests/BP_SixRarityStartup'
bp=u.load_asset(path) if lib.does_asset_exist(path) else None
if not bp:
    factory=u.BlueprintFactory()
    factory.set_editor_property('parent_class',u.UmbraLootTestSpawner)
    bp=u.AssetToolsHelpers.get_asset_tools().create_asset('BP_SixRarityStartup','/Game/Items/Tests',u.Blueprint,factory)
assert bp
u.BlueprintEditorLibrary.compile_blueprint(bp)
cdo=u.get_default_object(bp.generated_class())
table=u.load_asset('/Game/Items/GeneratedEquipment/V2/DA_Loot_Equipment')
items={}
for category in table.categories:
    for group in category.types:
        for template in group.templates:
            item=template.item_definition
            if item and item.generation_profile: items[item.get_path_name()]=item
assert len(items)>0
for key,value in dict(enabled=True,one_of_each_rarity=True,randomize_seed_each_play=True,
                      spacing=150.0,items=[items[k] for k in sorted(items)],
                      world_item_class=lib.load_blueprint_class('/Game/Items/Loot/BP_WorldItem')).items():
    cdo.set_editor_property(key,value)
context=u.UmbraGenerationContext();context.set_editor_property('source_level',10)
cdo.set_editor_property('generation_context',context)
u.BlueprintEditorLibrary.compile_blueprint(bp)
assert lib.save_loaded_asset(bp,False)
level=u.get_editor_subsystem(u.LevelEditorSubsystem)
assert level.load_level('/Game/Maps/L_Prototype')
actors=u.get_editor_subsystem(u.EditorActorSubsystem)
matches=[a for a in actors.get_all_level_actors() if a.get_class()==bp.generated_class()]
assert len(matches)<=1,'Duplicate startup fixtures: inspect before adding'
if matches:
    actor=matches[0]
else:
    starts=[a for a in actors.get_all_level_actors() if isinstance(a,u.PlayerStart)]
    assert starts,'No PlayerStart in prototype'
    pos=starts[0].get_actor_location()+u.Vector(250,-375,0)
    actor=actors.spawn_actor_from_class(bp.generated_class(),pos,u.Rotator())
actor.set_actor_label('Six Rarity Startup - Random Test Loot')
assert level.save_current_level()
out=Path(u.Paths.project_saved_dir())/'SixRarityStartup';out.mkdir(exist_ok=True)
(out/'configuration.json').write_text(json.dumps(dict(blueprint=path,actor=actor.get_path_name(),
    templates=len(items),source_level=10,spacing_cm=150,position=str(actor.get_actor_location())),indent=2),encoding='utf-8')
