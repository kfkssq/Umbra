"""Restore native Ability_E and original locomotion graph; retain the saved channel loop."""
import unreal as u,json,datetime,shutil
from pathlib import Path
root=Path(u.Paths.project_dir()).resolve();out=root/'Saved/SwordStorm';out.mkdir(exist_ok=True)
backup=out/'Backup'/datetime.datetime.now().strftime('%Y%m%d_%H%M%S')
def load(p):
 o=u.load_asset(p);assert o,p;return o
def save(o):
 src=root/'Content'/(o.get_path_name().split('.')[0].removeprefix('/Game/')+'.uasset')
 if src.exists():
  dst=backup/src.relative_to(root/'Content');dst.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(src,dst)
 assert u.EditorAssetLibrary.save_loaded_asset(o,False),o.get_path_name()
d=load('/Game/Skills/Greystone/DA_SwordStorm')
m=load('/Game/Skills/Greystone/AM_SwordStorm_Channel')
a=load('/Game/ParagonGreystone/Characters/Heroes/Greystone/Animations/Ability_E')
# SetSkillMontageSequence preserves existing self-linked Loop section and restores UpperBody.
assert u.UmbraSkillEditorLibrary.set_skill_montage_sequence(m,a,3)
save(m)
bp=load('/Game/Blueprints/Player/BP_UmbraPlayerCharacter')
abp=load('/Game/Blueprints/Player/Greystone_AnimBlueprint')
mesh=u.get_default_object(bp.generated_class()).get_component_by_class(u.SkeletalMeshComponent)
mesh.set_editor_property('anim_class',abp.generated_class());u.BlueprintEditorLibrary.compile_blueprint(bp);save(bp)
if d.montage!=m or not d.channel_while_held:
 d.set_editor_property('montage',m);d.set_editor_property('channel_while_held',True);save(d)
u.log('NATIVE_CHANNEL_RESTORED')
