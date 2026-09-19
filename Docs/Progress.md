# Umbra 进度与验证状态

维护日期：2026-09-19。入口：[架构与问题证据](Architecture.md)、[编辑器配置](EditorSetup.md)、[开发规则](../AGENTS.md)。以仓库实现为准，不以文件名或规划推断已完成功能。

## 2026-09-19 玩家普通攻击逻辑命中

- 增加一次性服务端命令 `umbra.Attack.MeasureSeconds n`：下一击起手开启 n 秒窗口，在实际 `IncomingDamage → Health` 结算点按普通攻击来源记录生命损失，输出起手数、有效命中数、总伤害和 DPS。其他技能、客户端预测与致死溢出伤害不计；真实 PIE 数值仍待测。
- 当前攻速最终倍率上限为 10.0（`AttackSpeedBonus` 上限 9.0）；Montage 播放率上限仍独立控制视觉表现。10 倍档的实际频率、DPS 和动画对齐尚待 PIE 测量。下文 5 倍/Bonus 4.0 的描述属于旧阶段记录。
- 本次最新源码已在 `Saved/AttackMeasureBuildValidation` 隔离副本通过 UE 5.8 `Umbra` 与 `UmbraEditor / Win64 / Development` 编译；后者使用 `-NoHotReloadFromIDE`，未替换当前运行编辑器的模块。原项目编辑器处于 Live Coding 状态，常规 `UmbraEditor` 构建被 UBT 拒绝。隔离副本的未烘焙 Game 启动在加载资源时触发 `BufferReader` 断言，未进入自动化测试；用户随后要求不再测试，因此未运行 PIE 或其他测试。

- 当前代码以本击开始时间快照周期与前摇，服务端前摇 Timer 对原目标做存活、可攻击、距离、Visibility 遮挡检查，再复用现有物理伤害 GE 与受击事件。普通攻击不再订阅 Hit Window，其他能力的扫掠 Notify 类保留。跨 Ability 间隔由 ASC 保留，前摇移动取消不新增间隔，出手后取消保留已提交间隔。
- Montage/Chain Point 不门控下一击；视觉出手时间优先读取 GA 覆盖值，否则读取首个旧 Hit Window Begin。动画请求率考虑资产 Rate Scale，触及 MaxAttackMontagePlayRate 时输出姿势赶不上逻辑出手的警告。`umbra.Attack.Log 1` 可记录服务端实例、时间、倍率、结果和动画速率。
- 此节为当前实现；下方关于武器扫掠、Hit Window 决定玩家伤害和 Montage 限制玩家攻速的记录是历史验证，不代表当前行为。真实 Montage 标记、蓝图默认值、PIE 频率/DPS/双人网络与动画观感仍待编辑器确认。详细操作见 EditorSetup。
- 前一版 UE 5.8 `UmbraEditor / Win64 / Development` 编译成功；NullRHI 命令行运行 `Umbra.Combat.Maintenance` 为 Success，覆盖理论周期快照、旧 Hit Window 不扣血、服务端单击一次结算、重复/旧回调无效、前摇移动取消。本次伤害统计与 10 倍上限修改后的原项目编辑器目标和自动化测试仍待验证。该测试未推进真实攻击周期的逐帧计时，也未做双人 PIE；1.0、2.99、3.0、4.0、5.0、10.0 的频率与 DPS 目前仅有公式预测，没有实测值。运行中仍出现既有 Greystone_AnimBlueprint 的 Divide by zero 警告。

## 2026-09-18 移速驱动起步/急停表现

- `AUmbraPlayerCharacter.GetLocomotionAnimationPlayRate` 已提供由 GAS 同步后的 `MaxWalkSpeed / LocomotionAnimationReferenceSpeed` 推导的安全倍率，默认参考速度500cm/s、范围0.25～3.0；未新增属性，也不在角色 Tick 轮询 GAS。
- 已确认实际 `/Game/Blueprints/Player/Greystone_AnimBlueprint` 使用 `Locomotion / JogStart / JogStop`，对应 `Jog_Fwd_Start`、`Jog_Fwd_Stop`。C++ 接口与参数已经就绪，二进制 AnimBP 的 Event Graph 缓存、两个 Sequence Player Play Rate 及转场规则仍需按 EditorSetup 手工接线并保存。
- UE 5.8 `UmbraEditor / Win64 / DebugGame` 增量构建成功；`Umbra.Attributes.Lifecycle` 与 `Umbra.Combat.Maintenance` 为 Success，覆盖默认MoveSpeed 500、250/500/750cm/s对应0.5/1.0/1.5倍，以及0.25～3.0安全边界。自动化不渲染状态机，不能替代起步/急停观感、脚步滑动、Distance Matching 或多人 PIE 验收。

