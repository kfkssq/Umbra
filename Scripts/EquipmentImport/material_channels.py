"""Explicit packed RG normal + blue gloss conversion; no format guessing.
NumPy is supplied by Blender. Input/output values are linear UNORM [0,1].
"""
def decode_rg_gloss(pixels, flip_green=False):
    import numpy as np
    p=np.asarray(pixels,dtype=np.float32)
    if p.ndim!=2 or p.shape[1]!=4 or not len(p):
        raise ValueError('Expected nonempty Nx4 RGBA')
    if not np.isfinite(p).all() or p.min()<0 or p.max()>1.00001:
        raise ValueError('Expected finite UNORM pixels')
    xy=p[:,:2]*2-1
    if flip_green: xy[:,1]*=-1
    squared=np.sum(xy*xy,axis=1)
    z=np.sqrt(np.maximum(0,1-squared))
    xyz=np.column_stack((xy,z))
    xyz/=np.maximum(np.linalg.norm(xyz,axis=1,keepdims=True),1e-8)
    normal=np.column_stack((xyz*.5+.5,np.ones(len(p),dtype=np.float32)))
    roughness=np.column_stack((np.repeat((1-p[:,2:3]),3,axis=1),np.ones(len(p),dtype=np.float32)))
    return normal,roughness,{'outside_unit_disk_fraction':float(np.mean(squared>1.001)),
                            'flip_green':bool(flip_green),'alpha_used':False}
