"""Persistent local import queue. No model calls; source failures are isolated, never retried automatically."""
import argparse
from collections import Counter
from contextlib import contextmanager
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import uuid

from bulk_import import select_items
from item_catalog import build, read_json, digest

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
SAVED = ROOT / 'Saved/EquipmentImport'
TERMINAL = {'created', 'preserved', 'asset_only', 'failed'}


def save(path, value):
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_suffix(path.suffix + '.tmp')
    temporary.write_text(json.dumps(value, ensure_ascii=False, indent=2), encoding='utf-8')
    os.replace(temporary, path)


@contextmanager
def queue_lock(path):
    """OS lock survives neither crashes nor reboots; lock file itself is harmless."""
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open('a+b') as stream:
        stream.seek(0)
        if not stream.read(1):
            stream.write(b'0'); stream.flush()
        stream.seek(0)
        if os.name == 'nt':
            import msvcrt
            try: msvcrt.locking(stream.fileno(), msvcrt.LK_NBLCK, 1)
            except OSError as exc: raise RuntimeError('Another import queue is running') from exc
        else:
            import fcntl
            fcntl.flock(stream, fcntl.LOCK_EX | fcntl.LOCK_NB)
        try: yield
        finally:
            stream.seek(0)
            if os.name == 'nt': msvcrt.locking(stream.fileno(), msvcrt.LK_UNLCK, 1)
            else: fcntl.flock(stream, fcntl.LOCK_UN)


def attempted_archives(root):
    """Only actual Library results count; dry runs and Staging tests don't."""
    excluded = set()
    for path in Path(root).rglob('report.json'):
        plan_path = path.parent / 'source_plan.json'
        if not plan_path.is_file(): continue
        report, plan = read_json(path), read_json(plan_path)
        if not report.get('apply') or plan.get('destination') != 'Library': continue
        by_id = {r['item']['source_id']: r['archive'] for r in plan['items']}
        for row in report.get('items', []):
            if infrastructure_error(report) and row.get('status') == 'failed': continue
            if row.get('status') in TERMINAL and row['source_id'] in by_id:
                excluded.add(by_id[row['source_id']])
    return excluded


def fingerprint(config, tools, rules):
    paths = {Path(config).resolve(), Path(tools).resolve(), Path(rules).resolve()}
    configuration = read_json(config)
    for key in ('weapon_csv', 'armor_csv', 'text_json', 'weapon_enum', 'mapping'):
        paths.add(Path(configuration[key]).resolve())
    index = read_json(tools).get('material_index')
    if index: paths.add(Path(index).resolve())
    paths.update(p for p in HERE.glob('*.py') if not p.name.startswith('test_'))
    return {str(p): digest(p) for p in sorted(paths)}


def infrastructure_error(report):
    if report.get('fatal'): return report['fatal']
    if not report.get('chunks') and report.get('items'): return 'No chunk result'
    for chunk in report.get('chunks', []):
        if chunk.get('error') and not chunk.get('editor') and chunk['error'] != 'No successful conversions; see per-item errors':
            return chunk['error']
    return None


def reconcile(unit):
    """Read subprocess checkpoints after interruption, before choosing more work."""
    results = {}
    for directory in unit['attempts']:
        path = Path(directory) / 'report.json'
        if not path.is_file(): continue
        report = read_json(path)
        infrastructure = infrastructure_error(report)
        for row in report.get('items', []):
            # Process failures aren't material rejections: retain these IDs for resume.
            if infrastructure and row.get('status') == 'failed': continue
            if row.get('status') in TERMINAL: results[row['source_id']] = row
    return results


def pending(unit):
    done = reconcile(unit)
    return [r for r in unit['selection']['items'] if r['source_id'] not in done]