## 2026-09-19 移速驱动玩家转向速度

- ASC 的既有 MoveSpeed 委托在同步玩家 MaxWalkSpeed 时同时调用角色辅助函数更新 CharacterMovement.RotationRate.Yaw，无 Tick 轮询；动画与Yaw共享参考点，默认500cm/s→动画1.0x及Yaw 600°/s，随MoveSpeed线性增加并将Yaw限制到默认1800°/s。AttributeSet和调试后备MoveSpeed也统一为500；敌人仍由自己的默认300播种。
- 攻击朝向所有权规则未改变：攻击期间只更新数值、不重新打开 `bOrientRotationToMovement`；攻击结束后移动旋转使用最新Yaw。
- `Umbra.Combat.Maintenance` 为 Success，覆盖初始Yaw 600°/s、MoveSpeed GameplayEffect实时更新Yaw、250/500/750cm/s对应300/600/900°/s，以及1800°/s上限。完整 `Umbra` NullRHI批次共8项，其中7项成功；`Umbra.UI.WorldPresentation` 因命令行环境没有Viewport而失败，与本次移动逻辑无关，仍需在PIE单独运行。自动化不替代不同帧率、网络延迟下的实际转身观感验收。

## 2026-09-17 持续攻击、高攻速动画与伤害数字

- 一次攻击目标指令会持续攻击：Controller 负责保存目标、复用 `PrimaryAttackRange` 追近、只缓存最新移动/换目标指令；普通指令在当前击的安全衔接点执行，不会额外等待手动连击宽限。目标失效会停止；`State.Dead`、新增的 `State.Stunned`、外部取消和 Avatar 更换立即清理。
- 新增原生 `Umbra Attack Chain Point` AnimNotify。代码只接受位于最后一个 `Umbra Attack Hit Window` 之后的衔接点；未配置或配置过早时回退 Montage 正常结束。每击开始重置命中集合，同击仍只伤害一次。
- 高攻速模式默认阈值3.0最终倍率（`AttackSpeedBonus=2.0`），每击独立决定；达到阈值按 `HighSpeedAttackMontages` 循环，配置A、B即A-B-A-B，空项跳过、全空回退普通第一段，降回阈值以下从普通第一段开始并重置高速索引。`BaseAttackInterval=0` 时由普通第一段有效时长自动校准，修复基础攻速被硬编码1秒意外加速；实际 Montage PlayRate 默认最多3.0，防止更高倍率把混合、抬手和命中窗压没。要在安全上限内保持4～5倍理论周期，A、B都应配置更短的专用高攻速 Montage。
- 飘字新增蓝图可读写字号档、暴击字体倍率1.15、最终字号上限28，以及可编辑缩写单位。默认 k/M/B/T、1位小数；`999960 → 1.0M`。选档使用缩写前实际伤害，每个 Widget 只随机一次字号，既有扇形移动/停留/淡出不变。
- 玩家角色新增蓝图纯函数 `GetLocomotionAnimationPlayRate` 和可编辑参考速度/倍率上下限，供 AnimBP 的起步与急停 Sequence Player 使用；C++ 只提供由 GAS 驱动 MaxWalkSpeed 推导的表现倍率，不直接修改二进制动画资产。
- 最新调整已完成 UE 5.8 `UmbraEditor / Win64 / DebugGame` 构建，`Umbra.Combat.Maintenance` 为 Success；新增覆盖300%阈值后的A-B-A高速循环、降速恢复普通第一段并重置高速索引，同时保留自动基础周期、Montage播放率上限和每击攻速快照验证。Development目标未在本轮重新构建；真实A/B资产的通知帧、混合观感和多人预测仍需PIE验收。战斗测试仍有现有Greystone AnimBlueprint的Divide by zero警告，以及无NavMesh临时World中预期的寻路警告。
- 尚未验证实际动画衔接观感、真实 NavMesh 上移动打断/追击、不同 Montage 的混合效果、Listen Server/双客户端/专服预测一致性，以及 WBP 最终字体视觉；构建和 NullRHI 自动化成功不等同于这些 PIE 验收通过。

