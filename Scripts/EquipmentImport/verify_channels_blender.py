"""Run inside Blender (bundled NumPy), no external dependency installation."""
import sys
from pathlib import Path
import unittest
import numpy as np
sys.path.insert(0,str(Path(__file__).resolve().parent))
from material_channels import decode_rg_gloss

class Channels(unittest.TestCase):
    def test_flat_normal(self):
        n,r,_=decode_rg_gloss([[.5,.5,.25,0]])
        np.testing.assert_allclose(n,[[.5,.5,1,1]])
        np.testing.assert_allclose(r,[[.75,.75,.75,1]])
    def test_green_flip(self):
        a,_,_=decode_rg_gloss([[.5,.75,0,1]])
        b,_,_=decode_rg_gloss([[.5,.75,0,1]],True)
        self.assertAlmostEqual(float(a[0,1]+b[0,1]),1)
        self.assertAlmostEqual(float(a[0,2]),float(b[0,2]))
    def test_outside_disk_normalized(self):
        n,_,m=decode_rg_gloss([[1,1,0,1]])
        self.assertAlmostEqual(float(np.linalg.norm(n[0,:3]*2-1)),1,places=6)
        self.assertEqual(m['outside_unit_disk_fraction'],1)
    def test_alpha_does_not_drive_normals(self):
        a,_,_=decode_rg_gloss([[.25,.5,.3,0]])
        b,_,_=decode_rg_gloss([[.25,.5,.3,1]])
        np.testing.assert_array_equal(a,b)
    def test_blue_only_affects_roughness(self):
        a,r,_=decode_rg_gloss([[.5,.5,0,1],[.5,.5,1,1]])
        np.testing.assert_array_equal(a[0],a[1])
        np.testing.assert_array_equal(r[:,0],[1,0])
    def test_invalid_dimensions(self):
        for p in [[],[.5,.5,1],[[.5,.5,1]]]:
            with self.assertRaises(ValueError): decode_rg_gloss(p)
    def test_invalid_values(self):
        for value in [float('nan'),float('inf'),-1,2]:
            with self.assertRaises(ValueError): decode_rg_gloss([[value,.5,1,1]])

result=unittest.TextTestRunner(verbosity=2).run(unittest.defaultTestLoader.loadTestsFromTestCase(Channels))
if not result.wasSuccessful(): raise RuntimeError('Channel conversion tests failed')
