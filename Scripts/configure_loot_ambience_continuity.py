"""UE-only: enable seek-preserving virtualization for currently configured rarity Waves."""
import json
import os
from pathlib import Path
import unreal as u

readback = os.environ.get('UMBRA_AUDIO_READBACK') == '1'
library = u.load_asset('/Game/Asset/Audio/Libraries/DA_Audio_Default')
sounds = {s.get_path_name(): s for entry in library.events if entry.event == u.UmbraAudioEvent.LOOT_RARITY
          for s in entry.sounds if s}
assert sounds
for sound in sounds.values():
    assert isinstance(sound, u.SoundWave) and sound.get_editor_property('looping') and sound.get_editor_property('duration') > 0, sound.get_path_name()
report = []
for path, sound in sounds.items():
    before = str(sound.get_editor_property('virtualization_mode'))
    if not readback:
        sound.set_editor_property('virtualization_mode', u.VirtualizationMode.SEEK_RESTART)
        assert u.EditorAssetLibrary.save_loaded_asset(sound, False)
    assert sound.get_editor_property('virtualization_mode') == u.VirtualizationMode.SEEK_RESTART
    report.append(dict(path=path, before=before, after=str(sound.get_editor_property('virtualization_mode')), duration=sound.get_editor_property('duration')))
out = Path(u.Paths.project_saved_dir()) / 'LootAmbienceContinuity'
out.mkdir(exist_ok=True)
(out / ('readback.json' if readback else 'assets.json')).write_text(json.dumps(report, indent=2), encoding='utf-8')
