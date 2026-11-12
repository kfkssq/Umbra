"""Audio-enabled PIE: actual component playback, cancellation, death and loop ownership. No saves."""
import unreal as u,json,time
from pathlib import Path
out=Path(u.Paths.project_saved_dir()).resolve()/'GreystoneAudio'
lib=u.UmbraSkillEditorLibrary;level=u.get_editor_subsystem(u.LevelEditorSubsystem);editor=u.get_editor_subsystem(u.UnrealEditorSubsystem)
previous=u.UmbraEquipmentNetworkTestLibrary.configure_play_settings(0,1,True)
names=['BreachSlash','SwordStorm','LeapSlam','Guard','StoneRecovery','BattleFocus']
defs=[u.load_asset('/Game/Skills/Greystone/DA_'+n) for n in names]
fixture=u.load_asset('/Game/Asset/Audio/Asset/Loot/Magic_env_epic');assert fixture.get_editor_property('looping')
cases=[(i,'full') for i in range(6)]+[(i,'cancel') for i in range(6)]+[(3,'loop_complete'),(3,'loop_cancel'),(3,'loop_death')]
voice_only='-VoiceDeathOnly' in u.SystemLibrary.get_command_line()
if voice_only:cases=[(5,'voice_death')]
state=dict(stage='start',cases=[]);started=time.monotonic();world=player=current=None
old_cd=u.SystemLibrary.get_console_variable_int_value('umbra.Skills.IgnoreCooldowns')
u.EditorPythonScripting.set_keep_python_script_alive(True)
def sounds():
 return [c.get_editor_property('sound').get_path_name() for c in player.get_components_by_class(u.AudioComponent) if c.is_playing() and c.get_editor_property('sound')]
def now():return u.GameplayStatics.get_time_seconds(world)
def finish(error=''):
 defs[3].set_editor_property('active_sound',None)
 state.update(result='failed' if error else 'passed',error=error)
 if level.is_in_play_in_editor():
  u.SystemLibrary.execute_console_command(world,'umbra.Skills.IgnoreCooldowns '+str(old_cd));level.editor_request_end_play()
 u.UmbraEquipmentNetworkTestLibrary.configure_play_settings(previous[0],previous[1],bool(previous[2]))
 (out/('voice-death-pie.json' if voice_only else 'pie.json')).write_text(json.dumps(state,ensure_ascii=False,indent=2),encoding='utf-8')
 u.unregister_slate_post_tick_callback(callback);u.EditorPythonScripting.set_keep_python_script_alive(False);u.SystemLibrary.quit_editor()
def tick(dt):
 global world,player,current
 try:
  assert time.monotonic()-started<230,'Timeout '+state['stage']
  if state['stage']=='start':level.editor_request_begin_play();state['stage']='wait';return
  if state['stage']=='wait' and level.is_in_play_in_editor() and time.monotonic()-started>8:
   world=editor.get_game_world();player=u.GameplayStatics.get_player_character(world,0)
   if not player:return
   u.SystemLibrary.execute_console_command(world,'umbra.Skills.IgnoreCooldowns 1');state['stage']='next';return
  if state['stage']=='next':
   if not cases:finish();return
   i,mode=cases.pop(0);lib.prepare_skill_test(world)
   defs[3].set_editor_property('active_sound',fixture if mode.startswith('loop_') else None)
   lib.set_skill_test_attribute(player,'HealthRegen',0.);lib.set_skill_test_attribute(player,'Health',60.)
   current=dict(skill=names[i],slot=i,mode=mode,start=now(),seen=[],samples=[]);state['cases'].append(current)
   assert lib.inject_skill_test_input(player,i),'Input rejected';state['stage']='cast';return
  if state['stage']=='cast':
   i=current['slot'];mode=current['mode'];t=now()-current['start'];playing=sounds()
   if i==1:lib.inject_skill_test_input(player,1,t<(.25 if 'cancel' in mode else 3.2))
   current['seen']=sorted(set(current['seen']+playing))
   if len(current['samples'])<100:current['samples'].append(dict(t=t,sounds=playing))
   if ('cancel' in mode or mode=='loop_death') and t>.25 and not current.get('interrupted'):
    if mode=='loop_death':lib.set_skill_test_attribute(player,'Health',0.)
    else:lib.interrupt_skill_test(player,i!=1)
    current['interrupted']=True
   if mode=='voice_death' and t>1.25 and not current.get('interrupted'):
    lib.set_skill_test_attribute(player,'Health',0.);current['interrupted']=True
   duration=1.1 if 'cancel' in mode or mode=='loop_death' else (2. if mode=='voice_death' else [3.,4.5,3.,3.5,8.,4.5][i])
   if t>duration:
    expected=defs[i].cast_voice or defs[i].resolve_voice
    if expected and not ('cancel' in mode and not defs[i].cast_voice):
     assert expected.get_path_name() in current['seen'],'Voice component never played: '+expected.get_name()
    if 'cancel' in mode and defs[i].resolve_voice:
     assert defs[i].resolve_voice.get_path_name() not in current['seen'],'Resolved voice on cancelled cast'
    if mode.startswith('loop_'):
     assert fixture.get_path_name() in current['seen'],'Loop did not start'
     assert fixture.get_path_name() not in playing,'Loop leaked'
    if 'cancel' in mode or 'death' in mode:
     assert not any('/ParagonGreystone/Audio/Cues/' in p and ('Effort_Block' in p or 'Status_Buffed' in p or 'Effort_Swing' in p or 'Effort_Ability_Q' in p or 'JumpHeavy' in p) for p in playing),'Skill voice leaked'
    current['result']='passed';state['stage']='next'
 except Exception as e:u.log_error('GREYSTONE_AUDIO_PIE_FAILED '+repr(e));finish(repr(e))
callback=u.register_slate_post_tick_callback(tick)
