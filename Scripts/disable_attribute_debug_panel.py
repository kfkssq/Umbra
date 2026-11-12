"""UE-only: disable the prototype player's attribute debug panel, preserving its wiring."""
import json
from pathlib import Path
import unreal as u

bp = u.load_asset('/Game/Blueprints/Player/BP_UmbraPlayerController')
assert bp
cdo = u.get_default_object(bp.generated_class())
before = cdo.get_editor_property('enable_attribute_debug_panel')
cdo.set_editor_property('enable_attribute_debug_panel', False)
u.BlueprintEditorLibrary.compile_blueprint(bp)
assert u.EditorAssetLibrary.save_loaded_asset(bp, False)
out = Path(u.Paths.project_saved_dir()) / 'AttributeDebugDisabled'
out.mkdir(exist_ok=True)
(out / 'configuration.json').write_text(json.dumps(dict(previously_enabled=before, enabled=False)), encoding='utf-8')
