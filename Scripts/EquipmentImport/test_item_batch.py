import unittest
from import_items import select_converted
from bulk_import import retry_selection


class BatchIsolationTests(unittest.TestCase):
    def test_retry_retains_failed_and_not_started(self):
        selection={'batch':'Test','items':[{'source_id':i} for i in ('a','b','c','d')]}
        results=[{'source_id':'a','status':'preserved'},{'source_id':'b','status':'failed'},{'source_id':'c','status':'asset_only'}]
        self.assertEqual([i['source_id'] for i in retry_selection(selection,results)['items']],['b','d'])

    def test_failed_archive_partial_models_do_not_leak(self):
        chosen=[{'archive':a,'primary_model':a.upper(),'item':{'source_id':a}} for a in ('a','b','c')]
        conversion={'items':[{'id':'a','status':'converted'}, {'id':'b','status':'failed'}, {'id':'c','status':'converted'}]}
        manifest={'items':[{'id':i} for i in ('a_A','a_EXTRA','b_B','c_OTHER')]}
        passed,failed=select_converted(chosen,conversion,manifest)
        self.assertEqual([r['archive'] for r in passed],['a'])
        self.assertEqual([r['source_id'] for r in failed],['b','c'])
        self.assertEqual([r['id'] for r in manifest['items']],['a_A','a_EXTRA'])


if __name__=='__main__':unittest.main()
