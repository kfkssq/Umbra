"""Read-only editor audit for the first skill prototype; does not save assets."""
import json
import os
import time
import unreal

root = '/Game/ParagonGreystone/'
anim_root = root + 'Characters/Heroes/Greystone/Animations/'
paths = [anim_root + name for name in (
    'Attack_PrimaryA', 'Attack_PrimaryB', 'Attack_PrimaryC',
    'Attack_PrimaryA_Montage', 'Attack_PrimaryB_Montage',
    'Ability_E', 'Ability_Q', 'Ability_R', 'Deflect_InPlace')]
paths += [root + 'FX/Particles/Greystone/Abilities/' + name for name in (
    'ClearAPath/FX/P_Greystone_ClearAPath_Swipe',
    'ClearAPath/FX/P_Greystone_ClearAPath_Impact',
    'Primary/FX/P_Greystone_Primary_Trails_R',
    'Primary/FX/P_Greystone_Primary_Impact')]
rows = []
for path in paths:
    asset = unreal.load_asset(path)
    row = {'path': path, 'class': asset.get_class().get_name() if asset else None}
    if isinstance(asset, unreal.AnimSequenceBase):
        row['length'] = asset.get_play_length()
        row['skeleton'] = asset.get_editor_property('skeleton').get_path_name()
        row['notifies'] = [str(n) for n in unreal.AnimationLibrary.get_animation_notify_events(asset)]
    if isinstance(asset, unreal.AnimSequence):
        row['root_motion'] = asset.get_editor_property('enable_root_motion')
    rows.append(row)
out = os.path.join(unreal.Paths.project_saved_dir(), 'SkillPrototype', 'greystone-audit.json')
with open(out, 'w', encoding='utf-8') as stream:
    json.dump(rows, stream, ensure_ascii=False, indent=2)
unreal.log('GREYSTONE_SKILL_AUDIT ' + out)
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
started = time.monotonic()
def open_preview(delta):
    if time.monotonic() - started < 10:
        return
    unreal.unregister_slate_post_tick_callback(callback)
    unreal.get_editor_subsystem(unreal.AssetEditorSubsystem).open_editor_for_assets([
        unreal.load_asset(anim_root + 'Attack_PrimaryB')])
    unreal.EditorPythonScripting.set_keep_python_script_alive(False)
callback = unreal.register_slate_post_tick_callback(open_preview)
