# Umbra 架构与维护地图

正式角色属性面板的数据流与 Editor 接线见 [CharacterStatsPanel](CharacterStatsPanel.md)。`UUmbraCharacterStatsPanel` 观察本地 PlayerState 的 ASC 委托，`UUmbraStatEntry` 通过 Blueprint 事件更新格式化数值，图标 Brush 由条目 WBP 设置，`UUmbraStatTooltip` 单独显示属性名称及说明；根 HUD 由既有 PlayerController 创建。

依据：2026-09-16 仓库 C++、配置、维护修复及回归检查。资产文件存在不代表其 Graph 或全部引用已确认；专项自动化读取的资产范围见 [Progress](Progress.md)。配置见 [EditorSetup](EditorSetup.md)。本文记录现有实现，不要求建立新的框架。

## 模块与关键类

只有一个游戏运行时模块 `Umbra`。下表路径均相对仓库根；同名 `.h` 声明契约，`.cpp` 实现行为。

| 职责 | 类 / 入口 | 准确实现路径 |
| --- | --- | --- |
| 默认 PlayerState 类 | `AUmbraGameMode` | [Source/Umbra/UmbraGameMode.cpp](../Source/Umbra/UmbraGameMode.cpp) |
| 玩家 GAS 所有者、一次性授予能力 | `AUmbraPlayerState` | [Source/Umbra/Player/UmbraPlayerState.cpp](../Source/Umbra/Player/UmbraPlayerState.cpp) |
| ASC 初始化、输入队列、攻击意图 RPC、就绪通知 | `UUmbraAbilitySystemComponent` | [Source/Umbra/AbilitySystem/UmbraAbilitySystemComponent.cpp](../Source/Umbra/AbilitySystem/UmbraAbilitySystemComponent.cpp) |
| 属性、边界、复制、最终扣血 | `UUmbraAttributeSet` | [Source/Umbra/AbilitySystem/UmbraAttributeSet.cpp](../Source/Umbra/AbilitySystem/UmbraAttributeSet.cpp) |
| 初始调试覆盖数据 | `FUmbraDebugInitialAttributes` | [Source/Umbra/AbilitySystem/UmbraDebugInitialAttributes.h](../Source/Umbra/AbilitySystem/UmbraDebugInitialAttributes.h) |
| GE 校验、构造并发送伤害 Spec | `UmbraPhysicalDamage::Apply` / `FUmbraPhysicalDamageConfig` | [Source/Umbra/AbilitySystem/Damage/UmbraPhysicalDamage.cpp](../Source/Umbra/AbilitySystem/Damage/UmbraPhysicalDamage.cpp) |
| 物理/魔法公式 | `UUmbraPhysicalDamageExecution` | [Source/Umbra/AbilitySystem/Damage/UmbraPhysicalDamageExecution.cpp](../Source/Umbra/AbilitySystem/Damage/UmbraPhysicalDamageExecution.cpp) |
| 结算后的表现路由快照 | `FUmbraDamageNotification` | [Source/Umbra/AbilitySystem/Damage/UmbraDamageNotification.cpp](../Source/Umbra/AbilitySystem/Damage/UmbraDamageNotification.cpp) |
| 技能输入标签与激活策略 | `UUmbraGameplayAbility` | [Source/Umbra/AbilitySystem/UmbraGameplayAbility.h](../Source/Umbra/AbilitySystem/UmbraGameplayAbility.h) |
| 玩家连招、目标命中检测 | `UUmbraBasicAttackAbility` | [Source/Umbra/AbilitySystem/Abilities/UmbraBasicAttackAbility.cpp](../Source/Umbra/AbilitySystem/Abilities/UmbraBasicAttackAbility.cpp) |
| 敌人近战与移动锁定 | `UUmbraEnemyBasicAttackAbility` | [Source/Umbra/AbilitySystem/Abilities/UmbraEnemyBasicAttackAbility.cpp](../Source/Umbra/AbilitySystem/Abilities/UmbraEnemyBasicAttackAbility.cpp) |
| 敌人受击动画 | `UUmbraHitReactAbility` | [Source/Umbra/AbilitySystem/Abilities/UmbraHitReactAbility.cpp](../Source/Umbra/AbilitySystem/Abilities/UmbraHitReactAbility.cpp) |
| 动画命中窗口事件 | `UUmbraAnimNotifyState_AttackHitWindow` | [Source/Umbra/Animation/UmbraAnimNotifyState_AttackHitWindow.cpp](../Source/Umbra/Animation/UmbraAnimNotifyState_AttackHitWindow.cpp) |
| 玩家攻击安全衔接点事件 | `UUmbraAnimNotify_AttackChainPoint` | [Source/Umbra/Animation/UmbraAnimNotify_AttackChainPoint.cpp](../Source/Umbra/Animation/UmbraAnimNotify_AttackChainPoint.cpp) |
| 标签定义（跨类契约） | `UmbraGameplayTags` | [Source/Umbra/GameplayTags/UmbraGameplayTags.cpp](../Source/Umbra/GameplayTags/UmbraGameplayTags.cpp) |
| 玩家移动、朝向、Avatar 绑定 | `AUmbraPlayerCharacter` | [Source/Umbra/Characters/UmbraPlayerCharacter.cpp](../Source/Umbra/Characters/UmbraPlayerCharacter.cpp) |
| 敌人 ASC、死亡、高亮、能力授予 | `AUmbraEnemyCharacter` | [Source/Umbra/Characters/UmbraEnemyCharacter.cpp](../Source/Umbra/Characters/UmbraEnemyCharacter.cpp) |
| 寻敌、追击、返程与攻击时机 | `AUmbraAIController` | [Source/Umbra/AI/UmbraAIController.cpp](../Source/Umbra/AI/UmbraAIController.cpp) |
| 可攻击性与高亮接口 | `IUmbraAttackable` | [Source/Umbra/Interfaces/UmbraAttackable.h](../Source/Umbra/Interfaces/UmbraAttackable.h) |
| 指针输入、导航、选敌、自动攻击与最新指令协调、调试高亮 | `AUmbraPlayerController` | [Source/Umbra/UmbraPlayerController.cpp](../Source/Umbra/UmbraPlayerController.cpp) |
| 调试输入、授权与 GE 句柄 | 同一 Controller 的实现分文件 | [Source/Umbra/UmbraPlayerControllerDebug.cpp](../Source/Umbra/UmbraPlayerControllerDebug.cpp) |
| 飘字 RPC、投影、Widget 管理 | 同一 Controller 的实现分文件 | [Source/Umbra/UmbraPlayerControllerDamage.cpp](../Source/Umbra/UmbraPlayerControllerDamage.cpp) |
| 调试属性观察 | `UUmbraAttributeDebugPanel` | [Source/Umbra/UI/UmbraAttributeDebugPanel.cpp](../Source/Umbra/UI/UmbraAttributeDebugPanel.cpp) |
| 敌人血条与目标注入 | `UUmbraEnemyHealthBar` / `UUmbraEnemyHealthBarComponent` | [Source/Umbra/UI/UmbraEnemyHealthBar.cpp](../Source/Umbra/UI/UmbraEnemyHealthBar.cpp)、[Source/Umbra/UI/UmbraEnemyHealthBarComponent.cpp](../Source/Umbra/UI/UmbraEnemyHealthBarComponent.cpp) |
| 飘字格式与动画 | `UUmbraDamageNumber` | [Source/Umbra/UI/UmbraDamageNumber.cpp](../Source/Umbra/UI/UmbraDamageNumber.cpp) |
| 固定调试效果 | `UUmbraDebugAttributeEffect` / `UUmbraDebugDamageEffect` / `UUmbraDebugHealEffect` | [Source/Umbra/AbilitySystem/Effects/UmbraDebugEffects.cpp](../Source/Umbra/AbilitySystem/Effects/UmbraDebugEffects.cpp) |

