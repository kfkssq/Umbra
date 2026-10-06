# 统一物品 Tooltip：第一阶段数据契约

2026-10-05 第三阶段：ItemDefinition.Presentation 新增 TooltipQualityBackgroundColor（FLinearColor，默认 White），FromDefinition 基础构建复制到 TooltipData，FromInventory / FromEquipment 沿用该基础结果。三个入口都不由 Rarity 推导颜色。该字段仅驱动 Tooltip 的 quality_bg / SlotBackground Tint；RarityColor 仍用于名称/品质文字，槽位两张贴图不变。正式 Hover 的新鲜快照、上下文订阅和无效清理见 [ItemTooltipUI](ItemTooltipUI.md)；下面第一阶段的“未接入 Hover”为历史范围。

第二阶段已添加显示专用ItemCategory/WeaponType、集中展示文本和两个纯展示控件，见[分类与WBP接入契约](ItemTooltipUI.md)。下文“尚无类型/尚无Widget”为第一阶段历史范围；原始数据、评级、身份与需求规则保持，当前类型文本优先使用具体武器类型/明确分类，Unknown才使用原后备。

2026-10-04。本阶段只有 C++ 数据、只读构建、格式化、参考缩放评级和自动化。没有新增 UUserWidget、WBP、悬停、定位、对比、随机词缀、传奇威能或资产写入。

## 实际接口审查与职责

- ItemDefinition 既有 Presentation 是字段的编辑器 Category，不是嵌套结构。名称、Icon、RarityColor、SlotBackgroundTexture、RarityFrameTexture 原样保留。项目此前没有品质枚举。
- WeaponProfile.Damage.Channels 使用既有 EUmbraWeaponDamageType 九类型和 EUmbraPrimaryAttribute 四维；每类型唯一、每通道每主属性最多一次，可选 PointCurve。Tooltip 读取原始配置，不读取角色 AD/AP 或惩罚后的 DerivedStats。
- InventorySnapshot 用 bReady / Capacity / SlotIndex / InstanceId / Definition；复用 InventoryMenu::ValidateSnapshot 验证全快照。EquipmentSnapshot 用 bValid / Revision / Slot / InstanceId / Definition / bRequirementsMet；构建入口校验唯一槽、唯一 GUID、合法 AllowedSlots。
- Equipment.Refresh 既有规则通过 GetFilteredAttributeValue 排除自身装备 GE；抽取为私有 ReadRequirements，正式刷新与公开只读 QueryRequirements 共用，不使用“最终属性减去词缀”。DerivedStats 的计算与 GAS 伤害规则保持原样。
- CombatInfo 的行名称由现有 WBP 配置，没有可复用的原生名称表。Tooltip 将其涉及的原生名称集中于 UmbraTooltipFormatting；数字/百分比渲染抽取为 UmbraCombatStats::FormatNumber，并让 CombatInfo 继续使用原参数，保持其现有输出。
- 名称后备复用 EquipmentItemDisplay::FromDefinition。真实物品类型暂不存在：Weapon 非空显示“武器”，有 AllowedSlots 显示“装备”，其余“物品”；不猜测剑、斧、护甲子类。

## 默认值与配置来源

ItemDefinition.Presentation 新增 ItemLevel=1、Rarity=Common、FlavorText=空。未序列化这些字段的旧资产继承原生默认值，无需批量保存或迁移。ItemLevel 与 RequiredLevel 独立，前者不被装备/属性/伤害逻辑读取。

EUmbraItemRarity 明确固定数值：Common=0、Magic=1、Rare=2、Epic=3、Legendary=4、Unique=5。它是新枚举，没有已有品质序号需要插入重排。后续扩展只能保留现有数值。品质没有伤害、词缀数量、掉落或需求效果。

实际颜色始终来自原有 RarityColor；枚举不覆盖颜色或两张品质贴图。建议人工配置普通灰白、魔法蓝、稀有黄、史诗紫、传奇橙、独特暗金；此建议没有新增自动调色规则。

## 参考缩放评级

同一主属性的参考系数为 `Σ(BaseDamage[channel] × Coefficient[channel, attribute]) / Σ(BaseDamage[channel])`；未配置该属性的通道按0贡献，仍计入分母。零基础通道权重为0，全部为0或空通道时自动为None；不显示零基础伤害通道。总基础伤害是九通道原始非零值之和，完全不含缩放、词缀、A/X、暴击、易伤、防御或需求惩罚。

PointCurve 不被采样，不读取角色四维；系数仍作为配置参考强度，并设置对应 Scaling.bHasCurve。曲线非线性、手动评级均不能被呈现为精确收益或 DPS。为避免 float 配置在边界被 double 累加误差降档，评级比较在 float 精度进行；原始 ReferenceCoefficient 保持 double。

