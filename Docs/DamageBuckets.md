# A/X 增伤、暴击与易伤（第五阶段）

## 模式与公式

在已有WeaponChannels/ExplicitChannels模式中新增`DamageConfig.Typed.Use Damage Buckets`，默认false。关闭时保持第四阶段：使用原CriticalDamageMultiplier总倍率、不读取A/X或易伤。Legacy模式始终保持旧公式，不受此开关影响。新规则必须显式开启，不自动改写已经验收的能力蓝图。

开启后，每个伤害类型使用：

`D = Raw × (1 + Σ适用A) × ∏适用X × C × V × K/(Defense+K) × (1-DR_type)`

- Raw及两层防御沿用[TypedDamage](TypedDamage.md)，武器补正/惩罚不重复计算。
- 暴击整次命中判定一次，仍读取CriticalChance与CanCritical。C为规则资产BaseCriticalMultiplier，默认1.5；未暴击为1。
- 目标具有`State.Vulnerable`（含其子标签）时V为BaseVulnerableMultiplier，默认1.2，否则1。普通易伤不叠乘多个基础V；多个授予易伤的GE按GAS标签计数持有，最后一个移除后失效。
- 新规则**不读取CriticalDamageMultiplier作为伤害倍率或A项**。该属性保留供Legacy/第四阶段兼容使用；旧总倍率2不能直接转换成A区+200%。如要迁移具体旧数值，应由设计明确配置新基础C及额外暴击A，本阶段不自动转换。
- BaseCriticalMultiplier/BaseVulnerableMultiplier位于已有UmbraDamageRules；Typed.Rules为空时用C++默认1.5/1.2，指定资产时用资产值。必须有限且至少1；仅在开启新规则时校验/使用。

## 加成数据及GAS所有权

新增[UmbraDamageBonusComponent](../Source/Umbra/AbilitySystem/Damage/UmbraDamageBonusComponent.h)，它是**GameplayEffect内部组件**，不是角色ActorComponent。添加到持续型/无限型GE的Components列表，显示名为`Umbra Damage Bonuses (A/X)`。每个GE拥有不可变Bonuses配置；实际生效状态由攻击者ASC的ActiveGameplayEffect句柄、等级、层数及Inhibition决定。命中时一次读取当前有效GE，为九个类型生成A/X数组；没有Tick或重复写入AD/AP。

每条FUmbraDamageBonus包含：

| 字段 | 语义 |
|---|---|
| Bucket=Additive | A区，Magnitude=0.1表示+10%；符合条件的项加到同一个A中 |
| Bucket=Multiplicative | X区，Magnitude=1.1表示×1.1；1是中性，0表示该项把符合条件的伤害归零 |
| Magnitude | FScalableFloat，常数或CurveTable按该Buff GE等级求值；不是攻击/目标等级 |
| Types | 空数组匹配全部九类型，指定数组只匹配对应类型；禁止重复/非法类型 |
| AttackSource | Any、BasicAttack、Skill；读取本次伤害Spec的明确来源标签，不按GameplayAbility父类判断 |
| RequiresCritical | 只有这次统一暴击判定成功时适用 |
| RequiresVulnerable | 只有目标当前易伤时适用 |
| SourceRequirements / TargetRequirements | 对双方当前持有标签的Require/Ignore/TagQuery条件；不把GE资产标签当角色状态标签 |

AllDamage就是无条件A项；各类型增伤就是限制Types的A项；CriticalDamage是RequiresCritical的A项；VulnerableDamage是RequiresVulnerable的A项。普攻/技能增伤限制AttackSource。其它明确指定条件的普通增伤可组合这些字段。X使用同一筛选条件，但进入独立乘积。

普通伤害加成没有批量扩成UI对应GAS Attribute。它们仍由GAS的GE管理，是条件化效果数据；未来UI应区分无条件面板加成与本次命中满足条件的加成，不能把所有条目无条件求和显示成真实伤害。已有AttributeSet的Health、移动、四维、AD/AP、攻速等不变。

现有读取CriticalDamageMultiplier的面板仍表示旧总倍率，不表示新基础C或A区暴击加成；本阶段未替换其接线，开启新规则后请用规则资产与伤害日志核验，后续CombatInfo绑定会区分这些语义。

## 添加、移除、叠层和校验接口

