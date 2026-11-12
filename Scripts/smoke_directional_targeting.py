"""Rendered PIE acceptance for directional targeting; no asset or map saves."""
import unreal as u,time,json,math
from pathlib import Path
out=Path(u.Paths.project_saved_dir()).resolve()/'DirectionalTargeting';out.mkdir(exist_ok=True)
lib=u.UmbraSkillEditorLibrary;level=u.get_editor_subsystem(u.LevelEditorSubsystem);editor=u.get_editor_subsystem(u.UnrealEditorSubsystem)
previous=u.UmbraEquipmentNetworkTestLibrary.configure_play_settings(0,1,True)
state=dict(stage='start',checks=[],cases=[]);started=time.monotonic();world=player=pc=aim=target=wall=second=None;stage_time=0.;cd=0;case=None
# distance/angle for sectors, along/across for lines; yaw exercises both axes and diagonals.
cases=[('sector_center',8,0,200,0,True),('sector_edge_in',8,90,250,59,True),('sector_edge_out',8,45,250,61,False),('sector_range_in',8,0,299,0,True),('sector_range_out',8,0,301,0,False),('sector_behind',8,0,200,180,False),('sector_cancel',8,0,200,0,False),('wave_center',9,0,600,0,True),('wave_edge_in',9,90,500,99,True),('wave_edge_out',9,45,500,101,False),('wave_range_in',9,0,799,0,True),('wave_range_out',9,0,801,0,False),('wave_behind',9,0,-150,0,False),('wave_cancel',9,0,600,0,False),('wave_wall',9,0,600,0,False)]
cases.extend([('wave_multi',9,0,250,0,True),('wave_rotated_corner',9,7,799,99,True)])
u.EditorPythonScripting.set_keep_python_script_alive(True)
def check(ok,name):
 assert ok,name
 state['checks'].append(name)
def key(s):
 k=u.Key();k.import_text(s);return k
