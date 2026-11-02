import copy
import json
from pathlib import Path
import struct
import tempfile
import unittest
import zlib
from pipeline import plan, png_info
from scan import scan

def png(path, alpha=False, normal=False):
    def chunk(tag, content):
        return struct.pack('>I',len(content))+tag+content+struct.pack('>I',zlib.crc32(tag+content)&0xffffffff)
    pixel = bytes((128,128,255) if normal else (190,100,50)) + (b'\x80' if alpha else b'')
    Path(path).write_bytes(b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',2,2,8,6 if alpha else 2,0,0,0))+
                          chunk(b'IDAT',zlib.compress((b'\0'+pixel*2)*2))+chunk(b'IEND',b''))

class PlanningTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        (self.root/'mesh.fbx').write_bytes(b'test placeholder; no UE import in unit tests')
        png(self.root/'base.png')
        png(self.root/'normal.png',normal=True)
        self.data = {'schema_version':1,'output_root':'/Game/EquipmentImport/Staging/Test', 'items':[
            {'id':'Sword','model':'mesh.fbx','kind':'static','usage':'WorldDrop','provenance':'test generated',
             'materials':{'Blade':{'base_color':'base.png','preset':'metal'}}}]}
    def run_plan(self):
        file = self.root/'manifest.json'
        file.write_text(json.dumps(self.data),encoding='utf-8')
        return plan(file)
    def spec(self):
        return self.data['items'][0]['materials']['Blade']
    def test_valid_and_deterministic(self):
        first = self.run_plan()
        self.assertEqual(first,self.run_plan())
        self.assertFalse(first['items'][0]['errors'])
        self.assertEqual(first['items'][0]['materials']['Blade']['metallic'],1)
    def test_source_change_changes_recipe(self):
        old = self.run_plan()['items'][0]['recipe']
        (self.root/'mesh.fbx').write_bytes(b'changed')
        self.assertNotEqual(old,self.run_plan()['items'][0]['recipe'])
    def test_library_destination_is_isolated(self):
        self.data['output_root']='/Game/EquipmentImport/Library/Equipment_001'
        row=self.run_plan()['items'][0]
        self.assertFalse(row['errors'])
        self.assertTrue(row['folder'].startswith('/Game/EquipmentImport/Library/Equipment_001/'))
        self.data['output_root']='/Game/EquipmentImport/Library/../Existing'
        with self.assertRaises(ValueError):self.run_plan()
    def test_combining_is_limited_to_static_world_drop(self):
        old=self.run_plan()['items'][0]['recipe']
        item=self.data['items'][0];item['combine_static_meshes']=True
        row=self.run_plan()['items'][0]
        self.assertFalse(row['errors']);self.assertNotEqual(old,row['recipe'])
        item['kind']='skeletal'
        self.assertTrue(self.run_plan()['items'][0]['errors'])
        item['kind']='static';item['usage']='Wearable'
        self.assertTrue(self.run_plan()['items'][0]['errors'])
    def test_mask_threshold_requires_masked_surface(self):
        png(self.root/'base.png',alpha=True)
        self.spec().update(surface='Masked',alpha_policy='mask',alpha_clip=128/255,two_sided=False)
        self.assertFalse(self.run_plan()['items'][0]['errors'])
        self.spec()['alpha_clip']=float('nan')
        self.assertTrue(self.run_plan()['items'][0]['errors'])
    def test_path_escape(self):
        self.data['items'][0]['model']='../escape.fbx'
        self.assertIn('escapes',self.run_plan()['items'][0]['errors'][0])
    def test_protected_destination(self):
        self.data['output_root']='/Game/Asset/Equipments'
        with self.assertRaises(ValueError): self.run_plan()
    def test_duplicate_ids(self):
        self.data['items'].append(copy.deepcopy(self.data['items'][0]))
        self.data['items'][1]['id']='sword'
        with self.assertRaises(ValueError): self.run_plan()
    def test_unknown_normal_not_connected(self):
        self.spec().update(normal='normal.png',normal_encoding='unknown')
        row=self.run_plan()['items'][0]
        self.assertNotIn('normal',row['materials']['Blade'])
        self.assertTrue(row['warnings'])
    def test_standard_normal_connected(self):
        self.spec().update(normal='normal.png',normal_encoding='standard_dx')
        self.assertIn('normal',self.run_plan()['items'][0]['materials']['Blade'])
    def test_er_rg_direct_normal_connected_without_mask(self):
        self.spec().update(normal='normal.png',normal_encoding='er_rg_direct')
        result=self.run_plan()['items'][0]['materials']['Blade']
        self.assertIn('normal',result)
        self.assertNotIn('roughness_texture',result)
    def test_alpha_requires_declaration(self):
        png(self.root/'base.png',alpha=True)
        self.assertTrue(self.run_plan()['items'][0]['errors'])
        self.spec().update(surface='Masked',alpha_policy='mask')
        self.assertFalse(self.run_plan()['items'][0]['errors'])
    def test_bad_scalar(self):
        self.spec()['roughness']=float('nan')
        self.assertTrue(self.run_plan()['items'][0]['errors'])
    def test_missing_texture(self):
        self.spec()['base_color']='missing.png'
        self.assertTrue(self.run_plan()['items'][0]['errors'])
    def test_reject_dds(self):
        self.spec()['base_color']='base.dds'
        self.assertTrue(self.run_plan()['items'][0]['errors'])
    def test_bad_png(self):
        (self.root/'base.png').write_bytes(b'not png')
        self.assertTrue(self.run_plan()['items'][0]['errors'])
    def test_roughness_map_defaults_multiplier_to_one(self):
        self.spec()['roughness_texture']='base.png'
        spec=self.run_plan()['items'][0]['materials']['Blade']
        self.assertEqual(spec['roughness'],1)
        self.assertIn('roughness_sha',spec)
    def test_roughness_map_missing_rejected(self):
        self.spec()['roughness_texture']='absent.png'
        self.assertTrue(self.run_plan()['items'][0]['errors'])
    def test_scan_exact_slot(self):
        folder=self.root/'WorldDrop/static/Sword'
        folder.mkdir(parents=True)
        (folder/'Sword.fbx').write_bytes(b'fixture')
        png(folder/'Blade_a.png')
        png(folder/'Blade_n.png',normal=True)
        result=scan(self.root,'Test')
        self.assertEqual(len(result['items']),1)
        self.assertIn('Blade',result['items'][0]['materials'])
        self.assertEqual(result['items'][0]['materials']['Blade']['normal_encoding'],'unknown')

if __name__=='__main__': unittest.main()
