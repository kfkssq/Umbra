# 第一阶段 Inventory：空背包与 Blueprint 完整接线

## 代码与范围

2026-09-30，按用户后续澄清修正布局：三栏同时显示，分类按钮复用属性页按钮的文字变体。`UUmbraInventoryMenu → WBP_Inventory` 管理容量、列数、动态 UniformGrid、委托及唯一选中格；`UUmbraInventorySlot → WBP_InventorySlot` 接收鼠标并提供视觉事件。没有 ItemDefinition、InventoryComponent、InventoryEntry、测试物品、拾取、拖拽、Tooltip 或装备联动。未来数据链可以是 ItemDefinition → InventoryComponent → InventoryEntry → InventoryMenu → InventorySlot；当前格子索引只是 UI 索引，不是持久物品标识。

- `UI/Inventory/UmbraInventoryMenu.h/.cpp`：默认40格、8列；`Index / Columns`、`Index % Columns`；Construct调用幂等的InitializeSlots。相同树/配置复用原格子；重建先解绑并清除旧格子。Destruct解绑但保留格子及选中，Construct重新绑定。重复关闭/打开不追加。CapacityText由C++写为0/容量。
- `UI/Inventory/UmbraInventorySlot.h/.cpp`：C++管理Hover、Selected；只有Menu能设置Selected；点击只发选择请求。C++给出ShouldHighlight，BP只配置视觉。Empty层在BP刷新后统一Collapsed，背景Visible。
- CharacterMenu保持原有并排组合：Attribute、Equipment、Inventory同时可见。本轮误加的CharacterPageSwitcher、三个Tab绑定和SetActiveTab已撤回，原有预览观察/启停实现不变。未来SkillTreeMenu与整个CharacterMenu在外层并列，当前不实现该外层切页。
- `UI/Equipment/UmbraEquipmentSlotWidget.cpp`：BP_RefreshVisual结束后按HasItem设置RarityFrame。空槽必Collapsed；现有有物品快照仍显示稀有度颜色。SlotType、Empty、Hovered、Selected、Locked均不决定稀有度可见性。

没有新增Tick或生产代码资产路径。FullBody Preview、GAS属性读取和装备计算未修改。

## 首轮已保存资产检查与编辑边界（历史快照）

本轮用关联UE 5.8.2命令行编辑器只读加载；报告为本地 `Saved/InventoryPhase1Inspection.json`，未保存资产，未读取GUI未保存改动。

| 资产 | 本轮读取到的内容 |
| --- | --- |
| WBP_CharacterMenu | 父类已为UmbraCharacterMenu。已保存树为HorizontalBox_0下SizeBox_105→ScaleBox_0→WBP_AttributeMenu，及SizeBox_0→ScaleBox_1→WBP_Equipment；没有顶层WidgetSwitcher/Tab |
| WBP_EquipmentSlot | 父类UmbraEquipmentSlotWidget；有EmptyIcon/ItemIcon/RarityFrame/HighlightFrame/LockedOverlay/slot_background；RarityFrame的Designer默认值仍为Visible |
| WBP_InventorySlot | 已存在，父类UserWidget。沿用装备槽外形，有SizeBox_32/Overlay_24/slot_background/EmptyIcon/ItemIcon/RarityFrame/HighlightFrame/LockedOverlay，缺StackCountText、EquippedMarker |
| WBP_Inventory | 尚无此资产 |
| M_UI_InteractionHighlight、MI_UI_InteractionHighlight_Equipment | 资产存在；沿用既有[高亮材质契约](UIInteractionHighlight.md)，最终外观待PIE |

下面资产表是首轮检查快照。后续用户已编辑WBP_InventorySlot，并新增WBP_Inventory和Inventory高亮MI；这些工作区资产完整保留，当前Graph与GUI未保存状态待编辑器确认。Codex只修改C++和指南，Designer/Graph/材质操作由用户执行。

## 1. 编译与打开