## 2026-09-17 属性驱动角色行为

- MoveSpeed 继续由 ASC 属性委托同步至当前 Avatar 的 MaxWalkSpeed：ActorInfo 首次绑定立即同步，GE 聚合变化实时同步，清除/更换 Avatar 时成对解绑；AttributeSet 保证不小于0，全程没有 Tick 轮询。
- 复用现有 AttackSpeedBonus，不新增属性：0为基础速度，最终倍率为 `1 + Bonus`；Bonus限制-0.8～4.0，对应最终倍率0.2～5.0。玩家普通攻击每击开始快照；实际 Montage Rate 由动画有效时长和目标攻击周期推导，Notify 命中窗口随实际 Rate 缩放。攻击中变化只影响下一击。
- 现有调试初值、Gameplay Effect、属性面板和二进制资产继续使用 AttackSpeedBonus，无迁移步骤。
- 服务端仍是伤害判定和 GameplayEffect 的唯一有效端；LocalPredicted 客户端与服务端均从复制属性读取同一倍率播放 Montage。
- 已验证 UE 5.8 `UmbraEditor / Win64 / Development` 构建成功；`Umbra.Attributes.Lifecycle`、`Umbra.Attributes.DebugOperations`、`Umbra.Attributes.DebugInputAndWidget` 与 `Umbra.Combat.Maintenance` 均 Success。战斗测试覆盖 Bonus=1 时实际 Montage PlayRate=2、0.35秒宽限缩放为0.175秒，以及攻击中把 Bonus 改为-0.5仍保留当前快照。未执行真实 PIE 动画观感、通知帧逐项检查或多人网络验收；战斗测试仍报告现有 Greystone AnimBlueprint 的 Divide by zero 警告。

## 2026-09-17 属性调试 Add Effect 叠加

- Add Effect 改为覆盖面板展示的15项属性：普通数值每层+20；AttackSpeedBonus、CriticalChance、CriticalDamageMultiplier 每层+0.2（20个百分点）。AttackSpeedBonus/CriticalChance 分别保持4.0/1.0封顶，IncomingDamage 不属于常驻展示属性，不加入增益。
- 每次点击都会新增一层，不设人为层数上限；Remove Effect 一次移除本控制器在当前目标上的全部层，并继续保留其他控制器/来源的同类效果。
- 蓝图按钮契约未改变，仍调用 `Request Operation` 的 Add Effect / Remove Effect；本轮不修改 WBP 资产。
- 已验证 UE 5.8.2 `UmbraEditor / Win64 / Development` 构建成功；`Umbra.Attributes.DebugInputAndWidget` 与 `Umbra.Attributes.DebugOperations` 均 Success（2/2、0 warning、0 error）。未执行真实 PIE 按钮、视觉与多人网络验收。

## 2026-09-17 属性调试百分比显示单位

- `FUmbraAttributeDebugViewState` 的 AttackSpeedBonus、CriticalChance、CriticalDamageMultiplier 为百分比显示值：分别把 GAS 的 `0.2/1/2` 传为 `20/100/200`。WBP 只需格式化数字并追加 `%`，不再乘100。
- GAS 内部数值与战斗计算不变；CriticalChance 仍在底层限制为0..1，Add Effect 的比例增量仍为0.2。
- 已验证 UE 5.8.2 `UmbraEditor / Win64 / DebugGame` 构建成功；DebugGame 下 `Umbra.Attributes.DebugInputAndWidget` 与 `Umbra.Attributes.DebugOperations` 均 Success（2/2、0 warning、0 error），包含 `0.25→25`、`0.4→40`、`1.75→175` 的状态断言。Development 重编因用户当前编辑器开启 Live Coding 而被 UBT 安全阻止；需在编辑器按 Ctrl+Alt+F11 或保存后关闭编辑器再编译。未执行真实 PIE 视觉与多人网络验收。

## 2026-09-17 属性调试面板 C++/蓝图边界

