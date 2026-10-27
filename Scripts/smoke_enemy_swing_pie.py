"""Reload saved swing configuration and exercise the enemy montage in PIE, without saving assets."""
import json
import os
import time
import unreal

api = unreal.AnimationLibrary
montage = unreal.load_asset('/Game/Blueprints/Enemies/Animation/AM_Enemy_BasicAttack')
events = api.get_animation_notify_events(montage)
swings = [e for e in events if isinstance(e.get_editor_property('notify'), unreal.UmbraAnimNotify_Audio) and e.get_editor_property('notify').get_editor_property('event') == unreal.UmbraAudioEvent.ATTACK_SWING]
hits = [e for e in events if isinstance(e.get_editor_property('notify_state_class'), unreal.UmbraAnimNotifyState_AttackHitWindow)]
assert len(swings) == 1 and len(hits) == 1
assert abs(api.get_anim_notify_event_trigger_time(swings[0]) - 0.33909455) < 0.001
assert abs(api.get_anim_notify_event_trigger_time(hits[0]) - 0.39909455) < 0.001
sequence = unreal.load_asset('/Game/ParagonMinions/Characters/Minions/Down_Minions/Animations/Melee/Attack_A')
assert not api.get_animation_notify_events(sequence)
lib = unreal.load_asset('/Game/Asset/Audio/Libraries/DA_Audio_Default')
assert any(r.get_editor_property('event') == unreal.UmbraAudioEvent.ATTACK_SWING and r.get_editor_property('sounds') for r in lib.get_editor_property('events'))
unreal.log('ENEMY_SWING_RELOAD_PASSED')
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
level = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
editor = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
started = time.monotonic()
state = {'requested': False, 'checked': False, 'ended': False}

def finish(result, detail):
    state.update(result=result, detail=detail)
    with open(os.path.join(unreal.Paths.project_saved_dir(), 'EnemySwingPIE.json'), 'w') as f:
        json.dump(state, f, indent=2)
    unreal.log('ENEMY_SWING_PIE ' + result + ' ' + detail)
    unreal.unregister_slate_post_tick_callback(callback)
    unreal.EditorPythonScripting.set_keep_python_script_alive(False)
    unreal.SystemLibrary.quit_editor()

def tick(delta):
    try:
        age = time.monotonic() - started
        if age > 90:
            level.editor_request_end_play()
            finish('failed', 'PIE timeout')
            return
        if not state['requested']:
            state['requested'] = True
            level.editor_request_begin_play()
            return
        if level.is_in_play_in_editor() and not state['checked']:
            world = editor.get_game_world()
            if not world: return
            enemies = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.UmbraEnemyCharacter)
            if not enemies: raise RuntimeError('No PIE enemies')
            enemy = enemies[0]
            assert enemy.get_component_by_class(unreal.UmbraAudioComponent)
            duration = enemy.play_anim_montage(montage)
            assert duration > 0
            state.update(checked=True, checked_at=age, enemy=enemy.get_name(), montage_duration=duration)
        if state['checked'] and not state['ended'] and age - state['checked_at'] > 5:
            level.editor_request_end_play()
            state['ended'] = True
        if state['ended'] and not level.is_in_play_in_editor():
            finish('passed', 'Saved notify reload, enemy audio component, montage starts, five seconds PIE and clean EndPIE; no audible-output assertion')
    except Exception as error:
        level.editor_request_end_play()
        finish('failed', str(error))
callback = unreal.register_slate_post_tick_callback(tick)
