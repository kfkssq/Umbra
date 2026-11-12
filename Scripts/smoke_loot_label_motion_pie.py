"""Rendered PIE: move the player/camera around a settled dense loot cluster; no saves."""
import json
import math
import time
from pathlib import Path
import unreal as u

level = u.get_editor_subsystem(u.LevelEditorSubsystem)
editor = u.get_editor_subsystem(u.UnrealEditorSubsystem)
previous = u.UmbraEquipmentNetworkTestLibrary.configure_play_settings(0, 1, True)
u.EditorPythonScripting.set_keep_python_script_alive(True)
started = time.monotonic()
state = {'stage': 'start', 'frames': 0, 'max_offset_step': 0.0, 'camera_travel': 0.0}
keep = []
last = {}

def check(ok, why):
    if not ok: raise RuntimeError(why)

def finish(result, detail):
    state.update(result=result, detail=detail)
    u.UmbraEquipmentNetworkTestLibrary.configure_play_settings(previous[0], previous[1], bool(previous[2]))
    out = Path(u.Paths.project_saved_dir()) / 'LootLabelMotion'
    out.mkdir(exist_ok=True)
    (out / 'pie.json').write_text(json.dumps(state, indent=2), encoding='utf-8')
    u.unregister_slate_post_tick_callback(callback)
    u.EditorPythonScripting.set_keep_python_script_alive(False)
    u.SystemLibrary.quit_editor()

def tick(delta):
    global origin, first_camera, pc
    try:
        check(time.monotonic()-started < 150, 'Timeout ' + state['stage'])
        if state['stage'] == 'start':
            level.editor_request_begin_play()
            state.update(stage='setup', at=time.monotonic())
            return
        if state['stage'] == 'setup' and level.is_in_play_in_editor() and time.monotonic()-state['at'] > 8:
            world = editor.get_game_world()
            pc = u.GameplayStatics.get_player_controller(world, 0)
            pc.set_mouse_location(5, 5)
            origin = pc.get_controlled_pawn().get_actor_location()
            drops = u.GameplayStatics.get_all_actors_of_class(world, u.UmbraWorldItem)
            stones = sorted([d for d in drops if d.get_item_instance().definition.item_category == u.UmbraItemCategory.MATERIAL],
                            key=lambda d: str(d.get_item_instance().definition_id))[:6]
            check(len(stones) == 6, 'Need six startup stones')
            for drop in drops:
                if drop not in stones: drop.destroy_actor()
            for drop in stones:
                drop.set_actor_location(u.Vector(origin.x+250, origin.y, drop.get_actor_location().z), False, True)
                component = next(c for c in drop.get_components_by_class(u.WidgetComponent) if c.get_name() == 'Label')
                keep.append((drop, component.get_user_widget_object()))
            state.update(stage='move', at=time.monotonic())
            return
        if state['stage'] == 'move' and time.monotonic()-state['at'] > 2:
            camera = pc.player_camera_manager.get_camera_location()
            if state['frames'] == 0: first_camera = camera
            state['camera_travel'] = max(state['camera_travel'], (camera-first_camera).length())
            for drop, widget in keep:
                check(widget.get_render_opacity() > 0, 'Dense label hidden')
                translation = widget.get_editor_property('render_transform').translation
                value = (translation.x, translation.y)
                key = drop.get_name()
                if key in last:
                    step = math.hypot(value[0]-last[key][0], value[1]-last[key][1])
                    state['max_offset_step'] = max(state['max_offset_step'], step)
                last[key] = value
            state['frames'] += 1
            # Reversals and stops expose camera-induced interpolation/snap cycles.
            y = 90 * math.sin(state['frames'] * math.pi / 90)
            pc.get_controlled_pawn().set_actor_location(origin+u.Vector(0, y, 0), False, True)
            if state['frames'] >= 360:
                check(state['camera_travel'] > 50, 'Camera did not follow player')
                check(state['max_offset_step'] < 0.1, 'Avoidance offset jitter during camera movement')
                level.editor_request_end_play()
                state['stage'] = 'end'
        if state['stage'] == 'end' and not level.is_in_play_in_editor():
            finish('passed', '360 moving-camera frames: six dense labels remain visible with stable avoidance offsets')
    except Exception as error:
        level.editor_request_end_play()
        finish('failed', str(error))

callback = u.register_slate_post_tick_callback(tick)
