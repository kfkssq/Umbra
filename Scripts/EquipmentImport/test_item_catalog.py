import tempfile
import unittest
from pathlib import Path
from item_catalog import icon_index, resolve_icon, model_candidates, text_index, resolve_text, load_rows, resource_gate


class CatalogTests(unittest.TestCase):
    def test_gate_requires_both_files_and_rejects_stale_or_ambiguous_matches(self):
        with tempfile.TemporaryDirectory() as folder:
            model, icon = Path(folder) / 'model.dcx', Path(folder) / 'icon.png'
            model.touch()
            icon.touch()
            item = dict(models=[dict(path=str(model))], model_status='matched_candidates',
                        icons={'iconId': dict(status='matched_by_id', paths=[str(icon)])})
            self.assertEqual(resource_gate(item), [])
            icon.unlink()
            self.assertIn('iconId_file_missing', resource_gate(item))
            icon.touch()
            item['models'].append(dict(path=str(model)))
            self.assertTrue(resource_gate(item))
            item['models'].pop()
            item['icons']['iconId']['status'] = 'ambiguous'
            self.assertTrue(resource_gate(item))
            item['icons']['iconId']['status'] = 'matched_by_id'
            model.unlink()
            self.assertIn('model_file_missing', resource_gate(item))

    def test_conflicting_text_is_not_guessed(self):
        data = {"FmgWrappers": [{"Name": name, "Fmg": {"Entries": [{"ID": 1, "Text": value}]}}
                                for name, value in [("WeaponName.fmg", "First"), ("WeaponName_dlc01.fmg", "Second")]]}
        result = resolve_text(text_index(data), "Weapon", "Name", 1)
        self.assertEqual(result["status"], "conflict")
        self.assertIsNone(result["value"])
        self.assertEqual(resolve_text(text_index(data), "Protector", "Name", 1)["status"], "missing")

    def test_placeholder_does_not_override_valid_text(self):
        data = {"FmgWrappers": [{"Name": name, "Fmg": {"Entries": [{"ID": 1, "Text": value}]}}
                                for name, value in [("WeaponName.fmg", "Sword"), ("WeaponName_dlc01.fmg", "[ERROR]")]]}
        self.assertEqual(resolve_text(text_index(data), "Weapon", "Name", 1)["value"], "Sword")

    def test_duplicate_icons_are_ambiguous_and_exact(self):
        with tempfile.TemporaryDirectory() as folder:
            for name in ["menu_knowledge_00123out.png", "menu_knowledge_123out.obj.png", "other123.png"]:
                (Path(folder) / name).touch()
            index = icon_index(folder)
            self.assertEqual(resolve_icon(index, 123)["status"], "ambiguous")
            self.assertEqual(resolve_icon(index, 12)["status"], "missing")

    def test_armor_parts_separate_and_no_low_res_fallback(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            for name in ["am_m_1730", "am_f_1730", "hd_m_1730", "am_m_1730_l"]:
                (root / (name + ".partsbnd.dcx")).touch()
            row = dict(equipModelId="1730", equipModelCategory="1", protectorCategory="2")
            found, status = model_candidates(row, "Protector", root)
            self.assertEqual({m["archive"] for m in found}, {"am_m_1730", "am_f_1730"})
            row["equipModelCategory"] = "5"
            self.assertEqual(model_candidates(row, "Protector", root)[1], "unsupported_model_category")

    def test_bad_csv_fails_before_export(self):
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder) / "bad.csv"
            path.write_text("ID,Name\n1,test\n")
            with self.assertRaises(ValueError):
                load_rows(path, "Weapon")


if __name__ == "__main__":
    unittest.main()
