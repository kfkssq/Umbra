"""Rendered PIE acceptance of native kit, numeric mappings and focus lifecycle. No saves."""
import json,time
from pathlib import Path
import unreal as u
out=Path(u.Paths.project_saved_dir()).resolve()/'GreystoneDirection'
lib=u.UmbraSkillEditorLibrary
level=u.get_editor_subsystem(u.LevelEditorSubsystem);editor=u.get_editor_subsystem(u.UnrealEditorSubsystem)
previous=u.UmbraEquipmentNetworkTestLibrary.configure_play_settings(0,1,True)
previous_cd=u.SystemLibrary.get_console_variable_int_value('umbra.Skills.IgnoreCooldowns')
no_cost='-NoCostSmoke' in u.SystemLibrary.get_command_line()
expected=['BreachSlash','SwordStorm','LeapSlam','Guard','StoneRecovery','BattleFocus']
keys=['One','Two','Three','Four','Five','Six']
saved_state=u.get_default_object(u.load_asset('/Game/Blueprints/Player/BP_UmbraPlayerState').generated_class())
saved_slots=list(saved_state.get_editor_property('initial_skill_slots'))
assert [str(s.definition.skill_id) for s in saved_slots]==expected
for slot in saved_slots:
    for track in slot.definition.montage.get_editor_property('slot_anim_tracks'):
        for segment in track.get_editor_property('anim_track').get_editor_property('anim_segments'):
            source=segment.get_editor_property('anim_reference').get_name()
            assert not source.startswith('Attack_') or (str(slot.definition.skill_id)=='BreachSlash' and source=='Attack_C_Slow'), 'Unexpected attack source: '+source
imc=u.load_asset('/Game/Input/IMC_Default')
mappings=list(imc.get_editor_property('default_key_mappings').get_editor_property('mappings'))
for i,key in enumerate(keys):
    action=u.load_asset('/Game/Input/IA_Skill'+str(i+1))
    bound=[str(m.get_editor_property('key').get_editor_property('key_name')) for m in mappings if m.get_editor_property('action')==action]
    assert bound==[key],str(bound)
state={'stage':'start','cases':[],'input':'Real Enhanced Input injection; not physical keyboard'}
cases=[(i,'full') for i in range(6)]+[(i,'cancel') for i in range(6)]+[(2,'wall'),(5,'refresh'),(5,'death')]
if no_cost:cases=[(i,'zero_resource') for i in range(6)]
started=time.monotonic();world=player=target=wall=current=None;slots={}
u.EditorPythonScripting.set_keep_python_script_alive(True)
def command(s):u.SystemLibrary.execute_console_command(world,s)
def attr(a,n):return lib.read_test_attribute(a,n)
def now():return u.GameplayStatics.get_time_seconds(world)
def finish(error=''):
    state.update(result='failed' if error else 'passed',error=error)
    if level.is_in_play_in_editor():command('umbra.Skills.IgnoreCooldowns '+str(previous_cd));level.editor_request_end_play()
    u.UmbraEquipmentNetworkTestLibrary.configure_play_settings(previous[0],previous[1],bool(previous[2]))
    (out/('no-cost-pie.json' if no_cost else 'native-pie.json')).write_text(json.dumps(state,ensure_ascii=False,indent=2),encoding='utf-8')
    u.unregister_slate_post_tick_callback(callback);u.EditorPythonScripting.set_keep_python_script_alive(False);u.SystemLibrary.quit_editor()
