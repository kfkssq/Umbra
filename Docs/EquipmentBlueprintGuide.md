# 装备页 Blueprint 逐步操作指南

2026-09-29 增量步骤见 [四维与空槽接线](CharacterMenuStatsAndIcons.md)：新增独立 EmptyIcon Image、在 WBP_EquipmentSlot Class Defaults 配置 Empty Slot Icons；slot_background 仍是通用底板。十槽实例配置已在已保存资产中核对正确。四维模板与 WBP_PrimaryAttribute 的接线也以该新文档为准，无需重做全身 Preview 或高亮。

## 工作分工

用户负责编辑、编译、保存 Blueprint / Material / RenderTarget / Animation 资产，并决定最终视觉。Codex 负责 C++ 数据、状态、委托、来源同步和资源生命周期，以及准确的接线说明。除非用户另外明确要求，Codex 不代做 Designer、不改蓝图 Graph、不调整或保存美术资产。

本指南的布局、颜色、图片和数值均为可选起点，不是必须接受的设计稿。资产读取证据与代码说明见 [EquipmentMenu](EquipmentMenu.md)，当前验证状态见 [Progress](Progress.md)。

## 第 0 步：让编辑器加载新增 C++ 类型

1. 保存你当前正在编辑的资产，然后关闭 Unreal Editor。保留你自己的未提交文件，不需要回退。
2. Rider 选择 `UmbraEditor / Development / Win64`，Build 当前原项目。上轮通过的是 Saved 下的隔离验证副本，不会自动替换原项目 DLL。
3. 打开原项目 `Umbra.uproject`。
4. 后面 Reparent 搜索类名时，一般不显示 C++ 的 U 前缀：例如搜 `UmbraEquipmentMenu`。
5. 如果找不到新类，先检查原项目构建结果和当前打开的项目路径，不要用反复 Live Coding 代替这次完整加载。

接线顺序：**EquipmentSlot → Equipment → 预览子 BP/材质配置 → CharacterMenu → PIE**。

## 第 1 步：接入已有 WBP_EquipmentSlot

### 1.1 父类与现有控件

1. 打开 `Content/UI/CharacterMenu/EquipmentMenu/WBP_EquipmentSlot`。
2. 在 Blueprint 编辑器使用 File → Reparent Blueprint，选择 `UmbraEquipmentSlotWidget`。也可在当前版本可用的 Class Settings 父类入口完成。
3. Compile。若同名成员冲突，核对是否手工新增了与 C++ 同名的普通变量；不要另外创建 SlotType、CurrentItem、bLocked、bSelected、bHovered。
4. Designer 中保留你现有的 `ItemIcon` 和 `RarityFrame`，两者应为 **Image**，勾选 **Is Variable**。保留 `slot_background` 作为空槽底图。
5. 若需要锁定图案，自己添加 Image，命名 `LockedOverlay`，勾选 Is Variable；选你自己的锁图案，初始 Visibility=Collapsed。不需要时可以不添加。
6. Compile、Save。

`ItemIcon`、`RarityFrame`、`LockedOverlay` 是可选绑定；名字匹配时 C++ 自动找到它们。它们的尺寸、位置、边距、Brush资源、圆角或描边仍由你配置。

### 1.2 明确当前 C++ 会写哪些显示值

| 控件 | 每次 RefreshVisual 的基础处理 | 你编辑的部分 |
| --- | --- | --- |
| ItemIcon | Brush 设为传入的 ItemDisplay.Icon；ColorAndOpacity 恢复白色/alpha=1；有 Item 时显示，无 Item 时 Collapsed | ItemDisplay 中采用的图标、Brush参数；Designer 尺寸/布局；事件末尾可加你自己的 tint/动画 |
| RarityFrame | 有 Item 时 tint=ItemDisplay.RarityColor，空槽时 tint=白色 | Designer 的边框资源与布局、外部传入的稀有度颜色；事件里可覆盖空槽/状态样式 |
| LockedOverlay | bLocked 时显示，否则 Collapsed | 图案、位置、尺寸、透明度 |
| slot_background | 不写它 | 全部由你配置 |

