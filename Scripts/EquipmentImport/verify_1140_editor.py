"""Verify isolated candidate, compile with real RHI, and prove Test01 bytes unchanged."""
import json
from pathlib import Path
import sys
import unreal as u
sys.path.insert(0,str(Path(__file__).resolve().parent))
from pipeline import run,sha
root=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
out=root/'Saved/EquipmentImport/Material1140'
old=json.loads((out/'ue_original_audit.json').read_text(encoding='utf-8'))['package_hashes']
assert all(sha(p)==digest for p,digest in old.items()), 'Original Test01 changed since audit'
base='/Game/EquipmentImport/Staging/Material1140Review/WorldDrop/am_m_1140_AM_M_1140'
mi=u.load_asset(base+'/Materials/MI_0')
assert isinstance(mi,u.MaterialInstanceConstant)
ml=u.MaterialEditingLibrary
textures={}
for parameter,suffix in [('BaseColor','a'),('Normal','n'),('RoughnessMap','r')]:
    tex=u.load_asset(base+'/Textures/T_0_'+suffix)
    assert ml.get_material_instance_texture_parameter_value(mi,parameter)==tex
    assert tex.srgb==(suffix=='a')
    if suffix=='n':
        assert tex.compression_settings==u.TextureCompressionSettings.TC_NORMALMAP
        assert not tex.flip_green_channel
    textures[parameter]={'path':tex.get_path_name(),'srgb':tex.srgb,'compression':str(tex.compression_settings)}
assert abs(ml.get_material_instance_scalar_parameter_value(mi,'Roughness')-1)<1e-6
assert abs(ml.get_material_instance_scalar_parameter_value(mi,'Metallic'))<1e-6
errors=list(ml.recompile_material(mi.parent))
assert not errors, errors
ml.update_material_instance(mi)
stats=ml.get_statistics(mi)
assert stats.num_samplers>0, 'No compiled shader statistics; launch with -AllowCommandletRendering'
report={'textures':textures,'roughness_multiplier':1,'metallic':0,'compile_errors':errors,
        'shader_statistics':{key:getattr(stats,key) for key in ['num_pixel_shader_instructions','num_vertex_shader_instructions','num_samplers']}}
reports=sorted((root/'Saved/EquipmentImport/DCX/Runs').glob('*/report.json'),reverse=True)
manifest=None
for path in reports:
    r=json.loads(path.read_text(encoding='utf-8'))
    if r.get('batch')=='Material1140Review' and r.get('ok'):
        manifest=r['manifest']; break
assert manifest
before={str(p):sha(p) for p in (root/'Content/EquipmentImport/Staging/Material1140Review').rglob('*.uasset')}
repeat=run(manifest,out/'repeat.json',apply=True)
assert repeat['ok'] and not repeat['saved']
assert all(sha(p)==digest for p,digest in before.items())
assert all(sha(p)==digest for p,digest in old.items())
report.update(repeat_saved=repeat['saved'],original_test01_unchanged=True,candidate_packages_unchanged=True)
(out/'acceptance.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
u.log('1140 material candidate checks passed')
