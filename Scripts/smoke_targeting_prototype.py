"""Rendered PIE: slot-8 targeting, mouse/key routing, range bounds, obstruction and landing damage. No saves."""
import unreal as u,time,json
from pathlib import Path
out=Path(u.Paths.project_saved_dir()).resolve()/'SkillTargeting';out.mkdir(exist_ok=True)
lib=u.UmbraSkillEditorLibrary;level=u.get_editor_subsystem(u.LevelEditorSubsystem);editor=u.get_editor_subsystem(u.UnrealEditorSubsystem)
previous=u.UmbraEquipmentNetworkTestLibrary.configure_play_settings(0,1,True)
state=dict(stage='start',checks=[]);started=time.monotonic();world=player=pc=aim=dest=start=inside=outside=wall=None;stage_time=0.;cd=0
u.EditorPythonScripting.set_keep_python_script_alive(True)
def check(ok,name):
 assert ok,name
 state['checks'].append(name)
def key(s):
 k=u.Key();k.import_text(s);return k
def click(s):
 check(lib.inject_target_pointer(player,key(s),True),'consume '+s)
 check(lib.inject_target_pointer(player,key(s),False),'consume release '+s)
def now():return u.GameplayStatics.get_time_seconds(world)
def stage(s):
 global stage_time
 state['stage']=s;stage_time=now()
def attr(a,n):return lib.read_test_attribute(a,n)
def finish(error=''):
 state.update(result='failed' if error else 'passed',error=error)
 if level.is_in_play_in_editor():
  u.SystemLibrary.execute_console_command(world,'umbra.Skills.IgnoreCooldowns '+str(cd));level.editor_request_end_play()
 u.UmbraEquipmentNetworkTestLibrary.configure_play_settings(previous[0],previous[1],bool(previous[2]))
 (out/'pie.json').write_text(json.dumps(state,ensure_ascii=False,indent=2),encoding='utf-8')
 u.unregister_slate_post_tick_callback(callback);u.EditorPythonScripting.set_keep_python_script_alive(False);u.SystemLibrary.quit_editor()