def summarize(state):
    rows = []
    saved = 0
    unverified = []
    for unit in state['units']:
        for row in reconcile(unit).values():
            rows.append({**row, 'batch': unit['selection']['batch']})
        for attempt in unit['attempts']:
            path = Path(attempt) / 'report.json'
            if not path.exists(): continue
            report = read_json(path)
            saved += report.get('saved_packages', sum(len(c.get('editor', {}).get('saved', [])) +
                         len(c.get('editor', {}).get('mesh_saved', [])) for c in report.get('chunks', [])))
            if any(r.get('status') in TERMINAL - {'failed'} for r in report.get('items', [])):
                verification = Path(attempt) / 'verification.json'
                if not verification.is_file() or not read_json(verification).get('ok'):
                    unverified.append(attempt)
    counts = Counter(r['status'] for r in rows)
    total = sum(len(u['selection']['items']) for u in state['units'])
    return {'queue': state['name'], 'selected': total, 'processed': len(rows),
            'pending': total - len(rows), 'counts': dict(counts), 'saved_packages': saved,
            'unverified_reports': unverified, 'items': rows,
            'finished': len(rows) == total and not unverified,
            'all_succeeded': len(rows) == total and not counts['failed'] and not unverified}


def publish(folder, state):
    summary = summarize(state)
    save(folder / 'summary.json', summary)
    lines = ['# Equipment import queue', '',
             f"Queue: {state['name']} | Selected: {summary['selected']} | Pending: {summary['pending']}",
             f"Results: {summary['counts']} | Saved packages: {summary['saved_packages']}",
             f"Unverified reports: {len(summary['unverified_reports'])}", '',
             'Presentation assets only; no gameplay or original-shader fidelity certification.', '',
             '| Item | Model | Batch | Result |', '|---|---|---|---|']
    for row in summary['items']:
        info = state['sources'][row['source_id']]
        lines.append('| ' + ' | '.join(str(v).replace('|', '/') for v in
                     (info['name'], info['archive'], row['batch'], row['status'])) + ' |')
    (folder / 'summary.md').write_text('\n'.join(lines) + '\n', encoding='utf-8')
    save(folder / 'failures.json', [r for r in summary['items'] if r['status'] == 'failed'])
    # Keep the original batch identity when a human later fixes a failed material.
    for unit in state['units']:
        failed = {r['source_id'] for r in reconcile(unit).values() if r['status'] == 'failed'}
        if failed:
            selection = unit['selection']
            save(folder / 'Retries' / (unit['id'] + '.json'),
                 {**selection, 'items': [r for r in selection['items'] if r['source_id'] in failed]})
    return summary


def child(command, log):
    with Path(log).open('ab') as stream:
        # Inherit the console so Ctrl+C also interrupts the child; don't detach jobs.
        return subprocess.call(command, cwd=ROOT, stdout=stream, stderr=subprocess.STDOUT)


def ensure_editor_closed():
    listing = subprocess.check_output(['tasklist', '/FO', 'CSV', '/NH'],
                                     creationflags=subprocess.CREATE_NO_WINDOW).decode(errors='replace')
    if re.search(r'"UnrealEditor(?:-Cmd)?\.exe"', listing, re.I):
        raise RuntimeError('Close Unreal Editor (including orphaned commandlets) before continuing')


