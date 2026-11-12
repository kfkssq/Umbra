"""Two real PIE starts: one random amulet + stone, source gates, pickup, existing six retained."""
import json,time
from pathlib import Path
import unreal as u

level=u.get_editor_subsystem(u.LevelEditorSubsystem)
editor=u.get_editor_subsystem(u.UnrealEditorSubsystem)
previous=u.UmbraEquipmentNetworkTestLibrary.configure_play_settings(0,1,True)
u.EditorPythonScripting.set_keep_python_script_alive(True)
start=time.monotonic();state={'stage':'start','runs':[]}
classes=[u.EditorAssetLibrary.load_blueprint_class('/Game/Items/Tests/BP_Startup'+n) for n in ('Amulet','SmithingStone')]
six=u.EditorAssetLibrary.load_blueprint_class('/Game/Items/Tests/BP_SixRarityStartup')

def check(ok,msg):
    if not ok: raise RuntimeError(msg)

def finish(result,detail):
    state.update(result=result,detail=detail)
    u.UmbraEquipmentNetworkTestLibrary.configure_play_settings(previous[0],previous[1],bool(previous[2]))
    out=Path(u.Paths.project_saved_dir())/'NativeStartup';out.mkdir(exist_ok=True)
    (out/'pie.json').write_text(json.dumps(state,indent=2),encoding='utf-8')
    u.unregister_slate_post_tick_callback(callback)
    u.EditorPythonScripting.set_keep_python_script_alive(False)
    u.SystemLibrary.quit_editor()

def tick(delta):
    try:
        check(time.monotonic()-start<150,'Timeout '+state['stage'])
        if state['stage']=='start':
            level.editor_request_begin_play();state.update(stage='inspect',at=time.monotonic());return
        if state['stage']=='inspect' and level.is_in_play_in_editor() and time.monotonic()-state['at']>8:
            world=editor.get_game_world();pc=u.GameplayStatics.get_player_controller(world,0)
            drops=u.GameplayStatics.get_all_actors_of_class(world,u.UmbraWorldItem)
            six_actors=u.GameplayStatics.get_all_actors_of_class(world,six)
            check(len(six_actors)==1 and len([d for d in drops if d.get_owner()==six_actors[0]])==6,'Existing six drops changed')
            rows=[]
            for cls,category in zip(classes,(u.UmbraItemCategory.ACCESSORY,u.UmbraItemCategory.MATERIAL)):
                fixtures=u.GameplayStatics.get_all_actors_of_class(world,cls)
                check(len(fixtures)==1,'Exactly one persisted fixture per category')
                own=[d for d in drops if d.get_owner()==fixtures[0]]
                check(len(own)==1,'Expected exactly one drop per fixture')
                drop=own[0];item=drop.get_item_instance();definition=item.definition
                check(definition.item_category==category,'Wrong category')
                rank=int(item.rarity.value);source=definition.source_rarity
                check(rank<=3 if source==1 else rank in (3,4) if source==2 else rank>=4,'Source rarity gate')
                check(len(item.rolled_affixes)==(0 if category==u.UmbraItemCategory.MATERIAL else min(rank+1,4)),'Affix count')
                check(item.item_level==1,'Startup item cannot equip at starting level')
                fx=drop.get_component_by_class(u.NiagaraComponent)
                check(fx and fx.is_active(),'Rarity VFX inactive')
                before=item.export_text()
                pc.get_controlled_pawn().set_actor_location(drop.get_actor_location()+u.Vector(0,0,100),False,True)
                check(drop.try_pickup(pc)==u.UmbraPickupResult.SUCCESS,'Pickup failed')
                stored=next(e for e in pc.player_state.get_inventory_component().get_snapshot().items if e.instance_id.to_string()==item.instance_id.to_string())
                check(stored.instance.export_text()==before,'Pickup changed payload')
                rows.append(dict(id=str(item.definition_id),rarity=rank,seed=item.generation_seed,guid=item.instance_id.to_string()))
            state['runs'].append(rows)
            level.editor_request_end_play();state['stage']='end';return
        if state['stage']=='end' and not level.is_in_play_in_editor():
            if len(state['runs'])<2: state['stage']='start';return
            for index in range(2):
                check(state['runs'][0][index]['seed']!=state['runs'][1][index]['seed'],'Seed not refreshed')
                check(state['runs'][0][index]['guid']!=state['runs'][1][index]['guid'],'Identity not refreshed')
            # Repeated definitions or qualities are valid random outcomes.
            finish('passed','Two starts: one amulet + one stone, existing six retained, source gates, level 1, VFX and pickup')
    except Exception as error:
        level.editor_request_end_play();finish('failed',str(error))

callback=u.register_slate_post_tick_callback(tick)
