# 物品 Tooltip：分类、WBP 契约与正式 Hover

## 2026-10-06 视口完整显示修复（当前定位契约）

背包和装备菜单统一调用 `PresentBeside(HoveredCell)`。槽位的桌面绝对坐标先经 `GetViewportWidgetGeometry().AbsoluteToLocal` 转为视口 Slate 单位；不得直接与视口局部尺寸混用。Tooltip 通过 AddToViewport 显示，隐藏由菜单调用 HideTooltip；下文第三阶段原生 SToolTip 定位说明为历史记录。

原生 RebuildWidget 在运行时给完整 WBP 内容包一层 SScaleBox（ScaleToFit、DownOnly），保留 Designer 布局。完整自然尺寸超过视口时按宽高约束中更小的比例整体缩小，全部文字、图标和边框一起缩放；不裁掉末尾条目。普通尺寸不放大。位置使用缩放后尺寸钳制，四边默认留 12 Slate 单位；极小视口边距最多为短边的四分之一。HorizontalOffsetFromSlot 仍取 C++ 默认 0 → WBP Class Defaults 覆盖，单位同为 Slate 单位，正值留间隔、负值重叠；最终服从视口边界。缩放比例由布局和当前视口推导，无资产配置项，不设会导致溢出的最低缩放比例；极长内容或极小窗口会使文字变小。

显示期间 NativeTick 只跟踪槽位几何、文本换行后的自然尺寸、DPI 和视口变化，不重建数据行、不查询 GAS。隐藏后停止跟踪；整体仍 HitTestInvisible，不接管滚轮或焦点。替代旧的一次性 GameThread 异步重定位；该任务不能保证发生在下一次布局之后。Designer 不加入缩放包装。本轮未修改 WBP 二进制。

回归测试：`Umbra.UI.Items.TooltipViewportFit` 覆盖超高、超宽、双轴超限、正常大小、四组视口、边缘/越界锚点和反复调整尺寸，检查完整边界、等比、不放大及数据保留。真实视觉仍需 PIE：分别 Hover 剑/锤及装备槽，检查最底部重量/说明；在四角、嵌入式 PIE/独立窗口以及缩放窗口后确认完整显示，并回归选择、穿戴、卸装、滚轮和移出隐藏。自动化不是 WBP 视觉验证。


## 2026-10-05 第三阶段：原生 Hover 接入

InventoryMenu 和 EquipmentMenu 各新增 Class Defaults 的 `ItemTooltipClass`（EditDefaultsOnly / BlueprintReadOnly）。用户在两个 WBP 中选择同一个 WBP_ItemTooltip，不需要 Graph、资产路径硬编码或额外 Tooltip Manager。槽位只广播原生 Hover 生命周期；菜单获取当前 PlayerState 的组件快照，构建成功后惰性创建并复用一个 Tooltip，调用 Slot.SetToolTip。引擎原生 SToolTip 负责显示、定位、移出隐藏；没有 AddToViewport、自定义鼠标跟随、Tick、DragDrop 或鼠标捕获。

### 品质与控件

ItemDefinition.Presentation.TooltipQualityBackgroundColor 默认 White，经三个 Builder 进入同名 TooltipData 字段。可选 UImage **quality_bg**（保留小写名称）与 **SlotBackground** 只调用 SetColorAndOpacity；既有 Designer Texture/Material/Brush 保留。Clear 时 Tint 回白，整体折叠；换物品使用新数据。ItemIcon 使用真实 Icon，不应用背景 Tint。ItemName 和 RarityText 使用 RarityColor；ItemTypeText / ItemLevelText 保留设计颜色。没有根据 Rarity 枚举推导颜色。Tooltip 不绑定、不要求 RarityFrame；背包与装备槽原有边框逻辑不变。

### 身份与生命周期

