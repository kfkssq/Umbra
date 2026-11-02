"""Acceptance checks on isolated M1 fixtures only; original assets are never changed."""
import json
from pathlib import Path
import sys
import unreal as u
sys.path.insert(0,str(Path(__file__).resolve().parent))
from pipeline import run, sha, Editor, plan

root=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
folder=root/'Saved/EquipmentImport'
results={}
def snapshot():
    paths=[]
    for sub in ['Content/EquipmentImport','Content/Asset/Equipments','Content/Asset/Weapons','Content/Items/Armor','Content/Items/Weapons']:
        paths.extend((root/sub).rglob('*.uasset'))
    return {str(p.relative_to(root)):sha(p) for p in paths}

before=snapshot()
for label,manifest in [('samples',folder/'samples_validated.json'),('probes',folder/'Probes/manifest.json')]:
    dry=run(manifest,folder/('verify_dry_'+label+'.json'))
    assert dry['ok'] and not dry['saved'] and snapshot()==before
    result=run(manifest,folder/('verify_'+label+'.json'),apply=True)
    assert result['ok'], result
    assert not result['saved'], 'Repeated run wrote assets'
    assert all(r['status']=='preserved' for r in result['items'])
    results[label]={'preserved':len(result['items']),'saved':0}
    editor=Editor(result['output_root'])
    for row in plan(manifest)['items']:
        for spec in row['materials'].values():
            base=row['folder']+'/Textures/T_'+str(spec['index'])
            texture=u.load_asset(base+'_a')
            assert texture.srgb and texture.compression_settings==u.TextureCompressionSettings.TC_DEFAULT
            mi=u.load_asset(row['folder']+'/Materials/MI_'+str(spec['index']))
            assert u.MaterialEditingLibrary.get_material_instance_texture_parameter_value(mi,'BaseColor')==texture
            if spec.get('normal'):
                normal=u.load_asset(base+'_n')
                assert not normal.srgb and normal.compression_settings==u.TextureCompressionSettings.TC_NORMALMAP
                assert not normal.flip_green_channel
                assert u.MaterialEditingLibrary.get_material_instance_texture_parameter_value(mi,'Normal')==normal
            if spec['surface']=='Masked':
                assert mi.parent.blend_mode==u.BlendMode.BLEND_MASKED
            if spec['surface']=='Cloth':
                assert abs(u.MaterialEditingLibrary.get_material_instance_scalar_parameter_value(mi,'Roughness')-.85)<1e-5
assert snapshot()==before,'Package bytes changed during repeat/readback'
results['reload_and_repeat_hashes_unchanged']=True

# In-memory artist edits: the importer must not reset them or save them on rerun.
probe='/Game/EquipmentImport/Staging/M1Probes/WorldDrop/ProbeOpaque/Materials/MI_0'
mi=u.load_asset(probe)
old=u.MaterialEditingLibrary.get_material_instance_scalar_parameter_value(mi,'Roughness')
try:
    u.MaterialEditingLibrary.set_material_instance_scalar_parameter_value(mi,'Roughness',.123)
    result=run(folder/'Probes/manifest.json',folder/'verify_manual.json',apply=True,ids=['ProbeOpaque'])
    assert result['ok'] and not result['saved']
    assert abs(u.MaterialEditingLibrary.get_material_instance_scalar_parameter_value(mi,'Roughness')-.123)<1e-5
finally:
    u.MaterialEditingLibrary.set_material_instance_scalar_parameter_value(mi,'Roughness',old)
assert snapshot()==before
results['manual_instance_edit_preserved_without_save']=True

# A changed recipe must be rejected, not silently treated as an unchanged output.
changed=json.loads((folder/'samples_validated.json').read_text(encoding='utf-8-sig'))
changed['items'][0]['scale']=2
changed_path=folder/'changed_recipe_test.json'
changed_path.write_text(json.dumps(changed),encoding='utf-8')
result=run(changed_path,folder/'verify_conflict.json',apply=True,ids=[changed['items'][0]['id']])
assert not result['ok'] and not result['saved']
assert snapshot()==before
results['changed_recipe_rejected_without_writes']=True
(folder/'acceptance.json').write_text(json.dumps(results,indent=2),encoding='utf-8')
u.log('EquipmentImport acceptance passed')
