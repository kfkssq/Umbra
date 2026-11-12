"""Real rendered PIE: verifies saved HUD nesting, geometry, one sweep and actual cooldown widgets."""
import unreal as u
import json,time,os
level=u.get_editor_subsystem(u.LevelEditorSubsystem)
editor=u.get_editor_subsystem(u.UnrealEditorSubsystem)
previous=u.UmbraEquipmentNetworkTestLibrary.configure_play_settings(0,1,True)
u.EditorPythonScripting.set_keep_python_script_alive(True)
start=time.monotonic()
keyboard='SkillHUDKeyboard' in u.SystemLibrary.get_command_line()
state={'stage':'start','samples':[],'activation':'OS keyboard' if keyboard else 'ASC'}
target=None
pc=None
cast_time=0

def check(condition,message):
    if not condition: raise RuntimeError(message)

def rect(w):
    g=w.get_cached_geometry()
    size=u.SlateLibrary.get_local_size(g)
    a=u.SlateLibrary.local_to_absolute(g,u.Vector2D(0,0))
    b=u.SlateLibrary.local_to_absolute(g,size)
    return dict(x=a.x,y=a.y,w=b.x-a.x,h=b.y-a.y)

def finish(result,error=''):
    state.update(result=result,error=error)
    u.UmbraEquipmentNetworkTestLibrary.configure_play_settings(previous[0],previous[1],bool(previous[2]))
    with open(os.path.join(u.Paths.project_saved_dir(),'SkillHUD','pie.json'),'w',encoding='utf-8') as f:
        json.dump(state,f,ensure_ascii=False,indent=2)
    u.unregister_slate_post_tick_callback(callback)
    u.EditorPythonScripting.set_keep_python_script_alive(False)
    u.SystemLibrary.quit_editor()

