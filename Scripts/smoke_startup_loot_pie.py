"""Two fresh PIE sessions verify persisted map fixture, randomized payloads and pickup."""
import json,time
from pathlib import Path
import unreal as u

level=u.get_editor_subsystem(u.LevelEditorSubsystem)
editor=u.get_editor_subsystem(u.UnrealEditorSubsystem)
previous=u.UmbraEquipmentNetworkTestLibrary.configure_play_settings(0,1,True)
u.EditorPythonScripting.set_keep_python_script_alive(True)
start=time.monotonic()
state={'stage':'start','runs':[]}
cls=u.EditorAssetLibrary.load_blueprint_class('/Game/Items/Tests/BP_SixRarityStartup')

def check(ok,msg):
    if not ok: raise RuntimeError(msg)

def finish(result,detail):
    state.update(result=result,detail=detail)
    u.UmbraEquipmentNetworkTestLibrary.configure_play_settings(previous[0],previous[1],bool(previous[2]))
    out=Path(u.Paths.project_saved_dir())/'SixRarityStartup';out.mkdir(exist_ok=True)
    (out/'pie.json').write_text(json.dumps(state,indent=2),encoding='utf-8')
    u.unregister_slate_post_tick_callback(callback)
    u.EditorPythonScripting.set_keep_python_script_alive(False)
    u.SystemLibrary.quit_editor()

def tick(delta):
    try:
        if time.monotonic()-start>150: raise RuntimeError('Timeout '+state['stage'])
        if state['stage']=='start':
            level.editor_request_begin_play();state['stage']='inspect';state['at']=time.monotonic();return
        if state['stage']=='inspect' and level.is_in_play_in_editor() and time.monotonic()-state['at']>8:
            world=editor.get_game_world()
            fixtures=u.GameplayStatics.get_all_actors_of_class(world,cls)
            check(len(fixtures)==1,'Exactly one persisted fixture')
            drops=[d for d in u.GameplayStatics.get_all_actors_of_class(world,u.UmbraWorldItem) if d.get_owner()==fixtures[0]]
            check(len(drops)==6,'Expected exactly six startup drops, got '+str(len(drops)))
            rows=[]
            pc=u.GameplayStatics.get_player_controller(world,0)
            for drop in drops:
                item=drop.get_item_instance()
                fx=drop.get_component_by_class(u.NiagaraComponent)
                check(fx and fx.is_active(),'Missing/inactive rarity effect')
                rows.append(dict(id=str(item.definition_id),rarity=int(item.rarity.value),seed=item.generation_seed,guid=item.instance_id.to_string()))
                pc.get_controlled_pawn().set_actor_location(drop.get_actor_location()+u.Vector(0,0,100),False,True)
                check(drop.try_pickup(pc)==u.UmbraPickupResult.SUCCESS,'Startup loot not collectible')
            rows.sort(key=lambda row:row['rarity'])
            check([r['rarity'] for r in rows]==list(range(6)),'Rarity coverage')
            state['runs'].append(rows)
            level.editor_request_end_play();state['stage']='end';return
        if state['stage']=='end' and not level.is_in_play_in_editor():
            if len(state['runs'])<2: state['stage']='start';return
            check([r['seed'] for r in state['runs'][0]]!=[r['seed'] for r in state['runs'][1]],'Seeds did not change')
            check([r['id'] for r in state['runs'][0]]!=[r['id'] for r in state['runs'][1]],'Templates did not change')
            finish('passed','Two fresh sessions: six rarities, new templates/seeds, active VFX and successful pickups')
    except Exception as e:
        level.editor_request_end_play();finish('failed',str(e))

callback=u.register_slate_post_tick_callback(tick)
