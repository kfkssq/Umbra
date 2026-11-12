"""Create missing skill assets and append their input/initial grants through UE 5.8.
Gameplay configuration only. HUD/WBP creation and wiring belong to the user.
Existing definitions are preserved. Reject occupied Q/E bindings.
"""
import json
import os
import unreal as u

assets = u.EditorAssetLibrary
tools = u.AssetToolsHelpers.get_asset_tools()
root = '/Game/Skills/Prototype'
assets.make_directory(root)
report = {'created': [], 'configured': []}
def save(obj):
    assert assets.save_loaded_asset(obj, only_if_is_dirty=False), obj.get_path_name()
def tag(name):
    value = u.GameplayTag()
    value.import_text('(TagName="' + name + '")')
    return value
def load(path):
    obj = u.load_asset(path)
    assert obj, path
    return obj

montage_path = root + '/AM_Sweep'
if not assets.does_asset_exist(montage_path):
    montage = assets.duplicate_asset('/Game/ParagonGreystone/Characters/Heroes/Greystone/Animations/Attack_PrimaryB_Montage', montage_path)
    u.AnimationLibrary.remove_all_animation_notify_tracks(montage)
    save(montage)
    report['created'].append(montage_path)
else: montage = load(montage_path)

definitions = []
for name, title, radius, arc, mult, cost, cooldown, icon in [
    ('Sweep', '横扫', 250., 120., 1.5, 15., 4., 'warrior_12'),
    ('WideSweep', '广域横扫', 350., 170., 1.0, 25., 6., 'warrior_48')]:
    path = root + '/DA_' + name
    if assets.does_asset_exist(path):
        definition = load(path)
    else:
        factory = u.DataAssetFactory()
        factory.set_editor_property('data_asset_class', u.UmbraSweepDefinition)
        definition = tools.create_asset('DA_' + name, root, u.UmbraSweepDefinition, factory)
        values = dict(skill_id=name, display_name=title, cooldown_tag=tag('Cooldown.Skill.' + name),
                      montage=montage, strike_time=0.4, play_rate=1., radius=radius, arc_degrees=arc,
                      height_tolerance=120., resource_cost=cost, cooldown_seconds=cooldown,
                      icon=load('/Game/Asset/Icons/flat_skills_icons_no_color/warrior/' + icon),
                      strike_effect=load('/Game/ParagonGreystone/FX/Particles/Greystone/Abilities/ClearAPath/FX/P_Greystone_ClearAPath_Swipe'),
                      effect_scale=u.Vector(radius / 250., radius / 250., 1.),
                      strike_sound=load('/Game/Asset/Audio/Asset/Attack/swordswing_fast_01'))
        for key, value in values.items(): definition.set_editor_property(key, value)
        damage = definition.get_editor_property('damage')
        typed = damage.get_editor_property('typed')
        typed.set_editor_property('model', u.UmbraDamageModel.WEAPON_CHANNELS)
        typed.set_editor_property('weapon_multiplier', mult)
        typed.set_editor_property('use_damage_buckets', True)
        damage.set_editor_property('typed', typed)
        definition.set_editor_property('damage', damage)
        save(definition)
        report['created'].append(path)
    definitions.append(definition)

imc = load('/Game/Input/IMC_Default')
player_bp = load('/Game/Blueprints/Player/BP_UmbraPlayerCharacter')
player = u.get_default_object(player_bp.generated_class())
bindings = list(player.get_editor_property('ability_input_actions'))
for index, key in enumerate(['Q', 'E']):
    name = 'IA_Skill' + str(index + 1)
    path = '/Game/Input/' + name
    factory = u.DataAssetFactory()
    factory.set_editor_property('data_asset_class', u.InputAction)
    action = load(path) if assets.does_asset_exist(path) else tools.create_asset(name, '/Game/Input', u.InputAction, factory)
    save(action)
    mappings = list(imc.get_editor_property('default_key_mappings').get_editor_property('mappings'))
    for mapping in mappings:
        if mapping.get_editor_property('action') == action and str(mapping.get_editor_property('key').get_editor_property('key_name')) == '(':
            imc.unmap_key(action, mapping.get_editor_property('key'))
    matching = [m for m in mappings if str(m.get_editor_property('key').get_editor_property('key_name')) == key]
    assert not any(m.get_editor_property('action') != action for m in matching), 'Occupied key: ' + key
    input_key = u.Key()
    input_key.import_text(key)  # FKey has a custom token importer, unlike GameplayTag.
    # Earlier 5.8 setup read the deprecated empty array; normalize only our repeated key/action pair.
    for duplicate in matching[1:]:
        imc.unmap_key(action, input_key)
    if not matching:
        imc.map_key(action, input_key)
    if not any(b.get_editor_property('input_action') == action for b in bindings):
        bindings.append(u.UmbraTaggedInputAction(input_action=action, input_tag=tag('Input.Skill.' + ['One','Two'][index])))
player.set_editor_property('ability_input_actions', bindings)
u.BlueprintEditorLibrary.compile_blueprint(player_bp)
save(player_bp)
save(imc)

state_bp = load('/Game/Blueprints/Player/BP_UmbraPlayerState')
state = u.get_default_object(state_bp.generated_class())
slots = list(state.get_editor_property('initial_skill_slots'))
for index, definition in enumerate(definitions):
    if not any(s.get_editor_property('definition') == definition for s in slots):
        slots.append(u.UmbraSkillSlot(definition=definition, input_tag=tag('Input.Skill.' + ['One','Two'][index]), key_label=['Q','E'][index]))
state.set_editor_property('initial_skill_slots', slots)
u.BlueprintEditorLibrary.compile_blueprint(state_bp)
save(state_bp)

for path in ['/Game/UI/HUD/WBP_CombatHUD','/Game/UI/HUD/WBP_PlayerHUDGroup','/Game/UI/Skills/WBP_SkillSlot','/Game/UI/Skills/WBP_SkillBar']:
    if assets.does_asset_exist(path):
        report[path] = u.UmbraSkillEditorLibrary.describe_widget(load(path))
with open(os.path.join(u.Paths.project_saved_dir(), 'SkillPrototype', 'configuration.json'), 'w', encoding='utf-8') as f:
    json.dump(report, f, ensure_ascii=False, indent=2)
u.log('SWEEP_CONFIGURATION_PASSED')