- Inventory Hover：菜单必须处于已构建、活动页面和实时绑定状态；槽位来自菜单投影的有效快照，索引、GUID、Definition 与当前组件快照一致，组件注册且属于当前 PS。调用 FromInventory，bHasTargetSlot=false，不选择戒指等目标槽。手动 SetItemDefinition 会清除快照来源标记，不能冒充真实背包。
- Equipment Hover：同样校验页面、所属 PS、组件与映射槽，要求 Display.bFromEquipmentSnapshot，然后 FromEquipment 校验最新完整快照、Slot、GUID、Definition 和 Revision。已装备需求状态来自该快照。
- 每次 MouseEnter 都重新构建。无物品、无类、抽象类、组件/ASC 不可用或 Builder 失败均清空 TooltipData 并 SetToolTip(nullptr)，不显示半成品。预期空槽/刷新中的失败仅 VeryVerbose 诊断，避免正常 Hover 日志刷屏。
- 快照更新立即刷新当前 Hover，GUID/Definition 变化不会沿用旧数据；物品消失时先清空已打开 Widget 的数据再解除槽位 ToolTip，不能等菜单重开。成功穿戴/卸装依赖现有快照通知；InventoryFull 失败不改快照，Tooltip 保持有效。
- CharacterMenu 递归可见性/页签通知现在也向 InventoryMenu 传递 SetPageActive；关闭页面立即清除。Destruct 解除 Hover、ASC、四主属性和装备上下文委托，清除缓存引用；重建时唯一订阅并读取当前状态。异步回调只持弱菜单引用，不捕获旧数据。

### Requirements 刷新

两个菜单只为当前 Hover 计算。四主属性变化及 EquipmentSnapshot 通知触发一个合并的 GameThread 任务，待 GAS/Equipment 调用栈返回后重取上下文。原因：Equipment.Refresh 在 bUpdating 保护内广播，正式 QueryRequirements 此时拒绝查询；UI 不放宽该保护、不修改装备规则。快照事件先清除失效内容，随后任务恢复仍有效的 Hover。此任务按事件排队，不是 Tick、Timer 或轮询；不会重建 Inventory Grid。

CharacterLevel 当前是 EquipmentComponent 的 EditDefaultsOnly 配置，没有运行时等级系统或专用变化事件。每次 Hover 读取最新值；若 C++ 调试代码修改它，可通过既有 Equipment.Refresh 或菜单 NotifyPlayerContextChanged 通知更新。没有事件的直接内存赋值不保证鼠标静止时立即刷新；未添加轮询来伪造等级系统。

### 用户编辑器配置（本轮未修改 Content）

1. WBP_Inventory → Class Defaults → ItemTooltipClass = WBP_ItemTooltip。
2. WBP_Equipment → Class Defaults → ItemTooltipClass = WBP_ItemTooltip。
3. 各 ItemDefinition → Presentation → TooltipQualityBackgroundColor 配置实际背景颜色。
4. Compile / Save。保留已有布局、父类、StatEntryClass 和控件名称；无需添加 Hover/CreateWidget/SetTooltipData Graph。

### 人工 PIE 验收（待执行）

1. 出生背包 Hover TestSword，再移出并 Hover TestMace。核对真实名称/Icon、两处背景色、品质/等级/具体武器类型、基础伤害/通道、评级、Stats、需求、Weight、Flavor；不得残留上一件的行、文字或颜色。
2. Hover Sword 双击穿戴：原背包 Tooltip 立即失效；MainHand Hover 显示同一 GUID，Source=Equipment，需求服从当前快照。改变四主属性后，静止 Hover 的需求也更新。
3. Hover MainHand 右键卸装：装备 Tooltip 立即清空；背包新位置 Hover 显示原 GUID。填满背包再卸装，应返回 InventoryFull、仍装备且 Tooltip 有效、GE/属性和物品数量不变。
4. 核对 Hover 高亮、左键选择、背包双击、装备右键、ScrollBox 滚轮、页签切换及关闭重开；Tooltip 不挡鼠标、不拿焦点，AttributeMenu 不发生素材包窗口拖动。Designer 占位预览仍可编辑。

自动化夹具不能证明真实 WBP 的子控件鼠标路由、屏幕位置和素材效果。以上视觉/PIE 与网络验证仍待编辑器确认；构建和测试结果见 [Progress](Progress.md)。

