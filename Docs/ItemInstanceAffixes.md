# 武器区间与实例词缀（2026-10-09）

本页是本轮新增契约；覆盖旧文档中“只有固定词缀”“所有属性使用 KeyValue/StatEntry”“特殊效果仅预留”的说明。未修改或保存 Content，实际资产内容与绑定均待编辑器确认。

## 武器与真实伤害

- `FUmbraWeaponDamageChannel.bUseDamageRange=false` 为兼容默认值：只读取旧 `BaseDamage`，作为 `[BaseDamage, BaseDamage]`；新字段不会叠加旧字段。启用后只读取 `MinBaseDamage/MaxBaseDamage`。
- `ResolveRange` 统一验证有限、Min≥0、Max≥Min；无效配置报出通道与端点并拒绝计算，不回退零值。重复/未知通道仍按原规则拒绝。
- DerivedStats 的 `BaseDamage` 使用均值；`ScalingDamage` 沿用原项目的主属性/曲线输出×系数之和。`Damage=均值+ScalingDamage`，AD/AP、CombatInfo 与 HUD 维持稳定参考值。快照额外携带需求惩罚后的 Min/Max，仍沿用原快照复制。
- 服务端 `UmbraTypedDamage::Prepare` 仅在 `WeaponChannels` 中按通道采样，计算 `(抽取基础伤害 + ScalingDamage) × WeaponMultiplier`，写入本次 Spec 的 Base SetByCaller；通道 AD/AP 系数仍为零。已准备的 Spec 不重新采样，Execution 不抽基础伤害，也不修改 GAS AD/AP。
- 仍由现有 Typed Execution 处理 A/X、整次命中的一次暴击、易伤、双抗与类型抗性，最后一次 IncomingDamage 扣血/聚合飘字。`Legacy` 和 `ExplicitChannels` 不启用随机武器采样。A/X 仍要求攻击已有 `Typed.bUseDamageBuckets=true`。
- Tooltip 的 `TotalMinBaseDamage/TotalMaxBaseDamage` 为原始通道端点求和，不含主属性、惩罚或增伤；固定端点简写单值。自动评级以通道均值加权，评级阈值不变。

## 定义、单位与离散候选

`ItemDefinition.AffixRollConfigs` 为新配置来源，每项必须有唯一非空 `AffixId`，`Kind` 选择 Primary、Attribute 或 Damage。Primary/Attribute 使用各自枚举；Damage 使用原有 `DamageBonus` 的桶、伤害类型与条件，模板中的 Magnitude 被生成值覆盖，不额外应用。

| 配置 | 规则 |
| --- | --- |
| MinValue / MaxValue | 非负有限，Max≥Min；使用 GAS 原始单位 |
| TierCount | 默认6，仅允许5或6 |
| ExplicitValues | 非空时覆盖均匀生成；必须有限、严格递增且在 Min/Max 内，数量以实际数组为准 |
| bIntegerValues | 默认 false；true 对均匀档位四舍五入到整数，显式候选必须已经是整数；重复档位拒绝，需增大范围或配置更少的显式候选 |
| 均匀档位 | `Min+(Max-Min)*i/(TierCount-1)`，等概率选一个；Min=Max 时只有一个有效候选 |

攻速、暴击率、A 区使用比例：`0.12..0.4`、6档得到 12%、17.6%、23.2%、28.8%、34.4%、40%。要得到23.8%，在 ExplicitValues 中填写 `0.238`。X 使用倍率，例如 `1.12..1.4` 表示提高12%..40%。其余属性使用原生点数/评分/cm/s，不在生成时乘100。实际值保存 float；Tooltip 按原有单位格式化，实际值至多两位小数、范围至多四位小数，不从中文文本反解析。

固定 `PrimaryBonuses/AttributeBonuses/DamageBonuses` 保持原行为，随机配置是另一个明确的数据来源；同一项奖励不要同时配置为固定与随机。定义资产在同一局中视为不可变；随机伤害词缀的过滤条件装备时与其保存身份核对，不允许把旧实例套用到不同条件的配置。

## 实例与转移