`Umbra.Build.cs` 引用 GAS、EnhancedInput、NavigationSystem、AIModule、UMG/Slate 和 StateTree 模块；当前列出的 AI C++ 流程由 Tick 驱动，不能因启用了 StateTree 插件就认定蓝图在使用它。`Source/Umbra/UmbraCharacter.h/.cpp` 是仍保留的第三人称移动/跳跃/环绕相机类，和 top-down `AUmbraPlayerCharacter` 各自直接继承 ACharacter；不可在没有编辑器引用检查时删除前者。

## 数据归属与原因

- 玩家：PlayerState 创建并拥有 ASC / AttributeSet，Character 是 Avatar。换 Pawn 不等于换属性容器；此布局支持属性与技能跨 Pawn 绑定存续，但**当前没有实现完整重生**。
- 敌人：Character 自己拥有 ASC / AttributeSet，生命周期随敌人结束；ASC 使用 Minimal 效果复制，玩家使用 Mixed。常驻属性由 AttributeSet 显式复制；是否在实际网络地图正确工作需 PIE。
- 15 个常驻属性是 GAS 当前数据源；`AttackSpeed` 直接存攻速倍率，1.0 是基础攻速。`IncomingDamage` 是瞬时结算中间值，不复制、不展示、不当作持续增益。技能只持有本次攻击目标、连招进度、攻速快照与命中集合。
- Controller 拥有本地指针操作状态与 UI 实例；UI 持有弱目标和委托句柄，不另存一套战斗数值。血条比例、百分比文字和显示舍入属于表现计算，不是重复伤害结算。
- 调试面板的 GE 句柄由服务器 Controller 按目标 ASC 保存，仅清除自己创建的增益。调试按钮不负责属性初始化。

