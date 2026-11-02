"""Deterministic project-authored pipeline fixtures, in centimeters. No third-party geometry or images.
Icons are a fixed orthographic projection of the same OBJ geometry; not production artwork.
"""
import hashlib
import json
import math
from pathlib import Path
import struct
import zlib

BASE=Path(__file__).resolve().parent

def box(center,size):
    x,y,z=center; a,b,c=[v/2 for v in size]
    vertices=[(x+i*a,y+j*b,z+k*c) for i,j,k in [(-1,-1,-1),(1,-1,-1),(1,1,-1),(-1,1,-1),(-1,-1,1),(1,-1,1),(1,1,1),(-1,1,1)]]
    return vertices,[(0,3,2,1),(4,5,6,7),(0,1,5,4),(1,2,6,5),(2,3,7,6),(3,0,4,7)]

def geometry(kind,index):
    parts=([(0,0,45),(5+index,.8,62+index*3)],[(0,0,12),(20+index,3,3)],[(0,0,3),(3,3,16)]) if kind=="Sword" else (
        [(0,0,24),(26+index,9,30+index)], [(-14-index/2,0,34),(9,12,10)], [(14+index/2,0,34),(9,12,10)],[(0,0,7),(30+index,10,5)])
    vertices=[]; faces=[]
    for center,size in parts:
        v,f=box(center,size); offset=len(vertices)
        vertices.extend(v); faces.extend(tuple(i+offset for i in face) for face in f)
    return vertices,faces

def obj(vertices,faces):
    lines=["# Umbra M5 project-authored fixture; centimeters", "o Fixture", "usemtl Body"]
    lines += ["v %.6f %.6f %.6f" % v for v in vertices]
    lines += ["f "+" ".join(str(i+1) for i in f) for f in faces]
    return ("\n".join(lines)+"\n").encode()

def icon(vertices,faces):
    width=height=128
    view=[(.8*x-.6*y, .9*z+.15*x+.2*y, .6*x+.8*y-.2*z) for x,y,z in vertices]
    xmin,xmax=min(p[0] for p in view),max(p[0] for p in view)
    ymin,ymax=min(p[1] for p in view),max(p[1] for p in view)
    scale=108/max(xmax-xmin,ymax-ymin)
    points=[(64+(x-(xmin+xmax)/2)*scale,64-(y-(ymin+ymax)/2)*scale,d) for x,y,d in view]
    pixels=bytearray(width*height*4)
    def edge(a,b,p): return (p[0]-a[0])*(b[1]-a[1])-(p[1]-a[1])*(b[0]-a[0])
    for face in sorted(faces,key=lambda f:sum(points[i][2] for i in f)/len(f)):
        color=(125+face[0]%50,145+face[0]%50,165+face[0]%50,255)
        for triangle in ((face[0],face[1],face[2]),(face[0],face[2],face[3])):
            a,b,c=[points[i] for i in triangle]
            for py in range(max(0,int(min(a[1],b[1],c[1]))),min(height,int(max(a[1],b[1],c[1]))+1)):
                for px in range(max(0,int(min(a[0],b[0],c[0]))),min(width,int(max(a[0],b[0],c[0]))+1)):
                    values=[edge(a,b,(px+.5,py+.5)),edge(b,c,(px+.5,py+.5)),edge(c,a,(px+.5,py+.5))]
                    if min(values)>=0 or max(values)<=0:
                        at=(py*width+px)*4; pixels[at:at+4]=bytes(color)
    def chunk(tag,data): return struct.pack(">I",len(data))+tag+data+struct.pack(">I",zlib.crc32(tag+data)&0xffffffff)
    raw=b"".join(b"\0"+pixels[y*width*4:(y+1)*width*4] for y in range(height))
    return b"\x89PNG\r\n\x1a\n"+chunk(b"IHDR",struct.pack(">IIBBBBB",width,height,8,6,0,0,0))+chunk(b"IDAT",zlib.compress(raw,9))+chunk(b"IEND",b"")

def write_missing(path,data):
    if path.exists():
        if path.read_bytes()!=data:
            raise RuntimeError("Preserving changed fixture; inspect manually: "+str(path))
    else:
        path.parent.mkdir(parents=True,exist_ok=True); path.write_bytes(data)

def main():
    root="/Game/Items/Tests/PipelineM5"
    rows=[]
    for kind in ("Sword","Chest"):
        for index in range(1,6):
            name=kind+str(index).zfill(2); vertices,faces=geometry(kind,index)
            files={"mesh":(".obj",obj(vertices,faces)),"icon":(".png",icon(vertices,faces))}
            specs={}
            for label,(extension,data) in files.items():
                relative="Source/"+name+extension
                write_missing(BASE/relative,data)
                specs[label]={"source_file":relative,"sha256":hashlib.sha256(data).hexdigest(),"asset_path":root+("/Meshes/SM_" if label=="mesh" else "/Icons/T_")+name}
            specs["mesh"]["kind"]="static"
            fields=dict(display_name="Pipeline "+name,item_category="WEAPON" if kind=="Sword" else "ARMOR",
                        weapon_type="ONE_HANDED_SWORD" if kind=="Sword" else "UNKNOWN",
                        allowed_slots=["MAIN_HAND" if kind=="Sword" else "CHEST"],required_level=1,item_level=1,
                        armor=0 if kind=="Sword" else 10+index,magic_resistance=0,weight=2+index,
                        world_drop_scale=[1,1,1],world_drop_rotation=[0,0,0],world_drop_offset=[0,0,0],
                        generation_profile="/Game/Items/Tests/RandomMVP/DA_Generation_MVP",
                        weapon="/Game/Items/Weapons/DA_Weapon_TestSword" if kind=="Sword" else None)
            managed=sorted(set(fields)|{"definition_id","icon","world_drop_mesh","world_drop_skeletal_mesh"})
            rows.append(dict(id="PipelineM5."+name,asset_path=root+"/Definitions/DA_"+name,fields=fields,
                             managed_fields=managed,locked_fields=[],fit_physics=True,**specs,
                             provenance=dict(author="Umbra procedural test generator",license="Project-authored test fixture; no third-party input",
                                             record="PROVENANCE.md",status="project_authored")))
    data=json.dumps(dict(schema_version=1,manifest_id="umbra.m5.samples",output_root=root,items=rows),indent=2).encode()+b"\n"
    write_missing(BASE/"samples.json",data)
    print("10 deterministic mesh/icon/definition recipes generated")

if __name__=="__main__": main()