1. 保存当前资产，关闭Unreal Editor。本次新增UCLASS、BindWidget和反射函数，使用完整编译，不依赖Live Coding/Hot Reload。
2. Rider选择 **UmbraEditor / Win64 / Development**，Build。没有改模块或Target，通常无需重新生成工程；Rider未显示新文件时刷新工程。
3. 本机安装位置已从Epic Launcher清单核对：`C:\Program Files\Epic Games\UE_5.8`，补丁版本5.8.2。PowerShell等价命令：

```powershell
& 'C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat' UmbraEditor Win64 Development '-Project=C:\Users\kfkssq\Documents\Unreal Projects\Umbra\Umbra.uproject' -WaitMutex
```

4. 构建成功后打开原项目。Parent Class搜索时不带U前缀，如UmbraInventorySlot。
5. 后续仅修改WBP布局/节点/默认值，使用蓝图Compile、Save即可。再次修改C++反射声明时仍应关闭编辑器重编。

## 2. WBP_InventorySlot：创建/迁移

已有 `/Game/UI/CharacterMenu/InventoryMenu/WBP_InventorySlot`，直接修改这一份：File → Reparent Blueprint → **UmbraInventorySlot**。若从零创建：Content Browser → User Interface → Widget Blueprint → All Classes → UmbraInventorySlot，命名WBP_InventorySlot。不要选UmbraEquipmentSlotWidget或普通UserWidget。

Designer里将SizeBox_32改为SizeBox_Root、Overlay_24改为Overlay_Root、slot_background改为SlotBackground。EmptyIcon和LockedOverlay是复制来的装备槽内容：本阶段不使用，可保持Collapsed并移除其旧显示接线；也可在确认无引用后从这个Inventory模板删除。不要删除Equipment模板的这些控件。

最终目标树（同层先画上面的，再画下面的）：

```text
WBP_InventorySlot [Parent: UmbraInventorySlot]
└─ SizeBox_Root              SizeBox
   └─ Overlay_Root           Overlay
      ├─ SlotBackground     Image
      ├─ ItemIcon           Image
      ├─ RarityFrame        Image
      ├─ StackCountText     TextBlock
      ├─ EquippedMarker     Image
      └─ HighlightFrame     Image
```

| 精确名称 | Is Variable | 初始Visibility | 基本设置 |
| --- | --- | --- | --- |
| SizeBox_Root | 不需要 | Not Hit-Testable (Self Only) | Width Override=72、Height Override=72；子内容水平/垂直Fill |
| Overlay_Root | 不需要 | Not Hit-Testable (Self Only) | 填充SizeBox；不要设Self & All Children |
| SlotBackground | **是** | **Visible** | Overlay Slot水平/垂直Fill、Padding0；沿用空槽底图；Render Opacity1 |
| ItemIcon | **是** | **Collapsed** | Fill、Padding6；Brush资源留空；白色/Alpha1 |
| RarityFrame | **是** | **Collapsed** | Fill、Padding0；资源可暂留空，未来物品稀有度边框；不要用交互高亮材质填这里 |
| StackCountText | **是** | **Collapsed** | 文本留空，字体12；Overlay右/下对齐、Padding4；不创建名为StackCount的变量 |
| EquippedMarker | **是** | **Collapsed** | 右/上对齐，Padding4；资源留空；未来才配置装备标记 |
| HighlightFrame | **是** | **Collapsed** | Fill、Padding0；白色/Alpha1、Render Opacity1；材质见下一节 |

所有Image的Brush Draw As可先用Image；底图若原本使用Box及边距则保留原样式。空资源的Image不要用Visible占位。WBP Class Defaults的Visibility用Visible，Is Focusable=false，Tick Frequency=Never；Menu创建格子时也会将实例设为Visible。SizeBox/Overlay不能设“Not Hit-Testable (Self & All Children)”，否则整棵子树无法命中。

这六个叶控件是必需BindWidget，类型和大小写必须匹配。不要手动新增同名普通Blueprint变量；变量由Designer控件提供。Compile提示缺失绑定时先补全树。继承变量看不到时开启My Blueprint → Show Inherited Variables。

## 3. 高亮材质和BP_RefreshVisual

