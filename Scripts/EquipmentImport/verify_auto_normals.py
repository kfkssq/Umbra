"""Fresh-editor readback: direct source PNG, normal compression, MI references, no inferred mask."""
import json
import os
from pathlib import Path
import hashlib
import unreal as u

folder=Path(os.environ['UMBRA_ITEM_PLAN']).parent
report=json.loads((folder/'meshes_report.json').read_text(encoding='utf-8'))
assert report['ok']
checks=[]
for row in report['items']:
    for slot,spec in row['materials'].items():
        texture=u.load_asset(row['folder']+'/Textures/T_'+str(spec['index'])+'_n')
        mi=u.load_asset(row['folder']+'/Materials/MI_'+str(spec['index']))
        assert isinstance(texture,u.Texture2D)
        assert texture.compression_settings==u.TextureCompressionSettings.TC_NORMALMAP
        assert not texture.srgb and not texture.flip_green_channel
        assert texture.lod_group==u.TextureGroup.TEXTUREGROUP_WORLD_NORMAL_MAP
        assert u.MaterialEditingLibrary.get_material_instance_texture_parameter_value(mi,'Normal')==texture
        imported=Path(texture.get_editor_property('asset_import_data').get_first_filename())
        assert hashlib.sha256(imported.read_bytes()).hexdigest()==spec['normal_sha']
        assert imported.name.lower().endswith('_n.png'), imported
        assert 'roughness_texture' not in spec
        checks.append({'model':row['id'],'slot':slot,'normal':texture.get_path_name(),'source':str(imported),'direct_source_hash_matches':True})
(folder/'normal_acceptance.json').write_text(json.dumps({'ok':True,'checks':checks},indent=2),encoding='utf-8')
