"""Approved material grade mapping; source row IDs are explicit, never inferred from names."""
SMITHING_IDS = (*range(10100, 10108), 10140)
SOMBER_IDS = (*range(10160, 10168), 10200, 10168)
RARITIES = ('COMMON', 'MAGIC', 'RARE', 'EPIC', 'LEGENDARY', 'UNIQUE')
DESCRIPTION = '锻造材料。品质由石头等级固定；同种可堆叠，强化功能暂未实现。'


def rarity_index(row_id):
    if row_id in (10140, 10168):
        return 5
    if 10100 <= row_id <= 10107:
        return (row_id - 10100) // 2
    if 10160 <= row_id <= 10167:
        return (row_id - 10160) // 2
    if row_id == 10200:
        return 4
    raise ValueError('Unknown smithing material ID: ' + str(row_id))


def presentation_fields(row_id, profile):
    import unreal as u
    rarity = getattr(u.UmbraItemRarity, RARITIES[rarity_index(row_id)])
    row = next(r for r in profile.rarities if r.rarity == rarity)
    fields = dict(rarity=rarity, rarity_color=row.color, tint_rarity_backgrounds=row.tint_backgrounds,
                  flavor_text=DESCRIPTION, stackable_material=True)
    for key in ('slot_background_texture', 'rarity_frame_texture',
                'tooltip_title_background_texture', 'tooltip_icon_background_texture'):
        fields[key] = row.get_editor_property(key)
    return fields
