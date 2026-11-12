"""Read-only Library identity/classification/requirements audit through the matching editor."""
import json
from pathlib import Path
import unreal as u

rows=[]
for path in sorted(u.EditorAssetLibrary.list_assets('/Game/EquipmentImport/Library',recursive=True,include_folder=False)):
    if '/Drafts/DA_Source_' not in path: continue
    item=u.load_asset(path)
    if not isinstance(item,u.UmbraItemDefinition): continue
    rows.append(dict(source=path,id=str(item.definition_id),name=str(item.display_name),
                     category=str(item.item_category),weapon_type=str(item.weapon_type),
                     requirements=item.requirements.export_text(),required_level=item.required_level))
output=Path(u.Paths.project_saved_dir())/'RandomEquipmentV2'
output.mkdir(exist_ok=True)
(output/'source-audit.json').write_text(json.dumps(rows,ensure_ascii=False,indent=2),encoding='utf-8')
