# 装备页基础与全身预览

2026-09-29：已保存的 WBP_Equipment 已有十槽且 SlotType 正确，不再是下文历史检查中的单槽。新空槽图标映射和四维属性迁移见 [CharacterMenu 四维与空槽](CharacterMenuStatsAndIcons.md)。该文的本轮资产证据优先于下文 09-28 快照；Preview 架构保持不变。

需要逐步操作时从 [Blueprint完整接线指南](EquipmentBlueprintGuide.md) 开始。用户负责最终视觉和全部资产编辑/保存；Codex负责逻辑及接线说明。本文保留资产证据、参数来源与架构边界。

## 边界与资产证据（2026-09-28）

只实现装备 UI、十类通用槽位、全身展示输出，没有 Inventory、装备属性/GAS 计算、掉落、词缀、拖拽、需求判定或完整 Tooltip。`FUmbraEquipmentItemDisplay` 是外部提供的显示快照，Item 是不透明 UObject 引用，不是正式物品定义。

本次用项目关联的 **UE 5.8.2** 命令行编辑器只读加载已保存资产，报告在本地 `Saved/EquipmentPreviewInspection.json`。没有修改二进制资产，没有读取 GUI 编辑器中的未保存状态。

| 已有资产/组件 | 读取结果 |
| --- | --- |
| `/Game/UI/HeroPortrait/BP_HerorPortraitCapture` | 已有独立 Actor BP；Source 没有头像专用 C++ 类 |
| PortraitMesh | `/Game/ParagonGreystone/Characters/Heroes/Greystone/Meshes/Greystone`；局部 yaw=-90°；Single Node 模式 |
| Animation | `/Game/ParagonGreystone/Characters/Heroes/Greystone/Animations/Idle`，循环、播放率1，无 AnimClass |
| PortraitCapture | SceneCapture2D；位置(80,-20,200) cm、pitch=-11.5°/yaw=162.5°、FOV=30°；SceneColor HDR、ShowOnly；EveryFrame=true、OnMovement=false |
| KeyLight / FillLight | 两个 Movable PointLight，位置(63,-42,193)/(64,54,226) cm，Intensity=3000/6000；强度单位沿用 BP，不根据数值推断 |
| RT_HeroPortrait | 256×256，RGBA16f，资产引用输出 |
| M_UI_HeroPortrait | UI Domain、Translucent；PortraitTexture 纹理参数指向头像 RT，存在 brightness 标量、Multiply、OneMinus 节点 |
| WBP_HeroPortrait | 引用上述 UI Material |
| WBP_EquipmentSlot | 已保存树有 ItemIcon、RarityFrame、slot_background；原 Icon tint alpha=0 |
| WBP_Equipment | 已保存树只有一个 WBP_EquipmentSlot，尚无 CharacterPreview Image/完整十槽布局 |
| WBP_CharacterMenu | 已保存树引用 AttributeMenu，未引用 Equipment；GUI 中未保存的 Switcher 改动不在检查结果中 |

Blueprint Graph 完整连线、头像实例在地图中的创建方式、OneMinus 到 Opacity 的实际连接、Idle Root Lock、最终画面：**待编辑器确认**。Paragon/UI 包继续保留原来源与许可，见 [EditorSetup](EditorSetup.md)。

## 复用与职责

全身页用现有 `BP_HerorPortraitCapture` 或其子 BP 创建本地临时实例，`UUmbraCharacterPreviewComponent` 直接接管已有 Mesh、Capture、灯光，不创建第二套 Actor 基类/组件组合。HUD 原实例、镜头、RT 不变。复用父 BP 的组件结构和 Mesh/Idle 资产；全身子 BP 使用用户新建的专用 RT / UI 材质，直接编辑组件构图。独立动画实例让装备 Idle 不受以后 HUD 战斗状态影响。

OwningPlayer Pawn → 只读 Mesh/Materials → 已有 BP 的展示实例 → 原 SceneCapture2D → 子 BP 指定的专用 RT → 新全身 UI 材质 MID → CharacterPreview。不修改玩家位置、朝向、动画、光照、GAS。源 Mesh 必须与展示 Idle/AnimBP 骨架兼容，本阶段不自动重定向。

- `UmbraEquipmentSlotWidget`：显示快照与五种状态；优先级 Locked > Selected > Hovered > Equipped/Empty，HasItem 独立保留。
- `UmbraEquipmentMenu`：枚举映射、互斥选择、预览创建/释放、Pawn 更换重建。
- `UmbraCharacterMenu`：监听嵌套树的 Visibility / WidgetSwitcher ActiveWidgetIndex FieldNotify，计算装备页是否真正可见。
- `UmbraCharacterPreviewComponent`：复用组件、复制外观、独立输出、可见时30 Hz捕获；无装备 Tick 轮询。
- Controller：在原 ToggleCharacterMenu 中通知 SetMenuOpen；不改变布局及输入规则。