def create_state(args, folder, hashes):
    catalog, _ = build(read_json(args.config))
    excluded = attempted_archives(SAVED / 'Bulk') | attempted_archives(SAVED / 'Queues')
    selected, rejected = select_items(catalog, len(catalog['items']), excluded)
    lookup = {r['source_id']: r for r in catalog['items']}
    state = {'schema_version': 1, 'name': args.queue, 'fingerprint': hashes, 'units': [], 'sources': {}}
    for row in selected:
        item = lookup[row['source_id']]
        state['sources'][row['source_id']] = {'archive': item['models'][0]['archive'], 'name': item['name']['value']}
    for start in range(0, len(selected), args.batch_size):
        batch = args.queue + '_' + str(start // args.batch_size + 1).zfill(3)
        if (ROOT / 'Content/EquipmentImport/Library' / batch).exists():
            raise RuntimeError('Destination already exists without this queue: ' + batch)
        batch_items = selected[start:start + args.batch_size]
        for offset in range(0, len(batch_items), args.chunk_size):
            selection = {'batch': batch, 'destination': 'Library', 'allow_unmapped_asset_only': True,
                         'items': batch_items[offset:offset + args.chunk_size]}
            state['units'].append({'id': batch + '_' + str(offset).zfill(3), 'selection': selection, 'attempts': []})
    save(folder / 'excluded_archives.json', sorted(excluded))
    save(folder / 'not_selected.json', rejected)
    save(folder / 'state.json', state)
    return state


def run(args):
    if not re.fullmatch('[A-Za-z][A-Za-z0-9_]*', args.queue): raise ValueError('Invalid queue name')
    if not 1 <= args.chunk_size <= 20 or not 1 <= args.batch_size <= 100: raise ValueError('Invalid batch/chunk size')
    folder = SAVED / 'Queues' / args.queue
    folder.mkdir(parents=True, exist_ok=True)
    with queue_lock(SAVED / 'Queues' / 'import.lock'):
        hashes = fingerprint(args.config, args.tools, args.rules)
        state = read_json(folder / 'state.json') if (folder / 'state.json').exists() else create_state(args, folder, hashes)
        if state['fingerprint'] != hashes:
            raise RuntimeError('Inputs or pipeline changed. Restore the frozen inputs to resume; inspect failures before starting a new queue.')
        summary = publish(folder, state)
        print(f"QUEUE {args.queue}: {summary['selected']} items; {summary['pending']} pending. REPORT {folder / 'summary.md'}", flush=True)
        if not args.apply: return 0
        ensure_editor_closed()
        for index, unit in enumerate(state['units']):
            # Validate recovered results before importing more assets.
            def verify_attempts():
                for directory in unit['attempts']:
                    path = Path(directory) / 'report.json'
                    if not path.exists(): continue
                    report = read_json(path)
                    if not any(r.get('status') in TERMINAL - {'failed'} for r in report.get('items', [])): continue
                    verification = path.parent / 'verification.json'
                    if verification.exists() and read_json(verification).get('ok'): continue
                    code = child([sys.executable, '-B', str(HERE / 'validate_bulk.py'), '--report', str(path)],
                                 path.parent / 'queue_verify.log')
                    if code or not verification.exists() or not read_json(verification).get('ok'):
                        raise RuntimeError('Readback failed; queue stopped, see ' + str(path.parent / 'queue_verify.log'))
            verify_attempts()
            todo = pending(unit)
            if not todo: continue
            ensure_editor_closed()
            attempt = folder / 'Attempts' / (unit['id'] + '_' + uuid.uuid4().hex[:8])
            selection_path = folder / 'Selections' / (attempt.name + '.json')
            save(selection_path, {**unit['selection'], 'items': todo})
            unit['attempts'].append(str(attempt))
            save(folder / 'state.json', state)  # Persist before launching the child.
            print(f"[{index + 1}/{len(state['units'])}] {unit['id']}: {len(todo)} items", flush=True)
            code = child([sys.executable, '-B', str(HERE / 'bulk_import.py'), '--config', args.config,
                          '--tools', args.tools, '--rules', args.rules, '--selection', str(selection_path),
                          '--report-dir', str(attempt), '--chunk-size', str(len(todo)), '--apply'],
                         folder / (attempt.name + '.log'))
            if not (attempt / 'report.json').exists():
                raise RuntimeError('Importer exited without a report: ' + str(code))
            report = read_json(attempt / 'report.json')
            publish(folder, state)
            if infrastructure_error(report):
                raise RuntimeError('Infrastructure failure; inspect ' + str(attempt / 'report.json'))
            if pending(unit): raise RuntimeError('Incomplete subprocess; rerun the same command to resume')
            verify_attempts()
            summary = publish(folder, state)
            print(f"Processed {summary['processed']}/{summary['selected']}: {summary['counts']}", flush=True)
        summary = publish(folder, state)
        print('FINISHED ' + str(folder / 'summary.md'), flush=True)
        return 0 if summary['all_succeeded'] else 1


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--queue', default='Equipment_All_001')
    parser.add_argument('--config', default=str(HERE / 'item_catalog.local.example.json'))
    parser.add_argument('--tools', default=str(SAVED / 'dcx_tools.json'))
    parser.add_argument('--rules', default=str(HERE / 'dcx_rules.json'))
    parser.add_argument('--batch-size', type=int, default=30)
    parser.add_argument('--chunk-size', type=int, default=6)
    parser.add_argument('--apply', action='store_true')
    args = parser.parse_args()
    try: sys.exit(run(args))
    except KeyboardInterrupt:
        print('Interrupted. Close any remaining UE process, then repeat the same command to resume.', flush=True)
        sys.exit(130)
    except Exception as exc:
        print('STOPPED: ' + str(exc), file=sys.stderr, flush=True)
        sys.exit(2)