## 主要流程

### 1. 出生与属性初始化

玩家 `PossessedBy` / `OnRep_PlayerState` → Character::InitializeAbilitySystem → 清输入队列 → ASC::InitAbilityActorInfo(PlayerState, Character) → PlayerState::InitializeAttributes / GrantInitialAbilities。

敌人 `BeginPlay` → 设置实际移动速度 → ASC::InitAbilityActorInfo(this, this) → InitializeAttributes → 订阅 Health → 权威端授予受击技能及配置的普攻。

首次 InitAbilityActorInfo 在权威端把 Character 原有 MaxWalkSpeed 作为 MoveSpeed 的兼容后备值。ASC::InitializeAttributes：AttributeSet 构造后备值（MoveSpeed 使用上述兼容值）→ 初始 Instant GE → 非 Shipping 下可选的整组 Debug Override GE → SetNumericAttributeBase 填满 Health/Resource。`bAttributesInitialized` 使同一 ASC 只初始化一次；重复绑定不会回血或重设移速。初始 GE 应简单、无拒绝条件；当前代码在施加 GE 前设置初始化标志，不能依赖失败后重试。

MoveSpeed 由 ASC 监听属性变化，统一写当前 Avatar 的 CharacterMovement.MaxWalkSpeed；玩家同时按共享的移动动画参考移速事件驱动更新 CharacterMovement.RotationRate.Yaw，参考点默认500cm/s→动画1.0x及Yaw 600°/s，Yaw上限1800°/s。AttributeSet 与调试结构的 MoveSpeed 后备也统一为500；敌人首次绑定仍以自己的 EnemyMoveSpeed（默认300）播种。初次绑定立即同步，换 Avatar 时先解绑再绑定，ClearActorInfo/OnUnregister 清理。修改速度不恢复 MovementMode 或朝向所有权，因此死亡/攻击锁移动仍生效；敌人转向不走玩家倍率。旧 EnemyMoveSpeed 和玩家 BP 的 MaxWalkSpeed 仅作兼容初值，正式调参以 GAS 为准。

运行时 GE 聚合变化 → AttributeSet::PreAttributeBaseChange / PreAttributeChange 限制合法范围；PostAttributeChange 在上限降低时裁剪池，上限增加不补血。不要把聚合后的所有 CurrentValue 回写 BaseValue，否则会固化临时增益。

### 2. 输入到玩家攻击

