# CharacterMenu 四维与空槽图标接线（2026-09-29）

本轮只扩展既有 UI 类；不改变 Preview、装备高亮、攻击/伤害或页面切换。用户在 UE Editor 中完成下列资产接线。代码验证结果见 [Progress](Progress.md)，以前文档的历史结果不代表本次验证。

本轮 UE 5.8.2 Development 验证副本构建成功；三个槽位测试通过，四维 PrimaryRows 修正测试初始化后单独重跑通过（1条既有 GameplayCueNotifyPaths 配置警告）。原项目 DLL 没有替换，下面 Designer 接线与实际 PIE 尚未执行。

## 已保存资产核对

用关联的 UE 5.8.2 命令行编辑器只读加载，报告在本地 `Saved/CharacterMenuStatsInspection.json`。没有保存资产，也没有读取当前 GUI 的未保存改动。检查脚本扩展到整个属性页时曾把 StaticHeroPortrait 非蓝图资产传入 LoadBlueprintClass，产生一条加载类型错误；下表针对成功加载的 Widget/CDO 和 WidgetTree。Graph 的完整连线仍待编辑器确认。

| 资产 | 检查结果 |
| --- | --- |
| WBP_EquipmentSlot | 原生父类 UmbraEquipmentSlotWidget；有 ItemIcon、RarityFrame、HighlightFrame、LockedOverlay、slot_background；没有 EmptyIcon |
| WBP_Equipment | 原生父类 UmbraEquipmentMenu；十个实例已各自配置正确的 SlotType，另有 CharacterPreview |
| WBP_CharacterMenu | 保存版本的原生父类仍是 UserWidget；这会缺少预览激活通知，需按第0节接回已有 UmbraCharacterMenu |
| WBP_PrimaryAttribute | 原生父类 UserWidget，包含四个下列独立 WBP 的实例，是本次四维数据父容器 |
| WBP_StatEntry_Strength / Dexterity / Intelligence / Faith | 原生父类均为 UserWidget；各有 staticon、TextBlock_115、statValue；未发现通用 WBP_StatEntry 资产 |

复用已有 `UUmbraStatEntry`、`UUmbraCharacterStatsPanel`、`EUmbraCharacterStat`、`EUmbraEquipmentSlot`、`FUmbraEquipmentItemDisplay` 和 `UUmbraAttributeSet` 的 Strength / Dexterity / Intelligence / Faith。没有新建属性、槽位枚举、ASC 或每种属性的 C++ Widget 类。测试用 Transient 类仅用于自动化。

## 0. 加载 C++

**预览启停前置条件**：WBP_CharacterMenu 的 Parent Class 必须是 `UmbraCharacterMenu`，WBP_Equipment 必须是 `UmbraEquipmentMenu`。2026-09-29 17:32 的只读复查发现，已保存 WBP_CharacterMenu 仍继承普通 UserWidget，Controller 的 Cast 因而失败，SetMenuOpen → SetPageActive → 启动预览的 C++ 链路不会执行。在 WBP_CharacterMenu 的 File → Reparent Blueprint 选择 UmbraCharacterMenu，再 Compile/Save、重新开始 PIE。这是接回已有父类功能，不重做页面布局或切换 Graph。下文上层菜单应保持各自菜单父类，不能误改为四维数据面板类。

保存当前 Editor 的工作并关闭 Editor，在 Rider 构建原项目 `UmbraEditor / Win64 / Development` 后重新打开。此次新增反射属性和枚举值需要完整加载，不依赖 Live Coding。模块与 Target 未改变，无须因本次修改重新配置模块。Saved 内的验证副本构建不会更新原项目 DLL。

## 1. WBP_EquipmentSlot：仍只有一个模板

打开 `/Game/UI/CharacterMenu/EquipmentMenu/WBP_EquipmentSlot`，保持父类 UmbraEquipmentSlotWidget。

| Designer 名称 | 类型 / Is Variable | 职责 |
| --- | --- | --- |
| EmptyIcon | Image / 是 | 新增或将真正的空槽图案 Image 改为此名；按 SlotType 查映射后赋纹理 |
| ItemIcon | Image / 是 | 已有；有 Item 时显示 ItemDisplay.Icon，否则 Collapsed |
| RarityFrame | Image / 是 | 已有；继续原有稀有度规则 |
| LockedOverlay | Image / 是 | 已有；继续原有锁定规则 |
| HighlightFrame | Image / 是 | 已有；继续由当前 BP_RefreshVisual 控制，无新增 C++ 绑定 |
| slot_background | 保留现状 | 通用底板，C++ 不覆盖；不要直接把底板重命名为空槽图案 |

把 EmptyIcon 放在底板上方、物品图标/边框下方，与 ItemIcon 对齐。Color and Opacity 设白色、Alpha=1，纯装饰层使用 Not Hit-Testable (Self & All Children)。前四个名字对应 C++ BindWidgetOptional；不要另外创建同名普通变量。

