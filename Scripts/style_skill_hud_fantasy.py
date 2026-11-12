"""Style the existing skill WBP through UE; preserve structure, game bindings and user layout."""
import os,json
import unreal as u

bar=u.load_asset('/Game/UI/Skills/WBP_SkillBar')
slot=u.load_asset('/Game/UI/Skills/WBP_SkillSlot')
def widgets(bp): return {w.get_name():w for w in u.UmbraEquipmentNetworkTestLibrary.get_blueprint_widgets(bp)}
b=widgets(bar); s=widgets(slot)
assert isinstance(b.get('Img_BarFrame'),u.Image)
assert all(isinstance(s.get(n),u.Image) for n in ['Img_Background','Img_Frame','Img_Icon'])
stone=u.load_asset('/Game/Asset/fantasy_gui_4/textures/borders/fg4_borders_stone')
metal=u.load_asset('/Game/Asset/fantasy_gui_4/textures/quality/fg4_slotGreyLight')
assert stone and metal
# Box preserves the thin perimeter while stretching the central stone texture.
brush=b['Img_BarFrame'].get_editor_property('brush')
brush.set_editor_property('resource_object',stone)
brush.set_editor_property('draw_as',u.SlateBrushDrawType.BOX)
brush.set_editor_property('margin',u.Margin(0.04,0.04,0.04,0.04))
b['Img_BarFrame'].set_editor_property('brush',brush)
s['Img_Background'].set_brush_from_texture(metal,False)
s['Img_Background'].slot.set_position(u.Vector2D(10,16))
s['Img_Background'].slot.set_size(u.Vector2D(72,72))
# Fantasy slot includes a dark opaque center, so it sits behind the icon.
s['Img_Frame'].set_visibility(u.SlateVisibility.COLLAPSED)
for bp in [slot,bar]:
    u.BlueprintEditorLibrary.compile_blueprint(bp)
    assert u.EditorAssetLibrary.save_loaded_asset(bp,False)
report={'bar_texture':stone.get_path_name(),'bar_draw_as':'Box','bar_margin':0.04,
        'slot_texture':metal.get_path_name(),'slot_frame_visibility':'Collapsed',
        'slot_background_position':[10,16],'slot_background_size':[72,72]}
os.makedirs(os.path.join(u.Paths.project_saved_dir(),'SkillHUD'),exist_ok=True)
with open(os.path.join(u.Paths.project_saved_dir(),'SkillHUD','fantasy-style.json'),'w',encoding='utf-8') as f:json.dump(report,f,ensure_ascii=False,indent=2)
u.log('FANTASY_SKILL_STYLE_SAVED')