Controller::SetupInputComponent 注册 PrimaryAction / PrimaryAttackAction。PrimaryAction 负责地面短按寻路、长按移动；命中可攻击目标时进入 Ability 上下文并返回，真正选敌攻击由独立 PrimaryAttackAction::Started → BeginAttackTarget → UpdatePendingAttack 负责。**两个 Action 的实际按键与触发关系必须在 IMC 中确认**，不能只接空的 HandlePrimaryAbilityPressed。

点击目标后 Controller 保存 PendingAttackTarget；距离使用 Character.IsTargetInPrimaryAttackRange（PrimaryAttackRange 加目标胶囊半径），超出范围沿现有导航追近。客户端通过 ASC RPC 传意图，服务器核对目标；玩家伤害只由服务端前摇计时器结算。自动攻击按本击起手时间加逻辑周期续击，出范围时结束当前 Ability 并恢复 Controller 追击。

Controller 只保留一条待执行指令。移动立即停止自动攻击并取消当前前摇或后摇；出手后的伤害与 ASC 已提交的攻击间隔仍保留。死亡、眩晕、外部 CancelAllAbilities、Controller EndPlay 和 Avatar 更换清理计时器、能力任务与目标。

BasicAttackAbility 每击快照原目标、倍率、周期、前摇和动画。服务端在前摇结束核对实例/存活/攻击标签/原目标/范围及可选 Visibility 遮挡，只对原目标调用统一 UmbraPhysicalDamage.Apply 一次，并沿用受击事件、暴击与飘字链。旧 Hit Window Notify 不再驱动玩家普通攻击伤害；敌人及其他技能的 Notify/扫掠能力保留。LocalPredicted 客户端仅播放表现，不能扣血。

激活首击和每个后续攻击段开始时，从 ASC 快照 `AttackSpeed`（AttributeSet 限制0.2～10.0）。倍率达到 `HighSpeedAttackThreshold`（默认3.0）时按 `HighSpeedAttackMontages` 顺序循环有效项；配置A、B即得到A-B-A-B，空项会跳过，数组无有效项则安全回退普通第一段。降回阈值以下从普通连招第一段恢复并重置高速索引，再次进入高速模式从A开始。模式、索引与倍率都只在每击开始决定。

逻辑周期=BaseAttackInterval/AttackSpeed，前摇=周期×AttackWindupRatio。BaseAttackInterval=0 只读取普通第一段原始完整长度和 Rate Scale；前摇出手时 ASC 提交从本击起手算起的下一次许可时间，取消前摇不新增间隔。Montage/Chain Point 不门控逻辑频率；视觉出手标记来自 GA 覆盖值或首个旧 Hit Window Begin，请求播放率考虑资产 Rate Scale，并受表现上限控制。旧高速动画赶不上逻辑出手时记录警告。

Controller 的鼠标 Pawn 射线显式忽略当前受控 Pawn；这是贴身时玩家胶囊遮挡敌人、导致攻击入口未触发的修复。仍保留最近其他 Pawn 的选择语义，不扩大攻击半径或添加无条件距离伤害。

### 3. 敌人攻击与死亡

AI::Tick → FindPlayerPawn（当前固定玩家索引 0）→ 检测距离/追击 → 朝向与冷却检查 → Enemy::TryActivateBasicAttack → EnemyBasicAttackAbility（ServerOnly）。蒙太奇命中 Tick 按二维 HitRadius 检测目标，每次攻击只结算一次；不是玩家的 socket 扫掠，也没有攻击扇形判断。

Health 从正数降到零 → Enemy::HandleHealthChanged → authority Die → bIsDead → ApplyDeathState：加 State.Dead、取消能力、清目标、停移动、禁碰撞、播放死亡动画、计时冻结姿势、OnDeathStarted 蓝图事件、可选尸体寿命。复制到客户端经 OnRep_IsDead 应用同一表现。治疗回正生命不撤销死亡状态。

玩家 C++ 未见对应 Health 死亡监听与重生流程；不得把敌人死亡规则推断为玩家已具备的功能，蓝图补充情况待编辑器确认。

### 4. 伤害到最终数据修改

