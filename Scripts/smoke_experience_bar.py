"""Rendered PIE check for the static XP strip; no ability, experience or input logic."""
import unreal as u, time, os, json
level=u.get_editor_subsystem(u.LevelEditorSubsystem)
editor=u.get_editor_subsystem(u.UnrealEditorSubsystem)
previous=u.UmbraEquipmentNetworkTestLibrary.configure_play_settings(0,1,True)
u.EditorPythonScripting.set_keep_python_script_alive(True)
start=time.monotonic(); state={'stage':'start','samples':[]}
def finish(result,error=''):
    state.update(result=result,error=error)
    u.UmbraEquipmentNetworkTestLibrary.configure_play_settings(previous[0],previous[1],bool(previous[2]))
    with open(os.path.join(u.Paths.project_saved_dir(),'SkillHUD','experience-pie.json'),'w',encoding='utf-8') as f:json.dump(state,f,ensure_ascii=False,indent=2)
    u.unregister_slate_post_tick_callback(callback)
    u.EditorPythonScripting.set_keep_python_script_alive(False)
    u.SystemLibrary.quit_editor()
def tick(delta):
    try:
        assert time.monotonic()-start<120,'PIE timed out'
        if state['stage']=='start':
            level.editor_request_begin_play();state['stage']='wait';return
        if state['stage']=='wait' and level.is_in_play_in_editor() and time.monotonic()-start>10:
            world=editor.get_game_world()
            widgets=u.WidgetLibrary.get_all_widgets_of_class(world,u.UserWidget,False)
            matches=[w for w in widgets if w.get_class().get_name()=='WBP_PlayerExperienceBar_C']
            assert len(matches)==1,'Experience widget count mismatch'
            xp=matches[0]
            assert xp.get_parent().get_name()=='VerticalBox_126'
            find=u.UmbraSkillEditorLibrary.find_presentation_widget
            progress=find(xp,'W_ProgressBar_Red')
            material=progress.get_editor_property('DynamicMaterial')
            assert material,'Source progress template did not construct material'
            value=material.get_scalar_parameter_value('Value')
            assert abs(value-.5)<.001,'Placeholder fill was overwritten'
            assert find(xp,'ValueText').get_visibility()==u.SlateVisibility.COLLAPSED
            state['samples'].append(value)
            if len(state['samples'])==1:
                state['material']=str(progress.get_editor_property('ProgressBarMaterial'))
                for w in widgets:
                    if isinstance(w,u.UmbraAttributeDebugPanel):w.set_visibility(u.SlateVisibility.COLLAPSED)
                state['capture_at']=time.monotonic()+2
            if time.monotonic()>state['capture_at']:
                u.SystemLibrary.execute_console_command(world,'Shot SHOWUI',u.GameplayStatics.get_player_controller(world,0))
                state['stage']='capture';state['stop_at']=time.monotonic()+1
        if state['stage']=='capture' and time.monotonic()>state['stop_at']:
            level.editor_request_end_play();state['stage']='finish'
        if state['stage']=='finish' and not level.is_in_play_in_editor():finish('passed')
    except Exception as e:
        u.log_error('EXPERIENCE_PIE_FAILED '+str(e))
        if level.is_in_play_in_editor():level.editor_request_end_play()
        finish('failed',str(e))
callback=u.register_slate_post_tick_callback(tick)
