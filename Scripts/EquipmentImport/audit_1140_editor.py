"""Read-only snapshot of the user's Test01 asset; never saves a package."""
import hashlib
import json
from pathlib import Path
import unreal as u
root=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
asset_root='/Game/EquipmentImport/Staging/Test01'
report={'assets':[],'package_hashes':{}}
for path in u.EditorAssetLibrary.list_assets(asset_root,recursive=True,include_folder=False):
    obj=u.load_asset(path)
    record={'path':path,'class':obj.get_class().get_name()}
    package=obj.get_path_name().split('.')[0]
    local=root/'Content'/(package.removeprefix('/Game/')+'.uasset')
    report['package_hashes'][str(local)]=hashlib.sha256(local.read_bytes()).hexdigest()
    if isinstance(obj,u.MaterialInstanceConstant):
        record['parent']=obj.parent.get_path_name() if obj.parent else None
        record['scalars']={p:u.MaterialEditingLibrary.get_material_instance_scalar_parameter_value(obj,p) for p in ['Metallic','Roughness']}
        record['textures']={}
        for p in ['BaseColor','Normal','RoughnessMap']:
            t=u.MaterialEditingLibrary.get_material_instance_texture_parameter_value(obj,p)
            record['textures'][p]=t.get_path_name() if t else None
    elif isinstance(obj,u.Texture2D):
        record.update(srgb=obj.srgb,compression=str(obj.compression_settings))
    elif isinstance(obj,u.SkeletalMesh):
        record.update(skeleton=obj.skeleton.get_path_name(),slots={str(s.material_slot_name):s.material_interface.get_path_name() if s.material_interface else None for s in obj.materials})
    report['assets'].append(record)
dest=root/'Saved/EquipmentImport/Material1140/ue_original_audit.json'
dest.parent.mkdir(parents=True,exist_ok=True)
dest.write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