双方普攻 → UmbraPhysicalDamage::Apply → 校验 Instant / 无 Modifiers / 恰好一个指定 Execution → Spec 写 Damage.Type、AD/AP 系数 → ApplyGameplayEffectSpecToTarget → PhysicalDamageExecution 实时捕获攻击方 AttackPower/AbilityPower、暴击率/倍率与目标对应抗性。

`Raw = max(0, AttackPower × AD系数 + AbilityPower × AP系数)`；`Damage = Raw × 暴击总倍率（未暴击为1）× 100 / (100 + 对应抗性)`。物理用 Armor，魔法用 MagicResistance；类型与 AD/AP 缩放独立。系数及其 SetByCaller Tag 保留既有名称，以兼容攻击蓝图。保留 Physical 类名是为了兼容已引用资产，不代表仅支持物理。

Execution 写本 Spec 的 Damage.ResultCritical，并输出正 IncomingDamage → AttributeSet::PostGameplayEffectExecute **先清 IncomingDamage，再 SetHealth(Health.BaseValue - Damage)** → 属性边界裁剪 → 属性委托 / 复制 → 死亡与 UI。旧负 Health GE 仍可改血，但绕过当前伤害公式/飘字链，不应叠加在普攻 GE 上。

### 5. 数据到 UI / 调试修改

- 血条组件 InitWidget 注入 Enemy → 血条 Bind 订阅 Health/MaxHealth → Refresh 读取 GAS、生成 `FUmbraEnemyHealthBarViewState` 并调用 Blueprint 表现事件；WBP 将0..1比例写入任意原生/材质/复合控件，Health≤0时由 C++ 隐藏整棵 Widget。C++ 不依赖 Designer 子控件名或具体 UMG 类型。
- 血条布局：WBP 根 SizeBox 定义宽高；组件 OnRegister 强制 Draw at Desired Size，覆盖旧资产固定尺寸模式，C++ 不指定数值宽高。预览用 Desired 模式，运行遵循相同布局和视口 DPI。
- 飘字：AttributeSet 仅在唯一扣血点调用 FUmbraDamageNotification::Capture / Dispatch；Notification 内部在扣血前捕获敌人 Mesh Bounds 与攻击者 PC，扣血后发 ClientShowDamageNumber（Unreliable）→ Controller 屏外过滤和 CreateWidget → DamageNumber::Start。Start 按缩写前实际伤害选择并规范化字体档位，只随机一次基础字号，暴击再乘字体倍率并受最终上限限制；数字格式独立按 k/M/B/T 格式化且在舍入到下一阈值时进位。随后保存世界起点/终点，NativeTick 仅处理既有投影、移动、缩放和淡出，不再随机字号。快照不引用受害者，过量伤害不裁成生命差；丢包只丢表现。
- 调试：F1/F2 → Controller → Panel::ViewPlayer/ViewEnemy → 订阅 15 个属性 → 生成 `FUmbraAttributeDebugViewState`；AttackSpeed 保留直接倍率并提供两位小数显示文本（1.00、2.30），CriticalChance/CriticalDamageMultiplier 乘100后交给 WBP。WBP 决定窗口尺寸、控件和样式。WBP 按钮调用 `RequestOperation` → Controller RPC 校验目标、距离、开发开关 → 原生调试 GE → GAS 修改。固定伤害10绕过公式，无飘字；治疗10修改 Health；Add Effect 每次给全部15项新增一层，普通值+20、攻速倍率及两个暴击值+0.2，层数不限但 AttackSpeed/CriticalChance 分别受10.0/1.0上限；Remove Effect 移除本控制器在当前目标上的全部层。

## 生命周期清理检查

血条 `Shutdown/UnbindASC` 移除属性、静态 ASC 生命周期及 Enemy EndPlay 监听；组件 EndPlay 和 Widget NativeDestruct 都调用清理。调试面板 `ShutdownPanel/UnbindTarget` 清理属性、Pawn、ASC、按钮与目标监听；Controller EndPlay 清理面板、专用输入映射、调试增益和飘字。未发现这些路径缺少清理的代码证据。

