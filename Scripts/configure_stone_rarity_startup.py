"""UE-only: replace the equipment showcase with all nineteen stone kinds.

Uses the fixed grade rarity on the nineteen material definitions. No production
definitions, profiles or enemy loot tables are changed by this setup script.
"""
import json
import sys
from pathlib import Path
import unreal as u

sys.path.insert(0, str(Path(__file__).resolve().parent / 'EquipmentImport'))
from stone_grades import SMITHING_IDS, SOMBER_IDS, rarity_index

lib = u.EditorAssetLibrary
level = u.get_editor_subsystem(u.LevelEditorSubsystem)
assert level.load_level('/Game/Maps/L_Prototype')
actors = u.get_editor_subsystem(u.EditorActorSubsystem)
starts = [a for a in actors.get_all_level_actors() if isinstance(a, u.PlayerStart)]
assert len(starts) == 1
report = {'disabled': [], 'rows': []}

# Keep the old fixtures for easy restoration; the random stone is superseded by the rows.
for name in ('BP_SixRarityStartup', 'BP_StartupSmithingStone'):
    bp = u.load_asset('/Game/Items/Tests/' + name)
    assert bp
    u.get_default_object(bp.generated_class()).set_editor_property('enabled', False)
    u.BlueprintEditorLibrary.compile_blueprint(bp)
    assert lib.save_loaded_asset(bp, False)
    for actor in actors.get_all_level_actors():
        if actor.get_class() == bp.generated_class():
            actor.set_editor_property('enabled', False)
            report['disabled'].append(actor.get_path_name())

for name, ids, offset, label in (
    ('BP_StartupAllSmithingStones', SMITHING_IDS, 300, 'Smithing Stones 1-8 + Ancient Dragon'),
    ('BP_StartupAllSomberStones', SOMBER_IDS, 550, 'Somber Stones 1-9 + Ancient Dragon'),
):
    path = '/Game/Items/Tests/' + name
    bp = u.load_asset(path) if lib.does_asset_exist(path) else None
    if not bp:
        factory = u.BlueprintFactory()
        factory.set_editor_property('parent_class', u.UmbraLootTestSpawner)
        bp = u.AssetToolsHelpers.get_asset_tools().create_asset(name, '/Game/Items/Tests', u.Blueprint, factory)
    assert bp
    items = [u.load_asset('/Game/Items/NativeAccessoriesV1/Items/DA_Item_EquipParamGoods_' + str(i)) for i in ids]
    assert all(isinstance(item, u.UmbraItemDefinition) for item in items)
    assert [item.source_rarity for item in items] == [1] * (len(items) - 1) + [3]
    assert [int(item.rarity.value) for item in items] == [rarity_index(i) for i in ids], 'Apply fixed stone grades first'
    context = u.UmbraGenerationContext()
    context.set_editor_property('source_level', 1)
    settings = dict(enabled=True, one_of_each_rarity=False, random_single_item=False,
                    randomize_seed_each_play=True, spacing=140.0, items=items,
                    generation_context=context,
                    world_item_class=lib.load_blueprint_class('/Game/Items/Loot/BP_WorldItem'))
    cdo = u.get_default_object(bp.generated_class())
    for key, value in settings.items():
        cdo.set_editor_property(key, value)
    u.BlueprintEditorLibrary.compile_blueprint(bp)
    assert lib.save_loaded_asset(bp, False)
    matches = [a for a in actors.get_all_level_actors() if a.get_class() == bp.generated_class()]
    assert len(matches) <= 1, 'Duplicate fixture: ' + name
    position = starts[0].get_actor_location() + u.Vector(offset, -(len(items)-1)*70, 0)
    actor = matches[0] if matches else actors.spawn_actor_from_class(bp.generated_class(), position, u.Rotator())
    actor.set_actor_location(position, False, True)
    actor.set_actor_rotation(u.Rotator(), False)
    actor.set_actor_label(label)
    for key, value in settings.items():
        actor.set_editor_property(key, value)
    report['rows'].append(dict(actor=actor.get_path_name(), blueprint=path,
                               items=[str(item.display_name) for item in items], position=str(position)))

assert level.save_current_level()
out = Path(u.Paths.project_saved_dir()) / 'StoneRarityStartup'
out.mkdir(exist_ok=True)
(out / 'configuration.json').write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
