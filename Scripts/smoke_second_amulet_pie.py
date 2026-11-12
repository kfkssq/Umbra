"""Read back the saved ten-slot WBP in a real prototype PIE menu. No assets saved."""
import json
import time
from pathlib import Path
import unreal as u

level = u.get_editor_subsystem(u.LevelEditorSubsystem)
editor = u.get_editor_subsystem(u.UnrealEditorSubsystem)
previous = u.UmbraEquipmentNetworkTestLibrary.configure_play_settings(0, 1, True)
u.EditorPythonScripting.set_keep_python_script_alive(True)
start = time.monotonic()
state = {'stage': 'start'}

def finish(result, detail):
    state.update(result=result, detail=detail)
    u.UmbraEquipmentNetworkTestLibrary.configure_play_settings(previous[0], previous[1], bool(previous[2]))
    out = Path(u.Paths.project_saved_dir()) / 'SecondAmulet'
    out.mkdir(exist_ok=True)
    (out / 'pie.json').write_text(json.dumps(state, indent=2), encoding='utf-8')
    u.unregister_slate_post_tick_callback(callback)
    u.EditorPythonScripting.set_keep_python_script_alive(False)
    u.SystemLibrary.quit_editor()

def tick(delta):
    try:
        if time.monotonic() - start > 120:
            raise RuntimeError('PIE timeout: ' + state['stage'])
        if state['stage'] == 'start':
            level.editor_request_begin_play()
            state['stage'] = 'open'
            return
        if state['stage'] == 'open' and level.is_in_play_in_editor():
            world = editor.get_game_world()
            pc = u.GameplayStatics.get_player_controller(world, 0)
            if not pc or not u.GameplayStatics.get_player_pawn(world, 0):
                return
            pc.toggle_character_menu()
            assert pc.is_character_menu_open()
            state['stage'] = 'inspect'
            state['at'] = time.monotonic()
        elif state['stage'] == 'inspect' and time.monotonic() - state['at'] > 3:
            world = editor.get_game_world()
            pc = u.GameplayStatics.get_player_controller(world, 0)
            menus = [w for w in u.ObjectIterator(u.UmbraEquipmentMenu) if w.get_owning_player() == pc]
            assert len(menus) == 1, 'Expected one real equipment menu'
            menu = menus[0]
            names = ('HEAD', 'CHEST', 'HANDS', 'LEGS', 'AMULET', 'AMULET2', 'RING1', 'RING2', 'MAIN_HAND', 'OFF_HAND')
            found = [menu.get_equipment_slot(getattr(u.UmbraEquipmentSlot, n)) for n in names]
            assert all(found) and len(set(w.get_path_name() for w in found)) == 10
            state['slots'] = {n: w.get_path_name() for n, w in zip(names, found)}
            for name in ('AMULET', 'AMULET2'):
                w = menu.get_equipment_slot(getattr(u.UmbraEquipmentSlot, name))
                assert w.get_editor_property('empty_slot_icons').get(getattr(u.UmbraEquipmentSlot, name))
            u.GameplayStatics.get_player_controller(world, 0).toggle_character_menu()
            level.editor_request_end_play()
            state['stage'] = 'end'
        elif state['stage'] == 'end' and not level.is_in_play_in_editor():
            finish('passed', 'Saved WBP loaded, menu opened, ten distinct active slots and both amulet icons resolved, menu closed and EndPIE completed')
    except Exception as error:
        level.editor_request_end_play()
        finish('failed', str(error))

callback = u.register_slate_post_tick_callback(tick)
