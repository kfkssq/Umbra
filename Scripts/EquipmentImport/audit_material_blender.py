"""Read-only source audit and isolated diagnostic outputs. Run in Blender with -- job.json."""
import hashlib
import json
from pathlib import Path
import struct
import sys
import bpy
import numpy as np
sys.path.insert(0,str(Path(__file__).resolve().parent))
from material_channels import decode_rg_gloss

def save_png(path,pixels,size):
    if path.exists(): raise ValueError('Existing output preserved: '+str(path))
    image=bpy.data.images.new(path.stem,width=size[0],height=size[1],alpha=True,float_buffer=False)
    image.colorspace_settings.name='Non-Color'
    image.alpha_mode='CHANNEL_PACKED'
    image.pixels.foreach_set(np.asarray(pixels,dtype=np.float32).reshape(-1))
    image.filepath_raw=str(path); image.file_format='PNG'; image.save()
    bpy.data.images.remove(image)

job=json.loads(Path(sys.argv[sys.argv.index('--')+1]).read_text(encoding='utf-8-sig'))
folder=Path(job['source']); output=Path(job['output'])
output.mkdir(parents=True,exist_ok=True)
report={'scope':'am_m_1140 only; candidate conversion, not original shader reconstruction','textures':[]}
for path in sorted(folder.glob('*.dds')):
    b=path.read_bytes()
    record={'source':str(path),'sha256':hashlib.sha256(b).hexdigest(),
            'fourcc':b[84:88].decode('ascii',errors='replace'),
            'dxgi_format':struct.unpack_from('<I',b,128)[0] if b[84:88]==b'DX10' else None}
    image=bpy.data.images.load(str(path),check_existing=False)
    image.colorspace_settings.name='Non-Color'; image.alpha_mode='CHANNEL_PACKED'
    size=list(image.size)
    pixels=np.empty(len(image.pixels),dtype=np.float32); image.pixels.foreach_get(pixels)
    p=pixels.reshape(-1,4)
    record.update(size=size,percentiles={name:np.percentile(p[:,i],[0,1,25,50,75,99,100]).tolist() for i,name in enumerate('RGBA')},
                  alpha_not_one_fraction=float(np.mean(p[:,3]<.999)))
    # Four equal-sized grayscale panels ordered R,G,B,A, for visual channel inspection.
    small=p.reshape(size[1],size[0],4)[::max(1,size[1]//256),::max(1,size[0]//256)]
    panels=[np.concatenate((np.repeat(small[:,:,i:i+1],3,axis=2),np.ones((*small.shape[:2],1))),axis=2) for i in range(4)]
    sheet=np.concatenate(panels,axis=1)
    save_png(output/(path.stem+'_RGBA.png'),sheet,(sheet.shape[1],sheet.shape[0]))
    if path.stem.endswith('_n'):
        n,r,metrics=decode_rg_gloss(p,False)
        record['candidate']=metrics
        record['candidate']['roughness_range']=[float(r[:,0].min()),float(r[:,0].max())]
        save_png(output/(path.stem+'_NormalCandidate.png'),n,size)
        save_png(output/(path.stem+'_RoughnessCandidate.png'),r,size)
        # Alternate sign is only for comparison, never silently chosen.
        flipped,_,_=decode_rg_gloss(p,True)
        save_png(output/(path.stem+'_NormalGreenFlippedComparison.png'),flipped,size)
    bpy.data.images.remove(image)
    report['textures'].append(record)
(output/'audit.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