集中配置类 UUmbraTooltipSettings，读取 GetDefault，配置源顺序为原生默认值 → UE Game 配置层（项目 DefaultGame.ini）→ 平台/运行配置层。可在 `[/Script/Umbra.UmbraTooltipSettings]` 下设置 S/A/B/C；默认1.0/0.8/0.6/0.4。必须有限且严格 S>A>B>C>0。S/A/B/C 为含下界，0<系数<C 为D，0为None。非法阈值返回 InvalidConfiguration，不静默采用假评级。本次不修改用户 DefaultGame.ini。

WeaponProfile → Weapon / Tooltip 的 StrengthGrade、DexterityGrade、IntelligenceGrade、FaithGrade 各包含 bOverride 与 Grade。默认关闭覆盖；开启时以手动 S/A/B/C/D/None 为准，明确允许手动 None 隐藏。手动值优先于自动评级，即使零基础伤害也保留手动值。负数、非有限值、重复类型/缩放及非法枚举的武器配置拒绝，不显示部分残留结果。

## 数据与公共 API

入口位于 [UmbraItemTooltipData.h](../Source/Umbra/Items/UmbraItemTooltipData.h)，实现位于 [UmbraItemTooltipData.cpp](../Source/Umbra/Items/UmbraItemTooltipData.cpp)。UUmbraItemTooltipDataBuilder 三个 BlueprintPure 函数均按值返回全新 FUmbraItemTooltipData：

| API | 输入与身份契约 |
| --- | --- |
| FromDefinition(Definition) | 纯定义预览；Source=DefinitionPreview，GUID 无效，角色需求 Unknown。 |
| FromInventory(Snapshot, SlotIndex, ExpectedId, ExpectedDefinition, Equipment, bHasTargetSlot, TargetSlot) | 校验全快照后匹配槽、GUID、Definition；保留原GUID。Equipment可为空；非空时提供需求预览，并拒绝已进入装备的同GUID。 |
| FromEquipment(Snapshot, Slot, ExpectedId, ExpectedDefinition, Equipment) | 校验全快照与身份；Source=Equipment，bEquipped=true。非空Equipment需匹配当前Revision及槽身份/需求结果，拒绝过期来源。 |

ExpectedDefinition 是调用者当前槽位显示的定义，防止过期槽与新定义混用。API 校验的是传入快照结构和期待身份，不是网络所有权验证；调用方必须从正确人物组件获取新快照，不能将任意合成快照当作真实库存。Inventory 没有 Revision；移除后仍拿旧快照且未传当前装备上下文时无法检测所有陈旧情况，第二阶段必须在库存变更事件重取快照。

成功：bValid=true、Result=Success。失败：全新空结构、bValid=false、Result=InvalidDefinition / InvalidSnapshot / IdentityMismatch / InvalidSlot / ContextUnavailable / InvalidConfiguration。不复用上一个物品的身份、Stats 或标题。无 Add/Remove/Equip/Unequip/Refresh/ApplyGE，也不生成 GUID。

FUmbraItemTooltipData 包含定义/实例/来源/槽/装备状态；标题、图标、品质/颜色、物品等级；原始武器总基础和类型明细、四条评级；统一 Stats；需求定义与当前上下文；Weight、FlavorText、空 SpecialEffects 扩展数组。

FUmbraTooltipStatLine 保留 StatId、RawValue、Unit、Source、bDamageBonus、完整 FUmbraDamageBonus 以及 Text。普通 StatId 使用已有主属性/词缀枚举名称（如 Armor、AttackSpeed）；伤害用 DamageBonus，并以 Bucket、Types、AttackSource、双方标签查询和暴击/易伤标志共同区分语义，不能只按 StatId 合并独立X。Source 为 Intrinsic / FixedAffix / RolledAffix；本阶段只生成前两种。固有防御和词缀在同一列表中，保留来源且不合并数值。不生成 RolledAffix 或 SpecialEffects 假内容。

## 格式化规则

| 类型 | 原始单位及展示 |
| --- | --- |
| 四主属性 | 实际加值，如 +10 力量 |
| MaxHealth / MaxResource | 实际数值加值 |
| 固有或词缀 Armor / MagicResistance | 实际加值；来源分别保留；展示原物品值，需求惩罚单独说明 |
| AttackSpeed | 词缀0.2显示+20%攻击速度；不把词缀当作攻击次数/秒。CombatInfo总攻速仍为既有理论次数/秒。 |
| CriticalChance | 0.1显示+10%暴击率，含义为增加10个百分点 |
| MoveSpeed | 原始cm/s，+25显示+25 cm/s移动速度 |
| 九种类型抗性 | 实际评分，如+25评分火焰抗性，绝不转换为减伤百分比 |
| A区伤害 | 比例0.15显示+15%，标记A加算区 |
| X区伤害 | 因子1.2显示×1.2，标记X独立乘区；不会显示为+120% |

