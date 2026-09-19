# 编辑器配置入口

2026-09-19 更新普通攻击逻辑命中。关联：[架构](Architecture.md)、[状态与验收](Progress.md)。

## 证据边界

- `Umbra.uproject` 的 EngineAssociation 为 `5.8`；历史文档记录使用过 5.8.2，本次未启动引擎核对本机补丁版本。
- `Config/DefaultEngine.ini` 明确默认地图与编辑器启动地图为 `/Game/Maps/L_Prototype`，默认 GameMode 为 `/Game/Blueprints/Core/BP_UmbraGameMode`。地图 World Settings 覆盖、实际 Pawn/Controller/PlayerState 类仍待编辑器确认。
- 下表资产路径均通过文件列表确认存在。**“所需父类”来自 C++ 契约/现有配置指南，不是本次读取资产确认的实际父类。** 所有实际父类、Graph、默认值和引用均待编辑器确认。旧文档的编辑器检查属于历史记录。

## 蓝图与引用核对表

`/Game` 对应仓库 `Content`。在已有资产上核对，不重复创建。

| 已存在资产 | 所需父类 / 类型 | 配置位置与引用契约 |
| --- | --- | --- |
| `/Game/Blueprints/Core/BP_UmbraGameMode` | AUmbraGameMode | Class Defaults：Default Pawn Class、Player Controller Class、Player State Class；原生 GameMode 仅设 PlayerStateClass=AUmbraPlayerState |
| `/Game/Blueprints/Player/BP_UmbraPlayerCharacter` | AUmbraPlayerCharacter | MoveAction、AbilityInputActions、PrimaryAttackRange、CharacterMovement（Rotation Rate.Yaw）、Mesh/Anim Class；Mesh 必须包含普攻配置的 socket |
| `/Game/Blueprints/Player/BP_UmbraPlayerState` | AUmbraPlayerState | InitialAttributesEffect、InitialAbilities（含所需普攻类）、Use Debug Initial Attributes / Debug Initial Attributes |
| `/Game/Blueprints/Player/BP_UmbraPlayerController` | AUmbraPlayerController | DefaultMappingContexts、MobileExcludedMappingContexts、PrimaryAction、PrimaryAttackAction；Combat/Auto Attack 的 bEnableAutoAttack、bChaseAttackTarget；DamageNumberClass；Debug/Attributes 专用面板、IMC、两个 Action 与启用开关 |
| `/Game/Blueprints/Enemies/BP_Enemy_Melee_01` | AUmbraEnemyCharacter | InitialAttributesEffect、Debug 初值、BasicAttackAbilityClass、HitReactMontage、DeathMontage、EnemyMoveSpeed、CorpseLifetime、AI 开关、HealthBarComponent.WidgetClass |
| `/Game/Blueprints/AI/BP_EnemyAIController` | AUmbraAIController | DetectionRange、AttackRange、LoseTargetRange、AttackCooldown、转向参数。敌人 C++ 默认使用原生 AUmbraAIController；是否引用本蓝图需核对敌人 AIControllerClass |
| `/Game/Blueprints/Abilities/Attack/GA_BasicAttack` | UUmbraBasicAttackAbility | AttackMontages（普通顺序）、HighSpeedAttackMontages、高速阈值、BaseAttackInterval、AttackWindupRatio、VisualStrikeTimeOverrides、HitDistanceTolerance、bCheckAttackOcclusion、MaxAttackMontagePlayRate、DamageEffectClass、DamageConfig；旧武器 socket/radius 字段对玩家普通攻击不再生效 |
| `/Game/Blueprints/Abilities/Attack/GA_EnemyBasicAttack` | UUmbraEnemyBasicAttackAbility | AttackMontage、HitRadius、DamageEffectClass、DamageConfig |
| `/Game/Blueprints/Abilities/Effects/GE_Damage_PlayerBasic`、`GE_Damage_EnemyBasic` | UGameplayEffect | Instant、Modifiers 为空、Executions 恰好一个 UUmbraPhysicalDamageExecution；分别由双方普攻引用 |
| `/Game/UI/Combat/WBP_DamageNumber` | UUmbraDamageNumber | 必需 TextBlock `DamageText`；Controller.DamageNumberClass 引用；Class Defaults / Damage Number 下分别调 Animation、Style 字号档和 Formatting 缩写 |
| `/Game/UI/Enemy/WBP_EnemyHealthBar` | UUmbraEnemyHealthBar | 实现 `Apply Enemy Health Bar State`，把0..1的 `Health Normalized` 传给任意表现控件；Enemy.HealthBarComponent.WidgetClass 引用 |
| `/Game/UI/Debug/WBP_AttributeDebugPanel` | UUmbraAttributeDebugPanel | 实现 `Apply Attribute Debug State`，自行控制控件树、格式、尺寸和样式；四个按钮调用 `Request Operation` |

