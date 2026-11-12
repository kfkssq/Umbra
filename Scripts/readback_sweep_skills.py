"""Independent, read-only check of persisted skill/input/widget configuration."""
import json
import os
import unreal as u

load = u.EditorAssetLibrary.load_asset
imc = load('/Game/Input/IMC_Default')
report = {'keys': [], 'definitions': []}
for key, name in [('Q', 'IA_Skill1'), ('E', 'IA_Skill2')]:
    matches = [m for m in imc.get_editor_property('default_key_mappings').get_editor_property('mappings')
               if str(m.get_editor_property('key').get_editor_property('key_name')) == key]
    assert len(matches) == 1 and matches[0].get_editor_property('action').get_name() == name, (key, str(matches))
    report['keys'].append(key + '=' + name)
ps = u.get_default_object(load('/Game/Blueprints/Player/BP_UmbraPlayerState').generated_class())
slots = list(ps.get_editor_property('initial_skill_slots'))
assert len(slots) == 2
for slot in slots:
    definition = slot.get_editor_property('definition')
    report['definitions'].append(dict(path=definition.get_path_name(),
        cooldown=definition.get_editor_property('cooldown_seconds'),
        cost=definition.get_editor_property('resource_cost'),
        icon=definition.get_editor_property('icon').get_path_name()))
player = u.get_default_object(load('/Game/Blueprints/Player/BP_UmbraPlayerCharacter').generated_class())
report['bindings'] = [str(b) for b in player.get_editor_property('ability_input_actions')]
for name in ['IA_Skill1', 'IA_Skill2']:
    assert any(name in b for b in report['bindings']), name
report['widgets'] = u.UmbraSkillEditorLibrary.describe_widget(load('/Game/UI/Skills/WBP_SkillSlot'))
with open(os.path.join(u.Paths.project_saved_dir(), 'SkillPrototype', 'readback.json'), 'w', encoding='utf-8') as f:
    json.dump(report, f, ensure_ascii=False, indent=2)
u.log('SWEEP_READBACK_PASSED')
