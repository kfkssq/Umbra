"""DCX folder -> isolated unpack/cache -> Blender -> existing UE pipeline.
Default inventory only. --apply performs conversion/import; --convert-only skips UE.
"""
import argparse
import datetime
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import uuid
from pipeline import sha, stable

HERE=Path(__file__).resolve().parent
PROJECT=HERE.parents[1]

def write(path,data):
    path.parent.mkdir(parents=True,exist_ok=True)
    temporary=path.with_suffix(path.suffix+'.tmp')
    temporary.write_text(json.dumps(data,ensure_ascii=False,indent=2),encoding='utf-8')
    temporary.replace(path)

def execute(args,log,env=None,timeout=900,console_only=False):
    # Witchy requires a Windows console, even when its stdout is redirected.
    flags=0
    startup=None
    if os.name=='nt':
        flags=subprocess.CREATE_NEW_CONSOLE
        startup=subprocess.STARTUPINFO()
        startup.dwFlags|=subprocess.STARTF_USESHOWWINDOW
        startup.wShowWindow=0
    if console_only:
        # PromptPlus rejects redirected handles even in silent mode. Give Witchy a
        # hidden native console; validate its exit code AND expected output files.
        process=subprocess.run([str(a) for a in args],env=env,timeout=timeout,creationflags=flags,startupinfo=startup)
        log.write_text('Hidden native console (Witchy forbids redirection). Exit='+str(process.returncode),encoding='utf-8')
    else:
        with log.open('w',encoding='utf-8') as stream:
            process=subprocess.run([str(a) for a in args],stdout=stream,stderr=subprocess.STDOUT,
                stdin=subprocess.DEVNULL,env=env,timeout=timeout,creationflags=flags,startupinfo=startup)
    if process.returncode:
        raise RuntimeError('Process exit '+str(process.returncode)+'; see '+str(log))

def inventory(source,names=None):
    files=sorted(source.glob('*.partsbnd.dcx'))
    selected=[]; skipped=[]
    lookup={p.name[:-len('.partsbnd.dcx')]:p for p in files}
    if names and set(names)-set(lookup):
        raise ValueError('Unknown archive names: '+str(set(names)-set(lookup)))
    for identity,path in lookup.items():
        if names and identity not in names: continue
        if identity.endswith('_l'):
            skipped.append({'id':identity,'reason':'low-resolution variant excluded; use full-resolution counterpart'})
        elif not re.fullmatch(r'(?:wp_a|(?:am|bd|hd|lg)_[mf])_[0-9]+',identity):
            skipped.append({'id':identity,'reason':'unsupported part naming/category'})
        else: selected.append((identity,path))
    return selected,skipped


def shared_textures(source,config,workspace,fingerprint):
    archive=source/'common_body.tpf.dcx'
    if not archive.is_file():return None
    digest=sha(archive)
    key=hashlib.sha256(stable({'source':digest,**fingerprint}).encode()).hexdigest()[:20]
    cache=workspace/'Shared'/key;complete=cache/'complete.json'
    if complete.exists():
        receipt=json.loads(complete.read_text(encoding='utf-8'))
        for name,value in receipt['files'].items():
            if not (cache/name).is_file() or sha(cache/name)!=value:raise ValueError('Shared texture cache modified; preserve for review')
        return str(cache/receipt['report'])
    attempt=cache/('attempt_'+uuid.uuid4().hex[:8]);attempt.mkdir(parents=True)
    copied=attempt/archive.name;shutil.copy2(archive,copied)
    if sha(copied)!=digest:raise ValueError('Shared source changed')
    execute([config['witchy'],'-s','-p','-u','-c',copied],attempt/'witchy.log',timeout=300,console_only=True)
    output=attempt/'Converted'
    write(attempt/'job.json',{'addon':config['addon'],'unpacked':str(attempt),'output':str(output),'textures_only':True})
    execute([config['blender'],'--background','--factory-startup','--python-exit-code','1','--python',HERE/'blender_dcx.py','--',attempt/'job.json'],attempt/'blender.log')
    if sha(archive)!=digest:raise ValueError('Shared source changed during conversion')
    report=output/'conversion.json'
    write(complete,{'report':str(report.relative_to(cache)),
         'files':{str(p.relative_to(cache)):sha(p) for p in output.iterdir() if p.is_file()}})
    return str(report)