1. 在 `/Game/UI/Material` 找到现有 **M_UI_InteractionHighlight**，右键 → Create Material Instance，命名 **MI_UI_InteractionHighlight_Inventory**。或先直接使用现有Equipment MI验证功能。
2. 新MI的Parent保持M_UI_InteractionHighlight，不复制/修改世界敌人描边材质。
3. 建议起点：SlotAspectRatio=1，HighlightColor线性RGB=(1,0.054443,0)，Opacity=1，BorderWidth=0.012，GlowIntensity=0.35，GlowWidth=0.04，Softness=0.008，Inset=0.06。按实际72×72外观微调；参数已在[材质指南](UIInteractionHighlight.md)定义，无需新增节点或Tick。
4. HighlightFrame → Brush → Image选择此MI，Draw As=Image；Color and Opacity白色Alpha1、Render Opacity1。
5. Graph → Overrides或右键搜索 **BP Refresh Visual**。本类事件只有 **Should Highlight** bool引脚；没有装备槽的State/ItemDisplay参数。
6. 从事件白色执行引脚接 **Branch**；Condition接Should Highlight。
7. 从Designer变量HighlightFrame拖Get，接两个 **Set Visibility** 的Target。
8. Branch的True执行引脚接第一个Set Visibility，值为 **Not Hit-Testable (Self & All Children)**（C++枚举HitTestInvisible）。False接另一个Set Visibility，值为 **Collapsed**。

```text
Event BP Refresh Visual(Should Highlight)
  → Branch [Condition = Should Highlight]
      True  → Set Visibility(Target=HighlightFrame, HitTestInvisible)
      False → Set Visibility(Target=HighlightFrame, Collapsed)
```

C++已提供相同的基础高亮可见性，所以上述视觉事件可作为你后续美术调整的入口。不要在BP重新计算Hovered OR Selected；不要在这个事件调用RefreshVisual、RequestSelection或其它状态入口，避免递归/重复通知。当前不添加延迟、动画、Tick、Tooltip或内部Button。

**输入边界**：C++自动处理Mouse Enter/Leave及左键，Menu自动取消旧Selected并选中新格；BP不接OnMouseEnter/Leave/MouseButtonDown，不维护Selected变量，不遍历其它格子。Hovered和Selected可以同时存在；鼠标移走后Selected仍亮。选中A后悬停B可以看到两个高亮，但只有A是Selected，符合规则；点击B后A取消Selected。

## 4. WBP_Inventory：创建与完整树

在 `/Game/UI/CharacterMenu/InventoryMenu` 新建Widget Blueprint，Parent Class选 **UmbraInventoryMenu**，命名 **WBP_Inventory**。

```text
WBP_Inventory [Parent: UmbraInventoryMenu]
└─ Overlay_Root                         Overlay
   ├─ Background                        Image
   └─ VerticalBox_Content               VerticalBox
      ├─ Header                         HorizontalBox
      │  ├─ TitleText                   TextBlock
      │  └─ CapacityText                TextBlock
      ├─ FilterBar                      HorizontalBox
      │  ├─ Filter_All                  W_ButtonBrownSquare_1 [全部]
      │  ├─ Filter_Weapon               W_ButtonBrownSquare_1 [武器]
      │  ├─ Filter_Armor                W_ButtonBrownSquare_1 [装备]
      │  ├─ Filter_Accessory            W_ButtonBrownSquare_1 [配件]
      │  └─ Filter_Consumable           W_ButtonBrownSquare_1 [消耗品]
      ├─ ScrollBox_Inventory            ScrollBox
      │  └─ InventoryGrid               UniformGridPanel
      └─ BottomBar                      HorizontalBox
         ├─ FooterText                  TextBlock（可留空/Collapsed）
         └─ SortButton                  W_ButtonBrownSquare_1 [整理]
```

**Is Variable**：CapacityText、InventoryGrid必须勾选。其余没有C++绑定，也不需要Graph引用，可不勾选；ScrollBox_Inventory为调试可选勾选。Filter按钮无需事件，勾选变量也不会实现过滤。

设置顺序：