**不要只改 RarityFrame 的 Designer tint 就期待它保留到运行时**，它是数据驱动色；要定制状态色，在下面的 BP_RefreshVisual 中设置。C++ 基础处理先执行，Blueprint 事件后执行，所以你在事件里定义的最终表现生效。

### 1.3 自己制作 Hover 与 Selected 外观

本节更新为**单个HighlightFrame**，Hovered/Selected逻辑分开、视觉共用。先按 [UI交互高亮材质步骤](UIInteractionHighlight.md) 在Material Editor创建 `M_UI_InteractionHighlight`；参考已读取的敌人描边橙红色，使用独立UI材质，不修改/直接复用Post Process材质。

1. Designer 中只保留一个 Image `HighlightFrame`，替代之前的HoverFrame/SelectedFrame，位于ItemIcon/RarityFrame上方、LockedOverlay下方。
2. 勾选 Is Variable，Brush使用新UI材质或它的MI，Color and Opacity=白色/Alpha1；尺寸和布局由你决定。初始Visibility=Collapsed。
3. 高亮开启时使用 **Not Hit-Testable (Self & All Children)**，不要让它挡住槽位。
4. Graph 中通过 Overrides 或右键搜索，添加 **BP Refresh Visual** 事件（C++名 `BP_RefreshVisual`）。它提供 `State`、`ItemDisplay`、`bHasItem`。
5. 接线顺序如下：

```text
Event BP Refresh Visual
    → HighlightFrame.SetVisibility(Collapsed)
    → Switch on EUmbraEquipmentSlotState(State)
        Empty     → 保持高亮关闭
        Equipped  → 保持高亮关闭
        Hovered   → HighlightFrame.SetVisibility(Not Hit-Testable)
        Selected  → HighlightFrame.SetVisibility(Not Hit-Testable)
        Locked    → 高亮关闭，保留原锁定样式（LockedOverlay 已由 C++ 控制）
```

先关闭再应用状态，避免旧高亮残留。不要合并C++的Hovered/Selected枚举，也不修改稀有度框。可选0.12秒淡入的节点顺序见上述材质指南；仅在高亮从关变开时启动，Hovered→Selected不重播。

`bHasItem` 表示是否有装备，与高亮状态分开。Selected/Locked 也可能是空槽，不要把 State=Selected 当作有物品的证据。要读取图标/颜色/名称，从 ItemDisplay 拖出 **Break Umbra Equipment Item Display**。

**BP Refresh Visual 内不要调用 RefreshVisual、SetHovered、SetSelected、SetLocked、SetItem 或 ClearItem**：这些函数都会再次触发本事件，可能递归。这里仅更新控件/播放你定义的视觉动画。

### 1.4 鼠标与选中

当前模板没有内部 Button：保持槽位根交互区域可命中，由 C++ 接收鼠标进入/离开和左键。

- 槽位 UserWidget 实例/必要根交互区域用 Visible。
- 纯装饰图片设 Not Hit-Testable。
- 带交互孩子的父容器，可以 Self Only；不要把整条槽位父链设成 Self & All Children，否则鼠标事件被排除。

如果你决定添加内部 Button，由该 Button 接收输入，接：

```text
Button.OnClicked   → RequestSelection(Target=Self)
Button.OnHovered   → SetHovered(Target=Self, true)
Button.OnUnhovered → SetHovered(Target=Self, false)
```

不要在 OnClicked 另写“选中自己并清空其它九格”。页级 C++ 统一处理互斥选择；Locked 会阻止请求。Compile、Save。

## 第 2 步：编辑 WBP_Equipment 的布局和十个实例

1. 打开现有 `WBP_Equipment`，Reparent 为 `UmbraEquipmentMenu`，Compile。
2. 保留自己的背景和外框。自己在 Designer 排列左区、中央预览区、右区；可以用现有 Panel、HorizontalBox、Overlay、SizeBox，不需要照抄固定布局。
3. 将同一个 WBP_EquipmentSlot 放置十次，选中 **Equipment Designer 中的每个实例**，在 Details 搜索 `Slot Type`，按表配置。不是在 WBP_EquipmentSlot 的 Class Defaults 中反复改默认值。

