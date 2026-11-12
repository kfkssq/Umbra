"""Real input + rendered bone sampling for mobile SwordStorm. Never saves test state."""
import unreal as u,time,json,math
from pathlib import Path
out=Path(u.Paths.project_saved_dir()).resolve()/'SwordStorm';out.mkdir(exist_ok=True)
lib=u.UmbraSkillEditorLibrary;level=u.get_editor_subsystem(u.LevelEditorSubsystem);editor=u.get_editor_subsystem(u.UnrealEditorSubsystem)
previous=u.UmbraEquipmentNetworkTestLibrary.configure_play_settings(0,1,True);oldcd=u.SystemLibrary.get_console_variable_int_value('umbra.Skills.IgnoreCooldowns')
started=time.monotonic();state=dict(stage='start',cases=[]);world=player=target=slot=mesh=cur=None
cases=['stationary','moving','stop_turn','movement_event','interrupt','other_skill','death']
if '-ClickStorm' in u.SystemLibrary.get_command_line():cases=['click_move']
definition=u.load_asset('/Game/Skills/Greystone/DA_SwordStorm');saved_channel=definition.channel_while_held
definition.set_editor_property('channel_while_held',False) # Legacy finite-duration regression, in memory only.
u.EditorPythonScripting.set_keep_python_script_alive(True)
def command(s):u.SystemLibrary.execute_console_command(world,s)
def now():return u.GameplayStatics.get_time_seconds(world)
def finish(error=''):
 definition.set_editor_property('channel_while_held',saved_channel)
 state.update(result='failed' if error else 'passed',error=error)
 if level.is_in_play_in_editor():command('umbra.Skills.IgnoreCooldowns '+str(oldcd));level.editor_request_end_play()
 u.UmbraEquipmentNetworkTestLibrary.configure_play_settings(previous[0],previous[1],bool(previous[2]))
 (out/('click-pie.json' if '-ClickStorm' in u.SystemLibrary.get_command_line() else 'pie.json')).write_text(json.dumps(state,indent=2),encoding='utf-8')
 u.unregister_slate_post_tick_callback(callback);u.EditorPythonScripting.set_keep_python_script_alive(False);u.SystemLibrary.quit_editor()
def tick(dt):
 global world,player,target,slot,mesh,cur
 try:
  assert time.monotonic()-started<170,'Timeout '+state['stage']
  if state['stage']=='start':level.editor_request_begin_play();state['stage']='wait';return
  if state['stage']=='wait' and level.is_in_play_in_editor() and time.monotonic()-started>8:
   world=editor.get_game_world();player=u.GameplayStatics.get_player_character(world,0)
   if not player:return
   mesh=player.get_component_by_class(u.SkeletalMeshComponent)
   player.get_component_by_class(u.SpringArmComponent).set_editor_property('target_arm_length',750.)
   slot=next(w for w in u.WidgetLibrary.get_all_widgets_of_class(world,u.UserWidget,False) if isinstance(w,u.UmbraSkillSlotWidget) and w.get_editor_property('slot_index')==1)
   command('umbra.Skills.IgnoreCooldowns 1');state['stage']='next';return
  if state['stage']=='next':
   if not cases:finish();return
   mode=cases.pop(0);target=lib.prepare_skill_test(world);assert target
   lib.set_skill_test_attribute(player,'Health',100.);lib.set_skill_test_attribute(player,'Resource',0.)
   p=player.get_actor_location();cur=dict(mode=mode,start=now(),origin=[p.x,p.y,p.z],samples=[],damage_events=0,last_health=10000.)
   state['cases'].append(cur)
   assert lib.inject_skill_test_input(player,0 if mode=='other_skill' else 1)
   state['stage']='sample';return
  if state['stage']=='sample':
   t=now()-cur['start'];mode=cur['mode'];p=player.get_actor_location()
   if mode in ['moving','other_skill'] or (mode=='stop_turn' and (t<1. or t>2.)):
    assert lib.inject_movement_test_input(player,u.Vector2D(-1 if mode=='stop_turn' and t>2 else 1,0))
   if mode=='click_move' and t>.3 and not cur.get('clicked'):
    dest=p+u.Vector(0,600,0);assert lib.inject_lab_ground_click(player,dest,True);cur['clicked']=True
   if mode=='click_move' and cur.get('clicked') and t>.4 and not cur.get('released'):
    assert lib.inject_lab_ground_click(player,p+u.Vector(0,600,0),False);cur['released']=True
   if mode in ['movement_event','interrupt','death'] and t>.65 and not cur.get('triggered'):
    if mode=='death':lib.set_skill_test_attribute(player,'Health',0.)
    else:lib.interrupt_skill_test(player,mode=='movement_event')
    cur['triggered']=True
   # Enemy follows at a fixed world offset: later damage proves the hit center moves with the caster.
   target.set_actor_location(p+u.Vector(0,120,0),False,True)
   health=lib.read_test_attribute(target,'Health')
   if health<cur['last_health']-.01:cur['damage_events']+=1
   cur['last_health']=health
   root=mesh.get_socket_transform('root',u.RelativeTransformSpace.RTS_COMPONENT).rotation.rotator()
   calf=mesh.get_socket_transform('calf_l',u.RelativeTransformSpace.RTS_PARENT_BONE_SPACE).rotation.rotator()
   active=slot.read_view_state().active
   cur['samples'].append(dict(t=t,x=p.x,y=p.y,speed=player.get_velocity().length(),active=active,root=root.yaw,calf=[calf.pitch,calf.yaw,calf.roll],resource=lib.read_test_attribute(player,'Resource')))
   if mode in ['stationary','moving']:
    index=int(t/.35)
    if .3<t<1.8 and index!=cur.get('shot'):
     command('Shot SHOWUI filename="'+(out/(mode+'-'+str(index)+'.png')).as_posix()+'" -nosuffix');cur['shot']=index
   if t>4.2:
    samples=cur['samples'];cur['max_displacement']=max(math.hypot(s['x']-cur['origin'][0],s['y']-cur['origin'][1]) for s in samples)
    assert not active,'Skill remained active'
    assert all(s['resource']==0 for s in samples),'Resource cost changed'
    if mode not in ['interrupt','other_skill','death']:
     assert any(s['active'] and s['t']>2.5 for s in samples),'Moving cancelled storm early'
     assert cur['damage_events']==6,'Expected six moving/stationary pulses: '+str(cur['damage_events'])
     assert max(s['root'] for s in samples)-min(s['root'] for s in samples)>270,'No full visual rotation'
     assert len({tuple(round(v,0) for v in s['calf']) for s in samples if s['active']})>10,'Leg pose frozen'
    if mode=='other_skill':assert cur['damage_events']==0,'Other skill no longer cancels on movement'
    if mode=='moving':assert cur['max_displacement']>600,'Movement blocked'
    if mode=='click_move':assert cur['max_displacement']>150,'Ground click blocked'
    if mode in ['interrupt','death']:assert not any(s['active'] and s['t']>1.1 for s in samples),'Interrupt failed'
    cur['result']='passed';state['stage']='next'
 except Exception as e:u.log_error('STORM_PIE_FAILED '+repr(e));finish(repr(e))
callback=u.register_slate_post_tick_callback(tick)