1. Overlay_Root使用Not Hit-Testable (Self Only)。Background在最底层，Fill/Fill；设置页面底图或颜色，Visibility=Not Hit-Testable (Self & All Children)，防止遮挡交互。
2. VerticalBox_Content的Overlay Slot设Fill/Fill、Padding16。Header、FilterBar、BottomBar在VerticalBox中Size=Auto；ScrollBox_Inventory的Size=Fill 1。
3. TitleText文本Inventory，字号24；其HorizontalBox Slot设Fill 1。CapacityText初始文本0/40，字号16、右对齐；Slot设Auto、垂直居中。文本只是Designer占位，运行由C++写。**不要建立Text属性绑定**，避免覆盖C++更新。
4. 五个分类实例使用第5节当前选定的W_ButtonBrownSquare_1文字按钮，Text依次填写全部、武器、装备、配件、消耗品。保留该模板的Normal/Hovered/Pressed样式；不接过滤或切页事件。
5. ScrollBox_Inventory：Orientation=Vertical，Visibility=Visible；Scrollbar Visibility按需用Visible；Allow Overscroll=false；Consume Mouse Wheel=When Scrolling Possible；Clipping=Clip to Bounds。
6. InventoryGrid是ScrollBox的**唯一直接子控件**。选中InventoryGrid，其ScrollBox Slot的Size设Auto（不是Fill），水平Left或Center、垂直Top、Padding0。ScrollBox本身在VerticalBox中的Size仍为Fill 1。Grid Visibility=Not Hit-Testable (Self Only)。Slot Padding先用(4,4,4,4)，Min Desired Slot Width/Height设0以跟随WBP_InventorySlot实际期望尺寸；C++为每个Grid Slot设水平/垂直Center。父容器仍必须容得下全部列，详见第8节。
7. **InventoryGrid保持零个Designer子控件**。不放40个Slot、不在Construct写ForLoop/AddChild。动态格子只在运行时创建，Designer里看到空Grid是正常的。
8. BottomBar保留整理按钮SortButton，Text设“整理”；FooterText可以留空或Collapsed避免重复。BottomBar保持Auto，高度按文字/Padding决定。本阶段整理按钮只有交互反馈，不执行排序。
9. 当前推荐格子连边距约80个Slate单位，8列需约640宽；页面加左右16后至少约672宽，再为滚动条预留空间，建议父级给700以上可用宽。ScrollBox必须从父容器获得有限高度，例如约560；无限Auto高度会让它一直展开而不滚动。若现有菜单过窄，按视觉需求减少格子尺寸或缩放整页；Columns不会自动响应宽度。
10. WBP_Inventory Class Defaults → **Inventory**：Inventory Capacity=40，Columns=8，Inventory Slot Class=**WBP_InventorySlot**。无需在CharacterMenu实例上另设这些参数。Compile/Save。

配置来源/覆盖：C++后备40/8 → WBP_Inventory的Class Defaults覆盖；参数为EditDefaultsOnly，运行没有Blueprint写入接口。容量运行夹到≥0，列数夹到≥1；SlotClass必须明确配置，没有资产路径或静默默认模板。格子尺寸/Brush来自WBP_InventorySlot，间距来自InventoryGrid，文字字体来自CapacityText。0/40中的0是本阶段明确的空内容，不是假造的InventoryComponent统计。

## 5. 三栏并排接入与复用文字分类按钮

### 5.1 CharacterMenu保持三栏同时显示

用户确认Attribute、Equipment、Inventory在同一CharacterMenu内同时显示。截图左侧属性、中间装备、右侧空区即背包位置。此前三页Tab方案已取消，不按旧指南创建CharacterPageSwitcher/TopTabs，也不调用CharacterMenu.SetActiveTab。WBP_AttributeMenu自身的内部页签仍保留原功能。