- 权威端通过既有ASC `MakeOutgoingSpec` / `ApplyGameplayEffectSpecToSelf`，或蓝图`Apply Gameplay Effect to Self`应用配置好的Buff GE；保存返回的ActiveGameplayEffectHandle。
- 通过`RemoveActiveGameplayEffect(Handle)`撤销这个来源；按指定层数移除仍遵守GAS语义。装备威能将来持有自己的句柄，不允许按全局属性回退固定值。当前EquipmentComponent不会自动读取新威能配置。
- HasDuration由GAS管理到期，Infinite由来源显式移除；被Inhibit的GE完全不参与，解除后重新参与下一次命中。条目没有独立计时器，普通GE的时间/刷新/堆叠策略仍由GE配置。
- 每层都贡献一次：N层A为`N×Magnitude`；N层X为`Magnitude^N`。不同有效句柄的X继续相乘。不需要叠加的威能应通过GE堆叠策略/层数上限约束，而不是用普通GAS乘法聚合替代独立X乘积。
- 仅允许非周期的持续/无限GE（Period=0）；Instant、非零Period、负数/非有限倍率及非法筛选数据在应用时拒绝。负增伤/削弱不在本阶段范围；X可使用0到1的倍率表达独立减伤效果，但这里始终是攻击者的输出效果。
- 命中时再次检查有效配置和层数，运行中修改共享GE资产不受支持；数据或double乘积溢出时整个命中失败，不提交半个类型结果。最终有限总和仍按原规则截到float上限。
- `State.Vulnerable`可由目标身上的GE通过原生`Grant Tags to Target Actor`组件授予。不要把标签仅填进Asset Tags。本阶段没有替用户创建或保存易伤GE蓝图。
- 未来CombatInfo订阅ASC的效果添加、移除、层数、抑制及标签变化事件；本阶段没有实现UI订阅，不使用Tick。

## 配置实例与预期数值

攻击含Slashing100和Fire50，目标无防御；攻击者有All A0.1、Slashing A0.2、暴击 A0.3、易伤 A0.2：

| 条件 | Slashing | Fire | 总伤害 |
|---|---:|---:|---:|
| 无暴击、无易伤 | 100×1.3=130 | 50×1.1=55 | 185 |
| 暴击、无易伤 | 100×1.6×1.5=240 | 50×1.4×1.5=105 | 345 |
| 无暴击、易伤 | 100×1.5×1.2=180 | 50×1.3×1.2=78 | 258 |
| 暴击且易伤 | 100×1.8×1.5×1.2=324 | 50×1.6×1.5×1.2=144 | 468 |

再加无条件X1.1与仅Slashing的X1.25（条件也满足），最后一行变为`324×1.1×1.25 + 144×1.1 = 603.9`。不是把10%与25%放进A，也不是X=1+0.1+0.25。

基础C/V不会写回AD/AP，旧CriticalDamageMultiplier即使为9，也不会在开启新规则时参与计算。关闭新规则则恢复旧总倍率语义，保证可比较迁移结果。

## 编辑器手动操作

第五阶段最初使用独立副本验证；最新构建状态以[Progress](Progress.md)为准。新增反射类时请保存并关闭编辑器，在Rider构建原项目UmbraEditor Win64 Development，再重新打开，不要通过Live Coding新增反射类。

1. 打开实际使用的普攻能力，保持Typed.Model=WeaponChannels，勾选`Use Damage Buckets`。先不应用增伤GE，将CriticalChance=1、目标无易伤/防御；默认新暴击应为基础伤害×1.5。关闭该开关应恢复旧总暴击倍率。
2. 创建GameplayEffect蓝图`GE_TestDamageBonuses`，Duration Policy设Infinite，Period=0。在Components添加`Umbra Damage Bonuses (A/X)`，Bonuses添加上述四条A：0.1全伤、0.2限制Slashing、0.3勾RequiresCritical、0.2勾RequiresVulnerable。Magnitude通常在ScalableFloat的Value字段填写小数。
3. 使用Controller的开发快捷键：`8`向玩家PlayerState的ASC应用一次Level 1测试GE，`9`按保存的句柄撤销；配置与蓝图替代接线见下节。换装测试与Buff测试分别保存各自句柄。
4. 创建`GE_TestVulnerable`，Infinite或指定持续时间。在Components添加`Grant Tags to Target Actor`，Add Tags添加`State.Vulnerable`。鼠标指向敌人按7应用，按6撤销，详细规则见下节。移除最后一个此类GE后，基础易伤倍率和易伤A同时不再生效。
5. 在Buff的Bonuses中添加Multiplicative项，Magnitude=1.1，再添加Types仅Slashing、Magnitude=1.25的X项。需要条件时填写RequiresCritical/RequiresVulnerable及双方Tag Requirements。核对各类型匹配，不让仅Skill的项增益普通攻击。
6. 用ExplicitChannels配置Slashing100、Fire50，或使用能派生出相同明细的武器，按上表验收。实际武器若AD/AP不同，请使用其真实类型明细重算，不套用表中固定结果。
7. `umbra.Damage.Log 1`的DamageChannel现在包含A、X、C、V；DamageTyped包含Buckets开关、统一Crit及Vulnerable。仍一次总伤害飘字；A/X的配置与实际条件结果以日志为准。

