"""Source equip requirements -> Umbra's four primary attributes. Never scaling coefficients."""
import json
import math

FIELDS = {'strength': 'properStrength', 'dexterity': 'properAgility',
          'intelligence': 'properMagic', 'faith': 'properFaith'}
TAG = 'UmbraItemImport.RequirementsBaseline'
SCALING_TAG = 'UmbraItemImport.SourceScaling'


def scaling_reference(item):
    raw = item['raw_params']
    fields = {'strength': 'correctStrength', 'dexterity': 'correctAgility',
              'intelligence': 'correctMagic', 'faith': 'correctFaith'}
    # Keep the source numbers unchanged: they are not Umbra damage-per-point coefficients.
    return {'schema_version': 1, 'source_id': item['source_id'], 'purpose': 'future_curve_authoring_only',
            'source_values': {attribute: raw.get(field) for attribute, field in fields.items()},
            'source_fields': fields, 'source_luck_not_used_by_umbra': raw.get('correctLuck'),
            'references': {key: value for key, value in raw.items()
                           if key.startswith('correctType') or key in ('reinforceTypeId', 'attackElementCorrectId')},
            'umbra_curve_assigned': False}


def values(item):
    if item['table'] != 'EquipParamWeapon' or item.get('definition_excluded'):
        return None
    if int(item.get('source_category_code', -1)) in (81, 83, 85, 86): return None
    raw = item['raw_params']
    result = {field: float(raw[source]) for field, source in FIELDS.items()}
    if any(not math.isfinite(v) or v < 0 or not v.is_integer() for v in result.values()):
        raise ValueError('Invalid source attribute requirements')
    return result


def decision(current, expected, baseline=None):
    if current == expected: return 'preserved'
    if current == (baseline if baseline is not None else dict.fromkeys(FIELDS, 0.0)):
        return 'update'
    raise ValueError('Hand-edited requirements differ from import baseline; preserved')


def apply(draft, item, lib):
    expected = values(item)
    if expected is None: return {'status': 'excluded', 'changed': False}
    struct = draft.get_editor_property('requirements')
    current = {field: float(struct.get_editor_property(field)) for field in FIELDS}
    text = lib.get_metadata_tag(draft, TAG)
    baseline = json.loads(text) if text else None
    action = decision(current, expected, baseline)
    if action == 'update':
        for field, value in expected.items(): struct.set_editor_property(field, value)
        draft.set_editor_property('requirements', struct)
    # Adopt matching current values without altering them; next run can detect manual edits.
    encoded = json.dumps(expected, sort_keys=True)
    metadata_changed = text != encoded
    if metadata_changed: lib.set_metadata_tag(draft, TAG, encoded)
    scaling = json.dumps(scaling_reference(item), ensure_ascii=False, sort_keys=True)
    scaling_changed = lib.get_metadata_tag(draft, SCALING_TAG) != scaling
    if scaling_changed: lib.set_metadata_tag(draft, SCALING_TAG, scaling)
    return {'status': action, 'changed': action == 'update' or metadata_changed or scaling_changed,
            'before': current, 'requirements': expected,
            'ignored_source_luck': item['raw_params'].get('properLuck')}