1. WBP_CharacterMenu父类保持UmbraCharacterMenu；保留现有Sizebox_Main、Overlay_95和HorizontalBox_0。
2. 原SizeBox_105 → ScaleBox_0 → WBP_AttributeMenu及SizeBox_0 → ScaleBox_1 → WBP_Equipment保持原尺寸、缩放和内容。
3. 在HorizontalBox_0右侧新增SizeBox，命名SizeBox_Inventory，作为第三个子项。若你已为截图右侧留了容器，直接使用该容器。
4. 把已创建的WBP_Inventory放入右侧容器，实例名InventoryPage。容器子项Fill/Fill。仅需要调试引用InventoryPage时勾选Is Variable。
5. 可用Width Override指定右栏宽度，或在HorizontalBox Slot分配Fill份额；根据整个菜单实际宽度调整。8列72格加Padding约需640单位宽，页面还有边距和滚动条；右栏过窄时统一缩小格子/间距或使用与现有两栏一致的ScaleBox，不修改中间FullBody的配置。
6. 三个内容区域均保持Visible或Not Hit-Testable (Self Only)。不要在它们外面放互斥显示的Switcher。保留PlayerController原菜单打开/关闭流程。
7. 如果已照旧指南建立三页Switcher，在Designer内将原三个页面容器移回HorizontalBox并排，再清理仅为该误解新增的Tab和相关Set Active Tab调用；保留属性页内部Switcher。不要在文件系统移动或删除uasset。

```text
WBP_CharacterMenu
└─ 原菜单外壳
   └─ HorizontalBox_0
      ├─ SizeBox_105 → ScaleBox_0 → WBP_AttributeMenu
      ├─ SizeBox_0   → ScaleBox_1 → WBP_Equipment
      └─ SizeBox_Inventory → InventoryPage : WBP_Inventory
```

未来若实现技能树，层级才是 `外层菜单容器 → CharacterMenu / SkillTreeMenu`；CharacterMenu内部仍保留三栏。当前不创建SkillTreeMenu或外层切页代码。

### 5.2 当前文字按钮：W_ButtonBrownSquare_1（后续选择，以本节为准）

用户改用资产现成文字按钮：顶部五个分类，下方一个整理。无需再执行之前的图标按钮复制/隐藏图标方案。只读检查确认六个实例都使用 `/Game/Asset/fantasy_gui_4/widgets/templates/buttons/W_ButtonBrownSquare_1`；父类UserWidget，内部Button_0包含TextBlock_0，已有Text参数，默认值“Text”。报告 `Saved/InventoryButtonInspection.json`。

Button_0已有Normal/Hovered/Pressed/Disabled四个Brush，分别引用fg4_buttonBrown1_Up、Over、Down、Disabled，Draw As=Box且配置了Margin；另有悬停/按下SoundCue。UMG自动按指针状态切换这些样式，不需要物品、GameplayTag、OnHovered/OnPressed图表或新的C++类。保留UserWidget父类，不Reparent为AttributeMenu或MenuTabButton。

1. 在WBP_Inventory Designer选中每个按钮实例，从Details搜索 **Text**，修改模板公开的文字参数。上方五个按顺序设“全部、武器、装备、配件、消耗品”，下方设“整理”。改的是各实例参数，不是所有实例共用的模板默认值。
2. 若Details没有Text字段，打开按钮WBP，My Blueprint选择已有Text变量，检查Instance Editable是否勾选；不要另建Text同名变量。为了不改变其他使用者，需要改模板Graph/变量标记/字体时，先在Editor中Duplicate为自己目录下的WBP_InventoryButton，再用副本替换六个实例；保留原资源引用和来源记录。
3. 如果修改实例参数后文字不更新，在按钮模板检查已有文字绑定/PreConstruct。确认TextBlock_0使用已有Text变量；已接好则不重复添加。需要补接时，TextBlock_0勾选Is Variable，在已有Event Pre Construct执行链末尾接 `Set Text(Target=TextBlock_0, In Text=Text变量)`。若Text属性已有Binding，选择保留绑定或使用SetText一条路径，不同时重复接线。
4. 文字设Not Hit-Testable (Self & All Children)；Button_0保持Visible、Is Enabled=true。按钮UserWidget和带按钮的父容器用Visible或Not Hit-Testable (Self Only)，不能用Self & All Children。背景/装饰Image不要挡住按钮。
5. 保留Button_0.Style中的全部Brush、Box Margin和声音。如果要调整反馈强度，编辑对应Hovered/Pressed Brush的Tint，先保留已有纹理。不要只连接事件却没有视觉写入，也不必用Event Tick。
6. 可将Button_0的Is Focusable设false，避免纯鼠标分类按钮抢键盘焦点；不影响鼠标悬停/按下。这需要修改模板时按第2步使用项目副本。
7. 当前TextBlock_0的字体资源是ArbutusSlab-Regular_Font，字号30；若中文出现缺字，给项目副本选支持中文的Font/Composite Font。字号与整体ScaleBox共同决定画面大小，按最终PIE调整，必要时增大按钮宽度容纳“消耗品”。
8. 底栏已有独立FooterText“整理”，按钮也设“整理”后会重复。若只想一个整理入口，可将FooterText留空或Collapsed，保留真正按钮。
9. Compile/Save按钮（若有修改）和Inventory，重新进入PIE。Designer画布主要用于编辑，不以在Designer中点击能否变色判断运行交互。