- 移除属性调试 Widget 的七个 `BindWidget`、C++ 文本格式化、按钮绑定和 Controller 中硬编码的锚点/位置/360×640尺寸。
- C++ 观察 GAS 并把15项属性及目标/就绪状态放入 `FUmbraAttributeDebugViewState`，通过 `Apply Attribute Debug State` 交给 WBP；三个比例/倍率字段由 C++ 转成百分比显示单位，其余单位、精度、等待/反馈文字、按钮状态、窗口尺寸和样式归蓝图。
- WBP 按钮通过唯一的 `Request Operation` 蓝图入口请求固定调试操作，服务器验证与 GE 所有权规则未改变。
- 已验证 UE 5.8.2 UmbraEditor / Win64 / Development 编译成功；`Umbra.Attributes.DebugInputAndWidget` 与 `Umbra.Attributes.DebugOperations` 均 Success（2/2、0 warning、0 error）。蓝图资产按用户分工未修改，事件实现、按钮连线和实际 PIE 视觉/输入验收待蓝图侧完成。

## 2026-09-16 血条尺寸与场景飘字修复

- 用户已确认血条只有外框的原因：旧组件120×12固定尺寸与控件上下各12 Padding冲突，Retainer Box运行高度为0。证据、原因和回归步骤已记录在 [EnemyHealthBar](EnemyHealthBar.md#2026-09-16-已确认-bug有外框但没有红色填充)。
- 血条：移除C++数值宽高，构造/注册时启用Draw at Desired Size，注册时覆盖旧资产固定尺寸模式。宽高唯一配置入口为实际Widget Class对应WBP的根SizeBox；Designer用Desired预览，运行遵循同一布局及视口DPI。
- 飘字：保存命中时世界起点，把生成时扇形位移转换为固定世界终点；逐帧用当前相机/DPI投影。角色、敌人和相机移动不再带走终点；离屏/镜头后透明但继续计时清理。实现与验收见 [DamageNumbers](DamageNumbers.md)。
- 已验证：关联UE 5.8.2的UmbraEditor / Win64 / Development最终构建成功，包含新增 `Umbra.UI.WorldPresentation` 测试；本次修改的源码/文档差异空白检查通过。未更改二进制资产，保留原工作区其他修改。
- 自动化尝试：NullRHI没有有效投影视口；默认D3D12离屏运行在测试开始前发生Renderer后台线程崩溃。DX11离屏测试执行后，除分屏位移误差断言外，其余断言通过（相机移动/旋转/距离、世界位置、离屏恢复/超时、旧尺寸模式和两组WBP根尺寸）。该误差断言后依据UE反投影整像素截断规则调整为一个像素的容差并重新编译，但最终测试尚未重跑，不能标为整项通过。日志位于 `Saved/Logs/WorldPresentation*Tests.log`。
- 用户选择自行进行现场验证；本次未完成PIE视觉、实际普攻漂字跟随镜头、帧率/DPI矩阵及多人网络验收。测试可在PIE中从Session Frontend运行，或用带实际视口的 `-game` 启动后运行；不使用NullRHI。测试只修改临时实例，结束后恢复视角。

## 已实现（静态代码确认）

- top-down 玩家移动/朝向、指针寻路与追击、可攻击目标高亮、Enhanced Input 与 GAS 标签输入。
- 玩家 PlayerState / 敌人 Character 各自持有 ASC 和属性；15 个常驻属性、一次性初始化、初始调试覆盖、边界裁剪与复制声明。
- 玩家普攻连招、socket 球扫掠；敌人简单 Tick AI 与近战距离命中；敌人受击、死亡状态和尸体处理。
- 服务器物理/魔法结算、AD/AP 混合系数、暴击、双抗、IncomingDamage 最终扣血及可开关日志。
- 敌人血条、攻击者飘字、F1/F2 属性调试面板与固定调试 GE；主要 UI 监听清理。
- 维护修复：伤害表现路由已提取为 `FUmbraDamageNotification`；MoveSpeed 已由 GAS 监听并同步到当前 Avatar 的 `MaxWalkSpeed`；玩家/敌人能力清理统一进入幂等 `EndAbility`；普通攻击 Pawn 射线忽略自身 Pawn；普攻命中检测补充剑根到剑尖的球扫掠。
- 当前玩家攻击配置实际读取到 `FX_Sword_Top`、45cm 半径、三段 Montage；玩家骨骼存在 `FX_Sword_Bottom`，已设为剑身扫掠默认根 socket。
- 不等同于当前蓝图资产已正确配置或当前地图已经跑通。

## 已验证：本次整理

- 读取现有 AGENTS.md、相关 C++、Config、已有 Docs 与目标资产文件列表，核对主要调用链及配置契约。
- 整理前工作区已有 `Content/UI/Combat/WBP_DamageNumber.uasset`、`Content/UI/Enemy/WBP_EnemyHealthBar.uasset` 修改，以及未跟踪 `Content/fantasy_gui_4/`；本次仅编辑 Markdown，保留上述内容。
- 69 处本地文件链接存在性检查通过（不包含标题锚点渲染验证）；已核对表中代码入口。`git diff --check -- AGENTS.md Docs` 通过；三份新增文档另查无行尾空白，Source/Config 无差异。
- 全仓库 diff 尝试因用户已改二进制触发 Git LFS clean filter，无法写入只读 `.git/lfs/tmp` 而失败；随后限定本次 Markdown 范围完成检查。未变更 LFS 配置或资产内容，不能把这次文档检查称为全仓库检查通过。
- 本次已运行 UE 5.8.2 `UmbraEditor / Win64 / Development` 构建；维护代码与 `UmbraCombatMaintenanceTests.cpp` 编译成功。`Umbra.Combat.Maintenance` 专项自动化成功，复现并确认了自身 Pawn 遮挡修复、实际蓝图读取、GAS 移速、攻击取消/连招清理及动画命中窗口配置。完整 `Umbra.` 测试运行中的 Lifecycle、Damage.Types、DebugInputAndWidget、DebugOperations、Maintenance 均成功；日志另有 Greystone 动画蓝图 Divide by zero 警告。
- 剑根默认值 `FX_Sword_Bottom` 及本轮持续攻击/飘字源码均已进入成功的 Development 构建；地图 PIE、网络测试和视觉验收仍未运行。

## 历史验证记录（不是本次重跑）

| 证据文档 | 文档记载结果 | 不能推断的范围 |
| --- | --- | --- |
| [AttributeSetPhase1](AttributeSetPhase1.md) 的2026-09-14记录 | UE5.8.2 UmbraEditor Win64 Development 构建成功；Umbra.Attributes.Lifecycle Success | 当前源码重跑、地图普攻、蓝图旧引用、多客户端、持续效果自然到期 |
| [AttributeDebugF1F2](AttributeDebugF1F2.md) | 构建及 DebugInputAndWidget、DebugOperations、Lifecycle Success；历史编辑器只读核对 | 当前 WBP 改动、真实鼠标射线/硬件按键、网络交互 |
| [DamageNumbers](DamageNumbers.md)、[EnemyHealthBar](EnemyHealthBar.md) | 对应阶段记录完整编译成功 | 当前 UI 视觉、实际引用、DPI/帧率、多人表现 |

这些结果是保留的历史文档记录，本次没有重新核验当时日志。MagicalDamage.md 的算例是验收步骤，不是已通过结果。

## 待验证与执行方法

先保存用户资产；若修改反射类型/模块，关闭编辑器并刷新 Rider 工程，再用关联引擎构建 **UmbraEditor / Win64 / Development**。本机引擎目录需实查；命令模板在 AGENTS.md。

Session Frontend → Automation 可运行下列现有测试：

| 准确测试名 | 源文件 | 覆盖 / 限制 |
| --- | --- | --- |
| `Umbra.Attributes.Lifecycle` | `Source/Umbra/Tests/UmbraAttributeSetTests.cpp` | ASC/GE 生命周期、初始化与属性边界；独立 World，不替代地图与网络 |
| `Umbra.Damage.Types` | `Source/Umbra/Tests/UmbraDamageTests.cpp` | 物理/魔法、混合系数、必暴、类型拒绝、IncomingDamage；直接构造 Spec，不覆盖真实普攻 GE 配置、命中和飘字 |
| `Umbra.Attributes.DebugOperations` | `Source/Umbra/Tests/UmbraAttributeDebugTests.cpp` | 权威端调试修改；依赖实际 BP_UmbraPlayerController 资产 |
| `Umbra.Attributes.DebugInputAndWidget` | `Source/Umbra/Tests/UmbraAttributeDebugInputTests.cpp` | 生效的 F1/F2 配置、实际 IMC/WBP 加载、原始属性状态刷新与部分监听清理；不验证待实现的蓝图格式/布局，也不模拟真实鼠标/按键或渲染 |
| `Umbra.Combat.Maintenance` | `Source/Umbra/Tests/UmbraCombatMaintenanceTests.cpp` | 实际玩家/敌人/GA 资产、MoveSpeed、连续攻击、同击去重、攻速阈值与快照、最新指令、目标越界/死亡、强制中断；临时 World 不含真实 NavMesh，不替代动画观感和网络 PIE |
| `Umbra.UI.DamageNumberLogic` | `Source/Umbra/Tests/UmbraDamageNumberLogicTests.cpp` | 字号档边界/非法配置、暴击倍率/上限、k/M/B/T 与舍入进位；纯逻辑，不渲染 WBP |
| `Umbra.UI.WorldPresentation` | `Source/Umbra/Tests/UmbraWorldPresentationTests.cpp` | 需已运行PIE/有渲染视口的独立游戏；验证飘字世界定位、相机移动/旋转/距离、分屏、离屏计时和血条WBP期望尺寸；不替代普攻视觉与真实多人 |

最小人工验收按修改领域选择：

1. **编辑器配置**：打开 `GA_BasicAttack` Class Defaults，核对普通Montage、命中sockets/radius、高攻速开关/阈值3.0/`HighSpeedAttackMontages`的A与B、BaseAttackInterval。逐Montage在最后一个命中窗之后添加 `Umbra Attack Chain Point`；留空时另测正常结束回退。Compile/Save后再PIE。
2. **属性**：出生满池；同 ASC 重新绑定不回血；上限增益不补血、移除裁剪；临时效果不残留。具体值见 AttributeSetPhase1。
3. **持续攻击/指令**：单击敌人观察至少三击且每击一次伤害；第二击中点击两个不同地面点，安全点后只去最后一点且不多打一击；换目标同理。把目标移出/移入范围验证追近和恢复，杀死目标验证立即停止；施加 `State.Stunned` 验证立即中断并清指令。
4. **攻速节奏**：在一击中把 Bonus 从1.99改为2.0，当前击 Montage/速率不变，下一击切高速 Montage；再降回1.99，下一击从普通第一段恢复。记录相邻起手间隔，核对 `BaseAttackInterval / (1+Bonus)`，并检查命中窗不漏伤害。
5. **伤害/UI**：按 MagicalDamage 设置 AD20/AP40、系数2/0.5、护甲100/魔抗300、暴击0，预期物理30/魔法15；开 `umbra.Damage.Log 1`。按 [DamageNumbers](DamageNumbers.md) 验证字号四档、暴击倍率/上限、999960进位，以及既有扇形/停留/淡出、血条与调试 UI。
6. **生命周期与网络**：Montage 中、手动宽限期、衔接点前后分别取消；Pawn/Controller 切换、目标销毁、反复进出 PIE。另跑 Listen Server、两个客户端和条件允许时专服，确认客户端 Montage 一致且每击只有服务端 GE/伤害。

记录结果时写日期、引擎版本、目标/测试名、地图与模式、结果和未测边界；生成日志放 Saved，不提交。

## 已知维护问题

按优先级：属性结算直接依赖飘字路由；GAS MoveSpeed 与实际速度双来源；能力自身清理依赖特定 Montage/Finish 回调。次要项为普通 IMC 添加/释放不对称与少量无名战斗常量。文件行号、影响和最小建议集中在 [Architecture 的维护问题表](Architecture.md#有依据的维护问题)，避免维护两套清单。

以上包含静态可见的设计负担与尚待复现的生命周期风险，不能全部称为已复现 Bug。主要 UI 订阅已有配对清理，未发现 UI 重算伤害公式。

## 下一步（尚未实施）

1. 先执行编辑器引用核对与最小战斗/UI 基线验收，将实际结果补回本文件。
2. 把前三项维护问题各自做成独立小修改；保留行为、补针对性验证，和新功能分开审查。
3. 原型功能候选仍是玩家死亡/重新挑战与近战可读性；当前 C++ 未实现玩家死亡/重生闭环。闪避、完整技能/装备/掉落/存档不属于本次交付。
