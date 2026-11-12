"""Two real PIE starts: all nineteen saved stone kinds, fixed rarity, VFX and pickup."""
import json
import sys
import time
from pathlib import Path
import unreal as u

sys.path.insert(0, str(Path(__file__).resolve().parent / 'EquipmentImport'))
from stone_grades import SMITHING_IDS, SOMBER_IDS, rarity_index

level = u.get_editor_subsystem(u.LevelEditorSubsystem)
editor = u.get_editor_subsystem(u.UnrealEditorSubsystem)
previous = u.UmbraEquipmentNetworkTestLibrary.configure_play_settings(0, 1, True)
u.EditorPythonScripting.set_keep_python_script_alive(True)
started = time.monotonic()
state = {'stage': 'start', 'runs': []}
systems = ('Common', 'Uncommon', 'Rare', 'Epic', 'Legendary', 'Mythic')
classes = {name: u.EditorAssetLibrary.load_blueprint_class('/Game/Items/Tests/' + name)
           for name in ('BP_SixRarityStartup', 'BP_StartupSmithingStone',
                        'BP_StartupAllSmithingStones', 'BP_StartupAllSomberStones')}

def check(ok, message):
    if not ok: raise RuntimeError(message)

def finish(result, detail):
    state.update(result=result, detail=detail)
    u.UmbraEquipmentNetworkTestLibrary.configure_play_settings(previous[0], previous[1], bool(previous[2]))
    out = Path(u.Paths.project_saved_dir()) / 'StoneRarityStartup'
    out.mkdir(exist_ok=True)
    (out / 'pie.json').write_text(json.dumps(state, ensure_ascii=False, indent=2), encoding='utf-8')
    u.unregister_slate_post_tick_callback(callback)
    u.EditorPythonScripting.set_keep_python_script_alive(False)
    u.SystemLibrary.quit_editor()

def tick(delta):
    try:
        check(time.monotonic() - started < 180, 'Timeout: ' + state['stage'])
        if state['stage'] == 'start':
            level.editor_request_begin_play()
            state.update(stage='inspect', at=time.monotonic())
            return
        if state['stage'] == 'inspect' and level.is_in_play_in_editor() and time.monotonic()-state['at'] > 8:
            world = editor.get_game_world()
            drops = u.GameplayStatics.get_all_actors_of_class(world, u.UmbraWorldItem)
            for name in ('BP_SixRarityStartup', 'BP_StartupSmithingStone'):
                cls = classes[name]
                old = u.GameplayStatics.get_all_actors_of_class(world, cls)
                check(all(not a.get_editor_property('enabled') for a in old), 'Old fixture still enabled')
                check(not any(d.get_owner() in old for d in drops), 'Old fixture spawned drops')
            checked = []
            for name, ids in (('BP_StartupAllSmithingStones', SMITHING_IDS), ('BP_StartupAllSomberStones', SOMBER_IDS)):
                cls = classes[name]
                owners = u.GameplayStatics.get_all_actors_of_class(world, cls)
                check(len(owners) == 1, 'Expected one row: ' + name + ', found ' + str(len(owners)))
                own = [d for d in drops if d.get_owner() == owners[0]]
                check(len(own) == len(ids), 'Missing or extra stone drops')
                by_id = {int(str(d.get_item_instance().definition_id).rsplit('_', 1)[1]): d for d in own}
                check(set(by_id) == set(ids), 'Wrong stone kinds')
                for identity in ids:
                    drop = by_id[identity]
                    instance = drop.get_item_instance()
                    rank = rarity_index(identity)
                    check(int(instance.rarity.value) == rank, 'Grade rarity mismatch')
                    check(instance.quantity == 1 and not instance.rolled_affixes, 'Unexpected quantity/affixes')
                    fx = drop.get_component_by_class(u.NiagaraComponent)
                    check(fx and fx.is_active(), 'Inactive stone effect')
                    check(fx.get_editor_property('asset').get_name() == 'NS_LootDrop_' + systems[rank], 'Wrong effect')
                    style = drop.get_editor_property('loot_vfx_styles')[instance.rarity]
                    row = next(r for r in instance.definition.generation_profile.rarities if r.rarity == instance.rarity)
                    expected = style.color if style.override_color else row.color
                    for parameter in drop.get_editor_property('loot_vfx_color_parameters'):
                        actual, valid = fx.get_variable_color(parameter)
                        check(valid and actual.r > 0, 'Missing color')
                        check(abs(actual.g/actual.r-expected.g/expected.r)<0.001 and
                              abs(actual.b/actual.r-expected.b/expected.r)<0.001, 'Wrong color hue')
                    checked.append(dict(id=identity, name=str(instance.definition.display_name), rarity=rank,
                                        seed=instance.generation_seed, guid=instance.instance_id.to_string()))
            state['runs'].append(checked)
            if len(state['runs']) == 1:
                u.SystemLibrary.execute_console_command(world, 'HighResShot 1 filename=StoneStartup.png')
            state.update(stage='pickup', at=time.monotonic())
            return
        if state['stage'] == 'pickup' and time.monotonic()-state['at'] > 2:
            world = editor.get_game_world()
            pc = u.GameplayStatics.get_player_controller(world, 0)
            for drop in u.GameplayStatics.get_all_actors_of_class(world, u.UmbraWorldItem):
                item = drop.get_item_instance()
                if item.definition.item_category != u.UmbraItemCategory.MATERIAL: continue
                fx = drop.get_component_by_class(u.NiagaraComponent)
                pc.get_controlled_pawn().set_actor_location(drop.get_actor_location()+u.Vector(0,0,100), False, True)
                check(drop.try_pickup(pc) == u.UmbraPickupResult.SUCCESS, 'Pickup failed')
                check(not fx.is_active(), 'Effect survived pickup')
                stored = next(e for e in pc.player_state.get_inventory_component().get_snapshot().items
                              if e.instance_id.to_string() == item.instance_id.to_string())
                check(stored.instance.rarity == item.rarity, 'Pickup changed rarity')
            level.editor_request_end_play()
            state['stage'] = 'end'
        if state['stage'] == 'end' and not level.is_in_play_in_editor():
            if len(state['runs']) < 2:
                state['stage'] = 'start'
                return
            for before, after in zip(*state['runs']):
                check(before['id'] == after['id'] and before['rarity'] == after['rarity'], 'Rarity changed between starts')
                check(before['guid'] != after['guid'], 'Identity reused')
            finish('passed', 'Two starts: nineteen distinct stones, fixed grade rarities, VFX colors and pickups; old fixtures disabled')
    except Exception as error:
        level.editor_request_end_play()
        finish('failed', str(error))

callback = u.register_slate_post_tick_callback(tick)
