import json
from pathlib import Path
import re
import unittest


class CategoryMappingTests(unittest.TestCase):
    def test_weapon_mappings_use_real_enum_names_and_exclude_ammunition(self):
        here = Path(__file__).resolve().parent
        mapping = json.loads((here / 'item_category_mapping.json').read_text(encoding='utf-8'))
        header = (here.parents[1] / 'Source/Umbra/Items/UmbraItemClassification.h').read_text(encoding='utf-8')
        names = set(re.findall(r'\b(\w+)\s*=\s*\d+', header))
        self.assertEqual(set(mapping['non_equippable_weapon_categories']), {81, 83, 85, 86})
        for code in mapping['non_equippable_weapon_categories']:
            self.assertNotIn(str(code), mapping['weapons'])
        for value in mapping['weapons'].values():
            self.assertIn(value['WeaponType'], names)
        self.assertEqual(mapping['weapons']['13']['WeaponType'], 'Katana')
        self.assertEqual(mapping['weapons']['69']['WeaponType'], 'Greatshield')
        self.assertEqual(mapping['weapons']['95']['WeaponType'], 'BeastClaw')


if __name__ == '__main__': unittest.main()
