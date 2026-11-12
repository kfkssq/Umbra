"""Held Enhanced Input acceptance for mobile channeled SwordStorm; no asset saves."""
import unreal as u,time,json,math
from pathlib import Path
out=Path(u.Paths.project_saved_dir()).resolve()/'SwordStorm'
lib=u.UmbraSkillEditorLibrary;level=u.get_editor_subsystem(u.LevelEditorSubsystem);editor=u.get_editor_subsystem(u.UnrealEditorSubsystem)
previous=u.UmbraEquipmentNetworkTestLibrary.configure_play_settings(0,1,True);oldcd=u.SystemLibrary.get_console_variable_int_value('umbra.Skills.IgnoreCooldowns')
definition=u.load_asset('/Game/Skills/Greystone/DA_SwordStorm');assert definition.channel_while_held and definition.resource_cost==0
stun=u.GameplayTagContainer();stun.import_text('(GameplayTags=((TagName="State.Stunned")))')
cases=['hold_stationary','hold_moving','stop_turn','tap','no_target','release_moving','stun','interrupt','other_skill','death']
if '-ClickStorm' in u.SystemLibrary.get_command_line():cases=['click_move']
started=time.monotonic();state=dict(stage='start',cases=[]);world=player=target=mesh=slot=cur=None
u.EditorPythonScripting.set_keep_python_script_alive(True)
def cmd(s):u.SystemLibrary.execute_console_command(world,s)
def now():return u.GameplayStatics.get_time_seconds(world)
def finish(error=''):
 state.update(result='failed' if error else 'passed',error=error)
 if level.is_in_play_in_editor():
  u.AbilitySystemLibrary.remove_loose_gameplay_tags(player,stun)
  cmd('umbra.Skills.IgnoreCooldowns '+str(oldcd));level.editor_request_end_play()
 u.UmbraEquipmentNetworkTestLibrary.configure_play_settings(previous[0],previous[1],bool(previous[2]))
 (out/('channel-click-pie.json' if '-ClickStorm' in u.SystemLibrary.get_command_line() else 'channel-pie.json')).write_text(json.dumps(state,indent=2,default=str),encoding='utf-8')
 u.unregister_slate_post_tick_callback(callback);u.EditorPythonScripting.set_keep_python_script_alive(False);u.SystemLibrary.quit_editor()
