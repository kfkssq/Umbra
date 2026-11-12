"""Audio-enabled PIE: near/far/near, continuous Wave position, actual fader and retry. No saves."""
import json
import time
from pathlib import Path
import unreal as u

level=u.get_editor_subsystem(u.LevelEditorSubsystem)
editor=u.get_editor_subsystem(u.UnrealEditorSubsystem)
actors=u.get_editor_subsystem(u.EditorActorSubsystem)
previous=u.UmbraEquipmentNetworkTestLibrary.configure_play_settings(0,1,True)
u.EditorPythonScripting.set_keep_python_script_alive(True)
started=time.monotonic()
state={'stage':'start','percent':[],'virtual':[],'comparisons':[],'initial_fade_seen':False}
cls=u.EditorAssetLibrary.load_blueprint_class('/Game/Items/Tests/BP_StartupSmithingStone')
stone=u.load_asset('/Game/Items/NativeAccessoriesV1/Items/DA_Item_EquipParamGoods_10106')
for actor in actors.get_all_level_actors():
    if isinstance(actor,u.UmbraLootTestSpawner):
        actor.set_editor_property('enabled',actor.get_class()==cls)
        if actor.get_class()==cls:
            actor.set_editor_property('items',[stone]);actor.set_editor_property('random_single_item',False)
            actor.set_editor_property('one_of_each_rarity',False)

def percent(wave,value):
    state['percent'].append(dict(at=time.monotonic(),value=value,phase=state['stage']))

def virtual(value):
    state['virtual'].append(dict(at=time.monotonic(),value=value,phase=state['stage']))

def check(ok,why):
    if not ok:raise RuntimeError(why)

def finish(result,detail):
    state.update(result=result,detail=detail)
    u.UmbraEquipmentNetworkTestLibrary.configure_play_settings(previous[0],previous[1],bool(previous[2]))
    out=Path(u.Paths.project_saved_dir())/'LootAmbienceContinuity';out.mkdir(exist_ok=True)
    (out/'pie.json').write_text(json.dumps(state,indent=2),encoding='utf-8')
    u.unregister_slate_post_tick_callback(callback)
    u.EditorPythonScripting.set_keep_python_script_alive(False)
    u.SystemLibrary.quit_editor()

def compare(before,after):
    expected=(before['value']+(after['at']-before['at'])*pitch/duration)%1.0
    actual=after['value']%1.0
    error=min(abs(actual-expected),1-abs(actual-expected))*duration
    state['comparisons'].append(dict(expected=expected,actual=actual,error_seconds=error,phase=state['stage']))
    check(error<1.0,'Playback resumed at wrong phase: '+str(error)+' seconds')

def tick(delta):
    global pc,drop,audio,duration,pitch,before,near,trace_at
    try:
        now=time.monotonic();check(now-started<150,'Timeout '+state['stage'])
        if state['stage']=='start':
            level.editor_request_begin_play();state.update(stage='find',at=now);return
        if state['stage']=='find' and level.is_in_play_in_editor() and now-state['at']>6:
            world=editor.get_game_world();pc=u.GameplayStatics.get_player_controller(world,0)
            drops=u.GameplayStatics.get_all_actors_of_class(world,u.UmbraWorldItem)
            check(len(drops)==1,'Expected isolated epic stone');drop=drops[0]
            matches=[a for a in drop.get_components_by_class(u.AudioComponent) if a.get_editor_property('sound')==u.load_asset('/Game/Asset/Audio/Asset/Loot/Magic_env_epic')]
            # Shader/startup hitches consume wall time before the drop has settled.
            if not matches and now-state['at']<60:return
            check(len(matches)==1,'Missing rarity loop');audio=matches[0]
            duration=audio.get_editor_property('sound').get_editor_property('duration');pitch=audio.get_editor_property('pitch_multiplier')
            state.update(duration=duration,pitch=pitch,fade_seconds=drop.get_editor_property('loot_ambience_fade_in_duration'))
            near=drop.get_actor_location()+u.Vector(100,0,0)
            pc.set_audio_listener_override(None,near,u.Rotator())
            audio.on_audio_playback_percent.add_callable(percent)
            audio.on_audio_virtualization_changed.add_callable(virtual)
            # Playback-percent notifications are enabled when starting an ActiveSound.
            audio.stop();trace_at=0
            state.update(stage='near',at=now);return
        if state['stage'] in ('near','far','return','retry'):
            if now-trace_at>.05:
                u.UmbraAudioTestLibrary.trace_loop_fader(audio,state['stage']);trace_at=now
            if str(audio.get_play_state()).find('FADING_IN')>=0:state['initial_fade_seen']=True
        if state['stage']=='near' and now-state['at']>4:
            check(audio.is_playing() and not audio.is_virtualized(),'Near voice not active')
            check(len(state['percent'])>10,'No real audio playback progress')
            before=state['percent'][-1]
            pc.set_audio_listener_override(None,near+u.Vector(10000,0,0),u.Rotator())
            state.update(stage='far',at=now);return
        if state['stage']=='far' and now-state['at']>4:
            check(audio.is_virtualized(),'Far loop did not virtualize')
            pc.set_audio_listener_override(None,near,u.Rotator())
            state.update(stage='return',at=now);return
        if state['stage']=='return' and now-state['at']>2:
            check(audio.is_playing() and not audio.is_virtualized(),'Return did not realize')
            after=state['percent'][-1];check(after['phase']=='return','No resumed audio callback')
            compare(before,after)
            before=after;audio.stop();state.update(stage='retry',at=now);return
        if state['stage']=='retry' and now-state['at']>3:
            after=state['percent'][-1];check(after['phase']=='retry','Retry did not resume')
            compare(before,after)
            check(state['initial_fade_seen'],'No FadeIn observed on startup/retry')
            pc.get_controlled_pawn().set_actor_location(drop.get_actor_location()+u.Vector(0,0,100),False,True)
            check(drop.try_pickup(pc)==u.UmbraPickupResult.SUCCESS,'Pickup failed')
            pc.clear_audio_listener_override();level.editor_request_end_play();state['stage']='end'
        if state['stage']=='end' and not level.is_in_play_in_editor():
            finish('passed','Real audio: far virtualization, continuous phase on return and stop/retry, fade-in observed, pickup successful')
    except Exception as error:
        level.editor_request_end_play();finish('failed',str(error))

callback=u.register_slate_post_tick_callback(tick)
