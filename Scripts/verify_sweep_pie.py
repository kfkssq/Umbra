"""Live keyboard acceptance recorder. Press Q/E in the PIE viewport; records real GAS/UI state.
Creates only transient PIE fixtures and leaves the editor open. No assets or maps are saved.
"""
import json
import os
import time
import unreal as u

u.EditorPythonScripting.set_keep_python_script_alive(True)
level = u.get_editor_subsystem(u.LevelEditorSubsystem)
editor = u.get_editor_subsystem(u.UnrealEditorSubsystem)
started = time.monotonic()
state = {'requested': False, 'prepared': False, 'samples': [], 'result': 'waiting_for_keyboard'}
target = None
last_sample = 0.
captured = set()
out = os.path.join(u.Paths.project_saved_dir(), 'SkillPrototype', 'keyboard-acceptance.json')
def write():
    with open(out, 'w', encoding='utf-8') as f: json.dump(state, f, ensure_ascii=False, indent=2)
def tick(delta):
    global target, last_sample
    try:
        age = time.monotonic() - started
        if age < 5: return
        if not state['requested']:
            state['requested'] = True
            level.editor_request_begin_play()
            return
        if level.is_in_play_in_editor():
            world = editor.get_game_world()
            if not world: return
            player = u.GameplayStatics.get_player_character(world, 0)
            if not player: return
            if not state['prepared']:
                target = u.UmbraSkillEditorLibrary.prepare_skill_test(world)
                if not target: return
                state['prepared'] = True
                u.log('SKILL_KEYBOARD_READY press Q then E after recovery; recorder active')
            if age - last_sample < .1: return
            last_sample = age
            widgets = u.WidgetLibrary.get_all_widgets_of_class(world, u.UmbraSkillSlotWidget, False)
            slots=[]
            for widget in widgets:
                view = widget.read_view_state()
                slots.append(dict(index=widget.get_editor_property('slot_index'),
                                  cooldown=view.cooldown_remaining, fraction=view.cooldown_fraction,
                                  granted=view.granted, active=view.active,
                                  text=u.UmbraSkillEditorLibrary.read_cooldown_text(widget)))
                elapsed = view.cooldown_duration - view.cooldown_remaining
                index = widget.get_editor_property('slot_index')
                if index not in captured and view.active and .45 < elapsed < .9:
                    captured.add(index)
                    u.SystemLibrary.execute_console_command(world, 'Shot showui')
                    u.log('SKILL_SCREENSHOT slot=' + str(index))
            sample=dict(t=age, resource=u.UmbraSkillEditorLibrary.read_test_attribute(player,'Resource'),
                        health=u.UmbraSkillEditorLibrary.read_test_attribute(target,'Health'), slots=slots)
            state['samples'].append(sample)
            if len(state['samples']) % 10 == 0: write()
        if state['prepared'] and not level.is_in_play_in_editor():
            state['result']='recorded'
            write()
            u.unregister_slate_post_tick_callback(callback)
            u.EditorPythonScripting.set_keep_python_script_alive(False)
        if age > 900:
            state['result']='timeout'
            write()
            u.unregister_slate_post_tick_callback(callback)
            u.EditorPythonScripting.set_keep_python_script_alive(False)
    except Exception as error:
        state['result']='failed'
        state['error']=str(error)
        write()
        u.log_error('SKILL_KEYBOARD_RECORDER ' + str(error))
        u.unregister_slate_post_tick_callback(callback)
        u.EditorPythonScripting.set_keep_python_script_alive(False)
callback=u.register_slate_post_tick_callback(tick)