## 编辑器接线

先保存并关闭当前编辑器，构建原项目 `UmbraEditor Win64 Development`，重开加载新增反射类型；不依赖 Live Coding 添加 UCLASS/USTRUCT。模块/Target 未改变。

### WBP_EquipmentSlot

1. Reparent 为 `UmbraEquipmentSlotWidget`。保留布局，Image `ItemIcon`、`RarityFrame` 勾选 Is Variable；可选添加 `LockedOverlay` Image，初始 Collapsed。
2. C++ 设置 Icon Brush、恢复 tint alpha=1、空槽隐藏 Icon、更新 RarityColor，保留 slot_background 的 Designer Brush。通过 `BP_RefreshVisual(State, ItemDisplay, bHasItem)` 实现 Hover/Selected/Locked 的视觉，不用 Tick。

   高亮方案已更新为单个 `HighlightFrame`：使用独立 `M_UI_InteractionHighlight`，在BP事件开始Collapsed，仅Hovered/Selected开启，Locked仍使用原锁定层。UI材质节点、敌人描边颜色参考和可选淡入见 [UIInteractionHighlight](UIInteractionHighlight.md)。旧双框方案不再采用，C++两种逻辑状态仍分开。
3. 根交互层保持可命中，装饰 Image 设 Not Hit-Testable。原模板没有内部 Button，原生鼠标进出/左键已处理；若增加内部 Button，OnClicked → RequestSelection，OnHovered/Unhovered → SetHovered。
4. SetItem 的 Item 必须非空，临时 UI 测试可用专用 DataAsset/UObject 标识并传 Icon/RarityColor。ClearItem 或空 Item 清空显示。Locked 禁止选择，但允许外部更新显示；不代表 Gameplay 装备需求判断。

### WBP_Equipment

Reparent 为 `UmbraEquipmentMenu`。完成 LeftEquipmentArea / CharacterPreviewArea / RightEquipmentArea，全部使用同一个 WBP_EquipmentSlot。在每个 Designer **实例**的 Details → Equipment → Slot Type 设置：

| 建议实例名（只用于可读性） | Slot Type |
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

实例可嵌套在普通 Panel/SizeBox 中，但应属于 Equipment 的 WidgetTree，不要另封装到子 UserWidget 中。Construct 按 C++ 类型搜集、按 SlotType 建表，不按名称字符串判断位置。`GetEquipmentSlot(Type)` / `SetSlotItem(Type, Display)` 是更新入口，无须手工绑定十个成员。重复类型警告并禁用歧义 key，不静默覆盖；SetSlotType 会事件重建，运行时增删槽后调用 RebuildSlotBindings。

中央添加 Image **CharacterPreview**，勾选 Is Variable，约1:2显示比例，ScaleBox 保持比例避免拉伸。

### FullBody 配置

当前默认流程见 [直接编辑 Capture 蓝图](BlueprintCapturePreview.md)。先让原项目加载最新 C++，再在 WBP_Equipment 的 Graph → Class Defaults 搜索 Preview。

| 参数/来源 | 配置与优先级 |
| --- | --- |
| PreviewActorClass | 现有 BP_CharacterFullBodyPreview；继承头像 BP，要求 Actor（不是 Pawn），初始化时恰好一个主体 SkeletalMesh 和一个 SceneCapture2D |
| PreviewSettings.bUseBlueprintConfiguration | 默认 true；以下镜头/动画/输出配置读取子 BP 组件，不再由旧 WBP 参数覆盖 |
| PortraitCapture | 子 BP 组件的相对 Transform、FOV、CaptureSource、ShowFlags、TextureTarget |
| PortraitMesh | 子 BP 组件的相对 Transform、Idle 或专用展示 AnimClass；运行时主体 Mesh/Materials 仍同步真实玩家 |
| RT_CharacterFullBody | 用户新建专用资产，建议512×1024 RGBA16f、ClearColor=(0,0,0,1)，直接填入 Capture.TextureTarget。运行时直接写此资产，不复制 |
| PreviewMaterial | 可选显式覆盖，优先于 CharacterPreview 的 Designer Brush 材质；推荐留空，在 Designer 填新建 M_UI_CharacterFullBody / MI |
| TextureParameterName | PortraitTexture；新材质默认纹理同样指向全身 RT，运行时 MID 使用相同 RT |
| PreviewActorTransform | 世界位置默认(0,0,-100000)cm；不改变组件局部构图 |
| CaptureRate | 默认30 Hz，限制1–60；有效角色且装备页激活时才工作 |
| 灯光 | 子 BP 两盏灯的位置/强度/衰减/可见性；关闭页时暂时关闭，重开恢复原可见性 |

