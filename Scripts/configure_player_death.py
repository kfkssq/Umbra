"""Run in UE 5.8 editor: configure existing player assets, with backups."""
import datetime
import pathlib
import shutil
import unreal

root = pathlib.Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
bp = unreal.load_asset('/Game/Blueprints/Player/BP_UmbraPlayerCharacter')
library = unreal.load_asset('/Game/Asset/Audio/Libraries/DA_Audio_Default')
animation = unreal.load_asset('/Game/ParagonGreystone/Characters/Heroes/Greystone/Animations/Death')
sound = unreal.load_asset('/Game/ParagonGreystone/Audio/Cues/Greystone_Effort_Death')
assert bp and library and animation and sound
cdo = unreal.get_default_object(bp.generated_class())
assert isinstance(cdo, unreal.UmbraPlayerCharacter)
mesh = cdo.get_editor_property('mesh').get_editor_property('skeletal_mesh_asset')
assert mesh.get_editor_property('skeleton') == animation.get_editor_property('skeleton')
backup = root / 'Saved' / 'PlayerDeathBackup' / datetime.datetime.now().strftime('%Y%m%d_%H%M%S')
for asset in (bp, library):
    relative = asset.get_path_name().split('.')[0].removeprefix('/Game/') + '.uasset'
    target = backup / relative
    target.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(root / 'Content' / relative, target)
cdo.set_editor_property('death_animation', animation)
audio = cdo.get_component_by_class(unreal.UmbraAudioComponent)
audio.set_editor_property('character_type', 'Player.Greystone')
rows = list(library.get_editor_property('events'))
matches = [r for r in rows if r.get_editor_property('event') == unreal.UmbraAudioEvent.DEATH
           and str(r.get_editor_property('match').get_editor_property('character_type')) == 'Player.Greystone']
if matches:
    row = matches[0]
else:
    row = unreal.UmbraAudioEntry()
    row.set_editor_property('event', unreal.UmbraAudioEvent.DEATH)
    context = unreal.UmbraAudioContext()
    context.set_editor_property('character_type', 'Player.Greystone')
    row.set_editor_property('match', context)
    rows.append(row)
row.set_editor_property('category', unreal.UmbraAudioCategory.CHARACTER)
row.set_editor_property('sounds', [sound])
row.set_editor_property('spatial', True)
row.set_editor_property('probability', 1.0)
row.set_editor_property('pitch_min', 1.0)
row.set_editor_property('pitch_max', 1.0)
row.set_editor_property('cooldown', 0.0)
library.set_editor_property('events', rows)
for asset in (bp, library):
    assert unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False)
unreal.log('PLAYER_DEATH_CONFIGURED compatible skeleton; Death sequence and Effort_Death cue; backup=' + str(backup))
