"""Explicitly reset Designer-authored target-card typography/material; do not run over custom styling accidentally."""
import unreal as u
bp=u.load_asset('/Game/UI/HUD/WBP_EnemyTargetPanel')
widgets={w.get_name():w for w in u.UmbraEquipmentNetworkTestLibrary.get_blueprint_widgets(bp)}
for name,path in {'TextBlock_0':'/Game/Asset/Font/SourceHanSerifSC-Regular_Font','TextBlock_5':'/Game/Asset/Font/SimplifiedChinese/SourceHanSansSC-Regular_Font','HealthText':'/Game/Asset/Font/SimplifiedChinese/SourceHanSansSC-Regular_Font'}.items():
 w=widgets[name];font=w.get_editor_property('font');font.set_editor_property('font_object',u.load_asset(path));font.set_editor_property('typeface_font_name','Default');w.set_editor_property('font',font)
widgets['W_ProgressBar_Parent'].set_editor_property('ProgressBarMaterial',u.load_asset('/Game/Asset/fantasy_gui_4/materials/progressbar/Mat_GUI_ProgressBar_Red'))
u.BlueprintEditorLibrary.compile_blueprint(bp)
assert u.EditorAssetLibrary.save_loaded_asset(bp,False)
u.SystemLibrary.quit_editor()