以下为第二阶段基础契约（历史范围）；正式 Hover 已由上述第三阶段接入。第一阶段的身份校验、伤害、评级、属性格式与装备需求继续由原数据构建器负责。不修改 Content、背包/装备转移、GAS、DerivedStats 或伤害规则。

## 实际审查与分类规则

项目此前没有 ItemCategory、WeaponType 或等价 GameplayTag；新增 [UmbraItemClassification.h](../Source/Umbra/Items/UmbraItemClassification.h) 的显示专用枚举，均采用显式数值。

| ItemCategory | 固定值 | 本地化名称 |
| --- | --- | --- |
| Unknown | 0 | 采用旧资产后备规则 |
| Weapon | 1 | 武器 |
| Armor | 2 | 防具 |
| Accessory | 3 | 饰品 |
| Consumable | 4 | 消耗品 |
| Material | 5 | 材料 |
| Misc | 6 | 杂项 |

| WeaponType | 固定值 | 本地化名称 |
| --- | --- | --- |
| Unknown | 0 | 武器（仅在可靠武器上下文中） |
| OneHandedSword / TwoHandedSword | 1 / 2 | 单手剑 / 双手剑 |
| Dagger / Rapier | 3 / 4 | 匕首 / 刺剑 |
| OneHandedAxe / TwoHandedAxe | 5 / 6 | 单手斧 / 双手斧 |
| OneHandedMace / TwoHandedMace | 7 / 8 | 单手锤 / 双手锤 |
| Spear / Halberd | 9 / 10 | 长枪 / 长戟 |
| Staff / Wand | 11 / 12 | 法杖 / 魔杖 |
| Bow / Crossbow | 13 / 14 | 弓 / 弩 |

ItemDefinition → Presentation / Classification 配置这两个字段；默认都是Unknown，旧资产无需迁移保存。后续加类型必须保留已有数值。未配置分类时：有效WeaponProfile→武器；否则AllowedSlots非空→装备；其余→物品。不会读取AllowedSlots第一个元素猜测具体类型。

标题优先级：明确Weapon，或Unknown且有有效WeaponProfile时，优先用具体WeaponType；否则用明确ItemCategory；最后用上述通用后备。明确Armor配置具体剑类型时显示“防具”并警告，不显示成剑。Weapon缺Profile时保留已配置武器分类并警告，不虚构伤害。非武器分类搭配Profile同样警告；原始Profile数据仍保留，显示分类不干预战斗或原始武器数据。

`UmbraItemClassification::Describe` 给出本地化诊断；构建器将它保存在ClassificationWarnings并输出含资产路径的`LogUmbra Warning: Item classification ...`。警告不修改资产，不把单纯展示配置冲突变成构建失败。非法分类枚举回退到可靠通用类型。用户应在编辑器修正配置，避免未来每次构建重复警告。

WeaponType不控制槽位、动作/Montage、速度/距离、缩放或伤害类型。火焰单手剑仍是OneHandedSword，Fire通道仍来自WeaponProfile。六级品质保持0..5，Epic=3；ItemLevel与RequiredLevel继续独立；名称颜色只读取原RarityColor。

## 数据流与公共接口

`ItemDefinition + Inventory/Equipment快照 → UUmbraItemTooltipDataBuilder → FUmbraItemTooltipData → UUmbraItemTooltip → UUmbraTooltipStatEntry/TextBlock`。

三个构建入口签名和身份/失败契约不变，详见[第一阶段数据契约](ItemTooltipData.md)。新增数据包括ItemCategory、WeaponType、ItemType本地化文本、ClassificationWarnings，以及RarityText、RarityAndTypeText（如“史诗 · 单手剑”）、ItemLevelText、TotalDamageText、ScalingNote、WeightText、RequirementRows、RequirementStatusText；伤害通道与评级各补充Text。原始数值、属性来源、条件、GUID和需求详情全部保留。