原生受击技能由 Enemy C++ 直接授予，不必另建 GA。已存在 `/Game/Blueprints/Enemies/Animation/AM_Enemy_BasicAttack`、`AM_Enemy_HitReact_Front`、`AM_Enemy_Death`；它们是否被正确引用、命中 Notify 窗口、Slot、动画蓝图连接均待编辑器确认。不能从 `Greystone_AnimBlueprint` 或 `ABP_Enemy_01` 名称推断内部实现。

## 配置来源与覆盖关系

1. 类参数通常按 C++ 构造后备值 → Blueprint 类默认值 → 允许实例编辑的关卡实例覆盖取值。BeginPlay 的显式赋值仍会覆盖某些编辑器值：敌人 MaxWalkSpeed 取 EnemyMoveSpeed；玩家/敌人高亮初始状态也由 C++ 重设。
2. GAS 初始数值：AttributeSet 后备值（角色 MoveSpeed 首次取已有 MaxWalkSpeed 作兼容初值）→ InitialAttributesEffect → 非 Shipping 且开启开关时的 **整组** DebugInitialAttributes Override → Health/Resource 填满。调试覆盖不是“仅覆盖你改过的字段”。每个 ASC 权威端只执行一次，PIE 中改配置不会重新初始化。
3. 玩家初值配在实际 PlayerState 类，敌人初值配在 Enemy；初始 GE 不要设置 Health、Resource、IncomingDamage，不依赖施加条件，也不要用持续 GE。当前仓库没有在此确认具体初始 GE 资产引用；不要臆造路径。
4. 普攻 DamageConfig 只配置类型和 AD/AP 系数；AD/AP、双抗和暴击参数来自 GAS 属性。当前默认 AttackPower=10、AD 系数=1、AP 系数=0；没有独立基础伤害或 Can Crit 参数。
5. **MoveSpeed 已驱动实际移动**：ASC 绑定和属性变化时把 GAS 当前值写入 Avatar.MaxWalkSpeed，清除/更换 Avatar 时解绑。兼容初值为玩家 MovementComponent 默认500或现有 BP 覆盖；敌人 EnemyMoveSpeed 默认300或现有 BP 覆盖。AttributeSet 与 DebugInitialAttributes 的 C++ MoveSpeed 后备均为500；初始 GE / 已保存的 Blueprint Debug MoveSpeed 仍优先于后备值。旧 EnemyMoveSpeed 不再是独立的运行时控制入口，不必删除或重命名已有资产属性。`AUmbraPlayerCharacter.GetLocomotionAnimationPlayRate` 向 AnimBP 提供由当前 MaxWalkSpeed 推导的倍率，但 C++ 不直接修改动画资产。
6. **普通攻击逻辑时序**：最终倍率=1+AttackSpeedBonus；每击开始快照倍率、周期、前摇和动画。周期=BaseAttackInterval/倍率，前摇=周期×AttackWindupRatio（默认0.3，运行时限制0.01～0.99）。前摇结束由服务端对原目标判定并提交本击间隔，挥空也消耗周期；前摇取消不新增周期。跨击间隔存在 ASC，不随 Ability End 清零。BaseAttackInterval=0 仅按普通 AttackMontages[0] 的原始完整时长和 Rate Scale 自动校准，不按高速动画或当前播放率重算。Montage 长度、Chain Point、Hit Window 和播放率上限均不推迟逻辑下一击。
7. **自动攻击与移动**：BP_UmbraPlayerController 的 bEnableAutoAttack、bChaseAttackTarget 继续控制持续攻击和追击。起手与出手使用同一个 Character.PrimaryAttackRange，距离按目标胶囊半径计入；出手可加 GA.HitDistanceTolerance。移动立即停止自动攻击并取消当前前摇或后摇，已结算的伤害与剩余间隔保留。重复点击同一目标不重启当前前摇。周期到期若目标失效或出范围，当前 Ability 结束，Controller 依原有规则追击。
8. UI：血条宽高只在 WBP 根 SizeBox 配置；WidgetComponent 注册时强制 Draw at Desired Size，不使用旧 Draw Size，Designer 使用 Desired 预览。C++ 监听 GAS 并通过 `FUmbraEnemyHealthBarViewState` 发送 Health、MaxHealth、0..1比例和可见性；WBP 只把状态写入原生/材质/复合控件。飘字数字、位置、字号和动画由 C++ 写，参数取 WBP 类默认值；按缩写前实际伤害选字号档并只随机一次，暴击字体倍率默认1.15、最终字号上限28。命中时保存世界起点/终点，每帧按当前相机投影，终点固定在场景中。不要加第二套属性绑定、伤害计算或重复 CreateWidget。属性调试面板接收 `FUmbraAttributeDebugViewState`；其中攻速、暴击率、暴击伤害已由 C++ 转成百分比显示值，WBP 直接追加 `%`，不要再次乘100。窗口锚点、位置、尺寸、精度和样式全部由 WBP 负责；C++ 只以 ZOrder20 添加实例。