| 建议实例名 | 实例 SlotType |
| --- | --- |
| HeadSlot | Head |
| ChestSlot | Chest |
| HandsSlot | Hands |
| LegsSlot | Legs |
| FeetSlot | Feet |
| AmuletSlot | Amulet |
| Ring1Slot | Ring1 |
| Ring2Slot | Ring2 |
| MainHandSlot | MainHand |
| OffHandSlot | OffHand |

4. 这些名字供你阅读，C++ 不按名字映射。每个枚举应恰好一次；不要把 Ring1/Ring2 都配置为同一个值。
5. 十个实例可以在本页普通 Panel 里多层嵌套；暂时不要把它们再包进独立的左右区 UserWidget，当前搜集范围是 WBP_Equipment 自己的 WidgetTree。
6. 外部访问某格时，从 Equipment 引用拉出 `GetEquipmentSlot`，选枚举，然后用 Is Valid 检查返回值。无需自己维护十个同名变量/数组。
7. Compile、Save。运行时日志若显示 duplicate SlotType 或少于10种，回到实例 Details 检查。

## 第 3 步：添加你自己的全身预览显示区域

1. 在中央区域添加 **Image**，命名 **CharacterPreview**，勾选 Is Variable。注意 Image 本身叫这个名字，不是外面的容器。
2. 由你配置大小、位置和边距。输出默认为1:2竖向比例，可先用 SizeBox 表达这个比例，再用 ScaleBox 按比例适应中央区域；不要直接拉成明显不同比例。
3. Image 的 Color and Opacity 先设白色、alpha=1，Render Opacity=1；否则会给预览额外染色/隐藏。
4. Image 设 Not Hit-Testable。暗底、边框、装饰、未来脚下底座都由你在这一页布局。
5. 将 Image 的 Brush.Image 设置为你新建的 `M_UI_CharacterFullBody` 或专用 MI。材质默认读取新的全身 RT，Designer 可以显示最近捕获的画面；运行时 C++ 以这个材质创建 MID，释放时恢复源材质。详细节点见 [Capture 蓝图编辑流程](BlueprintCapturePreview.md)。

## 第 4 步：通过继承复用头像展示 Actor

1. 找到 `Content/UI/HeroPortrait/BP_HerorPortraitCapture`（保留仓库现有拼写）。
2. 如果尚未创建，右键 → Create Child Blueprint Class，命名 `BP_CharacterFullBodyPreview`；已经创建则继续编辑现有子 BP。
3. 打开子 BP，确认继承得到 `PortraitMesh`、`PortraitCapture`、`KeyLight`、`FillLight`，不要再添加第二个主体 SkeletalMesh 或 SceneCapture2D。
4. 原头像 BP 和 HUD 的 RT 不动。全身使用子 BP，可以由你独立改灯光而不影响原头像。
5. 按你希望的效果调整子 BP 里继承灯的相对位置、强度、衰减范围等；不能编辑某个继承属性时，先检查父 BP 的组件是否允许继承编辑，不要直接重建整套组件。
6. Compile、Save。不需要把这个子 BP 拖进游戏地图，运行时由 Equipment 创建临时实例。

当前 C++ 会把玩家的主体 Mesh/Materials 同步到展示 Mesh，所以子 BP 的 Mesh 资源是编辑器预览/初始化来源，不是用于强行显示另一种角色的开关。完整未来武器/防具组件同步仅预留接口。

**默认配置已更新：镜头、FOV、人物相对变换、Idle、灯光和 TextureTarget 全部在子 BP 组件中编辑。** 保持 Use Blueprint Configuration=true，运行时不再用 WBP 的旧镜头参数覆盖它们。

## 第 5 步：新建专用 RT / 材质，并连接现有 Capture

完整节点与编辑器预览操作见 [Capture 蓝图编辑流程](BlueprintCapturePreview.md)，本节给出配置顺序：

1. 你新建 `RT_CharacterFullBody`：512×1024、RGBA16f、Clear Color=(0,0,0,1)。
2. 在子 BP 的 `PortraitCapture.TextureTarget` 指定这个新 RT；镜头 Transform/FOV、Mesh Transform/Idle 都在子 BP 组件中设置。
3. 你新建 `M_UI_CharacterFullBody`：UI / Translucent，纹理参数 `PortraitTexture` 默认新 RT；RGB×Brightness → Final Color，OneMinus(A) → Opacity。
4. 将新材质/MI填入 Equipment Designer 的 `CharacterPreview.Brush.Image`。
5. 切换到 WBP_Equipment 的 Graph，点击 Class Defaults。在 Details 搜索 `Preview`（不要把“Equipment / Preview”当作完整搜索词）。若没有下面属性，先确认父类为 UmbraEquipmentMenu、原项目已构建并重启编辑器。

