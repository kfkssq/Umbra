"""Load the persisted lab and test possession, locomotion, montage and Niagara through real GAS input."""
import json,time
from pathlib import Path
import unreal as u
out=Path(u.Paths.project_saved_dir()).resolve()/'SkillImport/NativeBaseline'
level=u.get_editor_subsystem(u.LevelEditorSubsystem)
editor=u.get_editor_subsystem(u.UnrealEditorSubsystem)
lib=u.UmbraSkillEditorLibrary
assert level.load_level('/Game/SkillLab/Maps/L_MannySkillLab')
previous=u.UmbraEquipmentNetworkTestLibrary.configure_play_settings(0,1,False)
visual='-MannyVisualPreview' in u.SystemLibrary.get_command_line()
report_path=out/('manny-visual.json' if visual else 'manny-pie.json')
state={'stage':'start','cases':[],'visual':visual}
cases=['hit'] if visual else ['hit','wall','cancel_before','cancel_after','miss','poor']
started=time.monotonic();world=player=target=slot=wall=current=None
definition=u.load_asset('/Game/SkillLab/DA_MannyArcBolt')
burst=definition.get_editor_property('impact_niagara')
previous_cd=u.SystemLibrary.get_console_variable_int_value('umbra.Skills.IgnoreCooldowns')
u.EditorPythonScripting.set_keep_python_script_alive(True)
def finish(error=''):
    state.update(result='failed' if error else 'passed',error=error)
    if world:
        u.GameplayStatics.set_global_time_dilation(world,1.)
        u.SystemLibrary.execute_console_command(world,'umbra.Skills.IgnoreCooldowns '+str(previous_cd))
    if level.is_in_play_in_editor():level.editor_request_end_play()
    u.UmbraEquipmentNetworkTestLibrary.configure_play_settings(previous[0],previous[1],bool(previous[2]))
    report_path.write_text(json.dumps(state,indent=2),encoding='utf-8')
    u.unregister_slate_post_tick_callback(callback)
    u.EditorPythonScripting.set_keep_python_script_alive(False);u.SystemLibrary.quit_editor()
def capture(name):
    if visual:u.SystemLibrary.execute_console_command(world,'HighResShot 1280x720 filename="'+(out/('manny-'+name+'.png')).as_posix()+'"')
