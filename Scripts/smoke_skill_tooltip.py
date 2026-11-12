"""Rendered skill-tooltip smoke: configured descriptions, empty/reused slot, and actual cursor hover."""
import unreal as u,json,time
from pathlib import Path
out=Path(u.Paths.project_saved_dir())/'UIExpansion';lib=u.UmbraSkillEditorLibrary
level=u.get_editor_subsystem(u.LevelEditorSubsystem);editor=u.get_editor_subsystem(u.UnrealEditorSubsystem)
previous=u.UmbraEquipmentNetworkTestLibrary.configure_play_settings(0,1,True)
performance=u.get_default_object(u.load_class(None,'/Script/UnrealEd.EditorPerformanceSettings'))
previous_throttle=performance.get_editor_property('bThrottleCPUWhenNotForeground')
performance.set_editor_property('bThrottleCPUWhenNotForeground',False)
u.EditorPythonScripting.set_keep_python_script_alive(True)
start=time.monotonic();stamp=start;state={'stage':'start'};slots=[];pc=world=None;sequence=[0,1,5,6];step=0

def tip(slot):
 if not slot.read_view_state().granted:return None
 return next((w for w in sorted(u.WidgetLibrary.get_all_widgets_of_class(world,u.UmbraSkillTooltip,False),key=lambda w:not w.is_in_viewport()) if str(lib.find_presentation_widget(w,'SkillName').get_text())==str(slot.read_view_state().name)),None)
def aim(slot):
 candidates=[slot,lib.find_presentation_widget(slot,'SizeBox_0'),lib.find_presentation_widget(slot,'ContentBorder'),lib.find_presentation_widget(slot,'W_Item_Slot.item_icon')]
 geometries=[]
 for candidate in candidates:
  if not candidate:continue
  geometry=candidate.get_paint_space_geometry();size=u.SlateLibrary.get_local_size(geometry)
  geometries.append({'path':candidate.get_path_name(),'size':str(size)})
  if size.x>0 and size.y>0:break
 state['geometries']=geometries
 assert size.x>0 and size.y>0,'No painted geometry '+str(geometries)
 pixel,_=u.SlateLibrary.local_to_viewport(world,geometry,u.Vector2D(size.x*.5,size.y*.5))
 pc.set_mouse_location(int(pixel.x),int(pixel.y))
 state['aim']={'pixel':str(pixel),'size':str(size),'visibility':str(slot.get_visibility()),'mouse':str(pc.get_mouse_position())}

def finish(error=None):
 state.update(result='failed' if error else 'passed',error=error)
 (out/'skill-tooltip-pie.json').write_text(json.dumps(state,ensure_ascii=False,indent=2),encoding='utf-8')
 performance.set_editor_property('bThrottleCPUWhenNotForeground',previous_throttle)
 u.UmbraEquipmentNetworkTestLibrary.configure_play_settings(previous[0],previous[1],bool(previous[2]))
 u.unregister_slate_post_tick_callback(callback);u.EditorPythonScripting.set_keep_python_script_alive(False);u.SystemLibrary.quit_editor()
def tick(delta):
 global world,pc,slots,stamp,step,sequence
 try:
  assert time.monotonic()-start<120,'Timeout '+state['stage']
  if state['stage']=='start':level.editor_request_begin_play();state['stage']='prepare';return
  if state['stage']=='prepare' and level.is_in_play_in_editor() and time.monotonic()-start>12:
   world=editor.get_game_world();pc=u.GameplayStatics.get_player_controller(world,0)
   widgets=u.WidgetLibrary.get_all_widgets_of_class(world,u.UserWidget,False)
   slots=sorted([w for w in widgets if isinstance(w,u.UmbraSkillSlotWidget)],key=lambda w:w.slot_index)
   state['slot_count']=len(slots)
   state['slots']=[{'index':w.slot_index,'name':str(w.read_view_state().name),'description':str(w.read_view_state().description),'granted':w.read_view_state().granted,'tooltip':str(tip(w)),'class':str(w.get_editor_property('skill_tooltip_class'))} for w in slots]
   assert len(slots)==10, 'Slot count='+str(len(slots))
   checked=[]
   for slot in slots:
    slot.refresh_skill();view=slot.read_view_state();tooltip=tip(slot)
    if view.granted:
     assert view.granted and str(view.description).strip(),'Missing description '+str(slot.slot_index)
     assert isinstance(tooltip,u.UmbraSkillTooltip),'Missing tooltip '+str(slot.slot_index)
     assert str(lib.find_presentation_widget(tooltip,'SkillName').get_text())==str(view.name)
     assert str(lib.find_presentation_widget(tooltip,'DescriptionText').get_text())==str(view.description)
     checked.append(str(view.name))
    else:assert tooltip is None,'Empty slot has a tooltip'
   # A reused slot must detach stale text when it becomes unconfigured.
   first=slots[0];old_tip=tip(first);first.set_editor_property('slot_index',99);first.refresh_skill();assert not old_tip.is_in_viewport()
   first.set_editor_property('slot_index',0);first.refresh_skill();assert tip(first) is not None
   assert checked,'No configured skills'
   empty=next(s.slot_index for s in slots if not s.read_view_state().granted)
   sequence=[0,1,max(s.slot_index for s in slots if s.read_view_state().granted),empty]
   state['descriptions']=checked
   for w in widgets:
    if isinstance(w,u.UmbraAttributeDebugPanel):w.set_visibility(u.SlateVisibility.COLLAPSED)
   state['stage']='hover';stamp=time.monotonic();return
  if state['stage']=='hover':
   slot=slots[sequence[step]];aim(slot)
   if time.monotonic()-stamp<1.5:return
   state['hovered']=slot.is_hovered()
   tooltip=tip(slot)
   if slot.read_view_state().granted:
    assert tooltip and tooltip.is_in_viewport(),'Hovered tooltip absent'
    tip_geo=tooltip.get_paint_space_geometry();tip_size=u.SlateLibrary.get_local_size(tip_geo)
    tip_bottom,_=u.SlateLibrary.local_to_viewport(world,tip_geo,u.Vector2D(0,tip_size.y))
    slot_top,_=u.SlateLibrary.local_to_viewport(world,slot.get_paint_space_geometry(),u.Vector2D(0,0))
    assert tip_bottom.y<=slot_top.y,'Tooltip is not above the HUD skill slot'
   u.SystemLibrary.execute_console_command(world,'Shot SHOWUI filename="'+(out/('skill-tooltip-'+str(slot.slot_index)+'.png')).resolve().as_posix()+'" -nosuffix')
   state['stage']='shot';stamp=time.monotonic();return
  if state['stage']=='shot':
   aim(slots[sequence[step]])
   if time.monotonic()-stamp<.6:return
   assert state['hovered'],'Slot not hovered at '+str(state['aim'])
   step+=1
   if step<len(sequence):state['stage']='hover';stamp=time.monotonic();return
   pc.set_mouse_location(5,5);state['stage']='leave';stamp=time.monotonic();return
  if state['stage']=='leave' and time.monotonic()-stamp>1:
   assert not any(s.is_hovered() for s in slots),'Hover remains after leave'
   level.editor_request_end_play();state['stage']='end';return
  if state['stage']=='end' and not level.is_in_play_in_editor():finish()
 except Exception as e:finish(repr(e))
callback=u.register_slate_post_tick_callback(tick)
