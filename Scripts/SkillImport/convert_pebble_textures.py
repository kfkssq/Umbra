"""Decode DDS to RGBA PNG for UE material authoring; source files stay read-only."""
from pathlib import Path
import sys
root = Path(__file__).resolve().parents[2]/'Saved/SkillImport/GlintstonePebble'
sys.path.insert(0, str(root.parent/'ImageDependencies'))
from PIL import Image
out = root/'Textures'
out.mkdir(exist_ok=True)
for path in sorted(root.glob('*.dds')):
    with Image.open(path) as image:
        image.convert('RGBA').save(out/(path.stem+'.png'))
        print('PEBBLE_TEXTURE', path.name, image.size, flush=True)
