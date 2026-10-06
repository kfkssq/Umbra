# CombatInfo 真实数据绑定（第六阶段）

2026-10-04：StatsPanel八个重叠属性已共用本文的读取与格式化逻辑，AD/AP补上派生事件通知；四主属性保留原订阅。装备槽位实时绑定见[EquipmentUIBinding](EquipmentUIBinding.md)。

本阶段扩展现有 UmbraCombatInfo / UmbraCombatStatEntry，保留WBP_CombatInfo、三分类、纸张背景、滚动和行布局，不修改Content资产。原[CombatInfoUI](CombatInfoUI.md)的测试文本方案仅作为历史布局说明；运行时TestValue不再作为真实数值。

## 数据与显示契约

| 行 | 来源 | 显示语义 |
|---|---|---|
| BaseWeaponDamage | DerivedStats Snapshot.Channels.BaseDamage之和 | 各类型基础值合计，含需求不足惩罚，不含主属性补正；徒手也使用快照 |
| AttackPower、AbilityPower | 武器派生启用时整份派生快照；旧模式GAS当前值 | 派生无效时显示—，不拿旧GAS值伪装新结果 |
| AttackSpeed | GAS倍率 / 普攻现有GetConfiguredBaseAttackInterval | 理论次/秒，两位小数，与第一页一致；不是百分比或实测攻速 |
| CriticalChance | GAS | 百分比 |
| AllDamage | 有效GE内无条件、全类型A条目 | 百分比，不含类型加成 |
| 九类型Damage | 有效GE内仅限制类型的A条目 | 百分比，不包含AllDamage |
| CriticalDamage、VulnerableDamage | 有效GE内仅要求暴击或易伤的全类型A条目 | 触发时才生效的额外A；不包含基础C/V或旧CriticalDamageMultiplier |
| MaxHealth、Armor、MagicResist、MaxResource、MoveSpeed | GAS当前值 | 移速cm/s；其余为数值 |
| 九类型Resistance | GAS当前值 | 抗性rating，不显示为减伤百分比 |
| EquipLoad、MaxEquipLoad | Equipment Snapshot | 仅穿戴重量和组件计算的容量；背包物品不计入 |
| HealthRegeneration、ResourceRegeneration、AbilityHaste | GAS存储值 | 数值后显示“仅存储”，尚无持续恢复/冷却玩法 |
| Thorn、DodgeChance、BlockChance、BlockReduction、ShieldStrength、HealingBonus、ShieldGeneration | 未实现 | “—（未实现）”，不伪装真实0 |
| 无上下文、无效快照、未配置普攻周期、未绑定行 | 数据不可用 | — |

全伤、类型、暴击和易伤是**分别展示的A分类**，不是各个完整命中总增伤。多个适用分类由战斗执行结算；UI不计算伤害。带额外来源/标签/组合条件的A项和所有X项均不压缩成这些标量。存在此类有效GE时A行追加“*”，Explanation说明排除规则，CombatValue暴露两个额外条件/X标志。来源标签变化不会把条件条目混入无条件行，所以这里无需标签轮询或目标选择。

A摘要计算位于现有伤害模块UmbraDamageBonuses::ReadPanelSummary，复用GE配置校验、等级、层数及抑制状态，不改变Evaluate命中结算。它显示的是有效新规则加成配置；某能力关闭UseDamageBuckets时不会使用这些加成。基础暴击/易伤倍率依然由该能力规则资产控制，未假定角色全局统一配置。

BaseWeaponDamage是惩罚后的有效基础值，不是ItemDefinition未经惩罚的原始卡片值。AD/AP与它统一读取快照，避免同一轮派生发布中的混合数值。装备、派生和GAS仍是独立复制对象，跨模块网络更新不是原子事务；本阶段未承诺多人UI原子刷新。

## 类与更新关系