## 测试GE快捷入口

`AUmbraPlayerController`默认启用`Enable Quick Gameplay Effect Keys`，普通数字键8应用，9撤销，无需新增Input Action。`Quick Test Gameplay Effect`默认软引用`/Game/Blueprints/GameplayEffect/GE_TestDamageBonuses.GE_TestDamageBonuses_C`；可在BP_UmbraPlayerController的Class Defaults → Debug → Quick Gameplay Effect改选其它持续或无限GE。蓝图保存的覆盖值优先，C++默认引用后备；Level固定1。资产内容、Bonuses和实际按键分发仍需编辑器确认。

C++检查权威端及ASC就绪后才应用。效果仍有效时再次按8返回同一`Quick Test Gameplay Effect Handle`，不新增实例或刷新时间；到期/外部移除后允许重新应用。按9只移除本入口保存的效果并清空句柄，重复撤销无副作用。同类GE若已由其它入口添加则拒绝重复应用，不接管或撤销其它来源。Controller结束时清理保存效果；切换PlayerState后下一次应用先清理旧ASC效果。

如需保留自己的蓝图按键事件，先关闭原生快捷键开关，再将8 Pressed接`Apply Quick Test Gameplay Effect`，9 Pressed接`Remove Quick Test Gameplay Effect`，Target为当前Controller。句柄已经以只读变量暴露，不必再搭一条直接Apply链。不要同时保留旧8键Apply逻辑，避免蓝图输入抢占或旁路重复应用。

快捷键仅用于非Shipping、非Test构建的本地Controller，应用/撤销仍要求Authority；Standalone或监听服务器本地玩家可使用，远端客户端不会自动RPC到服务器。未使用Tick。Output Log筛选`Quick GE:`可看到应用、已存在、撤销或拒绝原因。原项目重建后即可使用；此改动没有修改蓝图资产。

## 敌人易伤快捷键

同一`Enable Quick Gameplay Effect Keys`开关控制数字键7/6。7即时使用现有Pawn光标查询及Visibility遮挡检查选取可攻击的UmbraEnemyCharacter，不依赖F2面板锁定，不在Tick应用。鼠标没有指向有效敌人时保留上次效果并打印提示。6无需鼠标继续指向敌人，直接撤销保存的敌人效果；8/9的玩家Buff句柄完全独立。

`Quick Vulnerable Gameplay Effect`默认软引用`/Game/Blueprints/GameplayEffect/GE_TestVulnerable.GE_TestVulnerable_C`，BP_Controller类默认值可覆盖。C++从玩家PlayerState ASC创建Level1 Spec并应用到敌人ASC，玩家保留为效果来源；同一敌人重复应用返回原句柄。换敌人时先检查目标和配置，再撤销上一目标测试效果并应用新效果；若GAS最终拒绝新应用，旧效果已撤销，以日志为准。仅同时保留一个此入口的敌人效果，不接管其它来源同类GE；外部移除后可重加，EndPlay清理保存句柄。不存在客户端自动RPC。

蓝图可关闭原生快捷键后调用`Apply Quick Vulnerable Gameplay Effect(Target)` / `Remove Quick Vulnerable Gameplay Effect`，句柄通过`Quick Vulnerable Gameplay Effect Handle`只读暴露。Blueprint显式Target调用不执行光标/遮挡查询，但仍校验敌人、可攻击状态、权威和ASC就绪。避免重复7/6键接线。

C++只应用已配置GE，不自动补写`State.Vulnerable`。请在编辑器确认GE授予目标标签而非仅配置Asset Tags。开启`Use Damage Buckets`和`umbra.Damage.Log 1`后，对敌人应用易伤再攻击，检查DamageTyped的Vulnerable及DamageChannel的V；撤销最后一个易伤来源后再次攻击恢复。其它GE仍授予易伤时，撤销本测试效果不会强行清除标签。

## 验证和后续边界

新增`Umbra.Damage.BucketsAndMigration`测试上述四组合、来源隔离、混合类型X筛选、双方标签、叠层/单层移除、Inhibit/恢复、Buff撤销、可配置基础C/V、禁止暴击、旧总倍率兼容、非法效果拒绝与单次扣血。最终构建/回归证据见[Progress](Progress.md)。

没有实现完整传奇威能、特殊百分比AD/AP、装备词缀生成、伤害减益A或CombatInfo真实绑定。本阶段只提供通过GE管理的加成及统一结算。下一步再让CombatInfo读取明确的数据来源并订阅变化，保留条件加成的语义。