`UmbraTooltipFormatting::BuildDisplayText`集中格式化已计算结果，三个入口在完成上下文填充后调用；不会重新计算需求或伤害。词缀仍原样使用第一阶段StatLine.Text。需求行的Style为Neutral/Met/Unmet/Unknown；Unknown状态不能输出“已满足”。只展示非零四维门槛；需求等级始终保留。PenaltyText沿用第一阶段从配置倍率生成的文本。零重量是有效值，每件有效物品都显示重量0或实际值，复用CombatInfo数字格式，不擅加kg等未定义单位。

| 原生控件 | 公共接口 |
| --- | --- |
| UUmbraItemTooltip | SetTooltipData(const FUmbraItemTooltipData&)、ClearTooltipData()、GetTooltipData()、BP_TooltipDataChanged(Data) |
| UUmbraTooltipStatEntry | SetEntryText(Text, Style=Neutral)、GetEntryText()、GetEntryStyle()、BP_EntryChanged(Text, Style) |

Tooltip完全不查询ItemDefinition、ASC、Inventory或Equipment，不重算或解析RawValue/条件。bValid=false或Result非Success时清空身份、标题、图标、所有文本与动态行，并折叠整个Tooltip。每次有效更新清空旧行再填充；分离出来的旧条目也清空文字。Destruct清空数据与行，重开必须重新SetTooltipData；Construct保留之前已设置的有效数据并重新投影。PreConstruct不生成假物品。

2026-10-05 Designer预览例外：RefreshDisplay在IsDesignTime()下仅设置自身HitTestInvisible并关闭焦点，保留Designer已有文字、图标、静态占位行和各Section的设计显隐，不调用数据投影或数据变化事件，不制造有效TooltipData。Title、Legendary、285等可用于纯布局预览；运行时仍清理这些占位内容，以SetTooltipData传入的真实数据覆盖，无效数据仍清空并Collapsed。此前已被旧预览实例清空的内容可能需要重新打开Designer恢复资产中保存的占位值；本轮不修改资产。

所有可见状态强制为HitTestInvisible（包含全部子控件），焦点关闭；Hidden/Collapsed仍允许。没有Tick、Timer、UMG Text Binding、鼠标事件、拖动或父窗口操作。蓝图事件只用于本控件的表现，不能用于切换其它菜单或修改角色状态。

## 准确 BindWidget 名称与类型

以下名称区分大小写。必需项使用BindWidget；在WBP编译时校验，原生Construct也会输出清晰的缺失警告。可选项使用BindWidgetOptional，缺失时跳过；可选Section缺失时仍清理/隐藏对应已绑定子控件。纯装饰组件不需要绑定。

| WBP_ItemTooltip 控件名 | 原生类型 | 必需性 |
| --- | --- | --- |
| ItemName | UTextBlock（Text Block） | 必需 |
| StatRows | UVerticalBox（Vertical Box） | 必需，纯动态列表 |
| ItemIcon | UImage（Image） | 可选，无Icon时折叠并清Brush |
| quality_bg / SlotBackground | UImage | 可选，仅背景 Tint，保留 Designer Brush |
| ItemTypeText | UTextBlock | 可选，显示具体/通用类型 |
| RarityText | UTextBlock | 可选，显示六级中文品质 |
| ItemLevelText | UTextBlock | 可选 |
| TotalDamageText | UTextBlock | 可选 |
| ScalingNote | UTextBlock | 可选 |
| PenaltyText | UTextBlock | 可选 |
| WeightText | UTextBlock | 可选 |
| FlavorTextBlock | UTextBlock | 可选 |
| DamageRows | UVerticalBox | 可选，纯动态列表 |
| ScalingRows | UVerticalBox | 可选，纯动态列表 |
| RequirementRows | UVerticalBox | 可选，包含逐项需求和总体状态行 |
| SpecialEffectRows | UVerticalBox | 可选，纯动态列表 |
| HeaderSection | UWidget | 可选；可绑定Border、VerticalBox、Overlay等容器 |
| DamageSection | UWidget | 可选，同上 |
| ScalingSection | UWidget | 可选，同上 |
| StatsSection | UWidget | 可选，同上 |
| RequirementsSection | UWidget | 可选，同上 |
| WeightSection | UWidget | 可选，同上 |
| SpecialEffectsSection | UWidget | 可选，同上 |
| FlavorTextSection | UWidget | 可选，同上 |