## 玩家攻击动画配置与验证

在 Rider 对 `UmbraEditor / Win64 / Development` 执行 Build，或关闭编辑器后用 UE 5.8 `Engine/Build/BatchFiles/Build.bat UmbraEditor Win64 Development -Project=<Umbra.uproject绝对路径> -WaitMutex`；重新打开项目，不依赖 Live Coding 替换带 UPROPERTY 的类布局。打开 `/Game/Blueprints/Abilities/Attack/GA_BasicAttack` 的 Class Defaults，Compile/Save 并核对：

- `AttackMontages` 普通 A/B/C 顺序；`HighSpeedAttackMontages` 高速 A/B，资产路径分别为 `/Game/ParagonGreystone/Characters/Heroes/Greystone/Animations/Attack_A_Fast_Montage` 和 `Attack_B_Fast_Montage`。已有普通资产为同目录 `Attack_PrimaryA_Montage`、`Attack_PrimaryB_Montage`、`Attack_PrimaryC_Montage`。上述是实际文件路径，数组内部引用待编辑器确认。
- `HighSpeedAttackThreshold=3.0` 是最终倍率阈值，退回低速后普通连击从 A 重置。`BaseAttackInterval=0` 使用普通 A 的完整原始时长除以其 Rate Scale；也可指定正数秒。必须配置普通 A 或填写正周期。
- `AttackWindupRatio=0.3`，有效范围 0.01～0.99。`MaxAttackMontagePlayRate=3` 仅限制表现；触顶仍按逻辑周期出手和续击。`MovementCancelBlendOutTime=0.10s` 保留移动时动画淡出。
- `HitDistanceTolerance` 默认 0cm，按实测误差微调；`bCheckAttackOcclusion=true` 使用 Visibility 射线。确认墙体阻挡 Visibility，避免穿墙攻击。攻击进入距离由 `BP_UmbraPlayerCharacter.PrimaryAttackRange` 控制，起手/出手均复用该范围与目标胶囊半径。
- `DamageEffectClass` 和 `DamageConfig` 沿用现有 GE/伤害系数。旧 `HitDetectionSocketName`、`HitDetectionStartSocketName`、`HitDetectionRadius` 不再参与玩家普通攻击。