```text
按钮已有结构：
Button_0（保留原Style）
└─ TextBlock_0

仅在已有文字驱动缺失时补：
Event Pre Construct → Set Text
    Target  = TextBlock_0
    In Text = 已有Text变量
```

五个分类按钮当前只呈现Normal→Hovered→Pressed→松开回Hovered→移走回Normal；“按下反馈”不等于“松开后保持Selected”。不建立虚假的物品分类或标签。整理按钮同样有瞬时反馈，但本阶段不排序、不清空/重建Grid，不改变容量、顺序或Slot Selected。

如果需要单独确认点击事件通路，可在项目按钮副本内部Button_0的OnClicked临时接 `Print String("按钮点击")`，只用于PIE调试，完成后移除。一般仅观察Pressed样式和已有按下音效即可，无需该节点。以后真正做过滤/整理时，再让按钮转发点击给InventoryMenu，由C++管理选择/数据规则；本阶段不实现。

验证：六个实例显示各自文字；鼠标移入/按住/松开/移出依次切样式，整理同样响应；点击不切页面、不改背包格数或容量，三栏保持同时显示。若没有反馈，检查最内层Button_0、祖先Visibility和遮挡，而不是先添加物品或改父类。可用Widget Reflector定位实际命中控件。

## 6. EquipmentSlot的RarityFrame修复

C++已经在每次RefreshVisual的BP事件返回后应用HasItem约束，修复空槽可见性。已保存WBP的Designer默认仍为Visible，**需要手动把默认值改为Collapsed，并检查是否有旧的属性绑定/动画/异步回调再次显示它**。

1. 打开 `/Game/UI/CharacterMenu/EquipmentMenu/WBP_EquipmentSlot`，Parent保持UmbraEquipmentSlotWidget。
2. 选Image **RarityFrame**，Is Variable=是，Visibility=Collapsed。保留原边框Brush和布局。
3. 在Graph Find References查RarityFrame：取消依据Empty、SlotType、Hovered、Selected或Locked显示它的Set Visibility节点；删除Visibility属性绑定（若有）。移除RarityFrame的显示动画轨道或延迟恢复显示回调。ItemIcon、EmptyIcon映射、LockedOverlay、Preview相关内容不改。
4. 最简方式：BP中完全不写RarityFrame.Visibility，交给C++。若想在视觉事件里显式记录规则，接下列节点，它只使用C++传入的Has Item，不重新推断物品：

```text
Event BP Refresh Visual(State, ItemDisplay, Has Item)
  → Sequence
      Then 0 → Branch(Condition=Has Item)
                  False → RarityFrame.SetVisibility(Collapsed)
                  True  → RarityFrame.SetVisibility(HitTestInvisible)
      Then 1 → HighlightFrame.SetVisibility(Collapsed)
             → Switch on EUmbraEquipmentSlotState(State)
                 Hovered  → HighlightFrame.SetVisibility(HitTestInvisible)
                 Selected → HighlightFrame.SetVisibility(HitTestInvisible)
                 Empty / Equipped / Locked → 不再开启HighlightFrame
```

5. Rarity颜色仍由既有ItemDisplay.RarityColor决定；本阶段不创建物品来触发True。Has Item=False时，无论哪种交互状态都是Collapsed。不要把RarityFrame当作空槽底框；空底框属于slot_background。
6. 若已用单HighlightFrame接好Hovered/Selected，保留原逻辑，只清除与RarityFrame的交叉写入。BP_RefreshVisual内部不调用RefreshVisual/SetHovered/SetSelected/SetItem/ClearItem。
7. Compile、Save。十个EquipmentSlot实例无需逐个改SlotType或重接EmptySlotIcons；基础WBP默认修改适用于它们，仍需检查实例是否覆盖了视觉。