- UmbraCombatStatData：独立CombatStat枚举、状态（Live/StoredOnly/Placeholder/Unavailable）、读取/格式化入口；不新增任何GAS Attribute。
- UmbraCombatInfo：发现现有行，订阅所属玩家ASC属性、GE添加/移除、各效果层数/时间/抑制变化及DerivedStats/Equipment快照。任一事件刷新现有行，无Tick、计时轮询或重新创建布局。
- UmbraCombatStatEntry：复用StatEntry的SetDisplayValue及Tooltip显示路径。CombatValue含数值、状态、文本、来源说明和条件标记；行自身不订阅GAS。
- Controller.OnRep_PlayerState通知嵌套CombatInfo；页面同时监听Pawn更换与ASC生命周期。未就绪时清空旧数据，重新就绪时重绑。Destruct解绑全部委托，重开读取最新值，分类折叠状态保留。
- Future Inventory仅负责物品所有权；穿戴后的视图仍通过Equipment快照，不直接遍历背包、不保存独立装备属性副本。

现有Buff等级在应用时确定。UE5.8直接SetActiveGameplayEffectLevel不会为无Modifier的A/X GE发通用数值变化委托；本阶段运行中升级此类Buff应移除后重施，或在明确修改入口调用页面NotifyPlayerContextChanged刷新。不得通过修改共享GE资产运行时数据更新实例。本阶段没有增加通用效果等级通知框架。

## 编辑器操作

1. 若编辑器仍开着，请保存关闭，在Rider构建**原项目**UmbraEditor Win64 Development，再打开。新增反射字段不依赖Live Coding；最新构建位置见Progress。
2. 保持WBP_CombatInfo父类UmbraCombatInfo、行父类UmbraCombatStatEntry。不复制页面、不重建原菜单Switcher、不改布局。
3. Attack/Defense原六个BindWidget保持；UtilityToggle（Button）、UtilityArrow（TextBlock）、UtilityRows（VerticalBox）是新增可选绑定。已手动搭好的第三组按这些名称设置变量；有旧Utility按钮蓝图折叠逻辑时移除重复逻辑，避免一击切换两次。
4. 每个行实例设置 **Combat UI → Binding → Combat Stat**，与上述行对应。**不要使用继承的Character Stats → Stat**配置第二页：它仍服务于第一页。StatName、图标、样式保持蓝图配置。
5. CombatStat=None时只按**精确Widget实例名称**匹配枚举，例如AttackPower、MagicResist、ResourceRegeneration；不读取中文标签，也不猜测WBP_CombatStatEntry_12等名字。推荐显式设置枚举，未匹配显示—。
6. TestValue仅用于Designer预览；不要在Construct/Tick/属性Text Binding中再次写测试数字或计算值。已有Apply Stat Value事件可继续负责视觉；它收到C++格式化文本。TooltipClass使用已有WBP_StatTooltip时自动显示Explanation；没有Tooltip也能看到“未实现”“仅存储”和*标记。
7. 原布局需容纳“仅存储”等文字，按需要在编辑器调整该文本样式，C++不改尺寸或字体。本轮没有保存任何Content，父类与绑定仍待用户确认。

## 最小PIE验收

- 打开属性菜单第二页，43行身份逐项对应；未实现条目不再显示测试数字，三组折叠独立，切页/关闭重开不重复行或委托。
- 修改MaxHealth/MoveSpeed/CriticalChance，验证即刻刷新；九类型抗性显示数值而非百分比。
- 四维变化/穿戴TestSword后核对派生快照的AD/AP和有效基础伤害；卸下读取徒手；负重、力量容量随装备事件更新。
- 按8应用测试Buff：AllDamage10%、SlashingDamage20%、CriticalDamage30%、VulnerableDamage20%（以资产实际配置为准）；按9撤销恢复。旧CriticalDamageMultiplier即使为9也不会显示为新CriticalDamage900%。
- 加X/Skill专属条件时A行出现*而不将它们错误相加；实际伤害仍用umbra.Damage.Log 1确认。敌人7/6易伤状态不改变“拥有的易伤加成”面板值，它只决定战斗触发条件。
- ASC暂时未就绪/PlayerState更换时不保留旧角色数值；重开菜单显示当前角色数据。

自动化与当前构建结果见[Progress](Progress.md)。多人网络、实际布局与PIE由用户验收。

