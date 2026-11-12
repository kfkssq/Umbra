"""UE-only import of installed Epic template assets and isolated engineering VFX fixtures."""
import json
from pathlib import Path
import unreal as u
out=Path(u.Paths.project_saved_dir()).resolve()/'SkillImport/NativeBaseline'
out.mkdir(parents=True,exist_ok=True)
assets=u.EditorAssetLibrary
marker='/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple'
if not assets.does_asset_exist(marker):
    imported=u.UmbraSkillEditorLibrary.import_engine_character_baseline()
    assert imported>0,f'Character import failed ({imported}); existing assets are never overwritten'
else:imported=0
fixtures={}
for name,source in [('NS_TestFlight','/Niagara/DefaultAssets/Templates/Systems/FountainLightweight'),
                    ('NS_TestBurst','/Niagara/DefaultAssets/Templates/Systems/SimpleExplosion')]:
    path='/Game/SkillLab/Fixtures/'+name
    source_asset=u.load_asset(source)
    assert isinstance(source_asset,u.NiagaraSystem),source
    asset=u.load_asset(path) if assets.does_asset_exist(path) else u.AssetToolsHelpers.get_asset_tools().duplicate_asset(name,'/Game/SkillLab/Fixtures',source_asset)
    assert isinstance(asset,u.NiagaraSystem),source
    assets.set_metadata_tag(asset,'Umbra.Source',source)
    assets.set_metadata_tag(asset,'Umbra.Purpose','Engineering lifecycle fixture only; not final skill art or Elden Ring reconstruction')
    assert assets.save_loaded_asset(asset)
    fixtures[name]=asset.get_path_name()
(out/'import.json').write_text(json.dumps(dict(imported_character_packages=imported,
    character_packages_present=len(list((Path(u.Paths.project_content_dir())/'Characters').rglob('*.uasset'))),
    source='Associated UE 5.8.2 installed Templates/TemplateResources/High/Characters/Content',
    fixtures=fixtures,production_character_changed=False),indent=2),encoding='utf-8')
u.log('NATIVE_BASELINE_PREPARED')
