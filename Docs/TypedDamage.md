# 九类型伤害与混合普攻继承（第四阶段）

第五阶段现已实现可选[A/X及新暴击/易伤](DamageBuckets.md)。本页关于“未实现A/X”和总暴击倍率的描述是第四阶段边界，仍适用于默认关闭Use Damage Buckets的兼容模式；开启后遵循第五阶段公式。

## 已实现的边界

统一入口`UmbraDamage::Apply`现在支持旧模式、武器类型继承、显式类型攻击三种模式。玩家/敌人普攻继续沿用现有命中时机、去重、攻速快照与攻击周期。新的Instant原生GE只运行一次TypedDamageExecution，九类型分别计算防御后汇总一次IncomingDamage；Health、死亡、受击与玩家伤害测量沿用现有链。

没有实现A/X区、易伤、新暴击1.5倍率、特殊AD/AP威能、随机词缀、穿透、盾、闪避/格挡或CombatInfo真实绑定。本阶段新模式暴击仍使用现有CriticalChance和CriticalDamageMultiplier（默认总倍率2），一次攻击只判定一次；不能把它当作未来的A区CriticalDamage。

## 攻击定义与迁移

[FUmbraPhysicalDamageConfig](../Source/Umbra/AbilitySystem/Damage/UmbraPhysicalDamage.h)为了保留现有BP序列化字段保持原名，新增`Typed`配置，见[类型定义与规则资产](../Source/Umbra/AbilitySystem/Damage/UmbraTypedDamage.h)。每个能力配置一种模式：

| Typed.Model | 原始类型伤害 | 配置与兼容契约 |
|---|---|---|
| Legacy（默认） | 原AD×AD系数 + AP×AP系数，统一按Physical/Magical处理 | 继续使用原DamageEffectClass与旧Execution；不使用类型抗性、Typed.Rules或Typed.CanCritical |
| WeaponChannels | 武器派生快照每个类型Damage × WeaponMultiplier | 默认倍率1；不再加AD/AP，也不读旧外层系数。要求有效DerivedStats，已包含装备需求惩罚与四维补正；空手读取UnarmedProfile |
| ExplicitChannels | 每类型BaseDamage + AD×该类型AD系数 + AP×该类型AP系数 | 不自动附加任何武器伤害；技能或强化普攻可使用。每种类型最多一条，至多九条；空数组是明确零伤害 |

解决默认“100% AD、0% AP”和继承混合武器冲突的方法是：武器继承模式采用一个WeaponMultiplier，默认100%继承全部类型；AD/AP系数只在显式类型模式下存在计算意义。旧外层系数只服务Legacy，不会把武器魔法伤害额外重复加一次。WeaponChannels的Channels数组必须为空，防止把两种模型的输入混填后静默丢失。

例如Slashing派生160、Fire派生70：Legacy外层AD系数1/AP系数0.2在零防御时仍是174；WeaponChannels、倍率1在零防御时是230。技能Fire通道Base10、AD系数0.5、AP系数2则为230点Fire原始伤害，绝不会再附加160+70。

新模式由入口选择[UUmbraTypedDamageEffect](../Source/Umbra/AbilitySystem/Damage/UmbraTypedDamageExecution.h)，不用创建或修改GE蓝图；原DamageEffectClass仅在Legacy使用。原GE上的额外标签、Cue或其它配置不会自动复制到原生Typed GE，迁移有这类配置的攻击前需单独处理。当前旧入口对Modifier/Execution的兼容校验保留。

Typed必须显式声明BasicAttack或Skill来源；不能使用LegacyUnspecified。来源标签不由GameplayAbility继承关系推断。武器模式也可用于明确设计的武器技能，但其来源仍为Skill。

## GAS与防御公式

在原[UmbraAttributeSet](../Source/Umbra/AbilitySystem/UmbraAttributeSet.h)新增九个真实使用的GAS属性：SlashingResistance、BluntResistance、PiercingResistance、FireResistance、LightningResistance、ColdResistance、RadiantResistance、PoisonResistance、ShadowResistance。全部默认0，单位是非负**抗性点数**，不是百分比；沿用AttributeSet的非有限/负数钳制、复制与变化委托。通过初始GE或Buff GE修改，当前装备普通词缀仍只支持四维，并未批量接入其它UI占位属性。

每类型计算：

`Raw = 该模式生成的类型伤害`

`DR_type = min(MaxTypeReduction, Rating / (Rating + TypeResistanceK))`

`Final_type = Raw × (暴击 ? 现有总暴击倍率 : 1) × K / (Defense + K) × (1 - DR_type)`

