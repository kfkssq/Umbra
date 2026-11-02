import unittest
from matbin_rules import resolve_material


class MaterialDefinitionTests(unittest.TestCase):
    def setUp(self):
        self.base='C_DetailBlend__snp_Texture2D_7_AlbedoMap'
        self.normal='C_DetailBlend__snp_Texture2D_0_NormalMap'
        self.record={'template':'N:\\P[TEST]_Fabric.matxml','references':[]}
        self.definition={'shader':'C[DetailBlend].spx','sha256':'test',
          'samplers':[{'type':self.base,'path':'N:\\exact_a.tif'},{'type':self.normal,'path':'N:\\exact_n.tif'},
                      {'type':'C_DetailBlend__snp_Texture2D_1_AlbedoMap','path':'detail_a.tif'}],
          'parameters':[{'name':'g_BlendMode','value':0},{'name':'g_AlphaRef','value':[128,0]}]}
        self.index={'entries':{'p[test]_fabric':self.definition}}

    def test_primary_and_alpha_are_source_driven(self):
        r=resolve_material(self.record,self.index)
        self.assertEqual(r['base_key'],'exact_a');self.assertEqual(r['normal_key'],'exact_n')
        self.assertAlmostEqual(r['alpha_clip'],128/255)
        self.assertEqual(r['alpha_policy'],'mask');self.assertEqual(len(r['omitted_samplers']),1)

    def test_flver_override_is_exact_sampler_only(self):
        self.record['references']=[{'type_name':self.base,'path':'override_a.tif','scale':[1,1]},
          {'type_name':'C_DetailBlend__snp_Texture2D_1_AlbedoMap','path':'another_detail.tif','scale':[5,5]}]
        self.assertEqual(resolve_material(self.record,self.index)['base_key'],'override_a')

    def test_unknown_shader_and_blend_fail(self):
        self.definition['shader']='C[Fur].spx'
        with self.assertRaises(ValueError):resolve_material(self.record,self.index)
        self.definition['shader']='C[DetailBlend].spx';self.definition['parameters'][0]['value']=2
        with self.assertRaises(ValueError):resolve_material(self.record,self.index)

    def test_no_guessing_empty_texture_or_conflicting_definition(self):
        self.definition['samplers'][0]['path']=''
        with self.assertRaises(ValueError):resolve_material(self.record,self.index)
        self.index['conflicts']=['p[test]_fabric']
        with self.assertRaises(ValueError):resolve_material(self.record,self.index)


if __name__=='__main__':unittest.main()
