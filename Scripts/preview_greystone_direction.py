"""Fresh saved-asset readback, rendered HUD, and sampled source poses; no asset saves."""
import json
import time
from pathlib import Path
import unreal as u

out = Path(u.Paths.project_saved_dir()).resolve() / 'GreystoneDirection'
(out / 'Poses').mkdir(parents=True, exist_ok=True)
lib = u.UmbraSkillEditorLibrary
level = u.get_editor_subsystem(u.LevelEditorSubsystem)
editor = u.get_editor_subsystem(u.UnrealEditorSubsystem)
previous = u.UmbraEquipmentNetworkTestLibrary.configure_play_settings(0, 1, True)
state = {'stage': 'start', 'slots': [], 'poses': [], 'stat_widgets': {}}
names = ['Attack_PrimaryA', 'Attack_PrimaryB', 'Attack_PrimaryC', 'Attack_A_Fast',
    'Attack_B_Fast', 'Attack_C_Fast', 'Attack_D_Fast', 'Attack_A_Slow', 'Attack_B_Slow',
    'Attack_C_Slow', 'Attack_D_Slow', 'Attack_RMB', 'Ability_Q', 'Ability_E', 'Ability_R',
    'Deflect_InPlace', 'Ability_Ultimate', 'Ultimate_Entry', 'Ultimate_Part1', 'Ultimate_Part2',
    'Jump_Melee', 'Cast', 'CardCast', 'Emote_Taunt_BringItOn_T1', 'Emote_Taunt_SwordPoint_T1',
    'Emote_Master_SwordPlay_T3']
animations = {name: u.load_asset('/Game/ParagonGreystone/Characters/Heroes/Greystone/Animations/' + name) for name in names}
poses = [(name, f) for name in names for f in [.2, .5, .8]]
expected = ['12', '48', '11', '08', '13', '26']
bp = u.load_asset('/Game/Blueprints/Player/BP_UmbraPlayerState')
saved_slots = list(u.get_default_object(bp.generated_class()).get_editor_property('initial_skill_slots'))
assert len(saved_slots) == 6
action = u.load_asset('/Game/Input/IA_GlintstonePebble')
player_cdo = u.get_default_object(u.load_asset('/Game/Blueprints/Player/BP_UmbraPlayerCharacter').generated_class())
assert all(b.input_action != action for b in player_cdo.get_editor_property('ability_input_actions'))
imc = u.load_asset('/Game/Input/IMC_Default')
assert all(m.get_editor_property('action') != action for m in imc.get_editor_property('default_key_mappings').get_editor_property('mappings'))
for path in u.EditorAssetLibrary.list_assets('/Game/UI/HUD/StatsPanel', recursive=True):
    stat_bp = u.load_asset(path)
    if not isinstance(stat_bp, u.WidgetBlueprint): continue
    rows = []
    for widget in u.UmbraEquipmentNetworkTestLibrary.get_blueprint_widgets(stat_bp):
        row = {'name': widget.get_name(), 'class': widget.get_class().get_name()}
        if isinstance(widget, u.Image):
            row['icon'] = str(widget.get_editor_property('brush').get_editor_property('resource_object'))
        rows.append(row)
    state['stat_widgets'][path] = rows
started = time.monotonic()
stamp = started
world = player = None
u.EditorPythonScripting.set_keep_python_script_alive(True)

def command(text): u.SystemLibrary.execute_console_command(world, text)

def finish(error=''):
    state.update(result='failed' if error else 'passed', error=error)
    (out / 'preview.json').write_text(json.dumps(state, ensure_ascii=False, indent=2), encoding='utf-8')
    u.UmbraEquipmentNetworkTestLibrary.configure_play_settings(previous[0], previous[1], bool(previous[2]))
    u.unregister_slate_post_tick_callback(callback)
    u.EditorPythonScripting.set_keep_python_script_alive(False)
    u.SystemLibrary.quit_editor()

def tick(delta):
    global world, player, stamp
    try:
        assert time.monotonic() - started < 240, 'Preview timeout'
        if state['stage'] == 'start':
            level.editor_request_begin_play(); state['stage'] = 'wait'; return
        if state['stage'] == 'wait' and level.is_in_play_in_editor() and time.monotonic()-started > 8:
            world = editor.get_game_world(); player = u.GameplayStatics.get_player_character(world, 0)
            if not player: return
            widgets = u.WidgetLibrary.get_all_widgets_of_class(world, u.UserWidget, False)
            for w in widgets:
                if isinstance(w, u.UmbraAttributeDebugPanel): w.set_visibility(u.SlateVisibility.COLLAPSED)
                if not isinstance(w, u.UmbraSkillSlotWidget): continue
                index = w.get_editor_property('slot_index'); view = w.read_view_state()
                if index < 6:
                    assert view.granted and view.icon.get_name() == 'warrior_' + expected[index]
                    material = lib.find_presentation_widget(w, 'W_Item_Slot.item_icon').get_dynamic_material()
                    assert material.get_texture_parameter_value('IconTexture') == view.icon
                else: assert not view.granted and not view.icon and not str(view.key_label)
                state['slots'].append({'index': index, 'key': str(view.key_label), 'icon': str(view.icon), 'granted': view.granted})
            assert not lib.inject_skill_test_input(player, 6), 'Removed seventh slot still accepts skill input'
            command('Shot SHOWUI filename="'+(out/'hud.png').as_posix()+'" -nosuffix')
            stamp=time.monotonic(); state['stage']='hud'; return
        if state['stage']=='hud' and time.monotonic()-stamp>1:
            for widget in u.WidgetLibrary.get_all_widgets_of_class(world,u.UserWidget,True):
                widget.set_visibility(u.SlateVisibility.COLLAPSED)
            p=player.get_actor_location();player.set_actor_location(u.Vector(p.x-1000,p.y,p.z),False,True)
            arm=player.get_components_by_class(u.SpringArmComponent)[0]
            arm.set_editor_property('target_arm_length',500.)
            state['stage']='pose'
        if state['stage']=='pose':
            if not poses:
                level.editor_request_end_play();state['stage']='finish';return
            name,fraction=poses.pop(0);animation=animations[name]
            t=animation.get_play_length()*fraction
            player.mesh.play_animation(animation,False);player.mesh.set_position(t,False);player.mesh.set_play_rate(0.)
            root=u.AnimationLibrary.get_bone_pose_for_time(animation,'root',t,False).translation
            player.mesh.set_relative_location(u.Vector(-root.x,-root.y,-90-root.z),False,False)
            state['poses'].append({'name':name,'fraction':fraction,'seconds':t,'source_root':str(root),
                'note':'Root translation compensated only in transient visual preview'})
            stamp=time.monotonic();state['stage']='shot';return
        if state['stage']=='shot' and time.monotonic()-stamp>.25:
            row=state['poses'][-1];file=out/'Poses'/(row['name']+'_'+str(int(row['fraction']*100))+'.png')
            command('Shot filename="'+file.as_posix()+'" -nosuffix')
            stamp=time.monotonic();state['stage']='advance';return
        if state['stage']=='advance' and time.monotonic()-stamp>.25:state['stage']='pose'
        if state['stage']=='finish' and not level.is_in_play_in_editor():finish()
    except Exception as error:
        u.log_error('GREYSTONE_DIRECTION_PREVIEW_FAILED '+str(error));finish(str(error))

callback=u.register_slate_post_tick_callback(tick)
