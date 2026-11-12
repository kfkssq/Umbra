"""No-CD replay, actual montage/root pose and real-GE clockwise HUD check in rendered PIE."""
import unreal as u,time,json,os
lib=u.UmbraSkillEditorLibrary
level=u.get_editor_subsystem(u.LevelEditorSubsystem);editor=u.get_editor_subsystem(u.UnrealEditorSubsystem)
previous=u.UmbraEquipmentNetworkTestLibrary.configure_play_settings(0,1,True)
u.EditorPythonScripting.set_keep_python_script_alive(True)
start=time.monotonic();state={'stage':'start','casts':[]};cases=[(2,3.6),(2,3.6),(5,4.8)]
player=target=world=None;slots={};current=None;capture=set()
def command(text):u.SystemLibrary.execute_console_command(world,text)
def now():return u.GameplayStatics.get_time_seconds(world)
def finish(error=''):
    state.update(result='failed' if error else 'passed',error=error)
    if level.is_in_play_in_editor():command('umbra.Skills.IgnoreCooldowns 1')
    u.UmbraEquipmentNetworkTestLibrary.configure_play_settings(previous[0],previous[1],bool(previous[2]))
    with open(os.path.join(u.Paths.project_saved_dir(),'SkillPrototype','refinement-pie.json'),'w',encoding='utf-8') as f:json.dump(state,f,ensure_ascii=False,indent=2)
    u.unregister_slate_post_tick_callback(callback);u.EditorPythonScripting.set_keep_python_script_alive(False);u.SystemLibrary.quit_editor()
def tick(delta):
    global player,target,world,slots,current,capture
    try:
        assert time.monotonic()-start<180,'timeout '+state['stage']
        if state['stage']=='start':level.editor_request_begin_play();state['stage']='wait';return
        if state['stage']=='wait' and level.is_in_play_in_editor() and time.monotonic()-start>8:
            world=editor.get_game_world();player=u.GameplayStatics.get_player_character(world,0)
            if not player:return
            p=player.get_actor_location();player.set_actor_location(u.Vector(p.x-1000,p.y,p.z),False,True)
            player.get_components_by_class(u.SpringArmComponent)[0].set_editor_property('target_arm_length',750.)
            widgets=u.WidgetLibrary.get_all_widgets_of_class(world,u.UserWidget,False)
            slots={w.get_editor_property('slot_index'):w for w in widgets if isinstance(w,u.UmbraSkillSlotWidget)}
            for w in widgets:
                if isinstance(w,u.UmbraAttributeDebugPanel):w.set_visibility(u.SlateVisibility.COLLAPSED)
            # Do not force the switch here: the first two casts prove the saved default is no-CD.
            state['stage']='next'
        if state['stage']=='next':
            target=lib.prepare_skill_test(world);assert target
            lib.set_skill_test_attribute(player,'HealthRegen',0);lib.set_skill_test_attribute(player,'Health',60)
            if not cases:
                command('umbra.Skills.IgnoreCooldowns 0');assert lib.inject_skill_test_input(player,1)
                state['clock_start']=now();state['clock']=[];capture=set();state['stage']='clock';return
            index,duration=cases.pop(0);capture=set()
            current={'slot':index,'duration':duration,'start':now(),'samples':[]};state['casts'].append(current)
            assert lib.inject_skill_test_input(player,index);state['stage']='cast';return
        if state['stage']=='cast':
            elapsed=now()-current['start'];index=current['slot'];view=slots[index].read_view_state()
            montage=player.mesh.get_anim_instance().get_current_active_montage()
            root=player.mesh.get_socket_location('root');hand=player.mesh.get_socket_location('hand_r');p=player.get_actor_location()
            sample=dict(t=elapsed,active=view.active,cd=view.cooldown_remaining,resource=lib.read_test_attribute(player,'Resource'),
                        health=lib.read_test_attribute(player,'Health'),target_health=lib.read_test_attribute(target,'Health'),
                        root_height=root.z-p.z+90,hand=[hand.x-p.x,hand.y-p.y,hand.z-p.z],montage=montage.get_name() if montage else None)
            current['samples'].append(sample)
            assert view.cooldown_remaining==0,'Saved no-CD switch did not apply'
            if .15<elapsed<.5:assert view.active,'Input failed to activate'
            points=[.25,1.5,2.6] if index==2 else [1.1,2.3,3.1,3.4]
            for point in points:
                if elapsed>=point and point not in capture:
                    command('Shot SHOWUI');capture.add(point);u.log('REFINEMENT_CAPTURE slot='+str(index)+' phase='+str(point));break
            if elapsed>current['duration']:
                assert not view.active,'Action lock not released'
                assert abs(sample['resource']-(100-view.resource_cost))<.01,'Resource cost changed'
                if index==2:
                    assert abs(sample['target_health']-9973)<.1,'Expected six storm pulses'
                    assert any(s['t']>2.5 and s['montage']=='AM_SwordStorm' for s in current['samples']),'Storm montage stopped before final pulses'
                    hands=[s['hand'] for s in current['samples'] if 1.<s['t']<2.8]
                    assert max(v[0] for v in hands)-min(v[0] for v in hands)>15,'Storm final skeletal pose did not animate'
                else:
                    assert abs(sample['health']-90)<.01 and abs(sample['target_health']-9987.5)<.1
                    assert max(s['root_height'] for s in current['samples'])>180,'Second phase never left ground'
                    assert max(s['root_height'] for s in current['samples'])<240,'Root exceeded bounded visual height'
                    assert all(s['target_health']==10000 and s['health']==60 for s in current['samples'] if s['t']<3.30),'Impact fired before slam'
                    assert abs(sample['root_height'])<5,'Slam did not return to ground'
                current['result']='passed';state['stage']='next'
        if state['stage']=='clock':
            elapsed=now()-state['clock_start'];w=slots[1];view=w.read_view_state()
            image=lib.find_presentation_widget(w,'Img_Cooldown');assert image
            if .15<elapsed<5.8:
                assert view.cooldown_remaining>0
                assert image.get_visibility()==u.SlateVisibility.HIT_TEST_INVISIBLE
                fraction=image.get_dynamic_material().get_scalar_parameter_value('Remaining')
                assert abs(fraction-view.cooldown_fraction)<.03,'Material not driven by GAS fraction'
                assert lib.find_presentation_widget(w,'Pb_Cooldown').get_visibility()==u.SlateVisibility.HIDDEN
                state['clock'].append(dict(t=elapsed,fraction=fraction,cd=view.cooldown_remaining))
            for point in [1.5,3.,4.5]:
                if elapsed>=point and point not in capture:
                    command('Shot SHOWUI');capture.add(point);u.log('CLOCK_CAPTURE elapsed='+str(point));break
            if elapsed>6.3:
                assert view.cooldown_remaining==0 and image.get_visibility()==u.SlateVisibility.HIDDEN
                command('umbra.Skills.IgnoreCooldowns 1');level.editor_request_end_play();state['stage']='finish'
        if state['stage']=='finish' and not level.is_in_play_in_editor():finish()
    except Exception as error:
        u.log_error('SKILL_REFINEMENT_FAILED '+str(error))
        if level.is_in_play_in_editor():level.editor_request_end_play()
        finish(str(error))
callback=u.register_slate_post_tick_callback(tick)
