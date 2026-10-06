# 固定词缀与装备效果

本阶段扩展现有 ItemDefinition 和 EquipmentComponent，不生成随机词缀，不增加背包、卸装 RPC 或资产。四维仍使用 `PrimaryBonuses`，AD/AP 仍由 WeaponProfile 与四维派生，伤害仍通过统一入口。

## 配置来源与单位

`UUmbraItemDefinition.AttributeBonuses` 是固定加法词缀列表，同一装备每种 Stat 只允许一条，Magnitude 必须为有限非负数。多件装备的同种词缀通过 GAS 相加，再接受 AttributeSet 既有范围限制。

| Stat | 单位与作用 |
| --- | --- |
| MaxHealth、MaxResource | 上限点数；提升上限不会补满当前值，降低上限沿用现有钳制 |
| AttackSpeed | 攻速倍率增量，例如 0.2 表示增加 0.2；仍沿用普攻周期和既有上下限 |
| CriticalChance | 概率增量，0.1 为 10 个百分点 |
| Armor、MagicResistance | 防御 rating，加在装备固有防御之外 |
| 九种类型 Resistance | 各自类型抗性 rating，不是百分比 |
| MoveSpeed | UE 角色移动速度单位 cm/s 的增量 |

不提供直接 AD/AP、当前 Health/Resource、旧总暴击倍率、尚未接入实际逻辑的回复或占位属性词缀。装备固有 Armor/MagicResistance 先乘需求惩罚，再加完整固定防御词缀；四维、固定词缀和 A/X 不因四维需求未满足而减半。等级不足仍拒绝装备。

`DamageBonuses` 使用与普通 Buff 相同的 `FUmbraDamageBonus`：A 是比例（0.1 = +10%），X 是倍率（1.2 = ×1.2，0 会令匹配伤害归零）。支持类型、普攻/技能、暴击、易伤及双方标签条件。装备效果等级固定 1，曲线按 Level 1 求值；固定词缀本身没有曲线。类型列表不可重复；非法数值、类型或筛选枚举在穿戴前拒绝，不替换旧装备。

A/X 仅作用于明确开启 `Typed.bUseDamageBuckets` 的类型伤害路径，旧伤害公式保持原行为。CombatInfo 分类摘要共用此配置；条件项和 X 保留已有提示，不伪装成无条件总增伤，也不写回 AD/AP。

## 所有权与更新

每件装备仍只有一个原生 Infinite `UUmbraEquipmentEffect` 句柄，携带四维、固定词缀及固有防御 SetByCaller 值。其 `UUmbraEquipmentDamageBonusComponent` 通过 GE Context 的 SourceObject 读取 ItemDefinition 的 A/X。多个实例使用同一 GE 类但持有各自资产来源，不修改 GE CDO，也不共享可变角色数据。

应用新效果成功后才撤销被替换装备的旧句柄。卸下、组件 Shutdown/注销清理该句柄；A/X 的求值和面板汇总遵守 GAS 的移除和抑制状态。现有属性委托、效果通知、装备和派生快照负责 UI 更新，无 Tick。

ItemDefinition 是配置资产，穿戴期间不可通过运行时代码修改其词缀；需要改变配置时先卸下再穿戴。此阶段无实例词缀快照或随机生成。未来 Inventory 继续提供 Definition + InstanceId；实例独有词缀应另行设计，不应修改共享 DataAsset。SourceObject 资产引用随 GE Context 复制，但多人和跨组件复制时序尚待网络验收。

## UE 编辑器验收（用户有空时）

1. 使用本轮构建后的编辑器打开测试 ItemDefinition；保留 PrimaryBonuses、Weapon、AllowedSlots 等原配置。
2. 在 Equipment → Affixes 配置 AttributeBonuses，例如 MaxHealth=100、AttackSpeed=0.2、Armor=20；在 DamageBonuses 加 A=0.1、X=1.2（Bucket 选 Multiplicative）。无需额外创建或手工应用装备 GE。
3. 使用现有装备测试事件穿戴。确认装备槽位仍显示同一物品，StatsPanel 与 CombatInfo 重叠项一致，上限增加但不自动回血；攻速仍受已有范围限制。
4. 为便于独立检查伤害，撤销 8/9 测试 Buff、确认目标无其他易伤/抗性，使用已开启 A/X 的攻击。在非暴击、零防御、100 基础伤害条件下，上述 A/X 应得到 132；额外装备一件斩击 A=0.2 时斩击为 156。实际武器仍应代入自身基础伤害与补正，不能直接套用此固定 100 示例。
5. 重复替换不叠加；卸下第一件后只保留第二件斩击加成，100 斩击变为 120；全部卸下恢复原值。关闭/重开菜单确认无陈旧属性。需求不足时固有 Armor=100 使用默认 0.5 惩罚，但 Armor 词缀20完整生效，合计70。

代码自动化：`Umbra.Equipment.FixedAffixesAndDamage`，覆盖真实扣血、A/X 分类摘要、独立装备来源、需求惩罚、重复替换、非法词缀拒绝、卸下和 Shutdown 清理。具体本轮构建/回归结果见 [Progress](Progress.md)。真实 WBP 展示、PIE 按键以及多人网络不属于此自动化验证结果。
