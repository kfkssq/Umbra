"""UE commandlet entry. Settings supplied by Run.ps1 through inherited environment."""
import os
import sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parent))
from pipeline import run
import unreal

unreal.SystemLibrary.execute_console_command(None,'Interchange.FeatureFlags.Import.FBX 0')
result = run(os.environ['UMBRA_IMPORT_MANIFEST'], os.environ['UMBRA_IMPORT_REPORT'],
             apply=os.environ.get('UMBRA_IMPORT_APPLY')=='1',
             ids=os.environ.get('UMBRA_IMPORT_IDS','').split(',') if os.environ.get('UMBRA_IMPORT_IDS') else None)
if not result['ok']:
    raise RuntimeError('Equipment import failed; see '+os.environ['UMBRA_IMPORT_REPORT'])
