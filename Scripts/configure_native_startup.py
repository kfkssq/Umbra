"""Persist two additional single-item random startup fixtures through UE Editor."""
import json
from pathlib import Path
import unreal as u

lib=u.EditorAssetLibrary
level=u.get_editor_subsystem(u.LevelEditorSubsystem)
assert level.load_level('/Game/Maps/L_Prototype')
actors=u.get_editor_subsystem(u.EditorActorSubsystem)
starts=[a for a in actors.get_all_level_actors() if isinstance(a,u.PlayerStart)]
assert starts
table=u.load_asset('/Game/Items/NativeAccessoriesV1/DA_Loot_Prototype')
report=[]
for category,name,offset in [(u.UmbraItemCategory.ACCESSORY,'Amulet',-100),(u.UmbraItemCategory.MATERIAL,'SmithingStone',100)]:
    path='/Game/Items/Tests/BP_Startup'+name
    bp=u.load_asset(path) if lib.does_asset_exist(path) else None
    if not bp:
        factory=u.BlueprintFactory();factory.set_editor_property('parent_class',u.UmbraLootTestSpawner)
        bp=u.AssetToolsHelpers.get_asset_tools().create_asset('BP_Startup'+name,'/Game/Items/Tests',u.Blueprint,factory)
    assert bp
    u.BlueprintEditorLibrary.compile_blueprint(bp)
    cdo=u.get_default_object(bp.generated_class())
    items=[t.item_definition for c in table.categories if c.category==category for g in c.types for t in g.templates]
    assert len(items)==(116 if name=='Amulet' else 19)
    context=u.UmbraGenerationContext();context.set_editor_property('source_level',1)
    settings=dict(enabled=True,one_of_each_rarity=False,random_single_item=True,randomize_seed_each_play=True,
                  items=items,generation_context=context,world_item_class=lib.load_blueprint_class('/Game/Items/Loot/BP_WorldItem'))
    for key,value in settings.items(): cdo.set_editor_property(key,value)
    u.BlueprintEditorLibrary.compile_blueprint(bp)
    assert lib.save_loaded_asset(bp,False)
    matches=[a for a in actors.get_all_level_actors() if a.get_class()==bp.generated_class()]
    assert len(matches)<=1,'Duplicate fixture '+name
    actor=matches[0] if matches else actors.spawn_actor_from_class(bp.generated_class(),starts[0].get_actor_location()+u.Vector(450,offset,0),u.Rotator())
    for key,value in settings.items(): actor.set_editor_property(key,value)
    actor.set_actor_label('Startup Random '+name)
    report.append(dict(blueprint=path,actor=actor.get_path_name(),templates=len(items),source_level=1,position=str(actor.get_actor_location())))
assert level.save_current_level()
out=Path(u.Paths.project_saved_dir())/'NativeStartup';out.mkdir(exist_ok=True)
(out/'configuration.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
