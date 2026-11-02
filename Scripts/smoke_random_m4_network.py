"""Real in-process network PIE. Run twice: default listen server, or -UmbraM4Dedicated.
No map/assets saved. Restores editor play settings. Reports observed RPC and replicated GE evidence.
"""
import json
import os
import time
import unreal

dedicated="UmbraM4Dedicated" in unreal.SystemLibrary.get_command_line()
label="Dedicated" if dedicated else "Listen"
tooltip_regression="UmbraTooltipRegression" in unreal.SystemLibrary.get_command_line()
previous=unreal.UmbraEquipmentNetworkTestLibrary.configure_play_settings(2 if dedicated else 1,2,True)
if len(previous)!=3:
    raise RuntimeError("Cannot configure network play settings")
level=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
starts=[a for a in actors.get_all_level_actors() if isinstance(a,unreal.PlayerStart)]
fixture=actors.spawn_actor_from_class(unreal.load_class(None,"/Game/Items/Tests/RandomM4/BP_LootTest_Damage.BP_LootTest_Damage_C"),starts[0].get_actor_location())
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
started=time.monotonic()
state=dict(result="pending",mode=label,stage="start",checks=[],packet_lag_ms=100,packet_loss_percent=10)
client=None
server=None
server_world=None
saved=None
item_guid=None
last_request=0
stage_at=started

def payload(instance):
    return dict(guid=instance.instance_id.to_string(),definition=str(instance.definition_id),level=instance.item_level,
                rarity=str(instance.rarity),seed=instance.generation_seed,schema=instance.schema_version,rules=instance.generation_rules_version,
                rolls=[(str(r.affix_id),str(r.affix_tier_id),r.value,r.selected_tier,list(r.candidate_values)) for r in instance.rolled_affixes])
def check(ok,message):
    if not ok:
        raise RuntimeError(message)
def stage(name):
    global stage_at
    state["stage"]=name
    stage_at=time.monotonic()
def bag(owner):
    return owner.get_inventory_component().get_snapshot()
def gear(owner):
    return owner.get_equipment_component().get_snapshot()
def target(entries):
    return next((e for e in entries if e.instance_id.to_string()==saved["guid"]),None)
def synced():
    return bag(client.player_state).revision==bag(server.player_state).revision and gear(client.player_state).revision==gear(server.player_state).revision
def send(operation,cell=-1):
    global last_request
    last_request=client.last_item_transfer_response.request.request_id
    result=unreal.UmbraEquipmentNetworkTestLibrary.submit_remote_request(client,operation,item_guid,unreal.UmbraEquipmentSlot.MAIN_HAND,cell)
    check(result==unreal.UmbraTransferResult.PENDING,"Remote submission did not queue: "+str(result))
def acknowledged():
    reply=client.last_item_transfer_response
    if reply.request.request_id<=last_request:
        return False
    check(reply.result==unreal.UmbraTransferResult.SUCCESS,"RPC rejected: "+str(reply.result))
    return True
def finish(result,detail):
    state.update(result=result,detail=detail)
    unreal.UmbraEquipmentNetworkTestLibrary.configure_play_settings(previous[0],previous[1],bool(previous[2]))
    actors.destroy_actor(fixture)
    with open(os.path.join(unreal.Paths.project_saved_dir(),("TooltipNetwork" if tooltip_regression else "RandomM4Network")+label+".json"),"w",encoding="utf-8") as f:
        json.dump(state,f,indent=2)
    unreal.log("RANDOM_M4_NETWORK "+label+" "+result+" "+detail)
    unreal.unregister_slate_post_tick_callback(callback)
    unreal.EditorPythonScripting.set_keep_python_script_alive(False)
    unreal.SystemLibrary.quit_editor()