WBP_TooltipStatEntry只有一个必需绑定：`EntryText : UTextBlock`。可以在其外围放任意Border/Overlay/图标/背景，但文本名称与类型需匹配。

这五个Rows容器均由C++独占子项，每次刷新会ClearChildren；不要把栏目标题、边框、分隔线或固定示例行放进Rows。把它们放在Section内、Rows外。需要自动隐藏的装饰应放入对应Section，否则C++无法知道其归属。

## 推荐层级

```text
WBP_ItemTooltip（父类UmbraItemTooltip）
└─ SizeBox_Root                         自由设置宽度/最大宽度，不绑定
   └─ Border_Background                自由使用现有素材，不绑定
      └─ VerticalBox_Content
         ├─ HeaderSection
         │  ├─ ItemIcon                Image
         │  ├─ ItemName                TextBlock，必需
         │  ├─ RarityText              TextBlock
         │  ├─ ItemTypeText            TextBlock
         │  └─ ItemLevelText           TextBlock
         ├─ DamageSection
         │  ├─ TotalDamageText         TextBlock
         │  └─ DamageRows              VerticalBox，留空
         ├─ ScalingSection
         │  ├─ ScalingRows             VerticalBox，留空
         │  └─ ScalingNote             TextBlock
         ├─ StatsSection
         │  └─ StatRows                VerticalBox，必需，留空
         ├─ RequirementsSection
         │  ├─ RequirementRows         VerticalBox，留空
         │  └─ PenaltyText             TextBlock
         ├─ WeightSection
         │  └─ WeightText              TextBlock
         ├─ SpecialEffectsSection
         │  └─ SpecialEffectRows       VerticalBox，留空
         └─ FlavorTextSection
            └─ FlavorTextBlock         TextBlock

WBP_TooltipStatEntry（父类UmbraTooltipStatEntry）
└─ 可选Border/Overlay等装饰
   └─ EntryText                        TextBlock，必需
```

没有伤害通道时折叠Damage；没有非None评级时折叠Scaling；没有Stats时折叠Stats；SpecialEffects和FlavorText空时折叠对应区域。基础伤害直接显示构建器数值，评级曲线标记与参考说明直接显示构建器文本。固有防御与固定词缀只放同一StatRows列表。特殊效果当前构建器始终不生成内容，测试中的手工注入仅验证展示扩展。

默认Header分别填RarityText和ItemTypeText，用户可以在布局中添加“·”装饰。如果改用合并的一行，可在BP_TooltipDataChanged使用已有RarityAndTypeText写到自定义TextBlock，同时省略两个可选绑定；不要自行查枚举英文名称。

## 第二阶段首次创建 WBP 的历史步骤

已有 WBP_ItemTooltip 的项目无需重做以下创建步骤；第三阶段仅按本文开头四项配置接入。

1. 本轮原项目构建完成后打开UE5.8.2。在项目自有目录复制适合的现有Tooltip素材WBP，保留素材来源信息；不要直接改供应商原件。将复制件父类改为UmbraItemTooltip，命名例如WBP_ItemTooltip。
2. 按上表重命名所需控件并勾选Is Variable，删除冲突的同名变量；清空五个动态Rows。移除这些文本/Brush/Visibility上的旧Binding以及Construct中的示例赋值，避免覆盖原生值。背景、边框、分隔线、字体、宽度和动画由你决定。
3. 制作或复制行WBP，父类UmbraTooltipStatEntry，包含EntryText。用其Designer配置字体、字号、换行和Neutral颜色；Class Defaults可配置MetColor、UnmetColor、UnknownColor。
4. WBP_ItemTooltip Class Defaults设置`StatEntryClass = WBP_TooltipStatEntry`。它是唯一新增的Tooltip样式类参数；留空时会以自动换行的原生TextBlock降级，需求仍通过中文状态文字区分。抽象类无效，会警告并降级；不在C++写像素/资源路径。
5. ItemDefinition的Presentation/Classification手工选择ItemCategory和WeaponType。旧资产维持Unknown也能显示通用类型。可继续配置ItemLevel、Rarity、FlavorText和现有RarityColor/Icon；武器评级覆盖沿用第一阶段WeaponProfile，不需要重新设置基础伤害或装备要求。
6. 检查素材遗留图表：移除W_Window_Drag、DragDropOperation、DetectDragIfPressed、OnDragDetected窗口拖动，以及Construct中隐藏其他窗口的演示逻辑。不得修改AttributeMenu/InventoryMenu/EquipmentMenu或父窗口的Visibility、Canvas位置、RenderTranslation。原生控件不会执行这些行为，但不会自动清理你复制的资产图表。
7. 为独立预览临时Create Widget，再调用FromDefinition→SetTooltipData；切换两件不同物品、再传失败数据/调用Clear。不要在Designer创建虚构属性或在Tick重建Tooltip。实际WBP及图表仍待编辑器确认。

