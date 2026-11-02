"""Folder/catalog one-command import. Default dry run; immutable selection + bounded UE chunks."""
import argparse
from collections import defaultdict, deque
import datetime
import json
import re
from pathlib import Path
from types import SimpleNamespace
import uuid
import dcx_run
import import_items
from item_catalog import build, read_json, resource_gate

HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[1]


def retry_selection(selection, results):
    completed={i['source_id'] for i in results if i['status'] in ('created','preserved','asset_only')}
    return {**selection,'items':[i for i in selection['items'] if i['source_id'] not in completed]}


def readable_report(folder,report,prepared):
    lookup={i['item']['source_id']:i for i in prepared['items']}
    lines=['# Equipment import report','',f"Apply: {report['apply']} | Selected: {report['selected']} | OK: {report['ok']}",'',
           'All generated ItemDefinitions are presentation drafts; gameplay and original-game fidelity are not certified.','',
           '| Item | Model | Result | Detail |','|---|---|---|---|']
    for row in report['items']:
        source=lookup[row['source_id']]
        detail=row.get('error') or row.get('warnings') or ''
        if isinstance(detail,list):
            detail='; '.join((d.get('error','') if isinstance(d,dict) else str(d)).splitlines()[-1] for d in detail)
        detail=str(detail).replace('|','/').replace('\n',' ')
        lines.append('| '+ ' | '.join([source['item']['name']['value'],source['archive'],row['status'],detail])+' |')
    (folder/'report.md').write_text('\n'.join(lines)+'\n',encoding='utf-8')


def select_items(catalog, limit, excluded=()):
    groups=defaultdict(deque)
    rejected=[]
    seen=set(excluded)
    for item in sorted(catalog['items'],key=lambda i:(i['table'],i['row_id'])):
        reasons=resource_gate(item)
        if item['name']['status']!='matched':reasons.append('Chinese name unresolved')
        if reasons:
            rejected.append({'source_id':item['source_id'],'reasons':reasons});continue
        archive=item['models'][0]['archive']
        if archive in seen:
            rejected.append({'source_id':item['source_id'],'reasons':['Shared model; representative already selected or excluded']});continue
        seen.add(archive)
        category=json.dumps(item['source_category'],sort_keys=True)
        groups[category].append({'source_id':item['source_id'],'usage':'WorldDrop',
            'primary_model':archive.upper(),'icon_field':'iconId' if item['table']=='EquipParamWeapon' else 'iconIdM'})
    selected=[]
    while len(selected)<limit and any(groups.values()):
        for key in sorted(groups):
            if groups[key] and len(selected)<limit:selected.append(groups[key].popleft())
    for remaining in groups.values():
        rejected.extend({'source_id':i['source_id'],'reasons':['Deferred by batch limit']} for i in remaining)
    return selected,rejected