def tick(delta):
    global client,server,server_world,saved,item_guid
    try:
        now=time.monotonic()
        if now-started>150:
            raise RuntimeError("Timeout at "+state["stage"])
        if state["stage"]=="start":
            level.editor_request_begin_play()
            stage("connect")
            return
        if state["stage"]=="connect":
            worlds=unreal.EditorLevelLibrary.get_pie_worlds(True)
            clients=[]
            authorities=[]
            for world in worlds:
                for pc in unreal.GameplayStatics.get_all_actors_of_class(world,unreal.UmbraPlayerController):
                    if pc.has_authority():
                        authorities.append((world,pc))
                    else:
                        clients.append(pc)
            if not clients:
                return
            client=clients[0]
            if not client.player_state:
                return
            match=next(((w,p) for w,p in authorities if p.player_state and p.player_state.get_editor_property("player_id")==client.player_state.get_editor_property("player_id")),None)
            if not match or not bag(client.player_state).ready or not gear(client.player_state).valid:
                return
            server_world,server=match
            state["world_count"]=len(worlds)
            for world in worlds:
                unreal.SystemLibrary.execute_console_command(world,"Net PktLag=100")
                unreal.SystemLibrary.execute_console_command(world,"Net PktLoss=10")
            for existing in gear(server.player_state).items:
                server.player_state.unequip_to_inventory(existing.slot,existing.instance_id)
            stage("grant")
        elif state["stage"]=="grant":
            if now-stage_at<2:
                return
            drops=[d for d in unreal.GameplayStatics.get_all_actors_of_class(server_world,unreal.UmbraWorldItem)
                   if str(d.get_item_instance().definition_id)=="M4.DamageSword"]
            check(bool(drops),"Missing M4 drop")
            drop=drops[0]
            instance=drop.get_item_instance()
            saved=payload(instance)
            item_guid=instance.instance_id
            server.get_controlled_pawn().set_actor_location(drop.get_actor_location()+unreal.Vector(0,0,100),False,True)
            check(drop.try_pickup(server)==unreal.UmbraPickupResult.SUCCESS,"Server pickup failed")
            state["instance"]=saved
            stage("replicate_bag")
        elif state["stage"]=="replicate_bag":
            entry=target(bag(client.player_state).items)
            if not entry or not synced():
                return
            check(payload(entry.instance)==saved,"Replicated inventory changed payload")
            state["checks"].append("owner inventory replication")
            if tooltip_regression:
                data=unreal.UmbraItemTooltipDataBuilder.from_inventory(bag(client.player_state),entry.slot_index,
                    entry.instance_id,entry.definition,client.player_state.get_equipment_component(),False,unreal.UmbraEquipmentSlot.HEAD)
                check(data.valid,"Client inventory tooltip: "+str(data.result))
                check(not data.requirements.known,"Client invented authoritative requirement details")
                state["checks"].append("remote inventory tooltip valid with explicit unknown requirements")
            send(unreal.UmbraItemTransferOperation.EQUIP)
            stage("equip")
        elif state["stage"]=="equip":
            if not acknowledged() or not synced():
                return
            entry=target(gear(client.player_state).items)
            if not entry:
                return
            count=unreal.UmbraEquipmentNetworkTestLibrary.count_damage_snapshots(client.player_state)
            if count!=2:
                return
            check(payload(entry.instance)==saved,"Equip changed payload")
            check(unreal.UmbraEquipmentNetworkTestLibrary.count_damage_snapshots(server.player_state)==2,"Server GE count")
            state["checks"].append("remote equip RPC + two actual replicated GE damage snapshots")
            if tooltip_regression:
                data=unreal.UmbraItemTooltipDataBuilder.from_equipment(gear(client.player_state),entry.slot,
                    entry.instance_id,entry.definition,client.player_state.get_equipment_component())
                check(data.valid,"Client equipment tooltip: "+str(data.result))
                check(data.requirements_met==entry.requirements_met,"Worn status differs from server snapshot")
                state["checks"].append("remote equipment tooltip valid with server requirement status")
            send(unreal.UmbraItemTransferOperation.UNEQUIP)
            stage("unequip")
        elif state["stage"]=="unequip":
            if not acknowledged() or not synced():
                return
            entry=target(bag(client.player_state).items)
            if not entry or unreal.UmbraEquipmentNetworkTestLibrary.count_damage_snapshots(client.player_state)!=0:
                return
            check(payload(entry.instance)==saved,"Unequip changed payload")
            state["checks"].append("remote unequip RPC + GE removed on owner")
            send(unreal.UmbraItemTransferOperation.MOVE,23)
            stage("move")
        elif state["stage"]=="move":
            if not acknowledged() or not synced():
                return
            entry=target(bag(client.player_state).items)
            if not entry or entry.slot_index!=23:
                return
            check(payload(entry.instance)==saved,"Move changed payload")
            state["checks"].append("remote move RPC + exact destination + payload conservation")
            level.editor_request_end_play()
            stage("end")
        elif state["stage"]=="end" and not level.is_in_play_in_editor():
            finish("passed","Remote lifecycle and real GE context replicated under packet simulation")
    except Exception as error:
        level.editor_request_end_play()
        finish("failed",str(error))

callback=unreal.register_slate_post_tick_callback(tick)