- Slashing/Blunt/Piercing使用Armor，其余六种使用MagicResistance。
- 默认K=100、TypeResistanceK=100、MaxTypeReduction=0.3。Rating25转换成20%减伤；Rating100计算50%后钳到30%。类型抗性减伤与一般防御的保留倍率相乘。
- 规则来自Typed.Rules指定的共享UmbraDamageRules DataAsset；为空时用C++默认对象。字段来源顺序：默认对象或指定规则资产 → 若存在DefenseKByLevel曲线则覆盖DefenseK。
- 曲线X为目标等级，Y为正数K。当前项目尚无等级成长模块，临时从目标EquipmentComponent.CharacterLevel读取（即使装备开关关闭，固定等级参数仍有效），无该组件时用1；不把技能Request.Level当防御方等级。未来升级系统应替换此单一取值入口。
- 两个K必须有限且大于0，减伤上限必须在0到1；无效配置拒绝整个请求。此转换是本阶段明确采用的可调规则，非最终平衡参数。
- AD/AP、暴击和防御由Execution进行GAS非快照捕获，命中执行时求值。武器通道从命中提交时的有效派生快照复制进独立Spec；本阶段入口同步提交，没有预制/延迟伤害Spec。武器快照与GAS汇总不一致时拒绝攻击，避免非法直接AD/AP写入被悄悄忽略。
- 多类型汇总使用double中间值，最终统一限制到float上限；不对每个类型提前截断。一次输出IncomingDamage，因此不产生多次扣血或重复死亡通知。

## 日志与飘字

继续使用`umbra.Damage.Log 1`：新模式每个非零通道打印`[DamageChannel]`（类型、基础项、AD/AP系数、暴击、两层防御及结果），随后一条`[DamageTyped]`汇总；Health结算仍有`[DamageHealth]`。旧模式继续`[Damage]`日志。

飘字协议仍只有物理/魔法两种颜色。新模式把总伤害显示一次，颜色暂取最终伤害占比更大的一侧，相等用物理色；暴击标志与总伤害共用同一次判定。颜色不用于决定伤害公式，也不宣称已实现九种类型飘字。真实视觉、客户端RPC及多人PIE未由无界面测试覆盖。

## 用户手动配置与验收

本轮不修改或保存Content资产。完整重编译后重新打开编辑器：

1. 在实际授予玩家的普攻GameplayAbility蓝图中找到DamageConfig → Typed，Model改为WeaponChannels，WeaponMultiplier=1，Channels保持空，CanCritical=true。不是在GE_Damage_PlayerBasic上切换；旧GE可以继续保留供Legacy使用。
2. 保留已经验收成功的PlayerState DerivedStats/Equipment设置及穿戴测试。使用AD160/AP70混合武器，目标Armor/MagicResistance与九种抗性均0、源CriticalChance=0：一次攻击应扣230，且只显示一个伤害数字。切回Legacy后原系数1/0.2仍为174。
3. 用目标初始GE配置Armor100、MagicResistance300、SlashingResistance100、FireResistance25；对于Slashing160/Fire70应得到56+14=70。将CriticalChance设1且旧CriticalDamageMultiplier保持2，应为140；Typed.CanCritical关闭应回70。
4. 抗性点数通过GE的对应Attribute填写，不要把30%填成0.3。要改变转换方式/上限，新建UmbraDamageRules资产，填写正K/TypeResistanceK、MaxTypeReduction=0.3，可选配置DefenseKByLevel，并在各相关攻击Typed.Rules引用同一资产。
5. 若测试显式技能或强化普攻，使用ExplicitChannels，添加Fire通道Base10、AD系数0.5、AP系数2；AD160/AP70、MR300、FireRating25时伤害46。来源为Skill的调用使用统一C++入口；项目本轮没有新增完整主动技能蓝图。
6. 敌人普攻也可按能力选择新模式；WeaponChannels必须启用敌人自己的DerivedStats，未配置武器时继承徒手Blunt10。保留旧模式可继续原有敌人攻击。验收前摇、攻速、单击一次、受击/死亡和血条/飘字行为。

## 验证与后续

新增`Umbra.Damage.TypedChannels`测试固定伤害算例、完整混合继承且不重复AD/AP、单次GE与Health变化、来源/测量/暴击/飘字标记、九属性逐项映射、两层防御和30%上限、非暴击攻击、武器倍率、显式技能、目标等级曲线、无效参数拒绝、零伤害及174旧算例。实际构建/回归结果见[Progress](Progress.md)。

下一阶段实施按来源/类型/状态筛选的A区、独立X区及CriticalDamageMultiplier的兼容迁移；之后接入CombatInfo真实数据。不要提前把本阶段总暴击倍率当成A区暴击伤害加成。