逐个打开普通 A/B/C 和高速 A/B Montage，在可见出手姿势处确认第一个 `Umbra Attack Hit Window` Begin 是否准确。C++ 默认只读取这一时间作为视觉标记，Notify 本身不造成玩家普通攻击伤害；若不准确，在 GA 的 `VisualStrikeTimeOverrides` 里以 Montage 为键填写从资产起点起算的原始秒数。配置覆盖值后可移除这些普通攻击 Montage 的旧 Hit Window；敌人和其他技能仍可保留 Hit Window。旧 `Umbra Attack Chain Point` 可以保留作表现标记，但不控制逻辑频率。检查高速 A/B 的 Slot 与实际 AnimBP Slot 匹配，编辑器内检查 Rate Scale、Blend In/Out 和是否包含 Root Motion，不依据文件名推断其内部值。

运行时请求实际播放率=视觉出手原始时间/本击逻辑前摇；传给 Montage 的倍率会除以资产自身 Rate Scale，避免重复叠乘。若触及播放率上限导致动画赶不上逻辑出手，Output Log 会提示缩短或替换高速 Montage；伤害仍按服务端计时结算，视觉错位不会自动消失。

在 `/Game/Maps/L_Prototype` 双人 PIE 测试，服务端控制台输入 `umbra.Attack.Log 1` 开启日志，完成后设为 0。用固定单击伤害、无额外触发效果的目标，分别设最终倍率 1.0、2.99、3.0、4.0、5.0、10.0（AttackSpeedBonus 对应 0、1.99、2、3、4、9），稳定阶段统计服务端日志中的起手次数与时间跨度，再用实际总扣血/同一跨度算 DPS。理论起手频率为倍率/BaseAttackInterval，理论 DPS=每击固定伤害×该频率；将实测单列，勿把预测写成实测。另测同目标最多一次扣血、前摇移动、出手后反复移动取消、目标死亡/离距/遮挡、快速取消重攻、移动后无补刀与双人 PIE 服务端唯一伤害。服务器 Timer 在游戏线程按帧触发；低帧率或服务器更新间隔大于前摇/周期时，实际频率会低于理论值，调度不会在同帧无限补发。

### n 秒普通攻击伤害统计

在服务端控制台输入 `umbra.Attack.MeasureSeconds 10`（将 10 换成所需正数秒），然后对目标开始普通攻击。命令只武装**服务器上任意角色的下一次普攻起手**，由该角色独立统计，起手时自动归零；窗口结束后 Output Log 输出 `[AttackMeasure] Complete`，包含起止服务端时间、起手次数、造成有效生命损失的命中次数、总生命损失和 DPS（总损失/n）。统计该攻击者对所有目标的**普通攻击实际 Health 减少量**，不计其他技能、预测伤害或超出目标剩余生命值的溢出伤害。若 Avatar 在窗口结束前更换或 ASC 清理，输出 `Aborted` 和已统计时长。计时器可能因服务器帧延迟才打印结果，但到期后的伤害不会算入 n 秒窗口；这是服务端游戏时间，不是现实秒表。建议先使角色进入稳定连续攻击，再输入命令等待下一击起手，分别记录倍率、n、起手数、总伤害与 DPS。

需要反馈：服务端 `Attack start` / `Attack strike` 日志（实例号、目标、倍率、周期、前摇、服务端时间、命中/挥空原因、Montage 请求/实际速率和触顶），每档倍率的实际起手次数、计时区间、总伤害，以及动画出手是否提前/滞后、穿墙、重复扣血、移动后补刀或旧回调误结束新攻击。所有资产内部默认值、Notify 时间和 PIE 结果目前待编辑器确认。

## 攻击取消后的旋转与姿势混合

