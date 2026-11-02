"""Exact MATBIN sampler resolution for explicitly supported shader families.

FLVER nonempty paths override MATBIN defaults. No nearest-name or numbered fallback.
This is base-layer PBR translation; detail blending and packed masks are not reproduced.
"""
import argparse
import json
from pathlib import Path


def filename(path):
    return Path(path.replace('\\','/')).name.casefold()


def build_index(folder, output):
    entries={}; conflicts=set(); sources=[]
    for path in sorted(Path(folder).glob('*.dcx.json')):
        data=json.loads(path.read_text(encoding='utf-8'))
        sources.append({'archive':data['archive'],'sha256':data['sha256']})
        for row in data['entries']:
            key=Path(filename(row['name'])).stem
            previous=entries.get(key)
            if previous and any(previous[k]!=row[k] for k in ('shader','parameters','samplers')):
                conflicts.add(key)
            else:entries[key]=row
    for key in conflicts:entries.pop(key,None)
    result={'schema_version':1,'sources':sources,'entries':entries,'conflicts':sorted(conflicts)}
    target=Path(output)
    if target.exists():raise ValueError('Preserve existing index: '+str(target))
    target.write_text(json.dumps(result,ensure_ascii=False),encoding='utf-8')
    return result


def resolve_material(record,index):
    key=Path(filename(record['template'])).stem
    if key in index.get('conflicts',[]):raise ValueError('Conflicting MATBIN definitions: '+key)
    definition=index['entries'].get(key)
    if not definition:raise ValueError('MATBIN definition missing: '+key)
    shader=filename(definition['shader'])
    if shader in ('c[detailblend].spx','c[detailblend]_cloth.spx'):
        base_type='C_DetailBlend__snp_Texture2D_7_AlbedoMap'
        normal_type='C_DetailBlend__snp_Texture2D_0_NormalMap'
    elif shader in ('c[detailblend][s2].spx','c[detailblend][s2]_cloth.spx'):
        base_type='C_DetailBlend_S2__snp_Texture2D_7_AlbedoMap'
        normal_type='C_DetailBlend_S2__snp_Texture2D_0_NormalMap'
    elif shader in ('c[amsn].spx','c[amsn]_e.spx'):
        base_type='C_AMSN__snp_Texture2D_2_AlbedoMap_0'
        normal_type='C_AMSN__snp_Texture2D_7_NormalMap_4'
    else:raise ValueError('Shader family requires separate rule: '+shader)
    params={p['name']:p['value'] for p in definition['parameters']}
    if params.get('g_BlendMode')!=0:raise ValueError('Nonopaque blend mode requires separate rule: '+str(params.get('g_BlendMode')))
    if params.get('g_GX_bDisableDepthWrite') or params.get('g_GX_bMeshDecal'):
        raise ValueError('Depth/decal material requires separate rule')
    uv=params.get('group_1_CommonUV-UVParam',[1,1])
    if tuple(uv)!=(1,1):raise ValueError('Primary MATBIN UV transform requires separate rule')
    refs={}
    for t in record['references']:
        if t['type_name'] in refs:raise ValueError('Duplicate FLVER sampler')
        refs[t['type_name']]=t
    defaults={t['type']:t['path'] for t in definition['samplers']}
    def texture(kind):
        ref=refs.get(kind)
        if ref and tuple(ref.get('scale',[1,1]))!=(1,1):raise ValueError('Primary FLVER UV transform requires separate rule')
        path=(ref or {}).get('path') or defaults.get(kind)
        if not path:raise ValueError('Primary sampler has no texture: '+kind+'; constant/detail-only material requires separate rule')
        return Path(filename(path)).stem,{'type_name':kind,'path':path,'scale':[1,1]}
    base,_=texture(base_type);normal,normal_ref=texture(normal_type)
    alpha=params.get('g_AlphaRef')
    if not isinstance(alpha,list) or len(alpha)!=2 or not all(isinstance(x,(float,int)) and 0<=x<=255 for x in alpha):
        raise ValueError('Unsupported AlphaRef format')
    omitted=[{'type':t['type'],'path':refs.get(t['type'],{}).get('path') or t['path']} for t in definition['samplers'] if t['type'] not in (base_type,normal_type)]
    return {'base_key':base,'normal_key':normal,'normal_reference':normal_ref,
            'alpha_policy':'mask' if alpha[0]>0 else 'ignore','alpha_clip':alpha[0]/255.0,
            'two_sided':bool(params.get('g_GX_PrimCullNone',False)),
            'definition_sha256':definition['sha256'],'shader':shader,
            'source_alpha_ref':alpha,'omitted_samplers':[t for t in omitted if t['path']],
            'scope':'Base layer only; source AlphaRef.x threshold /255; secondary AlphaRef and detail/mask mixing not reproduced'}


if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--folder',required=True);p.add_argument('--output',required=True)
    a=p.parse_args();r=build_index(a.folder,a.output);print(len(r['entries']),'definitions;',len(r['conflicts']),'conflicts blocked')