def tick(dt):
 global world,player,pc,aim,dest,start,inside,outside,wall,cd
 try:
  assert time.monotonic()-started<180,'Timeout '+state['stage']
  if state['stage']=='start':level.editor_request_begin_play();state['stage']='wait';return
  if state['stage']=='wait' and level.is_in_play_in_editor() and time.monotonic()-started>8:
   world=editor.get_game_world();player=u.GameplayStatics.get_player_character(world,0)
   if not player:return
   pc=u.GameplayStatics.get_player_controller(world,0);aim=pc.get_component_by_class(u.UmbraSkillTargetingComponent);check(bool(aim),'component exists')
   cd=u.SystemLibrary.get_console_variable_int_value('umbra.Skills.IgnoreCooldowns');u.SystemLibrary.execute_console_command(world,'umbra.Skills.IgnoreCooldowns 1')
   inside=lib.prepare_skill_test(world);enemies=u.GameplayStatics.get_all_actors_of_class(world,u.UmbraEnemyCharacter);outside=next(e for e in enemies if e!=inside)
   lib.set_skill_test_attribute(player,'HealthRegen',0.);lib.set_skill_test_attribute(player,'Resource',0.)
   start=player.get_actor_location();dest=u.Vector(start.x+330,start.y,start.z-96.)
   check(lib.inject_skill_test_input(player,7),'slot8 input');stage('aim');return
  if state['stage']=='aim' and now()-stage_time>.3:
   check(aim.is_targeting(),'8 begins preview');check(attr(player,'Resource')==0,'preview no resource deduction')
   # Move the actual cursor through the existing projection helper; false never requests movement.
   lib.inject_lab_ground_click(player,dest,False);stage('cursor');return
  if state['stage']=='cursor' and now()-stage_time>.4:
   state['status']=str(aim.target_status);state['location']=str(aim.aim_location)
   check(aim.valid_target,'cursor valid '+str(aim.target_status));dest=aim.aim_location
   check(abs(dest.x-(start.x+330))<8,'mouse follows world location')
   u.SystemLibrary.execute_console_command(world,'Shot SHOWUI filename="'+(out/'targeting-preview.png').as_posix()+'" -nosuffix')
   stage('capture');return
  if state['stage']=='capture' and now()-stage_time>.4:
   click('RightMouseButton');check(not aim.is_targeting(),'right cancels');stage('cancelled');return
  if state['stage']=='cancelled' and now()-stage_time>.4:
   check((player.get_actor_location()-start).length()<2,'cancel does not move')
   check(attr(player,'Resource')==0,'cancel zero cost')
   check(lib.inject_skill_test_input(player,7),'reenter');stage('bounds');return
  if state['stage']=='bounds' and now()-stage_time>.3:
   check(aim.is_targeting(),'reenter preview')
   aim.update_target_location(u.Vector(start.x+5000,start.y,start.z-96))
   point=aim.aim_location;state['clamped']=str(point)
   check(((point.x-start.x)**2+(point.y-start.y)**2)**.5<=601,'range clamped to 600')
   lib.inject_lab_ground_click(player,dest,False);stage('confirm');return
  if state['stage']=='confirm' and now()-stage_time>.4:
   check(aim.valid_target,'valid before confirm');dest=aim.aim_location
   # Target centers on opposite sides of the same configured 220cm radius.
   inside.set_actor_location(u.Vector(dest.x,dest.y+210,start.z),False,True)
   outside.set_actor_location(u.Vector(dest.x,dest.y+230,start.z),False,True)
   state['inside_before']=attr(inside,'Health');state['outside_before']=attr(outside,'Health');state['destination']=str(dest)
   click('LeftMouseButton');check(not aim.is_targeting(),'left confirms and closes preview');stage('flight');return
  if state['stage']=='flight':
   pos=player.get_actor_location();state.setdefault('flight',[]).append(dict(t=now()-stage_time,x=pos.x,y=pos.y,z=pos.z))
   if now()-stage_time>3.:
    check(((pos.x-dest.x)**2+(pos.y-dest.y)**2)**.5<5,'lands at displayed location')
    check(max(p['z'] for p in state['flight'])>start.z+150,'actual airborne arc')
    check(attr(inside,'Health')<state['inside_before'],'inside displayed circle damaged')
    check(attr(outside,'Health')==state['outside_before'],'outside displayed circle not damaged')
    check(attr(player,'Resource')==0,'confirmed leap zero cost')
    lib.prepare_skill_test(world);check(lib.inject_skill_test_input(player,7),'obstruction aim');stage('wall');return
  if state['stage']=='wall' and now()-stage_time>.3:
   wall=lib.spawn_skill_test_wall(player);check(bool(wall),'wall fixture')
   p=player.get_actor_location();wall.set_actor_location(u.Vector(p.x+150,p.y,p.z),False,True)
   check(not aim.update_target_location(u.Vector(p.x+330,p.y,p.z-96)),'wall blocks arc')
   aim.confirm_targeting();check(aim.is_targeting(),'invalid confirm retains preview')
   click('Escape');check(not aim.is_targeting(),'escape cancels');wall.destroy_actor();wall=None
   check(lib.inject_skill_test_input(player,7),'death aim');stage('death');return
  if state['stage']=='death' and now()-stage_time>.3:
   check(aim.is_targeting(),'preview before death');lib.set_skill_test_attribute(player,'Health',0.);stage('dead');return
  if state['stage']=='dead' and now()-stage_time>.3:
   check(not aim.is_targeting(),'death removes preview');finish()
 except Exception as e:u.log_error('TARGETING_PIE_FAILED '+repr(e));finish(repr(e))
callback=u.register_slate_post_tick_callback(tick)