这部分的运行时所有权由 C++ 固定：普通移动只由 CharacterMovement 根据本帧输入或当前导航路径段方向旋转；不再保存点击终点朝向，也不再在 Character Tick 中执行 `RInterpTo/SetActorRotation`。攻击开始时，当前 `AttackInstanceId` 临时关闭 `Orient Rotation to Movement` 并朝目标定向；所有 Ability 结束路径只在实例号仍匹配时交还旋转所有权，不写 Actor Rotation。旧攻击回调不能释放新攻击的朝向，死亡标签存在时也不会恢复普通移动旋转。Controller Rotation 和 `Use Controller Desired Rotation` 始终不参与玩家 Yaw。

编辑器中按以下顺序核对；这些资产是仓库与运行日志中确认的真实路径，但二进制 Blueprint/AnimGraph 内部连接仍须在 UE5.8 编辑器中目视确认：

1. 保存全部资产并关闭 Editor，再执行完整 `UmbraEditor Win64 Development` 构建后重开。此次修改改变了原生类成员布局、旋转函数和 CDO 默认值；不要依赖 Live Coding/Reinstancing 作为最终验收。
2. 打开 `/Game/Blueprints/Player/BP_UmbraPlayerCharacter`，选中 `CharacterMovement`，在 Details 搜索 `Rotation Rate`，展开后把 **Yaw** 设为 `600 deg/s`。这是540～720建议区间的中值：180°急转约0.3秒完成，既避免瞬翻，也不会明显拖慢操作；需要更利落时可逐步提高到720。再搜索 `Orient Rotation to Movement`（应勾选）与 `Use Controller Desired Rotation`（应取消）。在 Class Defaults 搜索 `Use Controller Rotation Yaw`（应取消）。后三项运行时会由代码纠正，人工设置是为了清除旧 Blueprint 默认值造成的编辑器误导；`Rotation Rate.Yaw` 则由 Blueprint 实际调参，代码不会在 BeginPlay 覆盖合法值。每次改完 Compile/Save；移动时预期朝当前输入/路径段平滑转向，不朝最终点击点抢转。
3. 打开 `/Game/Blueprints/Abilities/Attack/GA_BasicAttack`，在 Class Defaults 搜索 `Movement Cancel Blend Out Time`，位置为 `Attack | Cancellation`，设为 `0.10 s`，Compile/Save。既有 Blueprint 可能仍序列化旧值0.05而覆盖新的 C++ 默认值，必须目视确认。预期是移动在取消帧立即生效，而攻击上身姿势约0.10秒淡出；若移动也延迟0.10秒，记录输入阶段与日志，这不是预期行为。
4. 打开 `/Game/Blueprints/Player/Greystone_AnimBlueprint`。当前仓库无法静态读取其二进制 AnimGraph，因此不指定或臆造节点名，也不要求本次新增节点。请在 AnimGraph/State Machine 中检查：实际输出姿势路径包含 `UpperBody` Slot；Locomotion 状态由实时 `Velocity/Speed` 更新；没有以 `State.Attacking` 把整个移动状态机硬锁到 Montage 完成；Event Graph 没有 `Set Actor Rotation`、`Set Control Rotation` 或自定义 Tick Yaw 插值。取消时预期基础 Locomotion 当帧开始更新、Slot 同时淡出。若仍突兀，请提供 Final Animation Pose 附近、`UpperBody` Slot 前后和 Locomotion 转换规则截图。
5. 逐个打开 `/Game/ParagonGreystone/Characters/Heroes/Greystone/Animations/Attack_PrimaryA_Montage`、`Attack_PrimaryB_Montage`、`Attack_PrimaryC_Montage` 及 `/Game/ParagonGreystone/Characters/Heroes/Greystone/Animations/Attack_A_Fast_Montage`。确认 Slot 都是 `UpperBody`。再打开它们引用的 Animation Sequence，在 Asset Details 搜索 `Root Motion`；普通原地挥击应不启用根位移/根旋转。代码只在检测到当前攻击 Montage 含 Root Motion 时采用0秒停止并打印 `Movement-cancelling root-motion attack montage` 警告。如果出现该警告，反馈 Montage 名称、Sequence 的 Root Motion 设置和是否确实需要根运动，不要全局修改 AnimBP Root Motion Mode。

