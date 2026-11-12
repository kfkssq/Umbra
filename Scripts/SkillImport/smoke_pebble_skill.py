"""Rendered PIE acceptance through the actual Enhanced Input binding; no map saves."""
import json,time
from pathlib import Path
import unreal as u

out=Path(u.Paths.project_saved_dir()).resolve()/'SkillImport/GlintstonePebble'
lib=u.UmbraSkillEditorLibrary
level=u.get_editor_subsystem(u.LevelEditorSubsystem)
editor=u.get_editor_subsystem(u.UnrealEditorSubsystem)
previous=u.UmbraEquipmentNetworkTestLibrary.configure_play_settings(0,1,True)
previous_cd=u.SystemLibrary.get_console_variable_int_value('umbra.Skills.IgnoreCooldowns')
visual_preview='-PebbleVisualPreview' in u.SystemLibrary.get_command_line()
native_smoke='-NativeSkillSmoke' in u.SystemLibrary.get_command_line()
report_path=out/('skill-visual-pie.json' if visual_preview else 'skill-pie.json')
native_definition=None;native_original={}
if native_smoke:
    report_path=out.parent/'NativeBaseline/niagara-pie.json'
    report_path.parent.mkdir(parents=True,exist_ok=True)
    native_definition=u.load_asset('/Game/Skills/GlintstonePebble/DA_GlintstonePebble')
    keys=['active_effect','strike_effect','projectile_effect','impact_effect','active_niagara','strike_niagara','projectile_niagara','impact_niagara']
    native_original={key:native_definition.get_editor_property(key) for key in keys}
    flight=u.load_asset('/Game/SkillLab/Fixtures/NS_TestFlight')
    burst=u.load_asset('/Game/SkillLab/Fixtures/NS_TestBurst')
    assert flight and burst,'Prepare native baseline fixtures first'
    native_definition.set_editor_properties(dict(active_effect=None,strike_effect=None,projectile_effect=None,impact_effect=None,
        active_niagara=flight,strike_niagara=burst,projectile_niagara=flight,impact_niagara=burst))
state={'stage':'start','cases':[],'input':'Real Enhanced Input injection; not OS keyboard'}
state['renderer']='Niagara engineering fixtures' if native_smoke else 'Existing production presentation'
cases=['hit','wall','cancel_before','cancel_after','miss','poor']
if visual_preview:cases=['hit']
started=time.monotonic()
player=target=world=slot=wall=current=None
u.EditorPythonScripting.set_keep_python_script_alive(True)
report_path.write_text(json.dumps({'result':'running'}),encoding='utf-8')

def finish(error=''):
    state.update(result='failed' if error else 'passed',error=error)
    if world and level.is_in_play_in_editor():
        u.GameplayStatics.set_global_time_dilation(world,1.)
        u.SystemLibrary.execute_console_command(world,'umbra.Skills.IgnoreCooldowns '+str(previous_cd))
        level.editor_request_end_play()
    u.UmbraEquipmentNetworkTestLibrary.configure_play_settings(previous[0],previous[1],bool(previous[2]))
    if native_definition:native_definition.set_editor_properties(native_original)
    report_path.write_text(json.dumps(state,ensure_ascii=False,indent=2),encoding='utf-8')
    u.unregister_slate_post_tick_callback(callback)
    u.EditorPythonScripting.set_keep_python_script_alive(False)
    u.SystemLibrary.quit_editor()

def capture(name):
    if not visual_preview:return # HighResShot stalls can skip the entire short flight in lifecycle tests.
    u.SystemLibrary.execute_console_command(world,'HighResShot 1280x720 filename="'+(out/(name+'.png')).as_posix()+'"')