## 7. PIE验收

1. Compile/Save上述WBP。打开Output Log，清理显示后启动L_Prototype → Play → New Editor Window (PIE)。沿用原按键打开CharacterMenu。
2. 打开CharacterMenu后，属性、装备、背包同时出现。属性内部页签正常；中间十槽和全身预览保持原位置与表现。
3. 右侧Inventory直接显示**8列×5行=40个空槽**，CapacityText为**0/40**。SlotBackground可见，所有ItemIcon/RarityFrame/StackCountText/EquippedMarker均隐藏。
4. 不点击，移入A格：Highlight显示，移出：消失。点A后移出：A保持高亮。
5. 悬停B：A仍Selected，B只是Hovered。点B后移出所有格子：仅B仍高亮；点B多次不取消选择，也不新增选中。
6. 关菜单再打开，连续至少5次。点击背包文字分类按钮及属性区域内部页签，确认三栏不会互相切走；分类按钮只呈现Hover/Pressed，不改变内容。仍为40格/0/40，同一菜单实例中的Selected保持；不存在80/120格。
7. 精确计数可用Blueprint Debug Filter选择PIE中的WBP_Inventory实例，观察InventoryGrid子数量；或临时用按键调试链 `InventoryPage.GetGeneratedSlotCount → Print String`，再打印GetSelectedSlotIndex。不要用Event Tick。完成后移除临时打印。只看5行视觉无法排除滚动区尾部重复格。
8. 临时在WBP_Inventory Class Defaults改Capacity=10、Columns=3，重新PIE：3列、4行，最后行1格、0/10；改为0容量：无格、0/0。测试后恢复40/8并Compile/Save。ScrollBox限定较小高度时滚动可用。
9. Equipment页：所有空槽RarityFrame隐藏，悬停/选中/切SlotType（如果调试了）都不出现稀有度框；EmptyIcon、LockedOverlay及交互高亮保留既有规则。本阶段不造物品来测试装备。
10. 在同一菜单内检查原四维/GAS属性显示和FullBody Preview大小/朝向/材质，关闭整个CharacterMenu确认预览停止，重开恢复，确认没有资产接线误改。Output Log没有BindWidget、Accessed None或WBP编译错误。
11. 停止并重启PIE确认新实例选择为空。每次菜单Construct都会幂等初始化；若使用RemoveFromParent后重新AddToViewport，也应保持原格数且点击仍有效。

Session Frontend → Automation运行 `Umbra.UI.Inventory.EmptyGridLifecycle`、`Umbra.UI.Inventory.CharacterMenuComposition`、`Umbra.UI.Equipment.EmptySlotIcons`，并回归 `Umbra.UI.Equipment.SlotContract`、`SlotBindings`、`PageVisibility`、`PreviewLifecycle`。确切本轮结果见[Progress](Progress.md)。这些原生测试覆盖逻辑/生命周期，不替代实际WBP鼠标命中、材质和PIE画面验收。

## 8. 2026-09-30 实际槽位压窄与行距诊断

本轮用关联UE 5.8.2命令行编辑器只读加载已保存WBP，成功0 error/0 warning，未保存资产。报告 `Saved/InventoryLayoutInspection.json`；不包括GUI未保存改动或运行时Graph覆盖。结合截图与引擎 `SUniformGridPanel::OnArrangeChildren/ComputeDesiredSize` 确认以下布局约束：

| 实际位置 | 已保存设置 | 影响 |
| --- | --- | --- |
| WBP_InventorySlot.SizeBox_Root | Width/Height Override均启用，200×200 | 单格期望尺寸是200×200，并非早期指南示例72×72 |
| WBP_Inventory.SizeBox_0（根） | Width=1000、Height=1500 | 8列200宽至少需1600；当前可用宽度不足 |
| InventoryGrid的ScrollBox Slot | Size=Fill 1，Horizontal/Vertical Alignment均Fill | Grid填满可用区域，UniformGrid再均分列宽/行高 |
| InventoryGrid | Slot Padding四边0；Min Desired Slot Width/Height均0 | 当前行间空白不是Padding产生，而是多余行高/居中产生 |
| CharacterMenu右栏ScaleBox_2 | Scale To Fit，Both | 等比缩放整张背包，改变屏幕尺寸但本身不改变宽高比；无需修改中间装备的ScaleBox_1 |

