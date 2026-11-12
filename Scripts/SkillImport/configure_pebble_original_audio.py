"""Import original event media and configure the playable skill through UE APIs."""
import json,math
from pathlib import Path
import unreal as u
out=Path(u.Paths.project_saved_dir()).resolve()/'SkillImport/GlintstonePebble/OriginalAudio'
manifest=json.loads((out/'manifest.json').read_text(encoding='utf-8'))
assets=u.EditorAssetLibrary;tools=u.AssetToolsHelpers.get_asset_tools()
root='/Game/Skills/GlintstonePebble';waves={};impacts=[]
attenuation=assets.load_asset(root+'/ATT_Pebble') if assets.does_asset_exist(root+'/ATT_Pebble') else tools.create_asset(
    'ATT_Pebble',root,u.SoundAttenuation,u.SoundAttenuationFactory())
settings=attenuation.get_editor_property('attenuation')
settings.set_editor_properties(dict(attenuate=True,spatialize=True,
    attenuation_shape_extents=u.Vector(400,0,0),falloff_distance=2400.))
attenuation.set_editor_property('attenuation',settings);assets.save_loaded_asset(attenuation)
for entry in manifest['entries']:
    name=f"S_ER_Pebble_{entry['phase']}_{entry['wem']}"
    task=u.AssetImportTask();task.set_editor_properties(dict(filename=str(out/entry['wav']),
        destination_path=root,destination_name=name,automated=True,replace_existing=True,save=True))
    tools.import_asset_tasks([task]);sound=assets.load_asset(root+'/'+name)
    assert sound
    sound.set_editor_properties(dict(looping=entry['phase']=='Flight',volume=10**(entry['gain_db']/20),attenuation_settings=attenuation))
    sound.set_sound_asset_compression_type(u.SoundAssetCompressionType.PCM)
    assets.set_metadata_tag(sound,'OriginalAudio.Source',json.dumps(entry))
    assets.save_loaded_asset(sound)
    waves[entry['phase']]=sound
    if entry['phase']=='Impact':impacts.append(sound)
cue=assets.load_asset(root+'/SC_ER_Pebble_Impact') if assets.does_asset_exist(root+'/SC_ER_Pebble_Impact') else None
if not cue:
    cue=tools.create_asset('SC_ER_Pebble_Impact',root,u.SoundCue,u.SoundCueFactoryNew())
    assert u.UmbraSkillEditorLibrary.configure_pebble_impact_audio(cue,impacts)
cue.set_editor_property('attenuation_settings',attenuation);assets.save_loaded_asset(cue)
definition=assets.load_asset(root+'/DA_GlintstonePebble')
definition.set_editor_properties(dict(cast_sound=None,strike_sound=waves['Release'],
    projectile_sound=waves['Flight'],impact_sound=cue))
assets.set_metadata_tag(definition,'OriginalAudio.Manifest',manifest['bank_sha256'])
assets.save_loaded_asset(definition)
report={key:(definition.get_editor_property(key).get_path_name() if definition.get_editor_property(key) else None)
    for key in ['cast_sound','strike_sound','projectile_sound','impact_sound']}
report['flight_looping']=waves['Flight'].get_editor_property('looping')
report['impact_variants']=[s.get_path_name() for s in impacts]
assert report['flight_looping'] and report['cast_sound'] is None
(out/'configuration.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
u.log('PEBBLE_ORIGINAL_AUDIO_CONFIGURED')