在 **WBP_EquipmentSlot → Class Defaults → Equipment / Visual → Empty Slot Icons** 添加十个 Key，Value 选择各自 `Texture2D`。Ring1、Ring2 可以使用同一纹理。它是类默认映射，不在十个实例各配一次，不硬编码 Content 路径。资源继续遵守 [EditorSetup](EditorSetup.md) 的来源/许可要求。

刷新顺序：SlotType 查映射 → C++ 设置 EmptyIcon/ItemIcon 的基础显示 → 原有 RarityFrame/LockedOverlay → BP_RefreshVisual。有装备时空图标隐藏；无装备且配置有效时物品图标隐藏、空图标显示；缺失 Key 或空纹理时清除旧 Brush 并隐藏空图标，底板保留。已装备但 ItemDisplay.Icon 为空仍算已装备，不回退为空槽身份图。

保留现有 Hover / Selected / Locked / HighlightFrame Graph。检查旧 Graph 是否另给 EmptyIcon/ItemIcon 强行设置 Brush 或可见性；只移除冲突的旧图标赋值，不删除高亮逻辑。最终 BP 表现仍晚于 C++ 执行；BP_RefreshVisual 内不能反调 RefreshVisual/SetItem 等刷新入口。

## 2. WBP_Equipment 十个实例

已保存的实例目前正确，只需检查、保留以下值；全部仍是同一 WBP_EquipmentSlot。

| 现有实例名 | Details → Slot Type |
| --- | --- |
| Head | Head |
| Chest | Chest |
| Hands | Hands |
| Legs | Legs |
| Feet | Feet |
| Amulet | Amulet |
| Ring1 | Ring1 |
| Ring2 | Ring2 |
| MainHand | MainHand |
| OffHand | OffHand |

编译 Slot 后再编译 Equipment，确认 Designer 空槽图案跟随 SlotType。SlotType 仍只表示身份和默认视觉，不参与属性计算。

## 3. 一个 WBP_StatEntry 模板

在 `/Game/UI/CharacterMenu/StatEntry` 中创建唯一 **WBP_StatEntry**，父类选 **UmbraStatEntry**。可以把原 Strength WBP 的 HorizontalBox/SizeBox 布局复制到这个模板，保留原字体、边距和颜色。若 GUI 中已经有未保存的同名模板，先核对并直接复用，不再创建第二个。

```text
WBP_StatEntry : UmbraStatEntry
└─ HorizontalBox（可以保留原 SizeBox 包装）
   ├─ StatIcon      : Image
   ├─ StatNameText  : TextBlock
   └─ StatValueText : TextBlock
```

准确改名：`staticon → StatIcon`、`TextBlock_115 → StatNameText`、`statValue → StatValueText`；三者都勾选 Is Variable。C++ 自动绑定并填充，不需要 Text 每帧 Binding，也不需要自己写 Apply Stat Value 接线。若复制了旧 Graph，清掉对这三者的固定文字/图标覆盖与 Text Binding。可以保留字体、颜色、材质和纯表现动画。TooltipClass 可留空；旧 HUD 的 Tooltip 和 Apply Stat Value 事件兼容保留。

`SetStatDisplay(Icon, DisplayName, Value)` 是纯显示接口；不读取 Player/ASC。Value 已由父容器格式化为 FText。通用模板单独预览尚无父级数据时可以显示“—”。父容器 Designer 可以预览名称/图标；没有运行时 ASC 时数值为“—”。图标未配置会隐藏但保留其布局空间。

## 4. 替换现有四套实例并接父级

1. 打开 **`/Game/UI/CharacterMenu/AttributeMenu/WBP_PrimaryAttribute`**，将父类改为 **UmbraCharacterStatsPanel**。仅此四维容器使用数据面板类；WBP_AttributeMenu、WBP_CharacterMenu、WBP_Equipment 保持各自菜单父类与页面结构，其中预览父类前置条件见第0节。
2. 记录四行原来的 Slot 布局参数。删除此 Designer 中的四个旧实例：`WBP_StatEntry`（实际 Class 是 WBP_StatEntry_Strength）、`WBP_StatEntry_Dexterity`、`WBP_StatEntry_Intelligence`、`WBP_StatEntry_Faith`。只删除四行实例，不删角色名、职业、等级、经验、头像、外框或页面容器。
3. 从 Palette 的 User Created 或 Content Browser 拖入同一个新 **WBP_StatEntry** 四次，放回原来四行。保持它们属于 WBP_PrimaryAttribute 的 WidgetTree，可以嵌套普通 Panel/SizeBox，不额外封装进另一层 UserWidget。
4. 设置实例名与唯一需要的实例数据参数：

| 新实例名 | Details → Character Stats → Stat |
| --- | --- |
| StrengthEntry | Strength |
| DexterityEntry | Dexterity |
| IntelligenceEntry | Intelligence |
| FaithEntry | Faith |