| 属性 | 设置 |
| --- | --- |
| Preview Actor Class | 现有 BP_CharacterFullBodyPreview |
| Preview Material | 留空以读取 Designer Brush；若保留显式赋值，必须改为新全身材质/MI |
| Texture Parameter Name | PortraitTexture |
| Preview Settings → Use Blueprint Configuration | true；旧镜头/人物/动画/RT模板字段隐藏且不参与覆盖 |
| Preview Settings → Capture Rate | 默认30 Hz |
| Preview Actor Transform | 默认世界位置(0,0,-100000)cm、Scale=1；不覆盖组件的局部构图 |

编译、保存。新 RT 是实际捕获输出，不再作为运行时复制模板。编辑器中先让 Capture 写入一帧，RT/材质/Designer 才有画面。不要把新 RT 分配给 HUD Capture，也不要在游戏地图额外放置第二个使用它的全身 Capture。

Idle 仍由 C++ 循环并忽略向 Actor 应用的 RootMotion；选择骨架兼容的原地 Idle 或专用展示 AnimBP。最终构图和视觉由你在上述资产中编辑。

## 第 6 步：把装备页接入你现有的 CharacterMenu

1. 打开 WBP_CharacterMenu，Reparent 为 `UmbraCharacterMenu`，Compile。
2. 找到负责“属性/装备”等大页面切换的 WidgetSwitcher。不要误把 WBP_AttributeMenu 内部已有的三个属性子Tab Switcher当成同一层。
3. 若你的编辑器里已有该大页面 Switcher，直接把 WBP_Equipment 放入对应页面；若尚未创建，你自己在Designer创建一个大页面Switcher，将原属性内容和装备页分别放入，保持原内容的布局参数。
4. 选中装备页实例，勾选Is Variable，给它一个易读名称，例如EquipmentPage。Switcher也勾选Is Variable。
5. 已有按钮Graph若已能正确切页，保留它。需要新增接线时，以直接引用方式为例：

```text
装备按钮 OnClicked
    → PageSwitcher.SetActiveWidget(Widget = EquipmentPage)

属性按钮 OnClicked
    → PageSwitcher.SetActiveWidget(Widget = 原属性页实例)
```

也可以沿用SetActiveWidgetIndex，但Index必须按你自己的Designer顺序填写，文档不替你固定数字。

6. 自己配置大页面Tab按钮的Normal/Hovered/Selected视觉；新增菜单父类只负责预览生命周期，不自动绘制这些按钮。
7. 需要默认属性页时，在你现有初始化逻辑中指定该页。无需在蓝图Tick持续SetActiveWidget。
8. 在BP_UmbraPlayerController的Class Defaults确认CharacterMenuClass仍是WBP_CharacterMenu。保留既有ToggleCharacterMenuAction/IMC配置，不另写CreateWidget/AddToViewport流程。
9. Compile、Save。

正常Controller路径已处理SetMenuOpen；正常Switcher切页已处理预览启停。无需在Blueprint另外创建展示Actor、设置Capture定时器、每帧Capture、生成MID或启动/停止Mesh Tick。

如果你另建测试入口绕过Controller，则该入口在显示菜单时调用SetMenuOpen(true)、关闭前调用SetMenuOpen(false)。这只针对自定义入口，正式入口无需重复调用。

## 第 7 步：只测 UI 数据，不制作装备 Gameplay 系统

需要观察Equipped状态时，可在你自己的测试Graph里临时注入显示快照：