用户在 Capture 选择 SceneColor HDR / inverse opacity，并按需关闭 Fog/Atmosphere。UI材质 RGB×Brightness → Final Color、1-Alpha → Opacity。运行时 C++ 仍重建 ShowOnly，只显示本展示 Actor 的 primitive，禁用展示碰撞/阴影并开启仅捕获可见；这些运行时操作不修改资产默认值。

专用资产 RT 只允许一个游戏 Capture 使用；检测到其它游戏世界的已注册 Capture 使用同 RT 时拒绝初始化并提示。HUD 与全身需要不同 RT。只有显式关闭 Use Blueprint Configuration 时才恢复旧 WBP 镜头/人物/动画覆盖、模板复制或新建 transient RT 模式。

Idle 循环，AnimInstance 使用 IgnoreRootMotion，不向 Actor 应用根运动，不改玩家 AnimBP。如果动画含未提取的根骨骼位移，选原地 Idle 或展示专用动画处理 Root Lock。最终全身、武器、脚下留白需 PIE 检查。

### CharacterMenu 与启停

2026-09-30用户确认：WBP_CharacterMenu父类保持 `UmbraCharacterMenu`，保留Controller的CharacterMenuClass；Attribute、Equipment、Inventory是HorizontalBox中同时显示的三栏，不建立三页Switcher。AttributeMenu内部页签和父类不变；未来SkillTreeMenu与整个CharacterMenu在外层并列。完整接入见 [Inventory指南](InventoryPhase1.md)。

打开 → Controller SetMenuOpen(true) → 激活分支中的装备页启动/恢复预览。切页/祖先 Hidden 或 Collapsed → FieldNotify → SetPageActive(false) → 清 Capture Timer、关闭 Mesh Tick/灯；EveryFrame、OnMovement 始终关闭。关闭菜单走同一路径，NativeDestruct 解除委托并销毁实例，换 Pawn 重建来源。HUD 原实例不参与。

绕过 Controller 展示菜单时自行调用 SetMenuOpen。运行时增删页面后调用 RefreshEquipmentPages。不要只监听 Equipment 自己的 Visibility，Switcher 非激活/祖先隐藏不一定改变它。

### 外观扩展边界

未来 EquipmentComponent 变化事件更新 UI 后调用页的 RefreshEquipmentVisuals；皮肤变化调用 RefreshAppearance。可将 UmbraCharacterPreviewComponent 的 Blueprint 子类加入 Preview Actor 子 BP，在 `BP_RefreshEquipmentVisuals(SourceCharacter, PreviewMesh)` 更新本展示 Actor 所属武器/模块化防具组件，按挂点或 Leader Pose 同步。事件后 C++ 重建 ShowOnly。禁止在这个表现事件计算属性、装备玩家或施加 GE。本阶段不自动复制玩家附属武器/防具组件。

## 验证与文件

新增：`UI/Equipment/UmbraEquipmentTypes.h`、`UmbraEquipmentSlotWidget.h/.cpp`、`UmbraEquipmentMenu.h/.cpp`；`UI/Preview/UmbraCharacterPreviewComponent.h/.cpp`；`UI/UmbraCharacterMenu.h/.cpp`；`Tests/UmbraEquipmentTests.cpp`。修改 Controller 的打开/关闭通知，其他玩法规则不变。

自动化名称：`Umbra.UI.Equipment.SlotContract`、`SlotBindings`、`PreviewLifecycle`、`PageVisibility`，覆盖状态、十槽映射/重复/改类型、蓝图构图/专用RT保留、RT缺失/冲突拒绝、旧隔离RT/捕获生命周期/玩家隔离、Switcher/菜单开关/隐藏。实际结果见 [Progress](Progress.md)。NullRHI 测试不证明画面正确。

最小 PIE 验收：

1. 编译/保存 Slot、Equipment、CharacterMenu，无 BindWidget/父类错误；十类型唯一，测试空槽/SetItem/Clear/悬停/选中/锁定。
2. 头、武器、腿、脚完整，人物居中，脚下留白；透明背景、Idle 稳定，真实玩家不受影响。
3. HUD 头像同时正常；开关装备不改变头像 RT/构图；全身 TextureTarget 等于你新建的专用 RT，且与 HUD RT 不同。
4. 切回属性/关闭/隐藏祖先时 IsPreviewActive=false、无持续 Capture；再打开恢复。RemoveFromParent 后无残留展示 Actor。
5. 换/移除 Pawn 不显示旧角色，无无效引用；多个同时显示的本地预览须分配不同 RT，或显式使用旧隔离模式；共用资产 RT 应报告冲突。

在完成资产接线与真实 PIE 前，只标记 **C++ 基础已实现，资产接入与画面待编辑器确认**。