def tick(delta):
    global player,target,world,slot,wall,current
    try:
        assert time.monotonic()-started<220,'Timeout '+state['stage']
        if state['stage']=='start':
            level.editor_request_begin_play();state['stage']='prepare';return
        if state['stage']=='prepare' and level.is_in_play_in_editor() and time.monotonic()-started>10:
            world=editor.get_game_world();player=u.GameplayStatics.get_player_character(world,0)
            if not player:return
            u.SystemLibrary.execute_console_command(world,'umbra.Skills.IgnoreCooldowns 0')
            if visual_preview:u.GameplayStatics.set_global_time_dilation(world,.15)
            widgets=u.WidgetLibrary.get_all_widgets_of_class(world,u.UserWidget,False)
            slot=next(w for w in widgets if isinstance(w,u.UmbraSkillSlotWidget) and w.get_editor_property('slot_index')==6)
            view=slot.read_view_state()
            assert view.granted and view.icon and str(view.key_label)=='5','Missing HUD/grant'
            state['skill']=str(view.name)
            state['icon']=view.icon.get_path_name()
            for w in widgets:
                if isinstance(w,u.UmbraAttributeDebugPanel):w.set_visibility(u.SlateVisibility.COLLAPSED)
            state['stage']='next'
        if state['stage']=='next':
            if slot.read_view_state().cooldown_remaining>0:return
            if not cases:
                finish();return
            name=cases.pop(0)
            target=lib.prepare_skill_test(world)
            assert target,'Missing target fixture'
            if name=='poor':lib.set_skill_test_attribute(player,'Resource',0.)
            current={'name':name,'start':u.GameplayStatics.get_time_seconds(world),'samples':[],'max_projectiles':0,'health_changes':0}
            current['initial_health']=lib.read_test_attribute(target,'Health')
            state['cases'].append(current)
            assert lib.inject_skill_test_input(player,6)
            state['stage']='cast';return
        if state['stage']=='cast':
            t=u.GameplayStatics.get_time_seconds(world)-current['start']
            view=slot.read_view_state()
            # Input may update facing from the mouse. Place fixtures along the actual authoritative aim.
            if not current.get('placed') and t>.04:
                forward=player.get_actor_forward_vector()
                position=player.get_actor_location()
                distance=600 if current['name']!='miss' else 5000
                target.set_actor_location(position+forward*distance,False,True)
                if current['name']=='wall':
                    wall=lib.spawn_skill_test_wall(player)
                    wall.set_actor_location(position+forward*220+u.Vector(0,0,100),False,True)
                    wall.set_actor_rotation(u.Rotator(0,player.get_actor_rotation().yaw-90,0),False)
                current['placed']=True
            projectiles=u.GameplayStatics.get_all_actors_of_class(world,u.UmbraSkillProjectile)
            for projectile in projectiles:
                audio=next(c for c in projectile.get_components_by_class(u.AudioComponent) if c.get_name()=='FlightAudio')
                visual=next(c for c in projectile.get_components_by_class(u.NiagaraComponent if native_smoke else u.ParticleSystemComponent)
                    if c.get_name()==('FlightNiagara' if native_smoke else 'Flight'))
                assert audio.get_editor_property('sound').get_name()=='S_ER_Pebble_Flight_1040010023','Wrong flight source'
                if visual.is_visible():
                    if native_smoke:assert visual.is_active(),'Niagara flight inactive'
                    assert audio.is_playing(),'Flight loop not playing'
                    assert audio.get_attach_parent()==projectile.root_component,'Flight audio detached'
                    current['flight_audio_samples']=current.get('flight_audio_samples',0)+1
                else:
                    if native_smoke:assert not visual.is_active(),'Niagara flight continued after impact'
                    assert not audio.is_playing(),'Flight audio continued after impact'
                    current['stopped_audio_samples']=current.get('stopped_audio_samples',0)+1
            health=lib.read_test_attribute(target,'Health')
            resource=lib.read_test_attribute(player,'Resource')
            samples=current['samples']
            before=samples[-1]['health'] if samples else current['initial_health']
            if health<before-.001:current['health_changes']+=1
            row=dict(t=t,health=health,resource=resource,active=view.active,cooldown=view.cooldown_remaining,projectiles=len(projectiles))
            samples.append(row)
            if native_smoke and t-current.get('last_burst_check',-1)>.1:
                current['last_burst_check']=t
                bursts=[c for a in u.GameplayStatics.get_all_actors_of_class(world,u.Actor)
                    for c in a.get_components_by_class(u.NiagaraComponent)
                    if c.get_asset()==burst and c.is_active()]
                current['max_burst_components']=max(current.get('max_burst_components',0),len(bursts))
                current['active_burst_components']=len(bursts)
            current['max_projectiles']=max(current['max_projectiles'],len(projectiles))
            if t>.12 and not current.get('checked'):
                current['checked']=True
                if current['name']=='poor':
                    assert not view.active and resource==0 and view.cooldown_remaining==0,'Poor cast was accepted'
                else:
                    assert view.active and abs(resource-93)<.01 and view.cooldown_remaining>0,'Cost/activation/cooldown mismatch'
                    if native_smoke:
                        assert any(c.is_active() for c in player.get_components_by_class(u.NiagaraComponent)), 'Cast Niagara missing'
                    lib.inject_skill_test_input(player,6)
            cancel_at=.14 if current['name']=='cancel_before' else .46 if current['name']=='cancel_after' else None
            if cancel_at and t>cancel_at and not current.get('cancelled'):
                current['cancelled']=True;lib.interrupt_skill_test(player,True)
            if current['name']=='hit':
                if .15<t<.3 and not current.get('cast_capture'):
                    current['cast_capture']=True;capture('skill-cast')
                if t>.48 and projectiles and health==current['initial_health'] and not current.get('flight_capture'):
                    current['flight_capture']=True;capture('skill-flight')
                if health<current['initial_health'] and not current.get('impact_capture'):
                    current['impact_capture']=True;capture('skill-impact')
                if visual_preview and t>.8 and not current.get('hud_capture'):
                    current['hud_capture']=True
                    u.SystemLibrary.execute_console_command(world,'Shot SHOWUI')
            if t>(8. if native_smoke else 2.5):
                assert not view.active and len(projectiles)==0,'Ability/projectile leaked'
                assert abs(resource-(0 if current['name']=='poor' else 93))<.01,'Double charge'
                damage=current['initial_health']-health
                current['damage']=damage
                if current['name'] in ['hit','cancel_after']:
                    assert damage>0 and current['health_changes']==1,'Expected exactly one GAS hit'
                else:assert damage==0,'Unexpected damage in '+current['name']
                if current['name'] in ['poor','cancel_before']:assert current['max_projectiles']==0,'Invalid projectile launch'
                else:
                    assert current['max_projectiles']==1,'Expected one projectile'
                    assert current.get('flight_audio_samples',0)>0,'Flight audio never observed'
                    if current['name']!='miss':assert current.get('stopped_audio_samples',0)>0,'Impact stop never observed'
                assert view.cooldown_remaining==0,'Cooldown did not finish'
                # Cast emitter is owned by the ASC, and must be explicitly stopped on all exits.
                leftovers=[c for c in player.get_components_by_class(u.ParticleSystemComponent)
                    if c.get_editor_property('template') and '/GlintstonePebble/' in c.get_editor_property('template').get_path_name() and c.is_active()]
                assert not leftovers,'Cast effect leaked'
                if native_smoke:
                    assert not any(c.is_active() for c in player.get_components_by_class(u.NiagaraComponent)), 'Cast Niagara leaked'
                    assert current.get('active_burst_components',0)==0,'One-shot Niagara leaked'
                    if current['name'] not in ['poor','cancel_before']:
                        assert current.get('max_burst_components',0)>0,'Release/impact Niagara never observed'
                current['result']='passed'
                if wall:wall.destroy_actor();wall=None
                state['stage']='next'
    except Exception as error:
        u.log_error('PEBBLE_PIE_FAILED '+str(error));finish(str(error))
callback=u.register_slate_post_tick_callback(tick)
