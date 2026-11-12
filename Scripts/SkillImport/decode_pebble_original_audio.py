"""Extract verified media from the locally extracted /enus/cs_main.bnk; no synthesis.

Requires vgmstream r2168 in Saved/SkillImport/AudioTools/vgmstream.
Event IDs and gains were resolved with wwiser v20260808 from the original bank.
"""
import hashlib,json,struct,subprocess,wave
from pathlib import Path
root=Path(__file__).resolve().parents[2]
out=root/'Saved/SkillImport/GlintstonePebble/OriginalAudio'
bank=out/'cs_main.bnk'
decoder=root/'Saved/SkillImport/AudioTools/vgmstream/vgmstream-cli.exe'
data=bank.read_bytes();chunks={};pos=0
while pos<len(data):
    name=data[pos:pos+4].decode('ascii');size=struct.unpack_from('<I',data,pos+4)[0]
    chunks[name]=data[pos+8:pos+8+size];pos+=8+size
index={i:(offset,size) for i,offset,size in struct.iter_unpack('<III',chunks['DIDX'])}
entries=[]
for phase,event,media,gain in [('Release',523001,524617251,-3),('Flight',523002,1040010023,-15),
        ('Impact',523003,82601330,-3),('Impact',523003,164447618,-3),('Impact',523003,123415567,-3)]:
    offset,size=index[media];wem=out/f'{media}.wem';wem.write_bytes(chunks['DATA'][offset:offset+size])
    raw=out/f'{media}-raw.wav';dest=out/f'{phase}_{media}.wav'
    metadata=subprocess.check_output([str(decoder),'-m',str(wem)],text=True)
    subprocess.run([str(decoder),'-i','-o',str(raw),str(wem)],check=True,capture_output=True)
    with wave.open(str(raw),'rb') as source:
        params=source.getparams();frames=source.readframes(source.getnframes())
    loop=None
    if phase=='Flight':
        assert params.framerate==44100 and '18687' in metadata
        loop=[0,18687];frames=frames[:18687*params.nchannels*params.sampwidth]
    with wave.open(str(dest),'wb') as target:
        target.setparams(params);target.writeframes(frames)
    entries.append(dict(phase=phase,event=f'Play_s{event:09d}',wem=media,gain_db=gain,
        data_offset=offset,size=size,wem_sha256=hashlib.sha256(wem.read_bytes()).hexdigest(),
        wav=dest.name,wav_sha256=hashlib.sha256(dest.read_bytes()).hexdigest(),
        loop_samples=loop,sample_rate=params.framerate,metadata=metadata))
manifest=dict(bank='/enus/cs_main.bnk',archive='ELDEN RING/Game/sd/sd.bdt',
    bank_sha256=hashlib.sha256(data).hexdigest(),decoder='vgmstream r2168',
    event_resolver='wwiser v20260808',entries=entries,
    limitations=['Default event gains only; game bus/RTPC/environment not reproduced',
        'UE impact random cue uses a three-item shuffle bag'])
(out/'manifest.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8')
print(json.dumps([{k:v for k,v in e.items() if k in ['phase','wem','wav','loop_samples']} for e in entries]))
