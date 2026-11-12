"""Explicit user-authorized UI migration; saves only project-owned WBP and controller config."""
import unreal as u
import os,json
assert u.UmbraSkillEditorLibrary.embed_skill_bar_in_hud(), 'Unexpected HUD hierarchy; inspect before changing'
pc=u.load_asset('/Game/Blueprints/Player/BP_UmbraPlayerController')
u.get_default_object(pc.generated_class()).set_editor_property('skill_bar_class',None)
u.BlueprintEditorLibrary.compile_blueprint(pc)
assert u.EditorAssetLibrary.save_loaded_asset(pc,only_if_is_dirty=False)
hud=u.load_asset('/Game/UI/HUD/WBP_CombatHUD')
u.BlueprintEditorLibrary.compile_blueprint(hud)
assert u.EditorAssetLibrary.save_loaded_asset(hud,only_if_is_dirty=False)
report={}
for path in ['/Game/UI/HUD/WBP_CombatHUD','/Game/UI/HUD/WBP_PlayerHUDGroup','/Game/UI/Skills/WBP_SkillBar','/Game/UI/Skills/WBP_SkillSlot']:
    report[path]=u.UmbraSkillEditorLibrary.describe_widget(u.load_asset(path))
report['independent_skill_bar_class']=str(u.get_default_object(pc.generated_class()).get_editor_property('skill_bar_class'))
out=os.path.join(u.Paths.project_saved_dir(),'SkillHUD','configuration.json')
with open(out,'w',encoding='utf-8') as f: json.dump(report,f,ensure_ascii=False,indent=2)
u.log('EMBEDDED_SKILL_HUD_PASSED')