def run(args):
    if not 1<=args.chunk_size<=20:raise ValueError('chunk-size must be 1..20')
    if args.limit<1:raise ValueError('limit must be positive')
    if args.batch and not re.fullmatch('[A-Za-z][A-Za-z0-9_]*',args.batch):raise ValueError('Invalid batch')
    folder=Path(args.report_dir).resolve() if getattr(args,'report_dir',None) else ROOT/'Saved/EquipmentImport/Bulk'/((args.batch or 'Bulk')+'_'+uuid.uuid4().hex[:8])
    if not folder.is_relative_to((ROOT/'Saved/EquipmentImport').resolve()):raise ValueError('Report directory must be under Saved/EquipmentImport')
    folder.mkdir(parents=True)
    if args.selection:
        selection=read_json(args.selection)
    else:
        catalog,_=build(read_json(args.config))
        excluded=read_json(args.exclude_archives) if args.exclude_archives else []
        chosen,rejected=select_items(catalog,args.limit,excluded)
        selection={'batch':args.batch or 'Bulk_'+datetime.datetime.now().strftime('%Y%m%d_%H%M%S'),
                   'allow_unmapped_asset_only':True,'items':chosen}
        dcx_run.write(folder/'not_selected.json',rejected)
    if args.selection and args.batch:selection={**selection,'batch':args.batch}
    if args.destination:selection={**selection,'destination':args.destination}
    if not selection['items']:
        report={'ok':True,'apply':args.apply,'selected':0,'items':[],'failed_ids':[],'saved_packages':0,'message':'Nothing to retry/import'}
        dcx_run.write(folder/'report.json',report)
        print('BULK REPORT '+str(folder/'report.json'),flush=True)
        return report
    # Validate the whole frozen selection before any external process writes assets.
    dcx_run.write(folder/'selection.json',selection)
    try:
        prepared=import_items.prepare(read_json(args.config),selection)
    except (ValueError,KeyError,OSError) as exc:
        report={'ok':False,'apply':args.apply,'selected':len(selection['items']),
                'fatal':'Preflight: '+str(exc),'items':[],'saved_packages':0,'selection':str(folder/'selection.json')}
        dcx_run.write(folder/'report.json',report)
        dcx_run.write(folder/'retry.json',selection)
        print('BULK REPORT '+str(folder/'report.json'),flush=True)
        return report
    dcx_run.write(folder/'source_plan.json',prepared)
    report={'ok':False,'apply':args.apply,'selection':str(folder/'selection.json'),'chunks':[],
            'items':[],'failed_ids':[],'selected':len(selection['items'])}
    try:
        if not args.apply:
            report.update(ok=True,actions=['resource gate + frozen IDs','sequential conversion',
                'UE chunks of '+str(args.chunk_size),'models/materials/icons + mapped presentation drafts',
                'failure isolation; unknown materials remain blocked; existing assets preserved'])
            return report
        for start in range(0,len(selection['items']),args.chunk_size):
            chunk={**selection,'items':selection['items'][start:start+args.chunk_size]}
            path=folder/('selection_'+str(start//args.chunk_size).zfill(3)+'.json')
            dcx_run.write(path,chunk)
            try:
                result=import_items.run(SimpleNamespace(config=args.config,selection=str(path),tools=args.tools,
                                         rules=args.rules,apply=True))
                report['chunks'].append(result)
                states={r['source_id']:r for r in result.get('items',[])}
                for item in chunk['items']:
                    report['items'].append(states.get(item['source_id'],{'source_id':item['source_id'],
                       'status':'failed','error':result.get('error','No UE result')}))
            except Exception as exc:
                report['items'].extend({'source_id':i['source_id'],'status':'failed','error':str(exc)} for i in chunk['items'])
            report['failed_ids']=[i['source_id'] for i in report['items'] if i['status']=='failed']
            dcx_run.write(folder/'report.json',report)
        report['ok']=not report['failed_ids'] and all(c['ok'] for c in report['chunks'])
        report['saved_packages']=sum(len(c.get('editor',{}).get('saved',[]))+len(c.get('editor',{}).get('mesh_saved',[])) for c in report['chunks'])
        report['summary']={state:sum(i['status']==state for i in report['items']) for state in ('created','preserved','asset_only','failed')}
    finally:
        if args.apply:
            # Also retain not-yet-started IDs after an interrupted run.
            retry=retry_selection(selection,report['items'])
            report['failed_ids']=[i['source_id'] for i in retry['items']]
            dcx_run.write(folder/'retry.json',retry)
        dcx_run.write(folder/'report.json',report)
        readable_report(folder,report,prepared)
        print('BULK REPORT '+str(folder/'report.json'),flush=True)
    return report


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--config',default=str(HERE/'item_catalog.local.example.json'))
    p.add_argument('--tools',default=str(ROOT/'Saved/EquipmentImport/dcx_tools.json'))
    p.add_argument('--rules',default=str(HERE/'dcx_rules.json'))
    p.add_argument('--selection')
    p.add_argument('--report-dir',help='New, non-existing report directory for queue orchestration')
    p.add_argument('--batch')
    p.add_argument('--destination',choices=['Staging','Library'])
    p.add_argument('--exclude-archives')
    p.add_argument('--limit',type=int,default=30)
    p.add_argument('--chunk-size',type=int,default=10)
    p.add_argument('--apply',action='store_true')
    raise SystemExit(0 if run(p.parse_args())['ok'] else 1)
