# 物理与魔法伤害配置
架构见 [Architecture](Architecture.md)，配置总入口见 [EditorSetup](EditorSetup.md)，验证状态见 [Progress](Progress.md)。以下算例为待执行的验收步骤，2026-09-16 文档整理未运行测试或确认蓝图内部。
保留 FUmbraPhysicalDamageConfig 和 UmbraPhysicalDamageExecution 的原类名以兼容已有资产引用；它们现在同时支持物理和魔法伤害。

## 编辑器步骤
1. 关闭 UE 编辑器，编译 UmbraEditor Win64 Development，重新打开项目以加载新枚举和属性。
2. 打开 /Game/Blueprints/Abilities/Attack/GA_BasicAttack，Class Defaults，展开 Damage Config。
   - Damage Type：Physical 或 Magical。
   - Attack Power Coefficient：AD 系数。
   - Ability Power Coefficient：AP 系数。
   敌人攻击在 /Game/Blueprints/Abilities/Attack/GA_EnemyBasicAttack 的同名配置中修改。
3. Damage Effect Class 保持引用原伤害 GE：
   /Game/Blueprints/Abilities/Effects/GE_Damage_PlayerBasic 或 GE_Damage_EnemyBasic。
   GE 的 Duration Policy 必须为 Instant；Modifiers 清空；Executions 只有一个，Calculation Class 为 UmbraPhysicalDamageExecution。不要额外添加魔法 Execution 或扣血 Modifier。
4. 玩家法术强度：/Game/Blueprints/Player/BP_UmbraPlayerState -> Class Defaults -> Ability System / Debug Attributes。
   勾选 Use Debug Initial Attributes，展开 Debug Initial Attributes，设置 AbilityPower。
5. 敌人魔抗：/Game/Blueprints/Enemies/BP_Enemy_Melee_01 -> 同一分类 -> Magic Resistance。注意关卡实例可能覆盖蓝图默认值。
6. 若使用正式 Initial Attributes Effect，关闭调试开关，在该 Instant GE 中配置 UmbraAttributeSet.AbilityPower / MagicResistance，Operation 为 Override，Magnitude 为目标初始值。调试开关开启时会覆盖初始 GE 对应值。
7. Compile、Save，重新开始 PIE；调试初始值仅在服务器首次初始化应用。
8. 控制台 umbra.Damage.Log 1 开日志，0 关闭。输出 [Damage] 包含类型、AttackPower/AbilityPower、两种系数、抗性种类与数值、暴击和最终伤害；[DamageHealth] 显示生命前后与实际扣血。

## Spec 参数
现有攻击由 C++ 自动传递，无需蓝图再绑定：
Damage.Type：0 Physical、1 Magical；缺失、非整数、未知或非有限值记录错误并拒绝结算。
Damage.AttackPowerCoefficient、Damage.AbilityPowerCoefficient。
AP 系数缺失默认为 0。旧攻击配置新增字段默认 Physical、AP 系数 0，原字段名与默认值保留。
公式为 max(0, AttackPower*AD系数 + AbilityPower*AP系数) * 暴击倍率 * 100/(100+max(0,对应抗性))。
伤害类型只控制抗性，和缩放属性独立。暴击与 IncomingDamage 仍走原服务器链路。

## PIE 验收（需手动执行）
- 敌人关闭 Enable AI Behavior，Max Health 1000，Armor 100，Magic Resistance 300。
- 玩家 AttackPower 20、AbilityPower 40。攻击 AD 系数 2、AP 系数 0.5，玩家 Critical Chance 设为 0。
- Physical 单次命中应扣 30；Magical 应扣 15。确认每个命中只出现一次生命结算；连招可有多个独立命中。
- Magical 时改变 Armor 应不影响伤害；改变 Magic Resistance 为 0 后应扣 60。
- Physical 时改变 Magic Resistance 应不影响伤害；改变 AbilityPower 应按 AP 系数影响伤害。
- F2 锁定敌人，观察正常攻击实时更新生命。固定 10 点伤害按钮仍应扣 10，与双抗/暴击无关。
- 恢复旧攻击设置，Physical、AP 系数 0，确认原伤害不变。

本次无需新增 GE 或输入资产，未修改任何二进制资产；上述能力类型、系数和调试数值由用户在编辑器中配置。自动化结算测试为 Umbra.Damage.Types；它不代替实际地图 PIE 或联网测试。

基础伤害与 Can Crit 已移除。统一按角色 Critical Chance 判定暴击，Critical Damage Multiplier 决定暴击总倍率。重新打开编辑器后 Compile/Save 攻击蓝图；旧字段不再参与结算。固定10点调试伤害仍绕过该公式。

