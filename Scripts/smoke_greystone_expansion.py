"""Rendered PIE acceptance through real Enhanced Input bindings; no map/fixture assets saved."""
import unreal as u
import time, json, os

lib=u.UmbraSkillEditorLibrary
level=u.get_editor_subsystem(u.LevelEditorSubsystem)
editor=u.get_editor_subsystem(u.UnrealEditorSubsystem)
previous=u.UmbraEquipmentNetworkTestLibrary.configure_play_settings(0,1,True)
previous_ignore_cd=u.SystemLibrary.get_console_variable_int_value('umbra.Skills.IgnoreCooldowns')
u.EditorPythonScripting.set_keep_python_script_alive(True)
start=time.monotonic()
state={'stage':'start','cases':[], 'input':'Enhanced Input injection, not OS keyboard'}
cases=[(0,False,1.5),(1,False,1.5),(2,False,3.5),(3,False,3.),(4,False,3.),(5,False,4.8),
       (2,True,3.5),(3,True,3.),(4,True,3.),(5,True,4.8),(3,'wall',3.)]
current=None
player=target=world=None
slots={}
wall=None

def finish(result,error=''):
    state.update(result=result,error=error)
    if level.is_in_play_in_editor() and world:
        u.SystemLibrary.execute_console_command(world,'umbra.Skills.IgnoreCooldowns '+str(previous_ignore_cd))
    u.UmbraEquipmentNetworkTestLibrary.configure_play_settings(previous[0],previous[1],bool(previous[2]))
    with open(os.path.join(u.Paths.project_saved_dir(),'SkillPrototype','expansion-pie.json'),'w',encoding='utf-8') as f:json.dump(state,f,ensure_ascii=False,indent=2)
    u.unregister_slate_post_tick_callback(callback)
    u.EditorPythonScripting.set_keep_python_script_alive(False)
    u.SystemLibrary.quit_editor()

def sample():
    view=slots[current['slot']].read_view_state()
    p=player.get_actor_location()
    return dict(t=u.GameplayStatics.get_time_seconds(world)-current['start'],resource=lib.read_test_attribute(player,'Resource'),
                target_health=lib.read_test_attribute(target,'Health'),health=lib.read_test_attribute(player,'Health'),
                armor=lib.read_test_attribute(player,'Armor'),resist=lib.read_test_attribute(player,'SlashingResistance'),
                active=view.active,cooldown=view.cooldown_remaining,fraction=view.cooldown_fraction,
                mesh_root_height=player.mesh.get_socket_location('root').z-p.z,
                text=lib.read_cooldown_text(slots[current['slot']]),location=[p.x,p.y,p.z])

