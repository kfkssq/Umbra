"""Observe first visible label frames for one isolated real startup drop; no saves."""
import json
import time
from pathlib import Path
import unreal as u

level = u.get_editor_subsystem(u.LevelEditorSubsystem)
editor = u.get_editor_subsystem(u.UnrealEditorSubsystem)
actors = u.get_editor_subsystem(u.EditorActorSubsystem)
previous = u.UmbraEquipmentNetworkTestLibrary.configure_play_settings(0, 1, True)
u.EditorPythonScripting.set_keep_python_script_alive(True)
started = time.monotonic()
state = {'stage': 'start', 'visible_frames': 0, 'max_translation': 0.0, 'min_anchor_height': None, 'runs': []}
fixture_class = u.EditorAssetLibrary.load_blueprint_class('/Game/Items/Tests/BP_StartupSmithingStone')
stone = u.load_asset('/Game/Items/NativeAccessoriesV1/Items/DA_Item_EquipParamGoods_10140')
for actor in actors.get_all_level_actors():
    if isinstance(actor, u.UmbraLootTestSpawner):
        actor.set_editor_property('enabled', actor.get_class() == fixture_class)
        if actor.get_class() == fixture_class:
            actor.set_editor_property('items', [stone])
            actor.set_editor_property('one_of_each_rarity', False)
            actor.set_editor_property('random_single_item', False)

def check(ok, why):
    if not ok: raise RuntimeError(why)

def finish(result, detail):
    state.update(result=result, detail=detail)
    u.UmbraEquipmentNetworkTestLibrary.configure_play_settings(previous[0], previous[1], bool(previous[2]))
    out = Path(u.Paths.project_saved_dir()) / 'LootLabelSpawn'
    out.mkdir(exist_ok=True)
    (out/'pie.json').write_text(json.dumps(state, indent=2), encoding='utf-8')
    u.unregister_slate_post_tick_callback(callback)
    u.EditorPythonScripting.set_keep_python_script_alive(False)
    u.SystemLibrary.quit_editor()

def tick(delta):
    try:
        check(time.monotonic()-started < 90, 'Timeout')
        if state['stage'] == 'start':
            level.editor_request_begin_play()
            state['stage'] = 'observe'
            return
        if state['stage'] == 'observe' and level.is_in_play_in_editor():
            world = editor.get_game_world()
            pc = u.GameplayStatics.get_player_controller(world, 0)
            pc.set_mouse_location(5, 5)
            drops = u.GameplayStatics.get_all_actors_of_class(world, u.UmbraWorldItem)
            if not drops: return
            check(len(drops) == 1, 'Expected isolated startup drop')
            drop = drops[0]
            label = next(c for c in drop.get_components_by_class(u.WidgetComponent) if c.get_name() == 'Label')
            widget = label.get_user_widget_object()
            if not widget or widget.get_render_opacity() == 0: return
            size = widget.get_desired_size()
            if size.x <= 0 or size.y <= 0: return
            offset = widget.get_editor_property('render_transform').translation
            magnitude = (offset.x**2+offset.y**2)**0.5
            state['max_translation'] = max(state['max_translation'], magnitude)
            if state['visible_frames'] < 5 or magnitude >= 0.01:
                state.setdefault('samples', []).append(dict(frame=state['visible_frames'], offset=str(offset),
                size=str(widget.get_desired_size()), anchor=str(label.get_world_location()),
                item=str(drop.get_actor_location()), viewport=str(u.WidgetLayoutLibrary.get_viewport_size(pc)),
                projection=str(u.WidgetLayoutLibrary.project_world_location_to_widget_position(pc,label.get_world_location(),False))))
            check(magnitude < 0.01, 'Isolated label was displaced from its item on initial display')
            height = label.get_world_location().z-drop.get_actor_location().z
            state['min_anchor_height'] = height if state['min_anchor_height'] is None else min(state['min_anchor_height'], height)
            check(height > 0, 'Label anchor is not above its item')
            state['visible_frames'] += 1
            if state['visible_frames'] >= 180 and drop.is_drop_settled():
                level.editor_request_end_play()
                state['stage'] = 'end'
        if state['stage'] == 'end' and not level.is_in_play_in_editor():
            state['runs'].append({k: state[k] for k in ('visible_frames', 'max_translation', 'min_anchor_height')})
            if len(state['runs']) < 2:
                state.update(stage='start', visible_frames=0, max_translation=0.0, min_anchor_height=None, samples=[])
                return
            finish('passed', 'Two starts, initial visible frames through settling: label stays above its item, zero avoidance translation')
    except Exception as error:
        level.editor_request_end_play()
        finish('failed', str(error))

callback = u.register_slate_post_tick_callback(tick)
