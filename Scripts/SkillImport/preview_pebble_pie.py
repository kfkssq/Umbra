"""Temporary rendered PIE preview of the retargeted cast; saves no map or BP."""
import json
import time
from pathlib import Path
import unreal as u

out = Path(u.Paths.project_saved_dir()).resolve() / 'SkillImport/GlintstonePebble'
animation = u.load_asset('/Game/Skills/Staging/GlintstonePebble/RetargetV4/AS_Greystone_Pebble_Cast_a401_045010')
assert animation and animation.get_editor_property('skeleton')
level = u.get_editor_subsystem(u.LevelEditorSubsystem)
editor = u.get_editor_subsystem(u.UnrealEditorSubsystem)
previous = u.UmbraEquipmentNetworkTestLibrary.configure_play_settings(0, 1, False)
actors = u.get_editor_subsystem(u.EditorActorSubsystem)
camera_template = actors.spawn_actor_from_class(u.CameraActor, u.Vector(0, 0, 0))
camera_template.set_editor_property('tags', ['UmbraPebblePreview'])
started = time.monotonic()
state = {'stage': 'start', 'samples': [], 'animation': animation.get_path_name()}
times = [0.0, 0.5, 1.0, 1.45]
u.EditorPythonScripting.set_keep_python_script_alive(True)

def finish(error=''):
    state['result'] = 'failed' if error else 'passed'
    state['error'] = error
    u.UmbraEquipmentNetworkTestLibrary.configure_play_settings(previous[0], previous[1], bool(previous[2]))
    actors.destroy_actor(camera_template)
    (out/'retarget-pie.json').write_text(json.dumps(state, indent=2), encoding='utf-8')
    u.unregister_slate_post_tick_callback(callback)
    u.EditorPythonScripting.set_keep_python_script_alive(False)
    u.SystemLibrary.quit_editor()

def tick(delta):
    global player, camera, pc, world
    try:
        now = time.monotonic()
        assert now-started < 180, 'Preview timed out: '+state['stage']
        if state['stage'] == 'start':
            level.editor_request_begin_play()
            state['stage'] = 'wait'
        elif state['stage'] == 'wait' and level.is_in_play_in_editor() and now-started > 12:
            world = editor.get_game_world()
            player = u.GameplayStatics.get_player_character(world, 0)
            pc = u.GameplayStatics.get_player_controller(world, 0)
            if not player or not pc:
                return
            position = player.get_actor_location()
            location = position + u.Vector(350, 350, 150)
            rotation = u.MathLibrary.find_look_at_rotation(location, position + u.Vector(0,0,35))
            camera = next(a for a in u.GameplayStatics.get_all_actors_of_class(world, u.CameraActor)
                          if a.actor_has_tag('UmbraPebblePreview'))
            camera.set_actor_location_and_rotation(location, rotation, False, False)
            camera.get_editor_property('camera_component').set_editor_property('field_of_view', 40.0)
            pc.set_view_target_with_blend(camera, 0.0)
            player.mesh.play_animation(animation, False)
            player.mesh.set_play_rate(0.0)
            state['stage'] = 'pose'
            state['at'] = now
        elif state['stage'] == 'pose':
            index = len(state['samples'])
            player.mesh.set_position(times[index], False)
            state['stage'] = 'capture'
            state['at'] = now
        elif state['stage'] == 'capture' and now-state['at'] > 2:
            index = len(state['samples'])
            filename = out / ('retarget-pose-'+str(index)+'.png')
            bones = {name: str(player.mesh.get_socket_location(name)) for name in ['head','pelvis','hand_l','hand_r','foot_l','foot_r']}
            state['samples'].append(dict(time=times[index], screenshot=str(filename), bones=bones))
            state['stage'] = 'next'
            state['at'] = now
            # Screenshot requests can pump Slate recursively: advance state first.
            u.SystemLibrary.execute_console_command(world, 'HighResShot 1280x720 filename="'+filename.as_posix()+'"', pc)
        elif state['stage'] == 'next' and now-state['at'] > 3:
            if len(state['samples']) < len(times):
                state['stage'] = 'pose'
            else:
                assert all(Path(row['screenshot']).exists() for row in state['samples']), 'Missing screenshot'
                level.editor_request_end_play()
                state['stage'] = 'finish'
        elif state['stage'] == 'finish' and not level.is_in_play_in_editor():
            finish()
    except Exception as error:
        if level.is_in_play_in_editor():
            level.editor_request_end_play()
        finish(str(error))

callback = u.register_slate_post_tick_callback(tick)
