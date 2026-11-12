"""Rendered slot-one projectile acceptance; no saves."""
import unreal as u,time,json
from pathlib import Path
out=Path(u.Paths.project_saved_dir()).resolve()/'SlashWave';out.mkdir(exist_ok=True)
level=u.get_editor_subsystem(u.LevelEditorSubsystem);editor=u.get_editor_subsystem(u.UnrealEditorSubsystem);lib=u.UmbraSkillEditorLibrary
prev=u.UmbraEquipmentNetworkTestLibrary.configure_play_settings(0,1,True);cd=u.SystemLibrary.get_console_variable_int_value('umbra.Skills.IgnoreCooldowns')
definition=u.load_asset('/Game/Skills/Greystone/DA_BreachSlash');release=definition.get_release_delay()
state=dict(stage='start',cases=[]);cases=['center','edge','miss','wall','cancel','cancel_late','straight'];started=time.monotonic();world=player=target=current=wall=None
pose_only='-PoseOnly' in u.SystemLibrary.get_command_line()
if pose_only:cases=['miss']
u.EditorPythonScripting.set_keep_python_script_alive(True)
def now():return u.GameplayStatics.get_time_seconds(world)
def finish(error=''):
 state.update(result='failed' if error else 'passed',error=error)
 if level.is_in_play_in_editor():u.SystemLibrary.execute_console_command(world,'umbra.Skills.IgnoreCooldowns '+str(cd));level.editor_request_end_play()
 u.UmbraEquipmentNetworkTestLibrary.configure_play_settings(*prev)
 (out/('release-pose-pie.json' if pose_only else 'pie.json')).write_text(json.dumps(state,ensure_ascii=False,indent=2),encoding='utf-8')
 u.unregister_slate_post_tick_callback(callback);u.EditorPythonScripting.set_keep_python_script_alive(False);u.SystemLibrary.quit_editor()
def tick(dt):
 global world,player,target,current,wall
 try:
  assert time.monotonic()-started<150,'Timeout'
  if state['stage']=='start':level.editor_request_begin_play();state['stage']='wait';return
  if state['stage']=='wait' and level.is_in_play_in_editor() and time.monotonic()-started>8:
   world=editor.get_game_world();player=u.GameplayStatics.get_player_character(world,0)
   if not player:return
   u.SystemLibrary.execute_console_command(world,'umbra.Skills.IgnoreCooldowns 1');state['stage']='next';return
  if state['stage']=='next':
   if not cases:finish();return
   mode=cases.pop(0);target=lib.prepare_skill_test(world);p=player.get_actor_location();x=110 if mode=='edge' else (350 if mode=='miss' else (80 if mode=='straight' else 0))
   target.set_actor_location(u.Vector(p.x+x,p.y+600,p.z),False,True)
   lib.set_skill_test_attribute(player,'Resource',0.);current=dict(mode=mode,start=now(),origin=[p.x,p.y,p.z],positions=[]);state['cases'].append(current)
   if mode=='wall':wall=lib.spawn_skill_test_wall(player)
   assert lib.inject_skill_test_input(player,0);state['stage']='cast';return
  if state['stage']=='cast':
   t=now()-current['start'];mode=current['mode'];actors=u.GameplayStatics.get_all_actors_of_class(world,u.UmbraSkillProjectile)
   for a in actors:
    p=a.get_actor_location();current['positions'].append([t,p.x,p.y,p.z])
   if mode=='miss' and .3<t<.4 and not current.get('windup_checked'):
    pos=player.mesh.get_anim_instance().montage_get_position(definition.montage)
    assert abs(pos-t*definition.windup_end_time/definition.windup_duration)<.06,'Windup motion did not slow down'
    current['windup_checked']=True;current['windup_position']=pos
    u.SystemLibrary.execute_console_command(world,'Shot SHOWUI filename="'+(out/'windup.png').as_posix()+'" -nosuffix')
   if mode.startswith('cancel') and t>(.55 if mode=='cancel_late' else .1) and not current.get('cancelled'):lib.interrupt_skill_test(player,True);current['cancelled']=True
   if mode=='miss' and actors and release<=t<release+.4 and not current.get('shot'):
    current['release_pose']=player.mesh.get_anim_instance().montage_get_position(definition.montage)
    assert abs(current['release_pose']-definition.strike_time)<.04,'Wave released outside the intended swing pose'
    u.SystemLibrary.execute_console_command(world,'Shot SHOWUI filename="'+(out/'wave.png').as_posix()+'" -nosuffix');current['shot']=True
   if t>2.2:
    hp=lib.read_test_attribute(target,'Health');current['target_health']=hp
    assert lib.read_test_attribute(player,'Resource')==0
    if mode in ['center','edge','straight']:assert hp<10000,'No projectile hit'
    else:assert hp==10000,'Unexpected hit'
    if mode.startswith('cancel'):assert not current['positions'],'Projectile before release'
    else:
     assert current['positions'],'No projectile spawned'
     assert release-.05<=current['positions'][0][0]<=release+.15,'Incorrect release time'
    if mode=='miss':assert current.get('windup_checked'),'Windup pose not sampled'
    if mode=='straight':assert all(abs(p[1]-current['origin'][0])<1 for p in current['positions']),'Unexpected auto-aim'
    assert not actors,'Projectile lifetime leak'
    if wall:wall.destroy_actor();wall=None
    current['result']='passed';state['stage']='next'
 except Exception as e:u.log_error('SLASH_WAVE_TEST_FAILED '+repr(e));finish(repr(e))
callback=u.register_slate_post_tick_callback(tick)
