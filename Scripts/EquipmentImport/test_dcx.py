from pathlib import Path
import tempfile
import unittest
from dcx_run import inventory,write

class DcxTests(unittest.TestCase):
    def setUp(self):
        self.tmp=tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root=Path(self.tmp.name)
        for name in ['wp_a_0100','wp_a_0100_l','am_m_1130','unknown_1234']:
            (self.root/(name+'.partsbnd.dcx')).write_bytes(b'fixture')
    def test_inventory_excludes_low_and_unknown(self):
        selected,skipped=inventory(self.root)
        self.assertEqual({i for i,p in selected},{'wp_a_0100','am_m_1130'})
        self.assertEqual(len(skipped),2)
    def test_select_one(self):
        selected,skipped=inventory(self.root,['am_m_1130'])
        self.assertEqual([i for i,p in selected],['am_m_1130'])
    def test_unknown_selection_rejected(self):
        with self.assertRaises(ValueError): inventory(self.root,['wp_a_9999'])
    def test_low_explicit_selection_still_skipped(self):
        selected,skipped=inventory(self.root,['wp_a_0100_l'])
        self.assertFalse(selected)
        self.assertEqual(len(skipped),1)
    def test_inventory_does_not_touch_source(self):
        before={p.name:p.read_bytes() for p in self.root.iterdir()}
        inventory(self.root)
        self.assertEqual(before,{p.name:p.read_bytes() for p in self.root.iterdir()})
    def test_atomic_report(self):
        target=self.root/'reports/run.json'
        write(target,{'state':'first'})
        write(target,{'state':'finished'})
        self.assertIn('finished',target.read_text())
        self.assertFalse(target.with_suffix('.json.tmp').exists())

if __name__=='__main__': unittest.main()
