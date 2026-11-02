import unittest
from normal_rules import validate_direct_normal


class NormalRulesTests(unittest.TestCase):
    def setUp(self):
        self.record={'references':[{'type_name':'NormalMap','path':'','scale':[1,1]}]}
        self.normal={'size':[256,1024],'png_max_channel_error':0}
        self.albedo={'size':[256,1024]}

    def test_template_semantic_and_exact_explicit_reference(self):
        self.assertEqual(validate_direct_normal(self.record,self.normal,self.albedo,'WP_A_0100')['encoding'],'er_rg_direct')
        self.record['references'][0]['scale']=(1.0,1.0)
        self.assertEqual(validate_direct_normal(self.record,self.normal,self.albedo,'WP_A_0100')['encoding'],'er_rg_direct')
        self.record['references'][0]['path']='N:\\textures\\WP_A_0100_n.tga'
        self.assertEqual(validate_direct_normal(self.record,self.normal,self.albedo,'WP_A_0100')['reference_basis'],'explicit')

    def test_conflicting_reference(self):
        self.record['references'][0]['path']='another_n.tga'
        with self.assertRaises(ValueError): validate_direct_normal(self.record,self.normal,self.albedo,'WP_A_0100')

    def test_detail_slot_does_not_override_primary(self):
        self.record['references'].append({'type_name':'C_DetailBlend__snp_Texture2D_6_NormalMap',
                                         'path':'AAT100_Fabric_03_n.tif','scale':[10,10]})
        result=validate_direct_normal(self.record,self.normal,self.albedo,'WP_A_0100')
        self.assertEqual(len(result['unimplemented_detail_layers']),1)
        self.record['references']=self.record['references'][1:]
        with self.assertRaises(ValueError): validate_direct_normal(self.record,self.normal,self.albedo,'WP_A_0100')

    def test_unknown_scaled_slot_still_fails(self):
        self.record['references'].append({'type_name':'Other_NormalMap','path':'detail_n.tif','scale':[10,10]})
        with self.assertRaises(ValueError): validate_direct_normal(self.record,self.normal,self.albedo,'WP_A_0100')

    def test_proportional_texture_resolution_is_supported(self):
        self.albedo['size']=[512,2048]
        self.assertEqual(validate_direct_normal(self.record,self.normal,self.albedo,'WP_A_0100')['encoding'],'er_rg_direct')

    def test_detail_scale_and_missing_semantic(self):
        self.record['references'][0]['scale']=[2,2]
        with self.assertRaises(ValueError): validate_direct_normal(self.record,self.normal,self.albedo,'WP_A_0100')
        self.record['references']=[]
        with self.assertRaises(ValueError): validate_direct_normal(self.record,self.normal,self.albedo,'WP_A_0100')

    def test_layout_or_conversion_error(self):
        self.normal['size']=[512,512]
        with self.assertRaises(ValueError): validate_direct_normal(self.record,self.normal,self.albedo,'WP_A_0100')
        self.normal['size']=[256,1024];self.normal['png_max_channel_error']=.02
        with self.assertRaises(ValueError): validate_direct_normal(self.record,self.normal,self.albedo,'WP_A_0100')


if __name__=='__main__': unittest.main()
