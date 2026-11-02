import tempfile
from pathlib import Path
from types import SimpleNamespace
import unittest
from unittest.mock import patch
import import_all as queue


class QueueTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)

    def unit(self, ids=('a', 'b', 'c')):
        return {'id': 'Batch_001_000', 'selection': {'batch': 'Batch_001', 'destination': 'Library',
                'items': [{'source_id': i} for i in ids]}, 'attempts': []}

    def attempt(self, unit, rows, name='attempt', **extra):
        folder = self.root / name
        queue.save(folder / 'report.json', {'apply': True, 'items': rows, 'chunks': [{}], **extra})
        unit['attempts'].append(str(folder))
        return folder

    def test_resume_skips_success_and_known_failure_but_keeps_unstarted(self):
        unit = self.unit()
        self.attempt(unit, [{'source_id': 'a', 'status': 'created'}, {'source_id': 'b', 'status': 'failed'}])
        self.assertEqual(queue.pending(unit), [{'source_id': 'c'}])

    def test_exclusion_only_counts_actual_library_results(self):
        for name, apply, destination in [('dry', False, 'Library'), ('test', True, 'Staging'), ('real', True, 'Library')]:
            folder = self.root / name
            queue.save(folder / 'report.json', {'apply': apply, 'items': [{'source_id': name, 'status': 'failed'}], 'chunks': [{}]})
            queue.save(folder / 'source_plan.json', {'destination': destination, 'items': [
                {'archive': name, 'item': {'source_id': name}}, {'archive': 'not_started', 'item': {'source_id': 'later'}}]})
        self.assertEqual(queue.attempted_archives(self.root), {'real'})

    def test_summary_distinguishes_imported_from_verified_and_pending(self):
        unit = self.unit()
        folder = self.attempt(unit, [{'source_id': 'a', 'status': 'asset_only'}, {'source_id': 'b', 'status': 'failed'}])
        state = {'name': 'Q', 'units': [unit]}
        summary = queue.summarize(state)
        self.assertEqual(summary['pending'], 1)
        self.assertEqual(summary['unverified_reports'], [str(folder)])
        queue.save(folder / 'verification.json', {'ok': True})
        self.assertEqual(queue.summarize(state)['unverified_reports'], [])
        self.assertFalse(queue.summarize(state)['all_succeeded'])

    def test_preflight_error_is_not_silently_consumed(self):
        unit = self.unit()
        self.attempt(unit, [], fatal='Preflight: source missing')
        self.assertEqual(len(queue.pending(unit)), 3)
        self.assertEqual(queue.infrastructure_error(queue.read_json(Path(unit['attempts'][0]) / 'report.json')), 'Preflight: source missing')

    def test_engine_failure_remains_pending_but_material_failure_is_terminal(self):
        unit = self.unit(('a',))
        self.attempt(unit, [{'source_id': 'a', 'status': 'failed'}],
                     chunks=[{'error': 'UE failed to launch'}])
        self.assertEqual(len(queue.pending(unit)), 1)
        self.attempt(unit, [{'source_id': 'a', 'status': 'failed'}], name='material',
                     chunks=[{'error': 'No successful conversions; see per-item errors'}])
        self.assertEqual(queue.pending(unit), [])

    def test_resume_after_crash_and_completed_rerun_launches_nothing(self):
        saved = self.root / 'Saved'
        folder = saved / 'Queues' / 'Q'
        unit = self.unit(('a', 'b'))
        state = {'name': 'Q', 'fingerprint': {}, 'units': [unit],
                 'sources': {i: {'archive': i, 'name': i} for i in ('a', 'b')}}
        queue.save(folder / 'state.json', state)
        args = SimpleNamespace(queue='Q', config='config', tools='tools', rules='rules',
                               batch_size=30, chunk_size=6, apply=True)
        imported = []
        def child(command, log):
            if '--selection' in command:
                selection = queue.read_json(command[command.index('--selection') + 1])
                directory = Path(command[command.index('--report-dir') + 1])
                ids = [i['source_id'] for i in selection['items']]
                imported.append(ids)
                # Crash after persisting the first item, before parent can checkpoint.
                rows = [{'source_id': ids[0], 'status': 'created'}]
                queue.save(directory / 'report.json', {'apply': True, 'items': rows, 'chunks': [{}]})
                if ids == ['a', 'b']: raise KeyboardInterrupt()
                return 0
            directory = Path(command[command.index('--report') + 1]).parent
            queue.save(directory / 'verification.json', {'ok': True})
            return 0
        with patch.object(queue, 'SAVED', saved), patch.object(queue, 'fingerprint', return_value={}), \
             patch.object(queue, 'ensure_editor_closed'), patch.object(queue, 'child', side_effect=child):
            with self.assertRaises(KeyboardInterrupt): queue.run(args)
            self.assertEqual(queue.run(args), 0)
            self.assertEqual(imported, [['a', 'b'], ['b']])
            self.assertEqual(queue.run(args), 0)
            self.assertEqual(len(imported), 2)


if __name__ == '__main__': unittest.main()