def tick(delta):
    global target,pc,cast_time
    try:
        check(time.monotonic()-start<240,'Timed out in '+state['stage'])
        if state['stage']=='start':
            level.editor_request_begin_play();state['stage']='inspect';return
        if state['stage']=='inspect' and level.is_in_play_in_editor() and time.monotonic()-start>8:
            if 'pie_ready_at' not in state:
                state['pie_ready_at']=time.monotonic()
                return
            if time.monotonic()-state['pie_ready_at']<2: return
            world=editor.get_game_world()
            pc=u.GameplayStatics.get_player_controller(world,0)
            widgets=u.WidgetLibrary.get_all_widgets_of_class(world,u.UserWidget,False)
            bars=[w for w in widgets if w.get_class().get_name()=='WBP_SkillBar_C']
            health=next(w for w in widgets if w.get_class().get_name()=='WBP_PlayerHealthBar_C')
            slots=[w for w in widgets if isinstance(w,u.UmbraSkillSlotWidget)]
            check(len(bars)==1 and len(slots)==10,'Expected one bar with ten template slots')
            check(bars[0].get_parent().get_name()=='VB_SkillsAndVitals','Bar is not embedded in HUD')
            check(all(w.get_owning_player()==pc for w in slots),'Wrong owning player')
            state['presentation']=[]
            find=u.UmbraSkillEditorLibrary.find_presentation_widget
            for w in sorted(slots,key=lambda x:x.get_editor_property('slot_index')):
                idx=w.get_editor_property('slot_index');view=w.read_view_state()
                image=find(w,'W_Item_Slot.item_icon'); key=find(w,'keyboard_shortcut_label')
                cost=find(w,'W_Item_Slot.item_quantity')
                check(image and key and cost,'Source template child missing')
                check(str(key.get_text())==str(view.key_label),'Key label binding missing')
                if idx<2:
                    material=image.get_dynamic_material()
                    check(material and material.get_texture_parameter_value('IconTexture')==view.icon,'Material icon binding missing')
                    check(str(cost.get_text())==str(int(view.resource_cost)),'Cost binding missing')
                else:
                    check(not view.granted and image.get_visibility()==u.SlateVisibility.HIDDEN,'Empty slot shows sample icon')
                    check(cost.get_visibility()==u.SlateVisibility.HIDDEN,'Empty slot shows sample quantity')
                    check(find(w,'W_Item_Slot').get_visibility()==u.SlateVisibility.HIDDEN,'Empty slot shows sample button')
                check(find(w,'ContentBorder').get_editor_property('background').get_editor_property('resource_object')==w.get_editor_property('frame_texture'),'Source instance border missing')
                state['presentation'].append({'index':idx,'key':str(key.get_text()),'cost':str(cost.get_text()),
                    'icon_texture':view.icon.get_path_name() if view.icon else None,
                    'icon':str(image.get_editor_property('brush').get_editor_property('resource_object')),'granted':view.granted})
            bar_rect=rect(bars[0]);health_rect=rect(health)
            state['geometry']={'bar':bar_rect,'health':health_rect,'slots':[rect(w) for w in slots]}
            state['geometry_available']=bar_rect['w']>0 and health_rect['w']>0
            if state['geometry_available']:
                check(abs(bar_rect['w']-health_rect['w'])<2,'Bar must match health width')
                check(abs(bar_rect['x']-health_rect['x'])<2,'Bar must align with health column')
                check(bar_rect['y']+bar_rect['h']<=health_rect['y']+1,'Bar overlaps health')
                for r in state['geometry']['slots']:
                    check(r['x']>=bar_rect['x'] and r['x']+r['w']<=bar_rect['x']+bar_rect['w']+1
                          and r['y']>=bar_rect['y'] and r['y']+r['h']<=bar_rect['y']+bar_rect['h']+1,'Slot outside frame')
            # Hide only the transient debug overlay so the UI capture is readable.
            for w in widgets:
                if isinstance(w,u.UmbraAttributeDebugPanel):w.set_visibility(u.SlateVisibility.COLLAPSED)
            target=u.UmbraSkillEditorLibrary.prepare_skill_test(world)
            check(target,'No target')
            player=u.GameplayStatics.get_player_character(world,0)
            state['initial_resource']=u.UmbraSkillEditorLibrary.read_test_attribute(player,'Resource')
            state['initial_health']=u.UmbraSkillEditorLibrary.read_test_attribute(target,'Health')
            if keyboard:
                state['stage']='wait_key'
                with open(os.path.join(u.Paths.project_saved_dir(),'SkillHUD','keyboard-ready.json'),'w') as f:json.dump({'ready':True},f)
                u.log('SKILL_HUD_READY_FOR_Q')
                return
            asc=u.AbilitySystemLibrary.get_ability_system_component(player)
            check(asc.try_activate_ability_by_class(u.UmbraSweepAbility),'Sweep activation failed')
            cast_time=time.monotonic();state['stage']='casting';return
        if state['stage']=='wait_key':
            world=editor.get_game_world()
            slots=u.WidgetLibrary.get_all_widgets_of_class(world,u.UmbraSkillSlotWidget,False)
            view=next(w for w in slots if w.get_editor_property('slot_index')==0).read_view_state()
            if view.cooldown_remaining>0:
                cast_time=time.monotonic();state['stage']='casting'
            return
        if state['stage']=='casting':
            world=editor.get_game_world();age=time.monotonic()-cast_time
            slots=u.WidgetLibrary.get_all_widgets_of_class(world,u.UmbraSkillSlotWidget,False)
            slot=next(w for w in slots if w.get_editor_property('slot_index')==0)
            view=slot.read_view_state()
            state['samples'].append({'t':age,'remaining':view.cooldown_remaining,'fraction':view.cooldown_fraction,
                                     'text':u.UmbraSkillEditorLibrary.read_cooldown_text(slot),'active':view.active})
            if .7<age<2 and not state.get('cooldown_capture'):
                check(view.cooldown_remaining>0 and u.UmbraSkillEditorLibrary.read_cooldown_text(slot),'Actual cooldown text missing')
                u.SystemLibrary.execute_console_command(world,'Shot SHOWUI',pc)
                state['cooldown_capture']=True
            if age>6:
                player=u.GameplayStatics.get_player_character(world,0)
                state['final_resource']=u.UmbraSkillEditorLibrary.read_test_attribute(player,'Resource')
                state['final_health']=u.UmbraSkillEditorLibrary.read_test_attribute(target,'Health')
                check(state['initial_resource']-state['final_resource']==15,'Cost mismatch')
                check(state['final_health']<state['initial_health'],'No damage')
                check(view.cooldown_remaining==0 and not view.active,'Did not recover')
                check(not u.UmbraSkillEditorLibrary.read_cooldown_text(slot),'Cooldown text not hidden')
                u.SystemLibrary.execute_console_command(world,'Shot SHOWUI',pc)
                state['stage']='stop';state['stop_at']=time.monotonic();return
        if state['stage']=='stop' and time.monotonic()-state['stop_at']>1:
            level.editor_request_end_play();state['stage']='finish';return
        if state['stage']=='finish' and not level.is_in_play_in_editor(): finish('passed')
    except Exception as error:
        u.log_error('EMBEDDED_SKILL_HUD_TEST '+str(error))
        if level.is_in_play_in_editor():level.editor_request_end_play()
        finish('failed',str(error))
callback=u.register_slate_post_tick_callback(tick)
