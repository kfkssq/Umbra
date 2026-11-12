"""Real rendered PIE: six enemy deaths, Niagara selection, upright beams and pickups.

Uses transient fixtures only; does not save the map or change production tuning.
"""
import json
import time
from pathlib import Path
import unreal as u

level=u.get_editor_subsystem(u.LevelEditorSubsystem)
editor=u.get_editor_subsystem(u.UnrealEditorSubsystem)
previous=u.UmbraEquipmentNetworkTestLibrary.configure_play_settings(0,1,True)
u.EditorPythonScripting.set_keep_python_script_alive(True)
started=time.monotonic()
state={'result':'pending','stage':'start','items':[]}
keep=[]
names=('Common','Uncommon','Rare','Epic','Legendary','Mythic')

def check(ok,why):
    if not ok: raise RuntimeError(why)

def finish(result,detail):
    state.update(result=result,detail=detail)
    u.UmbraEquipmentNetworkTestLibrary.configure_play_settings(previous[0],previous[1],bool(previous[2]))
    out=Path(u.Paths.project_saved_dir())/'LootVFX'
    out.mkdir(exist_ok=True)
    (out/'pie.json').write_text(json.dumps(state,indent=2),encoding='utf-8')
    u.unregister_slate_post_tick_callback(callback)
    u.EditorPythonScripting.set_keep_python_script_alive(False)
    u.SystemLibrary.quit_editor()

def tick(delta):
    try:
        if time.monotonic()-started>120: raise RuntimeError('Timeout: '+state['stage'])
        if state['stage']=='start':
            level.editor_request_begin_play();state['stage']='kill';return
        if state['stage']=='kill' and time.monotonic()-started>8 and level.is_in_play_in_editor():
            world=editor.get_game_world()
            enemies=u.GameplayStatics.get_all_actors_of_class(world,u.UmbraEnemyCharacter)
            check(bool(enemies),'No prototype enemy')
            source=u.load_asset('/Game/Items/Tests/RandomMVP/DA_Item_RandomSword')
            for i,name in enumerate(names):
                item=u.new_object(u.UmbraItemDefinition)
                for key in ('weapon','item_category','weapon_type','allowed_slots','world_drop_mesh','icon'):
                    item.set_editor_property(key,source.get_editor_property(key))
                item.set_editor_property('definition_id','LootVFXCheck.'+str(i))
                item.set_editor_property('display_name','Loot VFX '+name)
                item.set_editor_property('rarity',getattr(u.UmbraItemRarity,('COMMON','MAGIC','RARE','EPIC','LEGENDARY','UNIQUE')[i]))
                profile=u.load_asset('/Game/Items/GeneratedEquipment/V2/DA_Generation_Weapon')
                row=next(r for r in profile.rarities if r.rarity==item.rarity)
                item.set_editor_property('rarity_color',row.color)
                table=u.new_object(u.UmbraLootTable)
                entry=u.UmbraLootEntry();entry.set_editor_property('item_definition',item)
                table.set_editor_property('entries',[entry])
                enemy=u.UmbraEquipmentNetworkTestLibrary.spawn_enemy_for_loot_test(enemies[0])
                enemy.set_editor_property('enable_loot',True)
                enemy.set_editor_property('loot_table',table)
                keep.extend((item,table,enemy))
                check(u.UmbraEquipmentNetworkTestLibrary.kill_enemy_for_loot_test(enemy),'Death failed')
            state['stage']='inspect';state['killed_at']=time.monotonic();return
        if state['stage']=='inspect' and time.monotonic()-state['killed_at']>5:
            world=editor.get_game_world()
            pc=u.GameplayStatics.get_player_controller(world,0)
            drops=[d for d in u.GameplayStatics.get_all_actors_of_class(world,u.UmbraWorldItem)
                   if str(d.get_item_instance().definition_id).startswith('LootVFXCheck.')]
            # Present the transient six drops in a row around the player for visual inspection.
            origin=pc.get_controlled_pawn().get_actor_location()
            for d in drops:
                index=int(str(d.get_item_instance().definition_id).split('.')[-1])
                p=d.get_actor_location()
                d.set_actor_location(u.Vector(origin.x+(index-2.5)*150,origin.y,p.z),False,True)
            state['stage']='capture';state['capture_at']=time.monotonic();return
        if state['stage']=='capture' and time.monotonic()-state['capture_at']>3:
            u.SystemLibrary.execute_console_command(editor.get_game_world(),'HighResShot 1')
            state['stage']='verify';state['capture_at']=time.monotonic();return
        if state['stage']=='verify' and time.monotonic()-state['capture_at']>2:
            world=editor.get_game_world()
            drops=[d for d in u.GameplayStatics.get_all_actors_of_class(world,u.UmbraWorldItem)
                   if str(d.get_item_instance().definition_id).startswith('LootVFXCheck.')]
            check(len(drops)==6,'Expected six drops, got '+str(len(drops)))
            pc=u.GameplayStatics.get_player_controller(world,0)
            for drop in drops:
                instance=drop.get_item_instance()
                i=int(str(instance.definition_id).split('.')[-1])
                fx=drop.get_component_by_class(u.NiagaraComponent)
                check(fx is not None,'Missing Niagara component')
                asset=fx.get_editor_property('asset')
                check(asset.get_name()=='NS_LootDrop_'+names[i],'Wrong system')
                check(fx.is_active(),'Effect inactive after five seconds')
                rotation=fx.get_world_rotation()
                check(abs(rotation.pitch)<0.1 and abs(rotation.roll)<0.1,'Tilted beam')
                state['items'].append(dict(rarity=i,asset=asset.get_path_name(),active=True,rotation=str(rotation)))
                pc.get_controlled_pawn().set_actor_location(drop.get_actor_location()+u.Vector(0,0,100),False,True)
                check(drop.try_pickup(pc)==u.UmbraPickupResult.SUCCESS,'Pickup failed')
                check(not fx.is_active(),'Effect survived pickup')
            level.editor_request_end_play();state['stage']='end'
        if state['stage']=='end' and not level.is_in_play_in_editor():
            finish('passed','Six real deaths: all Niagara variants active, upright, and cleared on pickup')
    except Exception as error:
        level.editor_request_end_play();finish('failed',str(error))

callback=u.register_slate_post_tick_callback(tick)
