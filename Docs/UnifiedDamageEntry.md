# 统一伤害入口：兼容阶段

2026-10-02。当前只统一提交入口，不实施装备、派生 AD/AP、九类型伤害、A/X 区或新暴击公式。

## 调用与兼容契约

- [FUmbraDamageRequest / UmbraDamage::Apply](../Source/Umbra/AbilitySystem/Damage/UmbraDamage.h) 要求调用方明确 Source。请求保留既有 `FUmbraPhysicalDamageConfig`、EffectClass 和 Level，不替换蓝图字段或 GE 资产。
- 玩家、敌人普攻均使用 `BasicAttack`，Spec 动态资产标签为 `Damage.Source.BasicAttack`。主动技能入口使用 `Skill` / `Damage.Source.Skill`；尚未新增具体主动技能。标签属于单次 Spec，不授予角色、不依据 GameplayAbility 身份推断来源。
- 旧 `UmbraPhysicalDamage::Apply` 保留为适配器。旧 `bPrimaryAttack=true` 映射 BasicAttack 并保留玩家测量标记；false 为 LegacyUnspecified，不添加两种来源标签，因为历史 false 也用于敌人普攻，不能推断为 Skill。
- 玩家请求显式设置 `bRecordPrimaryAttackDamage=true`，沿用 `Damage.SourcePrimaryAttack` SetByCaller。敌人普攻只有来源标签，没有玩家测量标记。Skill 不允许携带该标记。
- 只有新的 Apply 构造并提交 Spec。双方权威检查、Instant / 无 Modifiers / 唯一指定 Execution 校验、Context 来源、AD/AP 系数及类型传递保持原行为。Apply 返回 true 仅表示提交成功，不保证 GE 未被免疫/阻挡或造成正伤害。
- 每次调用提交一个 Spec；重复命中防护仍由技能中的实例和已命中状态负责。入口不增加第二套跨攻击去重，不改变前摇、攻速快照、周期、受击事件或死亡逻辑。
- 公式仍是 `max(0, AD*AD系数 + AP*AP系数) * 暴击总倍率 * 100/(100+双抗)`，仍通过一次 IncomingDamage 结算。CriticalDamageMultiplier 默认2.0、AttackSpeed直接倍率、初始化/调试GE、UI数值含义均未迁移。

## 参数来源

DamageEffectClass / DamageConfig 继续来自现有玩家、敌人 GA 的 Class Defaults；Level 来自能力等级。来源由能力调用处指定，不暴露第二份可冲突的蓝图设置。玩家测量标记仅由玩家普攻调用处开启。旧 Config 的 C++ 后备仍为 Physical、AD系数1、AP系数0。

## 自动化与手动验收

`Umbra.Damage.EntryCompatibility` 使用已保存的玩家伤害 GE 和临时 ASC：验证新旧入口和固定预期伤害、物理/魔法、非暴击/必暴、一次提交/一次Health变化、IncomingDamage归零、互斥来源标签、玩家测量标记、Level传递、旧false语义、零伤害和非法请求拒绝。

回归：`Umbra.Attributes.Lifecycle`、`Umbra.Damage.Types`、`Umbra.Combat.Maintenance`、`Umbra.UI.DamageNumberLogic`、`Umbra.UI.PlayerAttributeBarState`、`Umbra.UI.CharacterStats.PrimaryRows`。实际结果见 [Progress](Progress.md)，NullRHI测试不等同于地图或网络PIE。

本阶段无需蓝图重接或保存资产。完整构建关联引擎的 UmbraEditor Win64 Development 后，在 L_Prototype 中：

编辑器端验收由用户执行；下列步骤是待验收清单，不代表代理已完成。代码端构建及7项自动化结果已记录在Progress。

1. 查看 Output Log，开启 `umbra.Damage.Log 1`；使用既有普攻确认连续攻击每击只结算一次，血条、飘字和受击正常。
2. 验证前摇移动取消、出手后移动、敌人死亡后停止攻击；再确认敌人攻击玩家仍扣血。
3. 检查 AttackSpeed 变化下一击生效，MoveSpeed与生命/资源条不退化。
4. 双客户端验证服务端唯一扣血。测试结束将日志开关设0。

不为该阶段修改伤害GE的Execution、Modifiers或暴击数值。未来新公式另开迁移阶段，不能把当前入口统一误认为新伤害体系已实现。