def tick(delta):
    global world,player,target,wall,current,slots
    try:
        assert time.monotonic()-started<220,'Timeout '+state['stage']
        if state['stage']=='start':level.editor_request_begin_play();state['stage']='wait';return
        if state['stage']=='wait' and level.is_in_play_in_editor() and time.monotonic()-started>8:
            world=editor.get_game_world();player=u.GameplayStatics.get_player_character(world,0)
            if not player:return
            command('umbra.Skills.IgnoreCooldowns 1')
            widgets=u.WidgetLibrary.get_all_widgets_of_class(world,u.UserWidget,False)
            slots={w.get_editor_property('slot_index'):w for w in widgets if isinstance(w,u.UmbraSkillSlotWidget)}
            for w in widgets:
                if isinstance(w,u.UmbraAttributeDebugPanel):w.set_visibility(u.SlateVisibility.COLLAPSED)
            state['slots']=[]
            for i,w in sorted(slots.items()):
                v=w.read_view_state()
                if i<6:
                    assert v.granted and str(v.key_label)==str(i+1)
                    assert '/flat_skills_icons_no_color/' in v.icon.get_path_name()
                    mat=lib.find_presentation_widget(w,'W_Item_Slot.item_icon').get_dynamic_material()
                    assert mat.get_texture_parameter_value('IconTexture')==v.icon
                else:assert not v.granted and not v.icon
                state['slots'].append(dict(index=i,key=str(v.key_label),icon=str(v.icon)))
            command('Shot SHOWUI filename="'+(out/'native-hud.png').as_posix()+'" -nosuffix')
            state['stage']='next';return
        if state['stage']=='next':
            if not cases:finish();return
            i,mode=cases.pop(0);target=lib.prepare_skill_test(world);assert target
            if no_cost:
                assert slots[i].read_view_state().resource_cost==0
                assert lib.set_skill_test_attribute(player,'Resource',0.)
            if i==2:
                p=player.get_actor_location()
                target.set_actor_location(u.Vector(p.x,p.y+450,p.z),False,True)
            lib.set_skill_test_attribute(player,'HealthRegen',0.)
            lib.set_skill_test_attribute(player,'Health',60.)
            assert lib.set_skill_test_attribute(player,'CriticalChance',0.)
            current=dict(slot=i,mode=mode,start=now(),samples=[],armor=attr(player,'Armor'))
            state['cases'].append(current)
            if mode=='wall':wall=lib.spawn_skill_test_wall(player);assert wall
            assert lib.inject_skill_test_input(player,i)
            state['stage']='cast';return
        if state['stage']=='cast':
            i=current['slot'];mode=current['mode'];elapsed=now()-current['start'];v=slots[i].read_view_state()
            if i==1:lib.inject_skill_test_input(player,1,elapsed<(.25 if no_cost else (.06 if mode=='cancel' else 3.2)))
            sample=dict(t=elapsed,active=v.active,health=attr(player,'Health'),resource=attr(player,'Resource'),
                target=attr(target,'Health'),armor=attr(player,'Armor'),critical=attr(player,'CriticalChance'))
            current['samples'].append(sample)
            if v.active:current['activated']=True
            if no_cost and elapsed>.25:
                assert current.get('activated'), 'Zero-resource activation failed'
                assert sample['resource']==0 and v.resource_cost==0 and v.affordable, 'Zero-cost contract failed'
                lib.interrupt_skill_test(player,i!=1)
                current['result']='passed';state['stage']='next';return
            if mode=='cancel' and elapsed>.06 and not current.get('cancelled'):
                lib.interrupt_skill_test(player,i!=1);current['cancelled']=True
            if mode=='refresh' and elapsed>2 and not current.get('refreshed'):
                lib.set_skill_test_attribute(player,'Resource',100.)
                assert lib.inject_skill_test_input(player,5);current['refreshed']=True
            if mode=='death' and elapsed>1.4 and not current.get('killed'):
                assert sample['critical']>.19
                lib.set_skill_test_attribute(player,'Health',0.);current['killed']=True
            if mode=='full' and elapsed>[.5,.8,.6,.6,3.4,.7][i] and not current.get('shot'):
                command('Shot SHOWUI filename="'+(out/('native-skill-'+str(i+1)+'.png')).as_posix()+'" -nosuffix');current['shot']=True
            duration=(9. if mode=='refresh' else 7.2) if i==5 else [1.4,3.6,3.,2.9,4.8,7.2][i]
            if mode=='death':duration=2.5
            if elapsed>duration:
                assert current.get('activated'),'Activation not observed'
                assert not v.active,'Action remained locked'
                assert abs(sample['armor']-current['armor'])<.01,'Armor leaked'
                assert abs(sample['critical'])<.001,'Focus leaked after expiry/death/cancel'
                if mode not in ['refresh','death']:assert abs(sample['resource']-((0 if no_cost else 100)-v.resource_cost))<.01,'Incorrect cost'
                if mode=='cancel':assert sample['target']==10000 and abs(sample['health']-60)<.01
                elif mode=='wall':assert sample['target']==10000;wall.destroy_actor();wall=None
                elif i==3:assert max(s['armor'] for s in current['samples'])>=current['armor']+99.9 and sample['target']==10000
                elif i==5:
                    assert max(s['critical'] for s in current['samples'])>.19
                    assert max(s['critical'] for s in current['samples'])<.201,'Focus stacked'
                    assert sample['target']==10000,'Focus dealt damage'
                    if mode=='refresh':assert any(s['t']>6.7 and s['critical']>.19 for s in current['samples']),'Refresh failed'
                else:
                    assert sample['target']<10000,'No damage'
                    if i==1:
                        h=[10000]+[s['target'] for s in current['samples']]
                        assert sum(a>b+.01 for a,b in zip(h,h[1:]))==6, 'Expected six observed storm pulses'
                    if i==4:assert abs(sample['health']-90)<.01
                current['result']='passed';state['stage']='next'
    except Exception as error:
        u.log_error('NATIVE_KIT_FAILED '+repr(error));finish(repr(error))
callback=u.register_slate_post_tick_callback(tick)
