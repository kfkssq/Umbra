"""User-approved six-rarity hue order; hex values are editable sRGB tuning."""
import unreal as u
NAMES=("COMMON","MAGIC","RARE","EPIC","LEGENDARY","UNIQUE")
HEX=("FFFFFF","4BCB63","448AFF","6B30B7","E83E8C","A88732")
def color(index):
    rgb=[int(HEX[index][i:i+2],16)/255 for i in (0,2,4)]
    return u.LinearColor(*[(v/12.92 if v<=.04045 else ((v+.055)/1.055)**2.4) for v in rgb],1)
def art():
    root="/Game/Asset/classic-rpg-ui/Assets/ClassicRPGUI/ClassicUI/SpecialFrames/"
    paths=dict(slot_background_texture=root+"Background_grey",rarity_frame_texture=root+"Frame_grey",
        tooltip_title_background_texture="/Game/Asset/fantasy_gui_4/textures/quality/fg4_glowGreyLight",
        tooltip_icon_background_texture="/Game/Asset/fantasy_gui_4/textures/quality/fg4_glowGreyLight")
    values={k:u.load_asset(v) for k,v in paths.items()}
    if not all(isinstance(v,u.Texture2D) for v in values.values()): raise RuntimeError("Missing neutral rarity art")
    return values
def style(row,index):
    row.set_editor_property("color",color(index))
    row.set_editor_property("tint_backgrounds",True)
    for key,value in art().items(): row.set_editor_property(key,value)
    return row
def six_rows():
    rows=[]
    for index,name in enumerate(NAMES):
        row=u.UmbraGenerationRarity()
        row.set_editor_property("rarity",getattr(u.UmbraItemRarity,name))
        row.set_editor_property("weight",1.0)
        row.set_editor_property("affix_count",index)
        rows.append(style(row,index))
    return rows