def run(args):
    source=Path(args.source).resolve()
    if not source.is_dir(): raise ValueError('Source directory missing')
    if not re.fullmatch('[A-Za-z][A-Za-z0-9_]*',args.batch): raise ValueError('Invalid batch identifier')
    config=json.loads(Path(args.tools).read_text(encoding='utf-8-sig'))
    rules_path=Path(args.rules).resolve()
    rules=json.loads(rules_path.read_text(encoding='utf-8-sig'))
    selected,skipped=inventory(source,args.names.split(',') if args.names else None)
    workspace=PROJECT/'Saved/EquipmentImport/DCX'
    if workspace.is_relative_to(source) or source.is_relative_to(workspace):
        raise ValueError('Source must be separate from generated workspace')
    stamp=datetime.datetime.now(datetime.timezone.utc).strftime('%Y%m%dT%H%M%SZ')+'_'+uuid.uuid4().hex[:8]
    report_path=workspace/'Runs'/stamp/'report.json'
    report={'apply':args.apply,'source':str(source),'batch':args.batch,'items':[], 'skipped':skipped,'ok':False}
    print('REPORT '+str(report_path),flush=True)
    manifest={'schema_version':1,'source_root':str(workspace),'output_root':'/Game/EquipmentImport/Staging/'+args.batch,
              'items':[],'scan_warnings':[]}
    try:
        for key in ('blender','witchy','addon','ue_root'):
            if not Path(config[key]).exists(): raise ValueError('Missing tool: '+key)
        plugin=Path(config['addon'])
        fingerprint={'converter':sha(HERE/'blender_dcx.py'),'channel_converter':sha(HERE/'material_channels.py'),'normal_rules':sha(HERE/'normal_rules.py'),'matbin_rules':sha(HERE/'matbin_rules.py'),'vertex_layout':sha(HERE/'flver_layout.py'),'rules':sha(rules_path),
                     'blender':sha(config['blender']),'witchy':sha(config['witchy']),
                     'plugin':{p.name:sha(p) for p in plugin.glob('*.py')}}
        if config.get('material_index'):fingerprint['material_index']=sha(config['material_index'])
        common=source/'common_body.tpf.dcx'
        if common.is_file():fingerprint['common_body']=sha(common)
        shared=shared_textures(source,config,workspace,fingerprint) if args.apply else None
        report['shared_textures']=shared
        if args.apply and not args.convert_only:
            version=json.loads((Path(config['ue_root'])/'Engine/Build/Build.version').read_text())
            association=json.loads((PROJECT/'Umbra.uproject').read_text())['EngineAssociation']
            if f"{version['MajorVersion']}.{version['MinorVersion']}"!=association:
                raise ValueError('UE version mismatch')
            if os.name=='nt':
                running=subprocess.check_output(['tasklist','/FO','CSV','/NH'],creationflags=subprocess.CREATE_NO_WINDOW).decode(errors='replace')
                if re.search(r'"UnrealEditor(?:-Cmd)?\.exe"',running,re.I):
                    raise ValueError('Close Unreal Editor before apply')
        for identity,path in selected:
            row={'id':identity,'source':str(path),'status':'planned'}
            report['items'].append(row)
            try:
                digest=sha(path)
                signature=hashlib.sha256(stable({'source':digest,**fingerprint}).encode()).hexdigest()
                cache=workspace/'Cache'/identity/signature[:20]
                row.update(source_sha256=digest,cache=str(cache))
                if not args.apply:
                    row['actions']=['copy source','Witchy unpack','Blender convert + material rules','UE create-only import']
                    continue
                completed=cache/'complete.json'
                if completed.exists():
                    receipt=json.loads(completed.read_text(encoding='utf-8'))
                    for name,d in receipt['files'].items():
                        candidate=cache/name
                        if not candidate.is_file() or sha(candidate)!=d:
                            raise ValueError('Generated cache changed; preserve and use a new cache/workspace after review')
                    converted=cache/receipt['converted']
                    row['cache_reused']=True
                else:
                    # Failed attempts remain inspectable; retries never overwrite them.
                    attempt=cache/('attempt_'+uuid.uuid4().hex[:8])
                    attempt.mkdir(parents=True)
                    copied=attempt/path.name
                    shutil.copy2(path,copied)
                    if sha(copied)!=digest: raise ValueError('Source changed while copying')
                    execute([config['witchy'],'-s','-p','-u','-c',copied],attempt/'witchy.log',timeout=300,console_only=True)
                    unpacked=attempt/(identity+'-partsbnd-dcx')
                    if not (unpacked/'_witchy-bnd4.xml').is_file():
                        raise ValueError('Witchy did not produce expected BND manifest')
                    converted=attempt/'Converted'
                    job={'addon':config['addon'],'unpacked':str(unpacked),'output':str(converted),
                         'ue_root':manifest['output_root'],'rules':rules,
                         'material_index':config.get('material_index'),
                         'shared_textures':shared,
                         'provenance':'User-local Elden Ring archive '+path.name+' SHA256 '+digest+'; original provenance retained'}
                    write(attempt/'job.json',job)
                    row['conversion_report']=str(converted/'conversion.json')
                    execute([config['blender'],'--background','--factory-startup','--python-exit-code','1',
                             '--python',HERE/'blender_dcx.py','--',attempt/'job.json'],attempt/'blender.log')
                    if sha(path)!=digest: raise ValueError('Original DCX changed during processing')
                    receipt={'converted':str(converted.relative_to(cache)),
                             'files':{str(p.relative_to(cache)):sha(p) for p in converted.iterdir() if p.is_file()}}
                    write(completed,receipt)
                    row['cache_reused']=False
                converted_manifest=json.loads((converted/'manifest.json').read_text(encoding='utf-8'))
                row['conversion_report']=str(converted/'conversion.json')
                row['model_ids']=[]
                for item in converted_manifest['items']:
                    item['id']=identity+'_'+item['id']
                    item['model']=str((converted/item['model']).relative_to(workspace))
                    for spec in item['materials'].values():
                        for key in ('base_color','normal','roughness_texture'):
                            if key in spec: spec[key]=str((converted/spec[key]).relative_to(workspace))
                    manifest['items'].append(item)
                    row['model_ids'].append(item['id'])
                manifest['scan_warnings'].extend(converted_manifest['scan_warnings'])
                row['status']='converted'
            except Exception as exc:
                row.update(status='failed',error=str(exc))
                detail=Path(row.get('conversion_report','__missing__'))
                if detail.is_file():
                    conversion=json.loads(detail.read_text(encoding='utf-8'))
                    row['conversion_errors']=[{'source':r['source'],'error':r['error']} for r in conversion.get('models',[])+conversion.get('textures',[]) if r.get('error')]
            finally:
                print(identity+': '+row['status'],flush=True)
                write(report_path,report)
        manifest_path=report_path.parent/'manifest.json'
        write(manifest_path,manifest)
        report['manifest']=str(manifest_path)
        if args.apply and not args.convert_only and manifest['items']:
            env=dict(os.environ,UMBRA_IMPORT_MANIFEST=str(manifest_path),UMBRA_IMPORT_REPORT=str(report_path.parent/'ue.json'),
                     UMBRA_IMPORT_APPLY='1',UMBRA_IMPORT_IDS='')
            ue=Path(config['ue_root'])/'Engine/Binaries/Win64/UnrealEditor-Cmd.exe'
            try:
                execute([ue,PROJECT/'Umbra.uproject','-run=pythonscript','-script='+str(HERE/'entry.py'),
                         '-EnablePlugins=PythonScriptPlugin,EditorScriptingUtilities','-ExecCmds=Interchange.FeatureFlags.Import.FBX 0',
                         '-unattended','-nop4','-nullrhi','-nosound','-stdout','-abslog='+str(report_path.parent/'ue_engine.log')],
                        report_path.parent/'ue_process.log',env=env,timeout=7200)
            finally:
                ue_report=report_path.parent/'ue.json'
                report['ue_report']=str(ue_report)
                if ue_report.exists():
                    result=json.loads(ue_report.read_text(encoding='utf-8'))
                    report['saved']=result['saved']
                    states={r['id']:r for r in result['items']}
                    for row in report['items']:
                        if row['status']=='converted':
                            failed=[i for i in row['model_ids'] if i not in states or states[i]['status']=='failed']
                            row['status']='failed' if failed else 'imported'
                            row['failed_model_ids']=failed
                    if result.get('fatal'): report['fatal']=result['fatal']
                    engine_log=report_path.parent/'ue_engine.log'
                    if engine_log.exists():
                        report['engine_warnings']=[line for line in engine_log.read_text(encoding='utf-8',errors='replace').splitlines()
                            if any(tag in line for tag in ('FBXImport: Warning:','LogMaterial: Warning:','LogTexture: Warning:'))]
        report['ok']=bool(selected) and not report.get('fatal') and all(r['status']!='failed' for r in report['items'])
    except Exception as exc:
        report['fatal']=str(exc)
    finally:
        report['failed_ids']=[r['id'] for r in report['items'] if r['status']=='failed']
        write(report_path,report)
    return report

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source',required=True)
    parser.add_argument('--batch',required=True)
    parser.add_argument('--tools',required=True)
    parser.add_argument('--rules',default=str(HERE/'dcx_rules.json'))
    parser.add_argument('--names',default='')
    parser.add_argument('--apply',action='store_true')
    parser.add_argument('--convert-only',action='store_true')
    options=parser.parse_args()
    if options.convert_only and not options.apply: parser.error('--convert-only requires --apply')
    result=run(options)
    print('OK' if result['ok'] else 'INCOMPLETE',flush=True)
    sys.exit(0 if result['ok'] else 1)