本轮没有创建上述WBP，因此不能声称已通过它们的BindWidget编译或真实渲染验证；推荐层级是用户手工制作契约。

## 展示控件的补充验收

正式槽位接入已由第三阶段原生 C++ 完成，流程见本文开头。需求 Unknown 不能显示为绿色“满足”；现有权威/客户端限制沿用第一阶段。Tooltip 展示不生成 GUID 或调用 Equip/Unequip。

人工最小验收：

- 编译两个WBP，确认必需绑定与StatEntryClass；用单手火焰剑、非武器材料、空故事/空属性物品连续切换。
- 确认None评级不占行、曲线有说明、零重量显示、固有/词缀同列表；Unknown、等级不足、主属性惩罚与正常状态的文字/颜色正确。
- 清空和重新Construct无旧标题、图标、属性或故事；长条件/标签说明正确换行，整体宽度/边框/DPI适合素材。
- 在真实PIE验证左键选择、双击穿戴、右键卸装、hover高亮、滚轮及页签；原生HitTestInvisible断言不能替代真实鼠标验收。

## 文件清单与测试

新增：

- [Items/UmbraItemClassification.h](../Source/Umbra/Items/UmbraItemClassification.h)、[.cpp](../Source/Umbra/Items/UmbraItemClassification.cpp)：枚举、本地化、非致命分类诊断。
- [Items/UmbraItemTooltipDisplay.cpp](../Source/Umbra/Items/UmbraItemTooltipDisplay.cpp)：已有结果的集中展示文本。
- [UI/Items/UmbraItemTooltip.h](../Source/Umbra/UI/Items/UmbraItemTooltip.h)、[.cpp](../Source/Umbra/UI/Items/UmbraItemTooltip.cpp)：纯展示基类、绑定与动态行生命周期。
- [UI/Items/UmbraTooltipStatEntry.h](../Source/Umbra/UI/Items/UmbraTooltipStatEntry.h)、[.cpp](../Source/Umbra/UI/Items/UmbraTooltipStatEntry.cpp)：通用文本/状态样式行。
- [Tests/UmbraItemTooltipUITestTypes.h](../Source/Umbra/Tests/UmbraItemTooltipUITestTypes.h)、[Tests/UmbraItemTooltipUITests.cpp](../Source/Umbra/Tests/UmbraItemTooltipUITests.cpp)：原生测试树、分类、生命周期和无副作用测试。
- 本文档Docs/ItemTooltipUI.md。

修改：Items/UmbraItemDefinition.h、Items/UmbraItemTooltipData.h/.cpp；Docs/ItemTooltipData.md、Architecture.md、EditorSetup.md、Progress.md。没有更改模块/target，无需因此重生成Rider工程。

新增自动化：`Umbra.Items.Tooltip.Classification`、`Umbra.UI.Items.TooltipLifecycle`、`Umbra.UI.Items.TooltipReadOnly`；第一阶段三个Tooltip测试继续回归。实际构建、最终报告、失败修复过程与未测范围见[Progress](Progress.md)。