def tick(delta):
    global world,player,target,slot,wall,current
    try:
        now=time.monotonic();assert now-started<240,'Timeout '+state['stage']
        if state['stage']=='start':
            level.editor_request_begin_play();state['stage']='prepare';return
        if state['stage']=='prepare' and level.is_in_play_in_editor() and now-started>10:
            world=editor.get_game_world();player=u.GameplayStatics.get_player_character(world,0)
            if not player:return
            assert player.get_class().get_name()=='BP_MannyLab_C','Wrong pawn'
            assert player.mesh.get_skeletal_mesh_asset().get_name()=='SKM_Manny_Simple'
            assert player.mesh.get_anim_instance().get_class().get_name()=='ABP_MannyLab_C'
            rotation=player.mesh.get_editor_property('relative_rotation')
            assert abs(rotation.pitch)<.01 and abs(rotation.roll)<.01 and abs(rotation.yaw+90)<.01,'Mesh rotation invalid'
            state['pawn']=player.get_class().get_path_name()
            state['spawn_location']=str(player.get_actor_location())
            state['standing_height_cm']=player.mesh.get_socket_location('head').z-player.mesh.get_socket_location('foot_r').z
            assert state['standing_height_cm']>100,'Body is not upright'
            widgets=u.WidgetLibrary.get_all_widgets_of_class(world,u.UserWidget,False)
            slot=next(w for w in widgets if isinstance(w,u.UmbraSkillSlotWidget) and w.get_editor_property('slot_index')==0)
            assert slot.read_view_state().granted and str(slot.read_view_state().key_label)=='1'
            assert slot.read_view_state().icon==definition.get_editor_property('icon'),'Skill icon mismatch'
            state['hud_icon']=slot.read_view_state().icon.get_path_name()
            state['hud_key']=str(slot.read_view_state().key_label)
            if visual:u.SystemLibrary.execute_console_command(world,'Shot SHOWUI')
            u.SystemLibrary.execute_console_command(world,'umbra.Skills.IgnoreCooldowns 0')
            state['move_origin']=[player.get_actor_location().x,player.get_actor_location().y]
            state['move_start']=u.GameplayStatics.get_time_seconds(world);state['stage']='move';return
        if state['stage']=='move':
            t=u.GameplayStatics.get_time_seconds(world)-state['move_start']
            if t<.7:
                assert lib.inject_movement_test_input(player,u.Vector2D(-1,0))
                if player.mesh.get_anim_instance().get_editor_property('moving'):state['locomotion_seen']=True
                return
            origin=state['move_origin'];pos=player.get_actor_location()
            state['movement_cm']=((pos.x-origin[0])**2+(pos.y-origin[1])**2)**.5
            assert state['movement_cm']>100 and state.get('locomotion_seen'),'Locomotion failed'
            if t<1.5:return
            assert not player.mesh.get_anim_instance().get_editor_property('moving'),'Idle transition failed'
            state['click_cases']=[];state['stage']='click_prepare';state['click_setup_time']=u.GameplayStatics.get_time_seconds(world)
            player.set_actor_location(u.Vector(100,450,95),False,True)
            return
        if state['stage']=='click_prepare':
            if u.GameplayStatics.get_time_seconds(world)-state['click_setup_time']<1.5:return
            destination=u.Vector(100,1050,0)
            path=lib.get_lab_navigation_path(player,destination)
            assert path,'Navigation path missing'
            state['click_path_points']=[str(p) for p in path]
            assert len(state['click_path_points'])>2,'Wall avoidance path missing'
            assert lib.inject_lab_ground_click(player,destination,True)
            state['click_start']=u.GameplayStatics.get_time_seconds(world);state['stage']='click_wait';return
        if state['stage']=='click_wait':
            t=u.GameplayStatics.get_time_seconds(world)-state['click_start']
            if not state.get('click_released') and t>.08:
                assert lib.inject_lab_ground_click(player,u.Vector(100,1050,0),False)
                state['click_released']=True
            pos=player.get_actor_location();distance=((pos.x-100)**2+(pos.y-1050)**2)**.5
            state['click_distance_cm']=distance
            if distance<85 and t>.5:
                state['click_result']='passed';state['stage']='next'
                if visual:u.GameplayStatics.set_global_time_dilation(world,.2)
            else:
                assert t<12,'Click movement failed to reach destination';return
        if state['stage']=='next':
            if slot.read_view_state().cooldown_remaining>0:return
            if not cases:finish();return
            name=cases.pop(0);target=lib.prepare_skill_test(world);assert target
            if name=='poor':lib.set_skill_test_attribute(player,'Resource',0.)
            current=dict(name=name,start=u.GameplayStatics.get_time_seconds(world),initial_health=lib.read_test_attribute(target,'Health'),
                max_projectiles=0,health_changes=0,max_bursts=0,flight_seen=False,impact_stop_seen=False)
            state['cases'].append(current)
            assert lib.inject_skill_test_input(player,0)
            state['stage']='cast';return
        if state['stage']=='cast':
            t=u.GameplayStatics.get_time_seconds(world)-current['start'];view=slot.read_view_state()
            if t>.04 and not current.get('placed'):
                target.set_actor_location(player.get_actor_location()+player.get_actor_forward_vector()*(5000 if current['name']=='miss' else 600),False,True)
                if current['name']=='wall':
                    wall=lib.spawn_skill_test_wall(player)
                    wall.set_actor_location(player.get_actor_location()+player.get_actor_forward_vector()*220+u.Vector(0,0,100),False,True)
                    wall.set_actor_rotation(u.Rotator(pitch=0,yaw=player.get_actor_rotation().yaw-90,roll=0),False)
                current['placed']=True
            projectiles=u.GameplayStatics.get_all_actors_of_class(world,u.UmbraSkillProjectile)
            current['max_projectiles']=max(current['max_projectiles'],len(projectiles))
            for p in projectiles:
                component=next(c for c in p.get_components_by_class(u.NiagaraComponent) if c.get_name()=='FlightNiagara')
                if component.is_visible():
                    assert component.is_active();current['flight_seen']=True
                else:
                    assert not component.is_active();current['impact_stop_seen']=True
            health=lib.read_test_attribute(target,'Health');resource=lib.read_test_attribute(player,'Resource')
            if health<current.get('last_health',current['initial_health'])-.001:current['health_changes']+=1
            current['last_health']=health
            if t>.12 and not current.get('checked'):
                current['checked']=True
                if current['name']=='poor':assert not view.active and resource==0 and view.cooldown_remaining==0
                else:
                    assert view.active and abs(resource-93)<.01 and view.cooldown_remaining>0,'Skill activation/cost failed'
                    assert player.mesh.get_anim_instance().get_current_active_montage()==definition.get_editor_property('montage'),'Montage not playing'
                    assert any(c.is_active() for c in player.get_components_by_class(u.NiagaraComponent)),'Cast missing'
                    current['montage_seen']=True
                    assert lib.inject_skill_test_input(player,0)
            cancel=.15 if current['name']=='cancel_before' else .7 if current['name']=='cancel_after' else None
            if cancel and t>cancel and not current.get('cancelled'):
                lib.interrupt_skill_test(player,True);current['cancelled']=True
            if t-current.get('last_check',-1)>.1:
                current['last_check']=t
                bursts=[c for a in u.GameplayStatics.get_all_actors_of_class(world,u.Actor)
                    for c in a.get_components_by_class(u.NiagaraComponent) if c.get_asset()==burst and c.is_active()]
                current['bursts']=len(bursts);current['max_bursts']=max(current['max_bursts'],len(bursts))
            if visual:
                if t>.3 and not current.get('cast_capture'):current['cast_capture']=True;capture('cast')
                if t>.85 and projectiles and health==current['initial_health'] and not current.get('flight_capture'):current['flight_capture']=True;capture('flight')
                if health<current['initial_health'] and not current.get('impact_capture'):current['impact_capture']=True;capture('impact')
            if t>8:
                assert not view.active and not projectiles,'Lifecycle leak'
                assert abs(resource-(0 if current['name']=='poor' else 93))<.01,'Wrong cost'
                damage=current['initial_health']-health;current['damage']=damage
                if current['name'] in ['hit','cancel_after']:assert abs(damage-24)<.01 and current['health_changes']==1,'Damage mismatch'
                else:assert damage==0,'Unexpected damage'
                if current['name'] in ['poor','cancel_before']:assert current['max_projectiles']==0
                else:
                    assert current['max_projectiles']==1 and current['flight_seen'] and current['max_bursts']>0
                    if current['name']!='miss':assert current['impact_stop_seen']
                assert current.get('bursts',0)==0 and not any(c.is_active() for c in player.get_components_by_class(u.NiagaraComponent)),'VFX leak'
                assert view.cooldown_remaining==0
                current['result']='passed'
                if wall:wall.destroy_actor();wall=None
                state['stage']='next'
    except Exception as error:
        u.log_error('MANNY_PIE_FAILED '+str(error));finish(str(error))
callback=u.register_slate_post_tick_callback(tick)
