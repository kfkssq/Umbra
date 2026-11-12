"""Use ActionbarDemo's existing slot/key textures while retaining GAS-bound project widgets."""
import unreal as u,json,os
bar=u.load_asset('/Game/UI/Skills/WBP_SkillBar'); slot=u.load_asset('/Game/UI/Skills/WBP_SkillSlot')
def tree(bp):return {w.get_name():w for w in u.UmbraEquipmentNetworkTestLibrary.get_blueprint_widgets(bp)}
b=tree(bar); s=tree(slot)
assert all(n in s for n in ['Size_Root','Img_Background','Img_Icon','Pb_Cooldown','Img_Frame','Txt_Key','Txt_Cooldown','Txt_Cost'])
assert all(n in b for n in ['HB_Skills','SkillSlot_0','SkillSlot_1'])
brown=u.load_asset('/Game/Asset/fantasy_gui_4/textures/buttons/fg4_slotBrown')
key=u.load_asset('/Game/Asset/fantasy_gui_4/textures/buttons/fg4_slotTooltip1')
assert brown and key
# The source frame includes transparent side margins; overlap those margins so visible edges touch.
s['Size_Root'].set_width_override(64);s['Size_Root'].set_height_override(90)
def place(name,x,y,w,h):
    cs=s[name].slot;cs.set_position(u.Vector2D(x,y));cs.set_size(u.Vector2D(w,h))
s['Img_Background'].set_brush_from_texture(brown,False);place('Img_Background',0,0,72,72)
place('Img_Icon',6,6,60,60);place('Pb_Cooldown',6,6,60,60)
place('Txt_Cooldown',6,20,60,32);place('Txt_Cost',47,52,20,18)
s['Img_Frame'].set_brush_from_texture(key,False);s['Img_Frame'].set_visibility(u.SlateVisibility.HIT_TEST_INVISIBLE)
place('Img_Frame',24,62,24,24);place('Txt_Key',24,62,24,24)
if 'Txt_Name' in s:s['Txt_Name'].set_visibility(u.SlateVisibility.COLLAPSED)
row=b['HB_Skills'];row.slot.set_horizontal_alignment(u.HorizontalAlignment.H_ALIGN_LEFT)
row.slot.set_vertical_alignment(u.VerticalAlignment.V_ALIGN_CENTER);row.slot.set_padding(u.Margin(4,0,0,0))
for n in ['SkillSlot_0','SkillSlot_1']:b[n].slot.set_padding(u.Margin(0,0,0,0))
for bp in [slot,bar]:
    u.BlueprintEditorLibrary.compile_blueprint(bp)
    assert u.EditorAssetLibrary.save_loaded_asset(bp,False)
report={'source':'W_Window_Actionbar / W_Actionbar_Item_Slot','slot':brown.get_path_name(),
        'key_background':key.get_path_name(),'slot_size':[64,90],'frame_size':[72,72],'row_alignment':'left','row_padding':[4,0,0,0],
        'slot_padding':[0,0,0,0]}
with open(os.path.join(u.Paths.project_saved_dir(),'SkillHUD','actionbar-style.json'),'w',encoding='utf-8') as f:json.dump(report,f,ensure_ascii=False,indent=2)
u.log('ACTIONBAR_STYLE_SAVED')
