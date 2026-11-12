"""UE-only narrow migration: six warrior icons, remove Pebble grant/input/previews.

Does not delete assets, reset skill tuning, or reconstruct HUD widget trees.
"""
import json
from pathlib import Path
import unreal as u

out = Path(u.Paths.project_saved_dir()) / 'GreystoneDirection'
out.mkdir(parents=True, exist_ok=True)
assets = u.EditorAssetLibrary
choices = [('Prototype/DA_Sweep', '12'), ('Prototype/DA_WideSweep', '48'),
           ('Greystone/DA_SwordStorm', '11'), ('Greystone/DA_LeapSlam', '08'),
           ('Greystone/DA_Guard', '13'), ('Greystone/DA_StoneRecovery', '26')]

def load(path):
    obj = u.load_asset(path)
    assert obj, path
    return obj

def save(obj):
    assert assets.save_loaded_asset(obj, False), obj.get_path_name()

definitions = [load('/Game/Skills/' + name) for name, _ in choices]
icons = [load('/Game/Asset/Icons/flat_skills_icons_color/warrior/warrior_' + number) for _, number in choices]
state_bp = load('/Game/Blueprints/Player/BP_UmbraPlayerState')
state = u.get_default_object(state_bp.generated_class())
old_slots = list(state.get_editor_property('initial_skill_slots'))
pebble = '/Game/Skills/GlintstonePebble/DA_GlintstonePebble.DA_GlintstonePebble'
assert [s.definition for s in old_slots[:6]] == definitions, 'Unexpected first six slots'
assert all(s.definition and s.definition.get_path_name() == pebble for s in old_slots[6:]), 'Unexpected additional skills'
player_bp = load('/Game/Blueprints/Player/BP_UmbraPlayerCharacter')
player = u.get_default_object(player_bp.generated_class())
action = load('/Game/Input/IA_GlintstonePebble')
imc = load('/Game/Input/IMC_Default')
bar = load('/Game/UI/Skills/WBP_SkillBar')
report = {'icons': [], 'removed_skill': pebble, 'previous_slots': [str(s.definition) for s in old_slots]}
for definition, icon in zip(definitions, icons):
    report['icons'].append({'definition': definition.get_path_name(), 'before': str(definition.icon), 'after': icon.get_path_name()})
    definition.set_editor_property('icon', icon)
    save(definition)
state.set_editor_property('initial_skill_slots', old_slots[:6])
player.set_editor_property('ability_input_actions', [b for b in player.get_editor_property('ability_input_actions') if b.input_action != action])
imc.unmap_all_keys_from_action(action)
save(imc)
for bp in [state_bp, player_bp]:
    u.BlueprintEditorLibrary.compile_blueprint(bp)
    save(bp)
for widget in u.UmbraEquipmentNetworkTestLibrary.get_blueprint_widgets(bar):
    if not isinstance(widget, u.UmbraSkillSlotWidget):
        continue
    index = widget.get_editor_property('slot_index')
    if 0 <= index < 6:
        widget.set_editor_properties(dict(preview_icon=icons[index], preview_key_label=old_slots[index].key_label,
            preview_resource_cost=definitions[index].resource_cost))
    else:
        widget.set_editor_properties(dict(preview_icon=None, preview_key_label='', preview_resource_cost=0., preview_cooldown_fraction=0.))
u.BlueprintEditorLibrary.compile_blueprint(bar)
save(bar)
(out / 'configuration.json').write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
u.log('GREYSTONE_WARRIOR_UI_CONFIGURED')