def click(s):
 check(lib.inject_target_pointer(player,key(s),True),'consume '+s);check(lib.inject_target_pointer(player,key(s),False),'consume release '+s)
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
 global world,player,pc,aim,target,wall,cd,case,second
 try:
  assert time.monotonic()-started<240,'Timeout '+state['stage']
  if case and state['stage'] in ['cursor','confirm']:
   lib.inject_lab_ground_click(player,u.Vector(*case['cursor']),False)
  if state['stage']=='start':level.editor_request_begin_play();state['stage']='wait';return
  if state['stage']=='wait' and level.is_in_play_in_editor() and time.monotonic()-started>8:
   world=editor.get_game_world();player=u.GameplayStatics.get_player_character(world,0)
   if not player:return
   pc=u.GameplayStatics.get_player_controller(world,0);aim=pc.get_component_by_class(u.UmbraSkillTargetingComponent);check(bool(aim),'component exists')
   saved=u.get_default_object(u.load_asset('/Game/Blueprints/Player/BP_UmbraPlayerState').generated_class()).initial_skill_slots
   check(len(saved)==10,'ten saved slots')
   original=json.loads((out/'configuration.json').read_text(encoding='utf-8'))['previous_slots']
   original=[s.split("'")[1] if s.startswith('<Object ') else s for s in original]
   check([s.definition.get_path_name() if s.definition else 'None' for s in saved[:8]]==original,'first eight slots preserved')
   check(all(not s.definition.targeted_direction for s in saved[:6]),'existing skills not integrated')
   for i in [8,9]:check(saved[i].definition.targeted_direction and saved[i].definition.resource_cost==0,'saved directional opt-in and zero cost')
   cd=u.SystemLibrary.get_console_variable_int_value('umbra.Skills.IgnoreCooldowns');u.SystemLibrary.execute_console_command(world,'umbra.Skills.IgnoreCooldowns 1');stage('next');return
  if state['stage']=='next':
   if not cases:finish();return
   name,index,yaw,a,b,hit=cases.pop(0);target=lib.prepare_skill_test(world);origin=player.get_actor_location();rad=math.radians(yaw);f=u.Vector(math.cos(rad),math.sin(rad),0);right=u.Vector(-f.y,f.x,0)
   position=origin+f*(a*math.cos(math.radians(b)))+right*(a*math.sin(math.radians(b))) if index==8 else origin+f*a+right*b
   target.set_actor_location(position,False,True);lib.set_skill_test_attribute(player,'Resource',0.);lib.set_skill_test_attribute(player,'HealthRegen',0.)
   case=dict(name=name,index=index,yaw=yaw,hit=hit,origin=[origin.x,origin.y,origin.z],cursor=[origin.x+f.x*400,origin.y+f.y*400,origin.z-96],health=[]);state['cases'].append(case)
   if name=='wave_multi':
    second=next(e for e in u.GameplayStatics.get_all_actors_of_class(world,u.UmbraEnemyCharacter) if e!=target)
    second.set_actor_location(origin+f*650,False,True);case['second_before']=attr(second,'Health')
   if name=='wave_wall':
    wall=lib.spawn_skill_test_wall(player);wall.set_actor_location(origin+f*300,False,True)
   check(lib.inject_skill_test_input(player,index),name+' input');stage('aim');return
  if state['stage']=='aim' and now()-stage_time>.2:
   check(aim.is_targeting(),case['name']+' preview')
   check(not aim.update_target_location(player.get_actor_location()),'zero direction rejected');aim.confirm_targeting();check(aim.is_targeting(),'invalid confirm stays aiming')
   lib.inject_lab_ground_click(player,u.Vector(*case['cursor']),False);stage('cursor');return
  if state['stage']=='cursor' and now()-stage_time>.3:
   if not aim.valid_target and now()-stage_time<2.:return
   check(aim.valid_target,case['name']+' valid: '+str(aim.target_status));check(attr(player,'Resource')==0,'preview zero cost')
   actual=aim.aim_location-player.get_actor_location();expected=u.Vector(*case['cursor'])-player.get_actor_location()
   cosine=(actual.x*expected.x+actual.y*expected.y)/max(1.,math.hypot(actual.x,actual.y)*math.hypot(expected.x,expected.y))
   check(cosine>.99,case['name']+' mouse selects direction')
   name=case['name']
   if name in ['sector_center','wave_center']:
    u.SystemLibrary.execute_console_command(world,'Shot SHOWUI filename="'+(out/(name+'-preview.png')).as_posix()+'" -nosuffix')
   stage('confirm');return
  if state['stage']=='confirm' and now()-stage_time>.3:
   name=case['name']
   # Reposition immediately before injection: other editor windows may move the OS cursor during screenshot frames.
   lib.inject_lab_ground_click(player,u.Vector(*case['cursor']),False)
   # Cursor projection is deliberately tested above; exact boundary cases use a known world direction.
   aim.update_target_location(u.Vector(*case['cursor']))
   if name.endswith('cancel'):
    click('RightMouseButton');check(not aim.is_targeting(),'cancel closes preview');stage('resolved');return
   click('LeftMouseButton');check(not aim.is_targeting(),'confirm closes preview: '+str(aim.target_status)+' valid='+str(aim.valid_target));stage('resolved');return
  if state['stage']=='resolved':
   elapsed=now()-stage_time;hp=attr(target,'Health');case['health'].append([elapsed,hp])
   if case['name'] in ['sector_center','wave_center'] and not case.get('mouse_moved_after_confirm'):
    origin=u.Vector(*case['origin']);lib.inject_lab_ground_click(player,origin+u.Vector(-400,0,-96),False);case['mouse_moved_after_confirm']=True
   if case['name']=='wave_center' and .5<elapsed<.6 and not case.get('shot'):
    u.SystemLibrary.execute_console_command(world,'Shot SHOWUI filename="'+(out/'wave-travel.png').as_posix()+'" -nosuffix');case['shot']=True
   if elapsed<1.7:return
   check((hp<10000)==case['hit'],case['name']+' damage expectation')
   if case['name']=='wave_multi':check(attr(second,'Health')<case['second_before'],'wave penetrates multiple targets')
   damages={round(10000-h,4) for t,h in case['health'] if h<10000};check(len(damages)<=1,case['name']+' at most one hit')
   check(attr(player,'Resource')==0,case['name']+' zero cost');origin=u.Vector(*case['origin']);check((player.get_actor_location()-origin).length()<5,case['name']+' no click movement')
   if case['name']=='wave_center':
    first=next(t for t,h in case['health'] if h<10000);check(.75<first<1.1,'wave damage waits for travel');case['first_hit_time']=first
   check(not u.GameplayStatics.get_all_actors_of_class(world,u.UmbraDirectionalSkillEffect),'effect lifetime cleanup')
   if wall:wall.destroy_actor();wall=None
   case['result']='passed';stage('next')
 except Exception as e:u.log_error('DIRECTIONAL_PIE_FAILED '+repr(e));finish(repr(e))
callback=u.register_slate_post_tick_callback(tick)