普通零值条目省略，伤害条目保留定义中每个独立配置。显示最多两位小数，原始值不舍入；百分比复用 CombatInfo 数值函数。DamageBonuses 与真实装备GE一致在Level1读取 FScalableFloat，不读取角色等级。类型限制显示全部指定类型；普通攻击、技能、暴击、易伤条件逐项列出，组合表示同时需要满足。双方 GameplayTag 条件使用引擎 ToString（Require/Ignore/TagQuery 描述）并始终标记“高级条件”；完整查询留在结构中，即使查询描述为空也不会退化成无条件全局增伤。

## 需求、惩罚与只读范围

QueryRequirements(Item, bHasTargetSlot, TargetSlot, Out) 与 Refresh 共用过滤算法。指定目标槽时排除该槽现有装备 GE，因此已装备物品排除自己，背包候选排除待替换装备；候选本身从未加入 ASC。不指定目标槽时，以所有当前装备为背景提供通用预览，没有自动挑选第一个 AllowedSlot。

这只是目标槽需求预览，不是替换操作承诺：当前 PlayerState.EquipFromInventory 仍拒绝占用槽，Tooltip 不改变转移规则。

当前角色等级读取 Equipment.CharacterLevel，四维读取过滤 GAS 且沿用非负钳制。等级不足=LevelTooLow（不可穿戴）；等级足够但四维不足=PrimaryPenalty（可穿戴、有惩罚）；全部满足=Met。每维满足状态和当前数值位于 Requirements.PrimaryMet / CurrentPrimaries（Strength、Dexterity、Intelligence、Faith顺序），等级在 CurrentLevel / bLevelMet，要求值在 RequiredLevel / RequiredPrimaries。

已装备的 bRequirementsMet 始终取 EquipmentSnapshot，表示既有系统的“四维是否满足”，不混同等级。传有效Equipment时可读取完整细节和配置倍率；未传时 bPrimaryStatusKnown=true，但 Requirements.bKnown=false，RequirementState=Unknown，不伪造等级、逐项判断或倍率。没有上下文的定义/背包预览所有角色判断均Unknown。

惩罚说明读取 UnmetWeaponBaseMultiplier / UnmetWeaponScalingMultiplier / UnmetDefenseMultiplier 的当前配置；固定词缀始终不受需求惩罚。传入未就绪Equipment返回ContextUnavailable，而不是输出假绿灯。现有组件仅在权威端初始化 BoundASC；远程客户端只能利用复制快照显示聚合状态，完整客户端穿戴前预览未实现，不能假称网络验证通过。

## 第二阶段接入与编辑器配置

1. 后续 UUmbraItemTooltip/WBP_ItemTooltip 仅接收 FUmbraItemTooltipData。先按 bValid/Result清空或显示；从所属人物的当前 Inventory/Equipment事件重新构建，换人、移除、无效快照时清空。不要缓存后继续展示旧实例。
2. InventorySlot传 SlotIndex + 原GUID + 当前Definition；EquipmentSlot传 Slot + 原GUID + 当前Definition。目标槽有歧义时由调用方明确提供；本阶段不创建选槽或对比UI。
3. 标题使用已构建字段，直接显示 Stats.Text；未来比较使用原始标识、来源、单位和条件，禁止解析字符串。DamageChannels为空隐藏基础伤害；Scaling中None隐藏；FlavorText为空隐藏背景故事；SpecialEffects为空隐藏扩展。
4. 根据 Requirements.bKnown / bPrimaryStatusKnown 显示逐项或未知状态；惩罚文字用 PenaltyText，不在WBP硬编码50%等数字；评级说明必须表明参考强度，bHasCurve时提示曲线影响实际收益。
5. 编辑器中 ItemDefinition 可手工配置 ItemLevel、六级Rarity、FlavorText；既有名称/图标/颜色/背景/框不必迁移。WeaponProfile可手工设置四项评级覆盖，默认无需改。阈值由Game配置集中调整，不放在WBP。

实际资产内容、WBP显示及PIE鼠标链均待编辑器确认。无需新建随机词缀/掉落/保存/效果系统。

## 文件清单与验证

- 新增 Items/UmbraItemTooltipData.h/.cpp、Tests/UmbraItemTooltipTests.cpp、本说明。
- 修改 Items/UmbraItemDefinition.h、Items/UmbraWeaponProfile.h、Equipment/UmbraEquipmentComponent.h/.cpp、UI/Combat/UmbraCombatStatData.h/.cpp。
- 同步 Docs/Architecture.md、Docs/EditorSetup.md、Docs/Progress.md。没有模块/目标变更，无需因本轮修改重新生成工程文件。
- 新测试：Umbra.Items.Tooltip.DefinitionAndScaling、Umbra.Items.Tooltip.Formatting、Umbra.Items.Tooltip.IdentityAndRequirements。覆盖默认兼容/真实旧资产加载、品质序号、等级独立、九通道、权重/边界/曲线/覆盖、所有固定属性单位和条件、需求/惩罚/身份/无副作用。
- 实际构建、自动化结果与未测边界统一记录在[Progress](Progress.md)。不把历史测试结果当成本轮验证。