def tick(delta):
    global current,player,target,world,slots,wall
    try:
        assert time.monotonic()-start<300,'Timeout: '+state['stage']
        if state['stage']=='start':
            level.editor_request_begin_play();state['stage']='prepare';return
        if state['stage']=='prepare' and level.is_in_play_in_editor() and time.monotonic()-start>8:
            world=editor.get_game_world();player=u.GameplayStatics.get_player_character(world,0)
            if not player:return
            # This suite explicitly verifies authored cooldowns, regardless of the iteration default.
            u.SystemLibrary.execute_console_command(world,'umbra.Skills.IgnoreCooldowns 0')
            widgets=u.WidgetLibrary.get_all_widgets_of_class(world,u.UserWidget,False)
            slots={w.get_editor_property('slot_index'):w for w in widgets if isinstance(w,u.UmbraSkillSlotWidget)}
            assert len(slots)==10,'Expected ten slots'
            state['slots']=[]
            find=lib.find_presentation_widget
            for index,w in sorted(slots.items()):
                view=w.read_view_state();entry={'index':index,'key':str(view.key_label),'granted':view.granted}
                if index<6:
                    assert view.granted and view.icon,'Unconfigured active slot'
                    assert str(find(w,'keyboard_shortcut_label').get_text())==str(view.key_label)
                    material=find(w,'W_Item_Slot.item_icon').get_dynamic_material()
                    assert material and material.get_texture_parameter_value('IconTexture')==view.icon,'Icon material mismatch'
                    entry['icon']=view.icon.get_path_name()
                else:assert not view.granted,'Expected unused slot'
                state['slots'].append(entry)
            for w in widgets:
                if isinstance(w,u.UmbraAttributeDebugPanel):w.set_visibility(u.SlateVisibility.COLLAPSED)
            state['stage']='next'
        if state['stage']=='next':
            if not cases:
                u.SystemLibrary.execute_console_command(world,'umbra.Skills.IgnoreCooldowns '+str(previous_ignore_cd))
                level.editor_request_end_play();state['stage']='finish';return
            index,cancel,duration=cases[0]
            if slots[index].read_view_state().cooldown_remaining>0:return
            cases.pop(0)
            target=lib.prepare_skill_test(world)
            assert target
            lib.set_skill_test_attribute(player,'HealthRegen',0.)
            lib.set_skill_test_attribute(player,'Health',60.)
            if index==3:
                p=player.get_actor_location()
                target.set_actor_location(u.Vector(p.x,p.y+450,p.z),False,True)
            is_wall=cancel=='wall'
            if is_wall:
                wall=lib.spawn_skill_test_wall(player)
                assert wall,'Could not create PIE-only wall'
            current={'slot':index,'cancel':cancel is True,'wall':is_wall,'duration':duration,'start':u.GameplayStatics.get_time_seconds(world),'samples':[],
                     'armor_before':lib.read_test_attribute(player,'Armor'),'resist_before':lib.read_test_attribute(player,'SlashingResistance'),
                     'origin':list((player.get_actor_location().x,player.get_actor_location().y,player.get_actor_location().z)),
                     'target_health_before':lib.read_test_attribute(target,'Health')}
            assert lib.inject_skill_test_input(player,index),'No Enhanced Input binding'
            state['cases'].append(current);state['stage']='cast';return
        if state['stage']=='cast':
            s=sample();current['samples'].append(s)
            if s['t']>.12 and not current.get('checked'):
                assert s['active'],'Skill did not activate from input: '+str(current['slot'])
                cost=slots[current['slot']].read_view_state().resource_cost
                assert abs(s['resource']-(100-cost))<.01,'Wrong resource cost'
                assert s['cooldown']>0 and s['text'],'Cooldown UI did not observe GE'
                current['checked']=True
                # Repeated input must not charge twice during cooldown.
                lib.inject_skill_test_input(player,current['slot'])
            if current['cancel'] and s['t']>.18 and not current.get('interrupted'):
                lib.interrupt_skill_test(player,True);current['interrupted']=True
            if not current['cancel'] and s['t']>{0:.5,1:.5,2:.9,3:.45,4:.6,5:2.}[current['slot']] and not current.get('capture'):
                u.SystemLibrary.execute_console_command(world,'Shot SHOWUI')
                u.log('GREYSTONE_CAPTURE slot='+str(current['slot']))
                current['capture']=True
            if s['t']>current['duration']:
                assert current.get('checked')
                assert not s['active'],'Skill remained locked after completion'
                assert abs(s['armor']-current['armor_before'])<.01 and abs(s['resist']-current['resist_before'])<.01,'Guard attributes leaked'
                cost=slots[current['slot']].read_view_state().resource_cost
                assert abs(s['resource']-(100-cost))<.01,'Duplicate resource charge'
                damage=current['target_health_before']-s['target_health']
                current['damage']=damage
                if current['cancel']:
                    assert abs(damage)<.01,'Interrupted windup dealt damage'
                    assert abs(s['health']-60)<.01,'Interrupted recovery healed'
                elif current['wall']:
                    current['travel']=s['location'][1]-current['origin'][1]
                    assert current['travel']<180,'Leap crossed the blocking wall'
                    assert abs(damage)<.01,'Landing damage passed through wall'
                    wall.destroy_actor();wall=None
                elif current['slot']==4:
                    assert abs(damage)<.01,'Guard dealt unexpected damage'
                    assert max(v['armor'] for v in current['samples'])>=current['armor_before']+99.9,'Guard never applied'
                else:
                    assert damage>0,'Skill did not hit fixture: '+str(current['slot'])
                    if current['slot']==2:
                        health=[current['target_health_before']]+[v['target_health'] for v in current['samples']]
                        changes=sum(a>b+.01 for a,b in zip(health,health[1:]))
                        assert changes==6,'Expected six independent pulses, got '+str(changes)
                    if current['slot']==3:
                        current['travel']=s['location'][1]-current['origin'][1]
                        current['apex']=max(v['location'][2] for v in current['samples'])-current['origin'][2]
                        assert current['travel']>200 and current['apex']>50,'Leap did not physically travel'
                    if current['slot']==5:
                        assert abs(s['health']-90)<.01,'Expected 30% MaxHealth restoration'
                        assert max(abs(v['mesh_root_height']) for v in current['samples'])<300,'Recovery mesh left the playable space'
                assert s['cooldown']>0 and s['fraction']<current['samples'][5]['fraction'],'Cooldown failed to decrease'
                current['result']='passed';state['stage']='next'
        if state['stage']=='finish' and not level.is_in_play_in_editor():finish('passed')
    except Exception as e:
        u.log_error('GREYSTONE_EXPANSION_PIE_FAILED '+str(e))
        if level.is_in_play_in_editor():level.editor_request_end_play()
        finish('failed',str(e))
callback=u.register_slate_post_tick_callback(tick)
