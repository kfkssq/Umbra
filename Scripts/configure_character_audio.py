"""Configure player voice routing in UE 5.8; preserve unrelated audio rows."""
import datetime
import pathlib
import shutil
import unreal

root = pathlib.Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
paths = ['/Game/Asset/Audio/Libraries/DA_Audio_Default', '/Game/Blueprints/Player/BP_UmbraPlayerCharacter']
backup = root / 'Saved' / 'CharacterAudioBackup' / datetime.datetime.now().strftime('%Y%m%d_%H%M%S')
lib = unreal.load_asset(paths[0])
bp = unreal.load_asset(paths[1])
voice = unreal.load_asset('/Game/ParagonGreystone/Audio/Cues/Greystone_Ability_ScoredCrit')
assert lib and bp and voice
critical_voice = unreal.UmbraAudioEvent.CRITICAL_VOICE
for path in paths:
    relative = path.removeprefix('/Game/') + '.uasset'
    dest = backup / relative
    dest.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(root / 'Content' / relative, dest)
component = unreal.get_default_object(bp.generated_class()).get_component_by_class(unreal.UmbraAudioComponent)
assert component
component.set_editor_property('character_type', 'Player.Greystone')
rows = list(lib.get_editor_property('events'))
assert any(r.get_editor_property('event') == unreal.UmbraAudioEvent.HURT and str(r.get_editor_property('match').get_editor_property('character_type')) == 'Player.Greystone' and r.get_editor_property('sounds') for r in rows)
if not any(r.get_editor_property('event') == critical_voice and str(r.get_editor_property('match').get_editor_property('character_type')) == 'Player.Greystone' for r in rows):
    row = unreal.UmbraAudioEntry()
    row.set_editor_property('event', critical_voice)
    row.set_editor_property('category', unreal.UmbraAudioCategory.CHARACTER)
    context = unreal.UmbraAudioContext()
    context.set_editor_property('character_type', 'Player.Greystone')
    row.set_editor_property('match', context)
    row.set_editor_property('sounds', [voice])
    row.set_editor_property('spatial', True)
    row.set_editor_property('pitch_min', 1.0)
    row.set_editor_property('pitch_max', 1.0)
    row.set_editor_property('probability', 1.0)
    row.set_editor_property('cooldown', 2.0)
    rows.append(row)
lib.set_editor_property('events', rows)
for obj in (bp, lib):
    assert unreal.EditorAssetLibrary.save_loaded_asset(obj, only_if_is_dirty=False)
unreal.log('CHARACTER_AUDIO_CONFIGURED player=Player.Greystone CriticalVoice=ScoredCrit cooldown=2s backup=' + str(backup))
