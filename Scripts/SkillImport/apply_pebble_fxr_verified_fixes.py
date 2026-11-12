"""Apply only the source-verified absence of a separate release flash.

This does not promote the old cast/flight/impact approximations to FXR replicas.
"""
import json
from pathlib import Path
import unreal as u
out=Path(u.Paths.project_saved_dir()).resolve()/'SkillImport/GlintstonePebble'
audit=json.loads((out/'FxrAudit/manifest.json').read_text(encoding='utf-8'))
release=next(e for e in audit['effects'] if e['id']==523001)
assert release['semanticRoundtrip'] and not release['appearances']
definition=u.load_asset('/Game/Skills/GlintstonePebble/DA_GlintstonePebble')
before=definition.get_editor_property('strike_effect')
definition.set_editor_property('strike_effect',None)
u.EditorAssetLibrary.set_metadata_tag(definition,'FXRFidelity.Status','Incomplete: legacy cast/flight/impact approximations; standalone release flash removed from source evidence')
assert u.EditorAssetLibrary.save_loaded_asset(definition)
assert definition.get_editor_property('strike_effect') is None
assert definition.get_editor_property('strike_sound').get_name()=='S_ER_Pebble_Release_524617251'
(out/'FxrAudit/verified-fixes.json').write_text(json.dumps(dict(
    removed_release_effect=before.get_path_name() if before else None,
    release_sound=definition.get_editor_property('strike_sound').get_path_name(),
    full_reconstruction=False),indent=2),encoding='utf-8')
u.log('PEBBLE_VERIFIED_RELEASE_FIX_APPLIED')
