"""UE-only: temporarily disable stone fixtures, retaining their items and transforms."""
import json
import os
from pathlib import Path
import unreal as u

readback = os.environ.get('UMBRA_STONE_DISABLED_READBACK') == '1'
level = u.get_editor_subsystem(u.LevelEditorSubsystem)
if globals().get('USE_CURRENT_LEVEL', False):
    assert u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world().get_name() == 'L_Prototype'
else:
    assert level.load_level('/Game/Maps/L_Prototype')
actors = u.get_editor_subsystem(u.EditorActorSubsystem)
report = []
for name in ('BP_StartupAllSmithingStones', 'BP_StartupAllSomberStones', 'BP_StartupSmithingStone'):
    bp = u.load_asset('/Game/Items/Tests/' + name)
    assert bp
    cls = bp.generated_class()
    cdo = u.get_default_object(cls)
    if not readback:
        cdo.set_editor_property('enabled', False)
        u.BlueprintEditorLibrary.compile_blueprint(bp)
        assert u.EditorAssetLibrary.save_loaded_asset(bp, False)
    assert not cdo.get_editor_property('enabled')
    matches = [actor for actor in actors.get_all_level_actors() if actor.get_class() == cls]
    assert matches, name
    for actor in matches:
        if not readback:
            actor.set_editor_property('enabled', False)
        assert not actor.get_editor_property('enabled')
        report.append(dict(actor=actor.get_path_name(), enabled=False))
if not readback:
    assert level.save_current_level()
out = Path(u.Paths.project_saved_dir()) / 'StartupStonesDisabled'
out.mkdir(exist_ok=True)
(out / ('readback.json' if readback else 'configuration.json')).write_text(json.dumps(report, indent=2), encoding='utf-8')