1. 准备一个你已有的具体资产引用作临时Item标识，例如测试图标Texture2D；这里Item只是非空UObject标识，不读取其Gameplay字段。不要Construct抽象UObject/UDataAsset基类。
2. 从图标Texture2D生成Slate Brush，使用Make Umbra Equipment Item Display，填Item、DisplayName、Icon、RarityColor。
3. 对Equipment调用SetSlotItem(Type=MainHand, Item=上述结构)。返回false时，检查该类型是否已登记且无重复。
4. 如把临时注入放在WBP_Equipment自身Event Construct：**先调用RebuildSlotBindings，再调用SetSlotItem**。Blueprint Construct可能早于原生Construct中后续的自动登记；显式先登记可避免测试数据过早写入。
5. 从GetEquipmentSlot(MainHand)的有效返回引用调用ClearItem、SetLocked(true/false)，分别观察空槽和锁定。点击其它格测试互斥选中。
6. 测完由你移除临时测试Graph。未来正式EquipmentComponent事件调用相同显示接口即可。

不要把测试显示结构当成装备系统；不在Widget中计算伤害、Scaling、需求或施加GE。

## 第 8 步：你来完成最后的构图与验收

1. 先让子 BP 向新 RT 捕获一帧，再查看材质和 Designer Image。正式验证时 PIE，使用原菜单快捷键打开装备页。编辑器预览方法见 [Capture 蓝图编辑流程](BlueprintCapturePreview.md)。
2. 全身不完整：检查Image比例，再在子 BP 调 PortraitCapture 的距离/高度、PortraitMesh 的位置/Scale。先保留约30°FOV，避免用极端广角硬塞全身。
3. 人物朝向不对：调子 BP 的 PortraitMesh 相对 Rotation.Yaw；不改真实玩家Rotation。
4. 镜头方向不对：调子 BP 的 PortraitCapture 相对 Rotation，确认镜头朝向模型；位置和旋转必须成对检查。
5. 光太亮/暗：调子BP灯光，或新材质/专用MI中的 Brightness；不要改HUD共用实例。
6. 留出头顶、武器外沿、脚部及脚下少量空间；边框、底色、装饰都由你编辑。
7. 每次调WBP Class Defaults/子BP资产后，Compile、Save，停止并重新PIE以读取新的配置。当前没有运行时热应用所有构图参数的接口，不要只改已缓存页面的默认值就判断参数无效。
8. 保持HUD头像同时显示，反复开关装备页，确认两者构图互不影响。
9. 切回属性、关闭菜单时，用Blueprint调试的GetPreviewComponent → IsPreviewActive检查false；首次未创建时组件可能为空，先Is Valid。重新进入为true。
10. 清理测试Graph后再保存最终资产。最终看到的布局、边框、灯光、镜头和动画由你的这些编辑决定。

## 常见问题定位

| 现象 | 先检查 |
| --- | --- |
| 找不到新父类 | 原项目是否完整构建、Editor是否重启、是否打开正确uproject |
| 缺失控件/同名属性错误 | 父类是否正确、Image名称/类型/Is Variable、是否重复创建了C++成员同名变量 |
| 所有槽显示为同一部位 | Equipment Designer的每个实例SlotType，不能只改模板Class Defaults |
| 悬停/点击无反应 | UserWidget及父容器命中设置、是否被装饰层覆盖、内部Button是否连接RequestSelection |
| 状态边框残留 | BP Refresh Visual是否先复位之前改变的所有视觉属性 |
| 节点无限递归 | BP Refresh Visual内是否又调用了RefreshVisual或状态/Item Setter |
| 有人物但仍是头像近景 | Use Blueprint Configuration是否为true、子BP的PortraitCapture是否已调整/保存，PreviewActorClass是否选中这个子BP |
| 预览完全空白 | CharacterPreview准确名称、PreviewActorClass/Material、OwningPlayer Pawn有效、Image alpha、镜头朝向、Output Log |
| 显示成矩形底色或反透明 | 材质Opacity、RT Alpha路径、PreviewMaterial是否为正确UI材质 |
| 改Designer图标/稀有度色运行时又变了 | 它们是数据驱动字段，按1.2表和BP事件末尾定制 |
| RT或Designer没有人物 | 新RT是否确实指定给子BP Capture和材质；ShowOnly是否包含PortraitMesh；是否已实际捕获一帧；默认模式直接写此RT，仅旧兼容模式使用transient副本 |
| 关闭后还捕获 | CharacterMenu正确父类、正式Controller入口、观察的是否是全身实例而非正常工作的HUD实例 |

本指南只提供操作，不表示这些资产已经接线或真实PIE已通过。