没有专门的急转/转身动画时，CharacterMovement 的平滑 Yaw 仍可能让脚底出现少量滑动。本阶段先用 `Rotation Rate.Yaw` 与0.10秒 Montage 混合验收；不要再叠加 Blueprint Tick 插值，否则会重新形成两个旋转写入者。

## 玩家移动动画速度配置

打开 `/Game/Blueprints/Player/BP_UmbraPlayerCharacter` → Class Defaults → `Animation / Locomotion`：

- `LocomotionAnimationReferenceSpeed=500 cm/s`：共享参考移速；默认玩家 MoveSpeed 500cm/s 时起步/急停动画为1x、移动Yaw为600°/s。只有动画实际标定速度不同且允许动画与Yaw使用不同基准时才应拆分。
- `MinLocomotionAnimationPlayRate=0.25`、`MaxLocomotionAnimationPlayRate=3.0`：只限制提供给 AnimBP 的表现倍率，不修改 GAS MoveSpeed。

同一角色 Class Defaults → `Movement / Rotation`：

- `BaseMovementYawRate=600 deg/s`：在共享的 `LocomotionAnimationReferenceSpeed` 时，CharacterMovement 使用的 `RotationRate.Yaw`。
- `MaxMovementYawRate=1800 deg/s`：高速安全上限。
- 实际公式为 `Clamp(BaseMovementYawRate * MoveSpeed / LocomotionAnimationReferenceSpeed, 0, MaxMovementYawRate)`；由 ASC 的 MoveSpeed 变化委托同步，不在 Tick 计算。默认出生 MoveSpeed 500cm/s 时严格为动画1.0x、Yaw 600°/s。
- 攻击期间技能临时关闭 `bOrientRotationToMovement`，倍率更新不会抢走攻击朝向；攻击结束恢复移动朝向后使用最新Yaw速率。敌人保持自己的AI转向配置。

实际玩家动画资产是 `/Game/Blueprints/Player/Greystone_AnimBlueprint`，其 `Locomotion` 状态机包含 `JogStart`、`JogStop` 和 `Idle/Jogs`。按以下方式接线：

1. 新建对象变量 `UmbraCharacterRef`，类型选 `UmbraPlayerCharacter Object Reference`；新建 float 变量 `LocomotionAnimationPlayRate`，默认1.0。
2. Event Graph 的 `Event Blueprint Initialize Animation`：`Try Get Pawn Owner` → `Cast To UmbraPlayerCharacter` → `Set UmbraCharacterRef`。预览窗口 Cast Failed 属正常情况，不发送 Gameplay Event，也不打印错误。
3. `Event Blueprint Update Animation`：对 `UmbraCharacterRef` 做 `Is Valid`；有效时调用 `Get Locomotion Animation Play Rate` 并写入同名 float，无效时写1.0。必须在 Event Graph 缓存，不在线程安全 AnimGraph 函数中直接访问 CharacterMovement。
4. 打开 `AnimGraph → Locomotion → JogStart`，选择播放 `Jog_Fwd_Start` 的 Sequence Player。在 Details 中把 `Play Rate` 暴露为 Pin，将 `LocomotionAnimationPlayRate` 连接到该 Pin；`Play Rate Basis` 保持1.0。
5. 打开 `JogStop`，对播放 `Jog_Fwd_Stop` 的 Sequence Player 做同样连接。
6. 检查离开 `JogStart/JogStop` 的 Transition Rule：优先使用 `Automatic Rule Based on Sequence Player in State` 或 `Time Remaining (Ratio)`，不要使用固定秒数 Delay。剩余时间节点会自然跟随动态 Play Rate；固定秒数会导致加速后被截断或等待空帧。
7. `Idle/Jogs` 当前使用 `JogFwdSlopeLean` BlendSpace。第一轮只保留原有 Speed 输入，不再乘本倍率，避免 BlendSpace 与 Sequence Play Rate 双重提速。只有 PIE 确认循环脚步仍明显打滑时，才单独处理循环动画。
8. Compile、Save AnimBP；关闭并重新进入 PIE，确保现有实例重新初始化并缓存角色引用。