Enemy 的 Health 监听使用 AddUObject 且 ASC 与 Enemy 共生命周期，没有保存句柄；这不足以证明内存泄漏。以后支持对象复用/重复注册时再加入明确解绑。双方普攻自身清理已统一到 EndAbility，遵循 GAS ScopeLock 延迟结束并防重入；玩家释放当前 Montage、连招/命中/标签等待任务和目标缓存，Controller 清自动攻击与最新指令。ASC 更换或清除 Avatar 前取消活动能力并清输入，避免任务引用旧 Avatar；敌人恢复攻击前 MovementMode（死亡时不恢复）。

## 有依据的维护问题

位置按本次静态检查行号，同时给出函数名便于代码变动后检索。前三项已在本次处理，表中保留验证边界。

| 优先级 / 问题 | 代码依据与影响 | 最小处理建议 / 验证 |
| --- | --- | --- |
| 1. 属性结算承担飘字路由 | `Source/Umbra/AbilitySystem/UmbraAttributeSet.cpp:66`–92，PostGameplayEffectExecute 直接 Cast Enemy、访问 Mesh、寻找 PlayerController 并发 UI RPC。扩展目标类型或显示接收者就需改底层属性代码。 | 后续独立小改动把表现路由移到明确的通知处理函数/现有 ASC 边界，保留唯一扣血点，无需新建通用事件框架。验证致死、过量伤害、调试伤害和多人只显示一次。 |
| 2. 移速曾有两套来源（已处理） | 历史上 AttributeSet MoveSpeed=600、玩家 MaxWalkSpeed=500，且属性变化不驱动物理移动。当前两项玩家后备均为500，ASC委托负责首次及实时同步；敌人仍用 EnemyMoveSpeed 默认300作为首次播种值。 | 自动化覆盖初始500、增益添加/移除及换Avatar；仍需PIE验证动画观感和多人复制。 |
| 3. 技能清理依赖特定回调 | `Source/Umbra/AbilitySystem/Abilities/UmbraBasicAttackAbility.cpp:167` 的 FinishAbility 清目标/连招；`Source/Umbra/AbilitySystem/Abilities/UmbraEnemyBasicAttackAbility.cpp:87` 的 FinishAttack 恢复移动。两类没有 EndAbility override，外部取消的清理依赖 Montage 回调；玩家连招等待期已无 ActiveMontageTask。 | 将自身状态释放收敛到幂等 EndAbility 清理，Finish 函数只请求结束，避免递归。不是已证实卡死；需要测试蒙太奇中、连招等待中 CancelAllAbilities、受击中断和死亡取消。 |
| 后续：普通输入映射清理不对称 | `Source/Umbra/UmbraPlayerController.cpp:144/151` 添加 Default/MobileExcluded 映射，EndPlay 没有对应移除；专用 Debug 映射已有清理。Controller 切换而 LocalPlayer 留存时可能留下旧映射。 | 跟踪自己添加的普通映射并成对释放，保护共享映射；验证切 Controller/关卡后移动攻击仅触发一次。当前未复现运行故障。 |
| 后续：参数意图隐藏于字面量 | `Source/Umbra/Characters/UmbraPlayerCharacter.cpp:207` 的攻击距离容差 +100cm；`Source/Umbra/AI/UmbraAIController.cpp:85` 的追击停止系数0.85；`Source/Umbra/AbilitySystem/Damage/UmbraPhysicalDamageExecution.cpp:93` 抗性尺度100。 | 分别命名容差、导航余量和抗性尺度，注明原因/单位；只有需要策划调整的值才暴露配置。三者含义不同，不合并为“通用战斗距离”。 |

表中前三项是本次整理时识别的维护问题，现已分别由 `FUmbraDamageNotification`、ASC 的 MoveSpeed 监听、攻击能力 `EndAbility` 覆写处理；验证边界见 [Progress](Progress.md)。

未发现 UI 重算伤害公式；重复的基础数值校验属于入口与结算防护，不建议为消除重复而移除。Controller 虽同时承担输入、导航和 UI 管理，已用 Debug/Damage 实现分文件隔开；当前不建议仅因文件长就新建继承层。