def tick(dt):
 global world,player,target,mesh,slot,cur
 try:
  assert time.monotonic()-started<200,'Timeout '+state['stage']
  if state['stage']=='start':level.editor_request_begin_play();state['stage']='wait';return
  if state['stage']=='wait' and level.is_in_play_in_editor() and time.monotonic()-started>8:
   world=editor.get_game_world();player=u.GameplayStatics.get_player_character(world,0)
   if not player:return
   mesh=player.get_component_by_class(u.SkeletalMeshComponent)
   player.get_component_by_class(u.SpringArmComponent).set_editor_property('target_arm_length',750.)
   slot=next(w for w in u.WidgetLibrary.get_all_widgets_of_class(world,u.UserWidget,False) if isinstance(w,u.UmbraSkillSlotWidget) and w.slot_index==1)
   cmd('umbra.Skills.IgnoreCooldowns 1');state['stage']='next';return
  if state['stage']=='next':
   if not cases:finish();return
   mode=cases.pop(0);u.AbilitySystemLibrary.remove_loose_gameplay_tags(player,stun)
   target=lib.prepare_skill_test(world);assert target
   lib.set_skill_test_attribute(player,'Health',100.);lib.set_skill_test_attribute(player,'Resource',0.)
   p=player.get_actor_location();release=30. if mode.startswith('hold_') else (5. if mode in ['stop_turn','click_move'] else (.08 if mode=='tap' else 1.5))
   cur=dict(mode=mode,start=now(),release=release,origin=[p.x,p.y],samples=[],damage_events=0,last_health=10000.)
   state['cases'].append(cur);assert lib.inject_skill_test_input(player,0 if mode=='other_skill' else 1,True)
   state['stage']='sample';return
  if state['stage']=='sample':
   t=now()-cur['start'];mode=cur['mode'];p=player.get_actor_location()
   if mode!='other_skill':lib.inject_skill_test_input(player,1,t<cur['release'] and not cur.get('interrupted',False))
   if mode in ['hold_moving','release_moving','other_skill'] or (mode=='stop_turn' and (t<1. or t>2.)):
    axis=[(1,0),(0,1),(-1,0),(0,-1)][int(t)%4] if mode=='hold_moving' else ((-1,0) if mode=='stop_turn' and t>2. else (1,0))
    assert lib.inject_movement_test_input(player,u.Vector2D(*axis))
   if mode=='click_move' and t>.35 and not cur.get('clicked'):
    cur['destination']=p+u.Vector(0,600,0);assert lib.inject_lab_ground_click(player,cur['destination'],True);cur['clicked']=True
   if mode=='click_move' and t>.45 and not cur.get('released_click'):
    assert lib.inject_lab_ground_click(player,cur['destination'],False);cur['released_click']=True
   if mode in ['stun','interrupt','death'] and t>.8 and not cur.get('interrupted'):
    if mode=='stun':u.AbilitySystemLibrary.add_loose_gameplay_tags(player,stun)
    elif mode=='death':lib.set_skill_test_attribute(player,'Health',0.)
    else:lib.interrupt_skill_test(player,False)
    cur['interrupted']=True;cur['stop_time']=t
   target.set_actor_location(p+u.Vector(0,1000 if mode=='no_target' else 120,0),False,True)
   health=lib.read_test_attribute(target,'Health')
   if health<cur['last_health']-.01:
    cur['damage_events']+=1;cur.setdefault('damage_times',[]).append(t)
   cur['last_health']=health
   root=mesh.get_socket_transform('root',u.RelativeTransformSpace.RTS_COMPONENT).rotation.rotator()
   calf=mesh.get_socket_transform('calf_l',u.RelativeTransformSpace.RTS_PARENT_BONE_SPACE).rotation.rotator()
   particles=[c for c in player.get_components_by_class(u.ParticleSystemComponent) if c.get_editor_property('template')==definition.active_effect and c.is_active()]
   sample=dict(t=t,x=p.x,y=p.y,active=slot.read_view_state().active,speed=player.get_velocity().length(),facing=player.get_actor_rotation().yaw,travel=math.degrees(math.atan2(player.get_velocity().y,player.get_velocity().x)),root=root.yaw,calf=[calf.pitch,calf.yaw,calf.roll],fx=len(particles),particle_count=lib.count_storm_particles(player,definition.active_effect),hit_particles=lib.count_storm_hit_particles_near(target,definition.strike_effect,60.),caster_hit_particles=lib.count_storm_hit_particles_near(player,definition.strike_effect,60.),health=health,resource=lib.read_test_attribute(player,'Resource'))
   cur['samples'].append(sample)
   if mode in ['hold_moving','hold_stationary']:
    index=int(t/.4)
    if (.35<t<1.7 or 7.<t<7.5) and index!=cur.get('shot'):
     cmd('Shot SHOWUI filename="'+(out/(mode+'-'+str(index)+'.png')).as_posix()+'" -nosuffix');cur['shot']=index
   if t>cur['release']+1.:
    samples=cur['samples'];stop=cur.get('stop_time',cur['release']);late=[s for s in samples if s['t']>stop+.25]
    assert not sample['active'],'Release left channel active'
    assert all(s['resource']==0 for s in samples),'Resource cost changed'
    if mode!='other_skill':
     assert not any(s['active'] or s['fx'] for s in late),'Channel or persistent FX leaked'
     assert max(s['health'] for s in late)==min(s['health'] for s in late),'Damage after release/interruption'
    if mode.startswith('hold_') or mode in ['stop_turn','click_move']:
     assert any(s['active'] and s['t']>cur['release']-.5 for s in samples),'Channel stopped while held'
     assert cur['damage_events']>1,'Missing recurring damage'
     if mode.startswith('hold_'):
      times=cur['damage_times'];expected=u.load_asset('/Game/ParagonGreystone/Characters/Heroes/Greystone/Animations/Ability_E').sequence_length/definition.play_rate/definition.storm_pulses_per_cycle
      actual=(times[-1]-times[0])/(len(times)-1);cur['measured_interval']=actual;cur['expected_interval']=expected
      assert abs(actual-expected)<.03,'Damage drifted from animation cycle'
     live=[s for s in samples if s['active'] and s['t']<cur['release']]
     # Native Ability_E is retained; actor facing follows locomotion, not an authored root spin.
     if mode=='hold_moving':
      steady=[s for s in live if s['speed']>100 and .45<s['t']%1<.9]
      assert steady and all(abs((s['facing']-s['travel']+180)%360-180)<15 for s in steady),'Facing no longer follows movement'
      assert len({tuple(round(v) for v in s['calf']) for s in live})>10,'Frozen moving legs'
     assert any(s['fx'] and s['particle_count']>0 and s['t']>cur['release']-.5 for s in live),'Persistent particles ended before channel'
     for second in range(2,int(cur['release'])-1):
      assert any(s['particle_count']>0 and second<=s['t']<second+1 for s in live),'Empty particle window '+str(second)
    distance=max(math.hypot(s['x']-cur['origin'][0],s['y']-cur['origin'][1]) for s in samples);cur['max_displacement']=distance
    if mode=='hold_moving':assert distance>300,'Movement blocked'
    if mode=='click_move':assert distance>150,'Ground click blocked'
    if mode=='release_moving':assert any(s['speed']>100 and s['t']>cur['release']+.4 for s in samples),'Release cancelled movement'
    if mode in ['tap','no_target','other_skill']:assert cur['damage_events']==0,'Short tap or movement-cancelled other skill dealt damage'
    if mode=='hold_stationary':
     assert any(s['hit_particles']>0 for s in samples),'Missing enemy hit sparks'
     assert not any(s['caster_hit_particles']>0 for s in samples),'Hit sparks at caster'
    if mode=='no_target':assert not any(s['hit_particles'] or s['caster_hit_particles'] for s in samples),'Sparks without damage'
    cur['result']='passed';state['stage']='next'
 except Exception as e:u.log_error('CHANNEL_STORM_FAILED '+repr(e));finish(repr(e))
callback=u.register_slate_post_tick_callback(tick)
