# 第一阶段属性集

当前入口见 [Architecture](Architecture.md)、[EditorSetup](EditorSetup.md)、[Progress](Progress.md)。下方历史验证仅代表当时结果。

## 配置

- 玩家实际使用的 PlayerState 蓝图：设置 Initial Attributes Effect。
- 每种敌人蓝图：分别设置 Initial Attributes Effect。
- 两者使用不同的 Instant Gameplay Effect；建议用 Override 指定初始常驻属性。
- 初始 GE 不要配置 Health、Resource、IncomingDamage，也不要依赖激活条件或免疫判定。全部初始修饰应用完成后，C++ 单独填满生命和资源。
- 未指定初始 GE 且未启用调试覆盖时沿用 C++ 后备值：生命/资源上限及当前值100，攻击力10，暴击总倍率2，其余非移速属性0。AttributeSet 原生移速后备500；角色首次 ASC 绑定时取已有 MaxWalkSpeed 作兼容初值（玩家原生500、敌人 EnemyMoveSpeed 原生300，BP可覆盖），随后由初始 GE/Debug 覆盖。MoveSpeed 已通过 ASC 驱动实际角色移动。
- 攻速加成、暴击率为小数比例；恢复为点/秒；移速为厘米/秒；技能急速为数值。GAS BaseValue 只是聚合输入，不是成长系统的数据模型。
- 原 Mana/MaxMana 已通过 Core Redirects 映射至 Resource/MaxResource。打开相关旧 GE/蓝图，确认引用正确并编译保存；本次没有改写二进制资产。
- 现有负 Health 的 Instant 伤害 GE 可以继续使用。若迁移至 IncomingDamage，使用正数 Additive，并移除同一 GE 原有的负 Health 修饰，避免双重扣血。
- IncomingDamage 只用于即时或周期执行，不能作为无周期的持续属性加成。当前生命/资源的消耗和补充使用 Instant GE；临时上限变化配置在 MaxHealth/MaxResource。
- 当前属性已接入伤害公式、移动速度、玩家普通攻击速度、血条、飘字与调试面板；尚未绑定恢复计时器、技能急速冷却、装备或角色成长系统。伤害、攻速和 UI 流程见 Architecture。

## 生命周期

玩家在现有 InitializeAbilitySystem 中绑定 ASC 后初始化，敌人在 BeginPlay 绑定后初始化。
ASC 的 authority-only 标志保证每个 ASC 生命周期只初始化一次；重新绑定 Avatar 不回血。
新 PlayerState/新敌人拥有新的 ASC，可以重新初始化；复用旧 ASC 的重生不会隐式复活，需要以后明确的重生流程。

PreAttributeBaseChange 约束即时基础数值；PreAttributeChange 约束持续效果添加、移除及聚合重算后的当前值。
上限降低时裁剪池，上限增加不补充。不会在每次 GE 执行后把所有 CurrentValue 写回 BaseValue，避免临时增益残留。
IncomingDamage 先读出并归零，再扣血，兼容 Health 变更委托触发的死亡逻辑。
15 个常驻属性均提供 C++ GAS 访问接口、BlueprintReadOnly、复制及 RepNotify；IncomingDamage 不复制。

## 最短验收

1. 为玩家/敌人分别配置 MaxHealth=150/80、MaxResource=100/0 的初始 Instant GE。PIE 中观察双方初始值满额；先扣玩家血，再重新 Possess/绑定同一 PlayerState，确认不回血。
2. 添加一个有限时长 GE：CriticalChance +2、Armor -100。确认暴击率不超过 1、护甲不小于 0；手动移除或等到期后恢复原值。加成期间执行无关 Instant GE，再移除加成，确认不会残留。
3. 生命 55/100 时添加 MaxHealth +100，仍为 55/200；治疗到 200，移除增益后为 100/100。添加 MaxHealth -60 后为 40/40，移除后为 40/100。资源按相同方式验证，最大资源允许 0。MaxHealth 的下限为 1。
4. 用已有玩家与敌人普攻命中，检查每次命中只扣一次生命、受击与敌人死亡仍正常。另用 Instant GE 添加 IncomingDamage=20，检查扣 20、该字段归零；再应用无关 GE，确认没有再次扣血。
5. 两客户端 PIE 检查双方常驻属性同步、重新绑定不初始化；IncomingDamage 不应出现在属性复制列表中。

自动化入口：Session Frontend → Automation → Umbra.Attributes.Lifecycle。
自动化使用真实 ASC/GE 和独立 Game World，覆盖默认初始化、防重复初始化、旧负 Health 伤害、IncomingDamage、有限时长/无限效果手动移除与多种边界；不等同于地图 PIE 或网络测试。

## 本次验证结果（2026-09-14）

- UE 5.8.2：UmbraEditor / Win64 / Development 编译成功。
- Umbra.Attributes.Lifecycle：Success，无测试事件错误，编辑器命令行进程退出码 0。
- git diff --check：通过。未修改二进制资产，未提交或推送。
- 日志：Saved/Logs/UmbraAttributesTests.log（生成文件，不提交）。
- 未执行地图 PIE、实际普攻/受击/死亡动画、多客户端复制、有限时长自然到期、蓝图旧引用重保存和实际初始 GE 资产配置测试。有限时长 GE 的添加及手动移除已覆盖。
- 编译有 MSVC 版本非首选及引擎 GetMovementBase 弃用警告。编辑器启动时另有引擎自动化注册的 Condition failed 日志；本项目测试 BeginEvents/EndEvents 内无错误，测试结果为 Success。
