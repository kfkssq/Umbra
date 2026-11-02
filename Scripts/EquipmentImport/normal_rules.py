"""Guards for the ER ordinary-parts RG-normal convention; B/A are not interpreted."""
from pathlib import Path


def split_normal_references(references):
    """Only the audited DetailBlend slot 6 is a detail layer; never infer from UV scale."""
    normal = [t for t in references if 'normalmap' in t['type_name'].lower()]
    detail = [t for t in normal if t['type_name'].casefold() == 'c_detailblend__snp_texture2d_6_normalmap']
    return [t for t in normal if t not in detail], detail


def validate_direct_normal(record, normal, albedo, stem):
    references, detail = split_normal_references(record['references'])
    if not references:
        raise ValueError('Direct normal rule requires FLVER NormalMap semantics')
    explicit = {Path(t['path'].replace('\\', '/')).stem.casefold() for t in references if t['path']}
    if explicit and explicit != {(stem + '_n').casefold()}:
        raise ValueError('Explicit normal references conflict with ordinary template rule')
    if any(tuple(t.get('scale', (1, 1))) != (1, 1) for t in references):
        raise ValueError('Scaled/detail normal requires a separate material rule')
    nw,nh=normal['size']; aw,ah=albedo['size']
    if min(nw,nh,aw,ah)<=0 or nw*ah!=aw*nh:
        raise ValueError('Normal/BaseColor aspect ratios differ; review UV/layout')
    if normal['png_max_channel_error'] > 1 / 255 + 1e-5:
        raise ValueError('Normal channels changed during DDS conversion')
    return {'encoding': 'er_rg_direct', 'channels': 'RG -> UE normal XY; UE reconstructs Z; B/A unused',
            'unimplemented_detail_layers': [t for t in detail if t.get('path')],
            'reference_basis': 'explicit' if explicit else 'FLVER NormalMap semantic + exact template texture stem',
            'png_max_channel_error': normal['png_max_channel_error'],
            'validation_scope': 'ordinary ER parts convention; per-asset visual fidelity not certified'}
