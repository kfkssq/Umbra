"""Independent editor reload, repeat-write guard, dependency audit and isolated PIE fixture."""
import sys
import copy
import json
import time
from pathlib import Path
import unreal as u

sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
import equipment_pipeline as p

base=Path(__file__).parent
manifest=p.load_manifest(base/'samples.json')
saved=Path(u.Paths.project_saved_dir())/'EquipmentPipeline'
content=Path(u.Paths.project_content_dir())/'Items/Tests/PipelineM5'
before={str(f):p.digest(f) for f in content.rglob('*.uasset')}
for mode in ('CreateMissingOnly','UpdateManagedFields'):
    result=p.run(manifest,p.UnrealBackend(base),mode,True,report_path=saved/(mode+'.json'))
    p.require(result['status']=='success' and not result['saved_packages'],mode+' was not a zero-save repeat')
p.require(before=={str(f):p.digest(f) for f in content.rglob('*.uasset')},'Repeated run changed package bytes')
registry=u.AssetRegistryHelpers.get_asset_registry()
options=u.AssetRegistryDependencyOptions(include_soft_package_references=True,include_hard_package_references=True,
    include_searchable_names=False,include_soft_management_references=False,include_hard_management_references=False)
backend=p.UnrealBackend(base)
audit=[]
items=[]
for row in manifest['items']:
    start=time.perf_counter()
    asset=backend.load(row['asset_path'],'UmbraItemDefinition')
    values=backend.read(asset,set(p.desired_fields(row))|set(p.PHYSICS))
    p.require(all(values[k]==v for k,v in p.desired_fields(row).items()),'Reload mismatch '+row['id'])
    p.require(all(1<=v<=200 for v in values['world_drop_physics_extent']),'Invalid fitted bounds')
    backend.native_validate(asset)
    deps=sorted(str(d) for d in registry.get_dependencies(row['asset_path'],options))
    p.require(row['mesh']['asset_path'] in deps and row['icon']['asset_path'] in deps,'Missing registry dependencies')
    audit.append(dict(id=row['id'],physics={k:values[k] for k in p.PHYSICS},dependencies=deps,
                      warm_load_validate_ms=round((time.perf_counter()-start)*1000,3)))
    items.append(asset)
# Exercise the real adapter, preserving manual edits and restoring the owned fixture.
first=manifest['items'][0]
asset=items[0]
original_weight=asset.get_editor_property('weight')
try:
    asset.set_editor_property('weight',original_weight+17)
    conflict=p.run(manifest,p.UnrealBackend(base),'UpdateManagedFields',True,
                   ids=[first['id']],report_path=saved/'manual-conflict.json')
    p.require(conflict['status']=='blocked' and not conflict['saved_packages'],'Manual edit was not protected')
finally:
    asset.set_editor_property('weight',original_weight)
changed=copy.deepcopy(manifest)
changed['items'][0]['fields']['display_name']+=' verification'
try:
    update=p.run(changed,p.UnrealBackend(base),'UpdateManagedFields',True,ids=[first['id']],report_path=saved/'selected-update.json')
    p.require(update['status']=='success' and update['saved_packages']==[first['asset_path']],'Selected update failed')
finally:
    restored=p.run(manifest,p.UnrealBackend(base),'UpdateManagedFields',True,ids=[first['id']],report_path=saved/'selected-restore.json')
    p.require(restored['status']=='success','Fixture restoration failed')
final=p.run(manifest,p.UnrealBackend(base),'UpdateManagedFields',True,report_path=saved/'final-noop.json')
p.require(final['status']=='success' and not final['saved_packages'],'Final state is not idempotent')
(saved/'audit.json').write_text(json.dumps(dict(status='passed',repeat_package_hashes_unchanged=True,
    manual_conflict_protected=True,selected_update_restored=True,
    package_count=len(before),package_bytes=sum(Path(f).stat().st_size for f in before),items=audit),indent=2),encoding='utf-8')
# Separate fixture is intentionally outside the production manifest.
root=manifest['output_root']
path=root+'/BP_LootTest_PipelineM5'
if not backend.exists(path):
    factory=u.BlueprintFactory()
    factory.set_editor_property('parent_class',u.UmbraLootTestSpawner)
    bp=u.AssetToolsHelpers.get_asset_tools().create_asset('BP_LootTest_PipelineM5',root,u.Blueprint,factory)
    defaults=u.get_default_object(bp.generated_class())
    defaults.set_editor_property('items',items)
    defaults.set_editor_property('world_item_class',u.load_class(None,'/Game/Items/Loot/BP_WorldItem.BP_WorldItem_C'))
    context=u.UmbraGenerationContext()
    context.set_editor_property('source_level',10)
    defaults.set_editor_property('generation_context',context)
    defaults.set_editor_property('generation_seed',12345)
    u.BlueprintEditorLibrary.compile_blueprint(bp)
    backend.save(bp)
u.log('EQUIPMENT_M5_VERIFY_PASSED')