这里使用 `MaxWalkSpeed / LocomotionAnimationReferenceSpeed` 而不是当前 Velocity：起步第一帧和急停末帧的 Velocity 接近0，若直接按 Velocity 算，正好会把需要加速的起步/急停动画压到最慢。AnimBP 每帧只读取已经由 GAS 委托同步好的 CharacterMovement 值，不重新轮询 GAS 属性。

跑步循环若 BlendSpace 已用 Speed 轴选择不同步幅，先不要再乘倍率，避免双重加速；只有确认脚步频率仍落后时才把同一倍率接到该循环。最佳观感方案是给起步/急停动画制作距离曲线并使用 Distance Matching，跑步循环使用 Stride Warping；这部分需要动画资产和 AnimBP 配置，C++ 无法替代。

若提高 MoveSpeed 后物理起步/刹停距离本身也变长，还需在 CharacterMovement 中按设计同步调节 Max Acceleration、Braking Deceleration Walking 和 Ground Friction。它们控制物理响应，AnimBP Play Rate 只解决姿态节奏；两者应分别验收。

最小 PIE 验收：分别令 MoveSpeed 为250、500、750、1500，预期动画倍率为0.5、1.0、1.5、3.0，Yaw为300、600、900、1800°/s；超过1500两者保持各自上限，动画倍率低于125时保持0.25。每档分别从静止按下移动、持续移动、松开移动，确认 `JogStart/JogStop` 不截断、不停留、脚步与位移方向一致，180°反向时高速角色明显更快转身但不瞬间抖动。再在一次起步、急停或攻击期间改变 MoveSpeed，确认表现倍率/Yaw实时更新但状态机不会重进、攻击朝向不会被移动旋转抢占；Listen Server和客户端各测试一次。

## 数值单位与边界

| 参数 | 单位 / 约定 |
| --- | --- |
| Health/Resource 与上限 | 点；Health∈[0,MaxHealth]，MaxHealth≥1；Resource∈[0,MaxResource]，MaxResource≥0 |
| HealthRegen / ResourceRegen | 点/秒，当前只存储 |
| AttackPower / AbilityPower / Armor / MagicResistance | 非负数值；抗性公式尺度100；AD/AP 系数无量纲，最终原始伤害整体裁到≥0 |
| AttackSpeedBonus | 加成比例，0.2=+20%；限制-0.8–9.0，最终倍率为1+Bonus（0.2～10倍） |
| HighSpeedAttackThreshold | 最终攻速倍率；默认3.0，无量纲 |
| MaxAttackMontagePlayRate | 实际Montage倍率安全上限；默认3.0，无量纲 |
| CriticalChance | 小数比例，0.2=20%；限制0–1 |
| CriticalDamageMultiplier | 总倍率，2=两倍，最低1；调试面板显示200% |
| AbilityHaste | 非负数值，当前未接入冷却规则 |
| 世界距离、范围、命中容差、组件位置 | 厘米；玩家普通攻击按二维中心距离和目标胶囊半径判定，并可启用三维 Visibility 遮挡射线 |
| MoveSpeed / EnemyMoveSpeed / MaxWalkSpeed | 厘米/秒；来源区别见上节 |
| Cooldown、ComboWindow、BaseAttackInterval、尸体寿命、动画阶段 | 秒；BaseAttackInterval 是1x攻速的相邻起手周期；敌人 AttackCooldown 从成功激活时计时，不是 Montage 结束时 |
| 玩家 / AI 转向速率与容差 | 度/秒 / 度；玩家共享LocomotionAnimationReferenceSpeed=500cm/s，对应Yaw 600°/秒并随MoveSpeed线性增加，默认上限1800°/秒；AI保持独立配置 |
| 飘字距离、面板位置尺寸 | DPI 换算后的 UI 布局单位；SpreadAngleDegrees 是向上方向两侧合计张角 |

