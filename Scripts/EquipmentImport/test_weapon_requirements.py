import unittest
from weapon_requirements import values, decision, FIELDS, scaling_reference


class WeaponRequirementsTests(unittest.TestCase):
    def test_requirements_not_scaling_and_no_luck(self):
        item = {'table': 'EquipParamWeapon', 'source_category_code': 13, 'raw_params':
                {'properStrength': '11', 'properAgility': '15', 'properMagic': '0', 'properFaith': '0',
                 'properLuck': '99', 'correctStrength': '36'}}
        self.assertEqual(values(item), dict(strength=11, dexterity=15, intelligence=0, faith=0))
        item['source_category_code'] = 81
        self.assertIsNone(values(item))

    def test_idempotence_and_manual_edit_protection(self):
        zero = dict.fromkeys(FIELDS, 0.0)
        wanted = {**zero, 'strength': 11.0}
        self.assertEqual(decision(zero, wanted), 'update')
        self.assertEqual(decision(wanted, wanted), 'preserved')
        with self.assertRaises(ValueError): decision({**wanted, 'faith': 9}, wanted, wanted)
        self.assertEqual(decision(wanted, {**wanted, 'strength': 12}, wanted), 'update')

    def test_scaling_source_values_and_references_remain_unconverted(self):
        item = {'source_id': 'ER:Weapon:1', 'raw_params': {'correctStrength': '36', 'correctAgility': '55',
            'correctMagic': '0', 'correctFaith': '0', 'correctLuck': '12', 'correctType_Physics': '0',
            'reinforceTypeId': '100', 'attackElementCorrectId': '10000'}}
        data = scaling_reference(item)
        self.assertEqual(data['source_values']['strength'], '36')
        self.assertEqual(data['references']['reinforceTypeId'], '100')
        self.assertEqual(data['source_luck_not_used_by_umbra'], '12')
        self.assertFalse(data['umbra_curve_assigned'])


if __name__ == '__main__': unittest.main()