5. 每个实例**只需手工设置 Stat 身份**和布局；不用手填数值、Icon、StatName 或 ASC。实例名便于阅读，注册实际按 Stat 枚举识别。
6. 在 **WBP_PrimaryAttribute → Class Defaults → Character Stats → Stat Display Data** 的四个默认 Key 下选择 Icon。DisplayName 默认已是本地化 FText：力量、敏捷、智力、信仰；可以在这里覆盖，不在每个实例上重复填。
7. Compile、Save 模板和父容器，再编译使用它们的菜单。原四个独立 WBP 资产暂时保留，替换引用验收通过后再在 Editor 的 Reference Viewer 检查其它引用，按需清理；本轮不直接删除资产。

父级只订阅实际注册行的属性，因此四维面板只有四个 Attribute Value Change Delegate；不会为了旧八项面板的数量要求报错。旧八项 HUD 行可以继续使用原 Brush/Apply Stat Value，不要求这次迁移。

## 5. 数据与配置优先级

- 数值：OwningPlayer 的 `AUmbraPlayerState` → 既有 `UUmbraAbilitySystemComponent` → `UUmbraAttributeSet` 的当前聚合值。初始化 GE / DebugInitialAttributes / 运行中 GE 的顺序沿用 [属性文档](AttributeSetPhase1.md)，本次不修改。四维按点数显示为本地化整数，只有显示舍入，不修改 GAS 原始值。
- 视觉：父级 StatDisplayData 的类默认值覆盖 C++ 默认名称；Icon 由 Blueprint 配置。父级 SetStatDisplay 覆盖行的显示快照。未配置映射的旧 HUD 行仍走 SetDisplayValue，保留 Designer Brush 和原 StatName。
- 生命周期：Construct 扫描行并立即同步；ASC ready / Pawn 变化 / PlayerState 通知时重新绑定并同步；Destruct/Shutdown 清除所有监听。同 ASC 重复通知不会重复订阅。隐藏菜单仍通过事件更新已有行，因此重开显示最新值；重新构建也立即同步。Controller 的既有 OnRep_PlayerState 现在同时遍历 HUD 与嵌套 CharacterMenu 面板。
- `EUmbraCharacterStat` 只在末尾追加四项，保留旧枚举序号。四维行不参与 GAS 订阅，不使用 Tick。

## 6. 验证

1. PIE 打开菜单，四行名称/图标对应，初值与 F1 玩家调试面板一致；未就绪时为“—”。
2. 使用既有 F1 的 **Add Effect**，四维按现有调试规则每层 +20；Remove Effect 后回退。再用临时测试 GameplayEffect **只改 Strength +1**，确认只有力量行数值变化；分别换成 Dexterity/Intelligence/Faith 重复。GE 仅用于临时验证，不在 UI 中计算/施加属性。
3. 关闭菜单，改变属性，再打开，数值应是当前值。反复开关、移除/重新创建菜单、换 Pawn/ASC，检查 Output Log 没有失效回调或重复注册警告。多人复制与实际换 Pawn 需另外进行 PIE 验收。
4. 十槽逐一检查空图案；SetItem（非空 Item 标识并传图标）后隐藏空图，ClearItem 后恢复。测试悬停、互斥选择、锁定、稀有度框；缺配置不残留上一个槽的图案。全身 Preview 继续原表现。
5. Session Frontend → Automation：`Umbra.UI.Equipment.EmptySlotIcons`、`Umbra.UI.CharacterStats.PrimaryRows`；同时回归原 `Umbra.UI.Equipment.SlotContract`、`SlotBindings`。前者覆盖十种映射、装备/清除/锁定和空配置；后者覆盖初次同步、单行更新、GE 添加移除、ASC 未就绪/恢复、换 ASC、销毁解绑及重构。NullRHI 结果不能证明字体、图标或真实地图 PIE 画面。

## 本轮文件清单

- `Source/Umbra/UI/Equipment/UmbraEquipmentSlotWidget.h/.cpp`
- `Source/Umbra/UI/UmbraStatEntry.h/.cpp`
- `Source/Umbra/UI/UmbraCharacterStatsPanel.h/.cpp`
- `Source/Umbra/UmbraPlayerControllerDebug.cpp`
- `Source/Umbra/Tests/UmbraStatWidgetTestTypes.h`、`UmbraStatWidgetTests.cpp`（新增）
- `Docs/CharacterMenuStatsAndIcons.md`（本文新增）
- `Docs/Architecture.md`、`EditorSetup.md`、`Progress.md`、`CharacterStatsPanel.md`、`EquipmentMenu.md`、`EquipmentBlueprintGuide.md`

本地 Saved 下另有只读检查脚本、JSON、验证副本与日志，属于生成/验证数据，不提交。没有修改 `.uasset`、`.umap`、Preview 源码或战斗源码。
