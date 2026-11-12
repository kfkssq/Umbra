"""Read existing UI trees and export existing warrior textures for visual selection."""
import json
import os
import unreal as u

out = os.path.join(u.Paths.project_saved_dir(), 'SkillPrototype')
os.makedirs(out, exist_ok=True)
report = {}
for path in ['/Game/UI/HUD/WBP_CombatHUD', '/Game/UI/HUD/WBP_PlayerHUDGroup',
             '/Game/Asset/classic-rpg-ui/Assets/ClassicRPGUI/Widgets/WB_HUD_SkillBar',
             '/Game/Blueprints/Player/BP_UmbraPlayerCharacter',
             '/Game/Blueprints/Player/BP_UmbraPlayerState',
             '/Game/Blueprints/BP_UmbraPlayerController']:
    obj = u.load_asset(path)
    if not obj: continue
    entry = {'class': obj.get_class().get_name()}
    try:
        tree = obj.get_editor_property('widget_tree')
        entry['tree'] = str(tree)
        entry['root'] = str(tree.get_editor_property('root_widget'))
    except Exception as e: entry['tree_error'] = str(e)
    report[path] = entry
icons = u.EditorAssetLibrary.list_assets('/Game/Asset/Icons/flat_skills_icons_no_color/warrior', recursive=True)
for path in icons:
    obj = u.load_asset(path)
    if not isinstance(obj, u.Texture2D): continue
    task = u.AssetExportTask()
    task.object = obj
    task.filename = os.path.join(out, obj.get_name() + '.png')
    task.automated = True
    task.prompt = False
    task.exporter = u.TextureExporterPNG()
    u.Exporter.run_asset_export_task(task)
with open(os.path.join(out, 'ui-audit.json'), 'w', encoding='utf-8') as f:
    json.dump(report, f, ensure_ascii=False, indent=2)
u.log('SKILL_UI_AUDIT_COMPLETE')