- `FUmbraItemInstance` 保存 Definition、GUID、RolledAffixes；每条保存身份、实际值、生成时全部有效候选和零基 SelectedTier，可供未来存档复用。
- `Inventory.AddItem(Definition, GUID)` 在服务端第一次接受该 GUID 时创建实例。初始化背包继续使用此入口。组件保留本局 KnownInstances，已知 GUID 再次加入使用原结果；不同 Definition 或修改过的完整 payload 被拒绝。
- `AddInstance` / `EquipInstance` 为可信 C++ 服务端转移入口，不是客户端 RPC，不随机。旧 `Equipment.Equip(Definition, GUID)` 继续兼容固定装备；配置随机词缀的装备需先创建实例，定义接口拒绝无实例装备。
- `PlayerState.EquipFromInventory/UnequipToInventory` 的参数保持不变，内部传完整实例，返还和失败回滚均保留 GUID 与全部词缀。原容量、槽占用、等级、GUID、权威与事务检查保留；没有新增自动替换交互。原生 EquipInstance 如需替换，必须传入 ReplacedItem 接收被替换的完整实例，否则拒绝；旧定义接口拒绝覆盖带随机词缀的实例。
- Inventory 临时注销保留内容；Equipment 注销时保存完整实例、移除 GE，重新注册/初始化恢复同一实例。不会重新生成词缀。
- Inventory 与 Equipment 的实例快照均 `COND_OwnerOnly`。ASC 保留 Mixed 模式，装备 GE 的实际 SetByCaller 数值随拥有者完整 GE 复制；没有 UI→服务端实例 RPC。

## 装备效果

每个装备实例仍只有一个 `UUmbraEquipmentEffect` 活跃句柄。固定属性先填入既有 SetByCaller，随机 Primary/Attribute 按实际值相加；护甲/魔抗刷新只对固有防御施加需求惩罚，固定和随机词缀均不被该惩罚缩减。主属性需求查询排除该装备的整个 GE，包含它的随机主属性。

随机 Damage 把实际值写入按 AffixId 命名的 Spec SetByCaller。EquipmentDamageBonusComponent 返回固定 DamageBonuses 与该 Spec 已存在的随机项，保留定义的过滤规则，Magnitude 只取实例实际值。没有把配置最小、最大或均值作为装备效果。卸装按同一句柄移除全部贡献；StatsPanel 继续读相同的 DamageBonus 来源。

## Tooltip 契约与最少 Designer 配置

1. 新建 `WBP_TooltipAffixEntry`，父类选 **UmbraTooltipAffixEntry**。
2. 必需 TextBlock：`AffixName`、`AffixValue`；可选 TextBlock：`AffixRange`。大小写必须一致，勾选 Is Variable。字体、颜色、间距、数值右对齐、横排/分行完全由 Designer 设置。没有 AffixRange 时，范围自动追加在 AffixValue 后，不需要 Graph。
3. `WBP_ItemTooltip` Class Defaults 的 **AffixEntryClass** 指定新 WBP。沿用 `StatRows`；普通四主属性、战斗属性、抗性、A 增伤与固有防御均为相同 Neutral 样式，不再经 KeyValueEntry/StatEntry 重复输出。未配置新类时有单行纯文本后备。
4. X 增伤投影为 `SpecialEffectStats`（保留原始来源/条件）及 `SpecialEffects` 文案，如“造成的所有类型伤害提高20%”。沿用 `SpecialEffectRows`（VerticalBox）和可选 `SpecialEffectsSection`；如当前 WBP 没有这个区域，只需添加绑定容器，无需 Graph。它们不混进普通属性。
5. 原 `DamageRows/KeyValueEntryClass` 继续显示各通道区间，`TotalDamageText` 显示总区间。既有两列缩放、品质背景、左侧优先视口定位和 Hover 生命周期不变。

数据 `FUmbraTooltipAffixDisplayData` 分开保存 AffixId、AttributeId/DamageBonus、Source、Unit、DisplayName、ValueText、bRolled、bHasActualValue、RolledValue、MinPossibleValue/MaxPossibleValue、TierCount、SelectedTier、RangeText、Style。固定词缀没有伪造随机范围；真实实例范围取生成时的 CandidateValues。

FromDefinition 只显示固定值与随机配置候选范围，`bHasActualValue=false`、SelectedTier=INDEX_NONE，不采样。FromInventory/FromEquipment 读取快照完整实例，替换配置预览。快照、GUID、装备或卸装后的刷新继续走现有事件，不新增数据 Tick；Tooltip 原有视口定位 Tick 保留。

启用新内容只需资产 Details：武器通道勾选 Use Damage Range 并填写端点；装备定义添加 Affix Roll Configs。旧配置不必迁移。需确认既有 WeaponDerivedPower/Equipment 开关与攻击 WeaponChannels 模式；不新增 Blueprint Graph。

## 验证边界

编译状态以 [Progress](Progress.md) 本轮记录为准。本轮按要求不新增、不运行自动化测试；未执行 PIE 或保存 Content。待 PIE：双通道总范围、固定旧武器、同定义不同 GUID 的档位、Hover/菜单/转移/重新注册稳定性、背包满/失败回滚、随机属性及 A/X 装卸恢复、单次暴击/扣血/飘字、拥有者复制与 Tooltip 样式。现有 Tooltip 测试仍有旧 StatEntries/X 混排断言，本轮未更新或宣称通过。
