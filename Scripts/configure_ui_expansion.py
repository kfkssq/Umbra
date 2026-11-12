"""Explicit UE migration: target card and whole-window character/skills pages."""
import unreal as u,json
from pathlib import Path
out=Path(u.Paths.project_saved_dir())/'UIExpansion'
assert u.UmbraSkillEditorLibrary.create_target_and_menu_pages()
b=u.load_asset('/Game/UI/HUD/WBP_EnemyTargetPanel')
c=u.get_default_object(b.generated_class())
c.set_editor_property('fallback_portrait',u.load_asset('/Game/Asset/fantasy_gui_4/textures/icons/gold/fg4_iconsGold_profile'))
u.BlueprintEditorLibrary.compile_blueprint(b)
assert u.EditorAssetLibrary.save_loaded_asset(b,False)
b=u.load_asset('/Game/Blueprints/Enemies/BP_Enemy_Melee_01')
c=u.get_default_object(b.generated_class())
c.set_editor_property('enemy_display_name','近战守卫')
# Display level is authored UI identity, not a claim about combat scaling or loot level.
c.set_editor_property('enemy_display_level',1)
u.BlueprintEditorLibrary.compile_blueprint(b)
assert u.EditorAssetLibrary.save_loaded_asset(b,False)
r={}
for p in ['/Game/UI/CharacterMenu/WBP_CharacterMenu','/Game/UI/HUD/WBP_EnemyTargetPanel','/Game/UI/HUD/WBP_CombatHUD']:
 r[p]=u.UmbraSkillEditorLibrary.describe_widget(u.load_asset(p))
(out/'configured.json').write_text(json.dumps(r,ensure_ascii=False,indent=2),encoding='utf-8')
u.log('UI_EXPANSION_CONFIGURED')
