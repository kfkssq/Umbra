"""Project-authored magic sounds, not extracted Elden Ring audio. Deterministic PCM WAV."""
import math, random, struct, wave
from pathlib import Path
out = Path(__file__).resolve().parents[2]/'Saved/SkillImport/GlintstonePebble/Audio'
out.mkdir(exist_ok=True)
rate = 48000
for name, duration, frequency in [('Cast', .32, 730), ('Release', .22, 1500), ('Impact', .48, 920)]:
    rng = random.Random(4000)
    samples=[]
    for i in range(round(rate*duration)):
        t=i/rate
        x=t/duration
        attack=min(t/.012,1)
        envelope=attack*((math.sin(math.pi*x/2)**.5) if name=='Cast' else math.exp(-5*x))
        envelope*=min((duration-t)/.025,1)
        # Chirped fundamentals plus inharmonic partials create a short glassy magic cue.
        phase=2*math.pi*frequency*(t+(.6 if name=='Cast' else -.25)*t*t/duration)
        tone=sum(math.sin(phase*r)*a for r,a in [(1,.45),(1.51,.2),(2.37,.12),(3.11,.07)])
        noise=rng.uniform(-1,1)*(.2 if name=='Release' else .1)
        samples.append(struct.pack('<h',int(max(-1,min(1,(tone+noise)*envelope*.5))*32767)))
    with wave.open(str(out/('S_Pebble_'+name+'.wav')),'wb') as stream:
        stream.setnchannels(1);stream.setsampwidth(2);stream.setframerate(rate);stream.writeframes(b''.join(samples))
    print(name,duration)
