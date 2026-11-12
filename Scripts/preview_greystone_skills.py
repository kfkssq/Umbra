"""Preview the source animation and Cascade effect in PIE without saving the map."""
import time
import unreal

unreal.EditorPythonScripting.set_keep_python_script_alive(True)
level = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
editor = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
start = time.monotonic()
state = {'requested': False, 'playing': False, 'fx': 0.0}
def tick(delta):
    age = time.monotonic() - start
    if age < 5: return
    if not state['requested']:
        state['requested'] = True
        level.editor_request_begin_play()
    if level.is_in_play_in_editor():
        world = editor.get_game_world()
        player = unreal.GameplayStatics.get_player_character(world, 0) if world else None
        if not player: return
        if not state['playing']:
            player.mesh.play_animation(unreal.load_asset('/Game/ParagonGreystone/Characters/Heroes/Greystone/Animations/Attack_PrimaryB'), True)
            state['playing'] = True
            unreal.log('GREYSTONE_VISUAL_PREVIEW playing Attack_PrimaryB')
        if age - state['fx'] > 1.4:
            state['fx'] = age
            unreal.GameplayStatics.spawn_emitter_at_location(world,
                unreal.load_asset('/Game/ParagonGreystone/FX/Particles/Greystone/Abilities/ClearAPath/FX/P_Greystone_ClearAPath_Swipe'),
                player.get_actor_location(), player.get_actor_rotation())
    if age > 65:
        level.editor_request_end_play()
        unreal.unregister_slate_post_tick_callback(callback)
        unreal.EditorPythonScripting.set_keep_python_script_alive(False)
callback = unreal.register_slate_post_tick_callback(tick)