属性调试面板的 Add Effect 每层把普通数值增加20，把 AttackSpeedBonus、CriticalChance、CriticalDamageMultiplier 增加0.2（20个百分点）。允许重复添加且不限制层数，但 AttackSpeedBonus/CriticalChance 继续受9.0/1.0边界约束；Remove Effect 一次清除本控制器对当前目标添加的全部层。

## 输入、导航与高亮

- `DefaultInput.ini` 使用 EnhancedPlayerInput / EnhancedInputComponent，并移除 F1/F2 引擎 viewmode 调试绑定。
- 已存在 `/Game/Input/IMC_Default`、`IMC_MouseLook`、`IMC_AttributeDebug` 及 `/Game/Input/Actions/IA_Move`、`IA_PrimaryAction`、`IA_Attack_Primary`、`IA_Debug_ViewPlayer`、`IA_Debug_LockHovered`。实际按键映射、轴修饰器和 Trigger 待编辑器确认。调试契约是两个 bool Action，分别 F1/F2，专用 IMC 优先级10；普通 IMC 由 Controller 以0加入。
- PrimaryAction 与 PrimaryAttackAction 是独立入口；不要把普攻再加入通用标签输入绑定。专用调试 IMC 不要重复放进普通 IMC 数组。详细步骤见 [AttributeDebugPanel](AttributeDebugPanel.md)、[AttributeDebugF1F2](AttributeDebugF1F2.md)。
- 鼠标选敌查 Pawn 对象及 Attackable，并显式忽略自己的 Pawn，避免贴身时自己的胶囊挡住敌人；地面查 Visibility；F2 额外做 Visibility 遮挡校验，普通攻击悬停不具有同样的遮挡校验。实际敌人碰撞响应待编辑器确认。
- 地面移动依赖可投影且完整的 NavMesh 路径；关卡导航覆盖待编辑器确认。`r.CustomDepth=3` 已配置，后处理材质与 stencil 引用待编辑器确认。

## 第三方资源与资产维护

- 保留资源包原始目录、授权/来源说明及版本信息；记录来源、作者、许可、允许的分发范围和项目使用方式后再扩大使用。不能依据目录名认定授权。
- 当前新增 `Content/fantasy_gui_4/` 为用户已有未跟踪内容；来源、许可及是否已被 UI 引用待用户记录/编辑器确认，本次未移动、导入或删除。
- 项目逻辑与可调默认值优先放在项目自有 C++ / BP / WBP；使用第三方资源通过引用、子蓝图或材质实例配置，避免直接改供应商原件后失去升级依据。不为遵守此条迁移现有资产。
- 已引用资产只能通过匹配版本 UE Editor 移动/重命名，并检查 Reference Viewer、重定向器、蓝图编译和地图加载；同时更新配置中的软路径、测试 LoadObject/LoadClass 路径和文档。
- `.gitattributes` 已将 `.uasset/.umap` 配置为 Git LFS；新增二进制类型先确认追踪规则。不得直接修改二进制内容，也不能仅凭文件存在判断引用安全。

## 专题入口

- [AttributeSetPhase1](AttributeSetPhase1.md)：属性生命周期与边界验收。
- [BlueprintAttributeAccess](BlueprintAttributeAccess.md)：初始调试参数与蓝图只读访问。
- [MagicalDamage](MagicalDamage.md)：当前物理/魔法公式配置；[PhysicalDamageTroubleshooting](PhysicalDamageTroubleshooting.md)：旧 GE 迁移与日志排查。
- [DamageNumbers](DamageNumbers.md)、[EnemyHealthBar](EnemyHealthBar.md)：现有 UI 的控件契约、表现参数与 PIE 步骤。