C++只设置格子的Row、Column及居中对齐，没有覆盖Slot宽高或Grid Slot Padding。SizeBox的Override是布局期望值，不保证父容器空间不足时仍得到该尺寸。当前1000宽（扣除实际边距后更少）/8约125，每格高度仍可接近200，所以截图是**横向压窄**。网格纵向Fill后均分多余高度，每格在行内居中，形成额外行间空白。

### 保留你设计的200×200及8列

1. 打开WBP_InventorySlot，保持SizeBox_Root的200×200；Overlay和图片Fill填充这个根，不用改Brush尺寸来补偿父布局。
2. 打开WBP_Inventory，选InventoryGrid → **Slot (Scroll Box Slot)**：Size改Auto；Horizontal Alignment=Left（也可Center），Vertical Alignment=Top；这里Padding先0。
3. 选ScrollBox_Inventory → **Slot (Vertical Box Slot)**：Size仍Fill 1。这样滚动视口占剩余高度，内部Grid按内容高度排列；不要把这两个层级的Size混淆。
4. 选InventoryGrid自己的属性 → Slot Padding。示例：Left=6、Right=6、Top=8、Bottom=8，则相邻列边缘间距12、相邻行边缘间距16个Slate单位。四周也各保留半个间距。Min Desired Slot Width/Height保持0，避免引入第二套格子尺寸。
5. 该示例Grid所需宽为 `8×(200+6+6)=1696`，高为 `5×(200+8+8)=1080`。根SizeBox_0的Width Override必须至少1696再加左右内容Padding和滚动条空间，例如先用1750并核对实际边距；不要继续用1000宽承载8列200格。
6. Height Override可保留1500或按面板设计调整。内容不足时Grid应留在顶部；超过ScrollBox可用高度时滚动。空白留在Grid下面，不再平均分进每行。
7. CharacterMenu的ScaleBox_2当前Scale To Fit会把整张背包等比缩放到右栏，因此200×200指UMG布局单位，不保证屏幕上200像素。若要求视觉同比例，保留等比缩放即可；若要求实际显示尺寸也与设计单位一致，需要取消该层额外缩放（或Stretch=None），并给右栏足够空间，同时考虑全局DPI，不能在窄右栏同时保留8列大格、原尺寸且全部可见。
8. 若右栏设计必须保留1000宽，则应减少Columns（200格示例可用4列），或明确在WBP_InventorySlot中设计更小的格子；这是另一种版式选择。仅修改Grid的Min Desired Slot Width=200、仅把Alignment改Center、或把Brush.ImageSize设200均不能凭空增加父级空间。

统一公式（父级足够宽、Grid按期望尺寸排列、所有格子同尺寸）：

```text
列间距 = Padding.Left + Padding.Right
行间距 = Padding.Top + Padding.Bottom
Grid宽 = 列数 × (Slot宽 + 列间距)
Grid高 = 行数 × (Slot高 + 行间距)
```

这是外部布局间距，不包含图片自身透明边缘或内部Overlay Padding。屏幕上的间距还会乘以ScaleBox比例及DPI比例。

### 验收与证据边界

Compile/Save Slot、Inventory、CharacterMenu后重新PIE，确认格子保持正方形、横纵间距分别可调、改变面板高度不会拉大行距、仍为40格/0/40、Hover和唯一Selected正常。关闭重开不追加格子。需要精确尺寸时用Widget Reflector查看局部布局尺寸与累计缩放，而不是比较Designer缩放后的屏幕像素。

本次仅做资产只读检查、引擎布局源码核对和文档修正；未修改C++、未编辑二进制资产、未运行修正布局后的PIE，不将前轮构建或自动化称为本次布局验证。
