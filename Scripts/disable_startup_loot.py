"""UE-only: disable all prototype startup loot instances without removing their configuration."""
import json
from pathlib import Path
import unreal as u

level = u.get_editor_subsystem(u.LevelEditorSubsystem)
assert level.load_level('/Game/Maps/L_Prototype')
actors = u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors()
spawners = [a for a in actors if isinstance(a, u.UmbraLootTestSpawner)]
assert spawners, 'No startup spawners found'
report = []
for actor in spawners:
    report.append(dict(actor=actor.get_path_name(), previously_enabled=actor.get_editor_property('enabled')))
    actor.set_editor_property('enabled', False)
assert level.save_current_level()
out = Path(u.Paths.project_saved_dir()) / 'StartupLootDisabled'
out.mkdir(exist_ok=True)
(out / 'configuration.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
