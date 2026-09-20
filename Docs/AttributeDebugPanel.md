# Umbra 最小属性调试面板：UE 5.8 配置与验收

本指南对应 `UUmbraAttributeDebugPanel` 和 `AUmbraPlayerController` 的当前实现。
面板只观察属性，不负责初始化、重置或填满属性。操作由本地控制器发送至服务器执行。

2026-09-20：`WBP_AttributeDebugPanel` 的格式文本已改为“攻击力”“法术强度”“攻速”。攻速连接 `AttackSpeedDisplay`，显示 `1.00`、`2.30` 等两位小数，不带百分号；蓝图已由 UE 5.8 编辑器保存并重编译。真实 PIE 视觉仍待验证。

2026-09-16 维护说明：下文“已读取确认”“本次验证”属于历史功能交付记录，不是本轮重跑。资产文件已存在；当前内部与实际引用待编辑器确认，按步骤核对而非重复创建。统一配置和验证状态见 [EditorSetup](EditorSetup.md)、[Progress](Progress.md)。

## 1. 已创建与待手动配置

**已创建的是 C++ 源码，不是下表的 .uasset 资产。**

- `Source/Umbra/UI/UmbraAttributeDebugPanel.h/.cpp`：Widget 基类、完整属性快照、ASC 变化委托、目标切换和生命周期清理。
- `Source/Umbra/UmbraPlayerControllerDebug.cpp`：本地创建、Enhanced Input、目标验证、服务器 RPC、每目标效果句柄与清理。
- `Source/Umbra/UmbraPlayerController.h/.cpp`：配置入口和面板内的鼠标输入保护。
- `Source/Umbra/AbilitySystem/UmbraAbilitySystemComponent.h/.cpp`：ActorInfo 就绪、清除、组件注销时的原生生命周期通知。
- `Source/Umbra/AbilitySystem/Effects/UmbraDebugEffects.h/.cpp`：三个固定的原生 Gameplay Effect。
- `Source/Umbra/Tests/UmbraAttributeDebugTests.cpp`：权威调试操作集成测试。
- `Source/Umbra/Umbra.Build.cs`：新增 SlateCore 依赖。

**下面四个资产现已由你在 UE Editor 创建。F1/F2 问题排查已读取并确认控制器引用正确；不需要重复创建。** 下文保留完整创建步骤供核对，本次修复没有改写这些 .uasset/.umap。

| Content Browser 路径（/Game 对应 Content） | 资产类型 | 准确名称 | 父类/类型 |
| --- | --- | --- | --- |
| /Game/UI/Debug/WBP_AttributeDebugPanel | Widget Blueprint | WBP_AttributeDebugPanel | UmbraAttributeDebugPanel（UUmbraAttributeDebugPanel） |
| /Game/Input/Actions/IA_Debug_ViewPlayer | Input Action | IA_Debug_ViewPlayer | UInputAction；Digital (bool) |
| /Game/Input/Actions/IA_Debug_LockHovered | Input Action | IA_Debug_LockHovered | UInputAction；Digital (bool) |
| /Game/Input/IMC_AttributeDebug | Input Mapping Context | IMC_AttributeDebug | UInputMappingContext |

现有资产已经在仓库中，直接配置，不要重复创建：

- /Game/Blueprints/Player/BP_UmbraPlayerController
- /Game/Blueprints/Core/BP_UmbraGameMode
- /Game/Blueprints/Enemies/BP_Enemy_Melee_01
- /Game/Maps/L_Prototype

## 2. 编译与识别 C++ 父类

1. 保存编辑器中自己的资产修改，然后关闭 Unreal Editor。本次新增了 BlueprintType 状态结构和 Blueprint 事件，首次加载使用完整编译，不依赖 Live Coding 热替换。
2. 用 UE 5.8.2 打开项目对应的 Rider 工程。目标选择 **UmbraEditor / Win64 / Development** 并 Build。
3. 本次已经执行过 Rider 项目文件刷新；如果 Rider 的新文件/依赖未更新，右键 Umbra.uproject → Generate project files，或从 Rider 重新加载 .uproject。
4. 完整编译成功后，重新打开 Umbra.uproject。
5. Content Browser 的 Settings 中开启 Show C++ Classes。C++ Classes → Umbra → UI 下应能看到 UmbraAttributeDebugPanel。
6. 创建 Widget Blueprint 时展开 All Classes，搜索 **UmbraAttributeDebugPanel** 并选择。该类是抽象的 C++ 基类，供 WBP 继承，不直接放入关卡。
7. 如果已经误建为普通 UserWidget，可在该 WBP 的 File → Reparent Blueprint 中选择 UmbraAttributeDebugPanel，然后 Compile。

若父类不存在，先看 Rider 是否真正编译了 UmbraEditor，而不是只有 Umbra 游戏目标；再检查 Output Log 的模块加载/UHT 错误。不要在缺类时把父类改回 UserWidget 来绕过。

## 3. WBP 表现与窗口布局

`WBP_AttributeDebugPanel` 不再有任何 `BindWidget` 名称或控件类型要求。Designer 可以自由调整控件树、窗口尺寸、字体、颜色、单位、格式与按钮布局。C++ 仅保留 `AddToPlayerScreen(20)`；锚点、对齐、位置和 Desired Size 不再硬编码，因此必须在 WBP 的 Construct（或等价蓝图表现入口）中设置局部 Viewport Slot，避免默认全屏槽位拦截输入。

若要保留旧外观，可在 WBP 中继续使用左上角锚点、对齐 `(0,0)`、位置 `(16,64)`、尺寸 `(360,640)`；这些值现在只是蓝图选择，不是 C++ 契约。根控件应只覆盖可见窗口范围，按钮关闭 Is Focusable，文字可设为 Not Hit-Testable。Class Defaults 中关闭 UserWidget 的 Is Focusable，Tick Frequency 设为 Never。

## 4. 属性状态事件与按钮事件

C++ 会在目标绑定、任一展示属性变化、选择反馈变化和 ASC 生命周期变化时调用蓝图事件 **Apply Attribute Debug State**，参数为 `FUmbraAttributeDebugViewState`。蓝图负责把它写入任意控件：

- `TargetActor`、`TargetName`、`bViewingPlayer`、`bReady` 与 `Feedback` 提供目标及状态上下文；等待文字、反馈文字和按钮 Enabled 均由蓝图决定。
- 15 项 GAS 数据都以 float 传递：Health、MaxHealth、HealthRegen、Resource、MaxResource、ResourceRegen、AttackPower、AbilityPower、AttackSpeed、CriticalChance、CriticalDamageMultiplier、Armor、MagicResistance、AbilityHaste、MoveSpeed。
- AttackSpeed 仍以直接倍率浮点值传递，另有 `AttackSpeedDisplay` 按两位小数给面板显示（`1.00`、`2.30`），不追加 `%`。CriticalChance `1 → 100`、CriticalDamageMultiplier `2 → 200`，这两个字段的文字才追加 `%`。对应标签显示“攻击力”“法术强度”“攻速”。
- IncomingDamage 不传递，也不注册 UI 监听。
- C++ 仍负责 ASC 委托、Pawn/PlayerState/目标生命周期及成对清理，没有 Tick 或计时轮询。

四个按钮在 WBP Graph 的 OnClicked 中分别调用 **Request Operation**，枚举值为 Add Effect、Remove Effect、Damage、Heal。此入口只发送当前目标与固定操作；服务器仍验证权限、距离和目标，蓝图不得自行 Set Attribute 或 Apply Gameplay Effect。调用后 C++ 会把焦点交回游戏 Viewport。

不要在蓝图重复 Create Widget、Add to Viewport、GAS 初始化、属性监听或 F1/F2 输入绑定。2026-09-16 当时未修改 `WBP_AttributeDebugPanel.uasset`；当前资产已在 2026-09-20 由 UE 编辑器更新属性格式文本及连线，按钮和实际 PIE 视觉仍应在编辑器中检查。

## 5. Enhanced Input 配置

1. 在 /Game/Input/Actions 创建 Input Action，命名 **IA_Debug_ViewPlayer**。
2. Value Type 设置 **Digital (bool)**。Triggers 与 Modifiers 留空，Consume Input 保持开启。
3. 同样创建 **IA_Debug_LockHovered**，同样为 Digital (bool)，Triggers/Modifiers 留空。
4. 在 /Game/Input 创建 Input Mapping Context，命名 **IMC_AttributeDebug**。
5. 添加两条 Mapping：IA_Debug_ViewPlayer → **F1**；IA_Debug_LockHovered → **F2**。每条 Mapping 的 Triggers/Modifiers 也留空。
6. 不添加 Hold、Pulse 或重复 Trigger。C++ 使用 ETriggerEvent::Started，一次按下只切换一次。
7. 不把这两个 Action 再加入玩家角色的 Ability Input Actions，也不在其他蓝图中 BindAction。
8. **不要把 IMC_AttributeDebug 加入 Default Mapping Contexts / Mobile Excluded Mapping Contexts。** 下一节专门配置 Attribute Debug Mapping Context；C++ 自动按优先级 10 添加一次，并在清理面板时移除自己添加的映射与绑定。
9. 现有 IMC_Default、移动和攻击 Action 不需要改动。

项目 DefaultInput.ini 已移除引擎默认的 F1 → viewmode wireframe、F2 → viewmode unlit 调试绑定。它们独立于 Enhanced Input，单靠 Consume Input 不能解决冲突。修改生效需要重新启动编辑器；若旧 PIE 画面已变成线框/无光照，按 F3 或在控制台执行 viewmode lit 恢复。

## 6. PlayerController 与 GameMode 配置

打开 /Game/Blueprints/Player/BP_UmbraPlayerController，进入 Class Defaults，在 **Debug → Attributes** 配置：

| 显示名 | C++ 属性名 | 应设置为 |
| --- | --- | --- |
| Enable Attribute Debug Panel | bEnableAttributeDebugPanel | **勾选**（C++ 默认关闭） |
| Attribute Debug Panel Class | AttributeDebugPanelClass | **WBP_AttributeDebugPanel** |
| Attribute Debug Mapping Context | AttributeDebugMappingContext | **IMC_AttributeDebug** |
| View Player Attributes Action | ViewPlayerAttributesAction | **IA_Debug_ViewPlayer** |
| Lock Hovered Attributes Action | LockHoveredAttributesAction | **IA_Debug_LockHovered** |

Compile、Save。不要在 BeginPlay 再创建一份 Widget，也不要重复设置输入模式。

建议同时关闭现有的 **Show Attack Highlight Debug** 与 **Force Highlight Debug**，避免旧的大段屏幕诊断文字遮住面板。这两个开关不是新面板的启用条件，也不改变正常攻击输入。

打开 /Game/Blueprints/Core/BP_UmbraGameMode 的 Class Defaults，确认 Player Controller Class 为 BP_UmbraPlayerController，保留现有 PlayerState Class 和 Default Pawn Class。
打开 /Game/Maps/L_Prototype，检查 World Settings 的 GameMode Override 没有覆盖成其他不匹配的 GameMode。项目配置中的默认地图与默认 GameMode 已指向上述资产，但仍需检查地图级覆盖。

多人 PIE 时，服务器必须使用同一已开启 Enable Attribute Debug Panel 的 Controller 类。仅在客户端实例临时开启不会授权服务器修改。

现有控制器已经设置 Game And UI、显示鼠标；调试面板不替换这些设置。面板空白处吞掉鼠标按下/双击，按钮使用标准 UMG 点击；鼠标进入面板时取消尚未完成的鼠标寻路/追击/长按状态，避免松键后穿透。已经开始的攻击 Ability 不会被调试面板强制取消。

移除面板时调用控制器的 **Remove Attribute Debug Panel** 节点（如临时关闭调试功能），它会移除 UI、监听、专用映射与绑定，并请求服务器清理本控制器创建的测试增益。不会填血或初始化属性。移除上限增益仍会按现有规则裁剪生命。
没有新增输入模式需要恢复；既有 Game And UI 与移动/攻击映射保持原设置。要再次创建面板，当前最小版本重新进入 PIE。

## 7. Gameplay Effect 配置：无需手建资产

这三个类已经在 C++ 中固定定义。**不要再创建额外 GE 蓝图，也不要在按钮里追加应用 GE。**

| 原生类 | Duration Policy | Modifier Attribute | Operation | Magnitude |
| --- | --- | --- | --- | --- |
| UmbraDebugAttributeEffect | Infinite | Health、MaxHealth、HealthRegen、Resource、MaxResource、ResourceRegen、AttackPower、AbilityPower、Armor、MagicResistance、AbilityHaste、MoveSpeed | Additive | 每层 +20 |
| 同一 UmbraDebugAttributeEffect | Infinite | AttackSpeed、CriticalChance、CriticalDamageMultiplier | Additive | 每层 +0.2；AttackSpeed/CriticalChance 最终分别封顶 10.0/1.0 |
| UmbraDebugDamageEffect | Instant | UmbraAttributeSet.IncomingDamage | Additive | 10 |
| UmbraDebugHealEffect | Instant | UmbraAttributeSet.Health | Additive | 10 |

所有 GE 的 Period 都为 0，无周期恢复。每次点击 Add Effect 都新增一层，没有人为层数上限；测试增益持续到点击移除或面板/控制器清理。Health/Resource、CriticalChance 等仍遵守 AttributeSet 自身边界。
上述固定 10 点调试伤害不走常规 AttackPower/AbilityPower 伤害公式、护甲计算或暴击随机计算。

服务器按 ASC 保存每一层的 ActiveGameplayEffectHandle：同一个控制器对同一目标可重复添加；Remove Effect 会一次移除该控制器在当前目标上添加的全部层。切换目标不会移除或遗失句柄；只按本控制器存储的句柄移除，不按效果类别/Tag 批量删除其他来源的效果。
两个不同控制器各自添加的增益是各自的调试实例，互不拥有对方的移除权限。

客户端只发送目标和操作枚举；不能指定 GE 类、伤害量或修饰值。服务器验证目标属于本世界，且是自己的 Pawn/PlayerState，或距离自己 Pawn **10000 厘米以内**的 UmbraEnemyCharacter。超过范围的敌人仍可被锁定显示，但测试操作会拒绝；走近后再操作，或移除面板清理自己创建的效果。
Shipping 和 Test 构建不创建面板、不绑定调试输入、RPC 修改分支不执行，三个原生 GE 也不包含测试 Modifier。

## 8. 找到敌人并进入 PIE

1. 打开 L_Prototype，在 World Outliner 搜索 BP_Enemy_Melee_01。若已经有实例，直接使用。
2. 若没有，从 /Game/Blueprints/Enemies 将 BP_Enemy_Melee_01 拖入场景，放在现有 NavMesh 可行走区域、玩家出生点附近；不改动其 ASC、攻击 Ability、动画和初始化 GE。
   如需不反击的测试目标，在敌人蓝图 Class Defaults → AI 中取消 **Enable AI Behavior**；也可选中关卡中的单个敌人，在 Details → AI 中只对该实例关闭。默认开启，这是开局配置，修改后重新进入 PIE。关闭会阻止寻敌、追击、转向和普攻，仍保留受击、死亡、属性和悬停高亮。
3. F2 现在使用与攻击悬停相同的 **Pawn 对象查询及 Attackable 接口筛选**，限定 UmbraEnemyCharacter，再通过 GetCursorGroundHit 的 **Visibility** 首个命中检查遮挡。
4. 已检查 BP_Enemy_Melee_01，其 Capsule/Mesh 忽略 Visibility。当前修复兼容这个配置，不需要把它们改为 Block，也不修改原有碰撞通道。
5. 前方障碍物若先阻挡 Visibility，F2 不会穿过它锁定敌人。仍不能选中时，检查鼠标是否在敌人可碰撞的身体上、是否被遮挡、是否处于可被攻击状态，以及面板下方的新选择反馈。
6. Play → Selected Viewport 或 New Editor Window。默认左侧显示本地玩家。若 ASC 尚未准备好，短暂显示等待状态，随后由生命周期通知绑定。
7. 鼠标放在活着且可被攻击的敌人上，按 F2。名称变为该敌人；移开鼠标后保持该目标。
8. F2 在地面、UI 或无效目标上按下时，保持原目标；F1 随时切回自己的玩家。
9. 锁定后敌人死亡且 Actor 仍存在时显示 0 生命，不主动切换。现有敌人默认 Corpse Lifetime 为 5 秒；Actor 销毁后自动回玩家。若想观察尸体，可在测试实例/蓝图把 Corpse Lifetime 设为 0，测试后恢复自己的设置。

## 9. 配置缺失时如何定位

| 现象 | 按顺序检查 |
| --- | --- |
| 新 C++ 父类不出现 | 关闭编辑器完整编译 UmbraEditor；确认 UE 5.8.2；Show C++ Classes；检查 UHT/模块错误 |
| WBP 编译后不刷新 | 确认父类正确，并实现 `Apply Attribute Debug State`；不再有固定控件名或 BindWidget 契约 |
| 没有面板 | 实际 GameMode/Controller 类；Enable Attribute Debug Panel；Widget Class；Output Log 搜索“Attribute debug”；不要用 Shipping/Test |
| 面板存在但长时间等待 | Pawn/PlayerState 是否为现有 GAS 角色；玩家 PossessedBy/OnRep_PlayerState 是否调用初始化入口；ASC 是否有 UmbraAttributeSet；不要通过 UI 重初始化来掩盖问题 |
| 文本始终不变化 | 检查 `Apply Attribute Debug State` 是否把 State 写入控件；确认使用目标的 ASC 和服务器 GE；客户端检查复制及 Actor 网络相关性 |
| F1/F2 无反应 | 两个 Action 是 Digital bool；三个 Input 资产引用完整；只加入专用映射属性；按钮 Is Focusable 关闭；PIE 窗口有焦点 |
| 点击 UI 同时移动/攻击 | 根是局部 Border，Visible；按钮 Visible；不要全树设为 Not Hit-Testable；不要绕过现有控制器在蓝图另绑 LMB 攻击/移动 |
| UI 外完全不能操作 | WBP Construct 必须把默认全屏 Viewport Slot 改成局部锚点/位置/尺寸；删除全屏命中控件；不要用 Set Input Mode UI Only，也不要再次 AddToViewport |
| 按钮可点但属性不变 | 服务器 Controller 开关是否开启；目标 ASC 就绪；敌人是否在 10000cm 范围；Output Log 是否有“Attribute debug rejected target”；等待复制 |
| F2 无法选中高亮敌人 | 鼠标是否位于身体碰撞范围，是否被更近的 Visibility 障碍物遮挡，是否仍 CanBeAttacked；看面板反馈及 Output Log 的 Attribute debug: F2 行 |
| F1/F2 使画面变色/线框 | 关闭并重启编辑器加载新的 DefaultInput.ini；PIE 中 F3 或控制台 viewmode lit 恢复正常光照。不要在编辑器未 Play 的场景视口中执行游戏调试键 |
| 单击一次却增加两层 | 每个按钮只调用一次 `Request Operation`，不要自行应用 GE；确认没有在另一控制器窗口也加了独立增益 |
| 普攻旧行为异常 | 确认未改旧 DamageEffectClass、IMC_Default 或角色 Ability Input Actions；本调试功能没有重接原攻击 GE |

Output Log 打开方式：Window → Developer Tools → Output Log，或底部 Output Log 标签。

## 10. 按顺序验收

下面是待执行的编辑器验收清单，不代表已经实测通过：

1. **初始玩家**：PIE 后默认玩家名称正确，生命/资源与原初始数据一致，暴击总倍率 2 显示 200%；重复 F1 不改变任何属性。
2. **添加/移除多层**：记下15项原值；连点添加三次，普通数值增加60，三个比例/倍率值增加0.6，其中 CriticalChance 不超过1.0。点击一次 Remove Effect 后，本控制器在当前目标添加的三层应全部恢复；再次移除不改变数值。
3. **当前池与上限**：先受到10点伤害再添加一层，Health 与 MaxHealth 都增加20（90/100 → 110/120）；Resource 与 MaxResource 同理。移除后回到原池值与原上限；治疗仍不超过当前 MaxHealth。
4. **治疗与伤害**：连续治疗不超过 MaxHealth；每次伤害扣10，后续添加/移除效果不会重复扣血。
5. **F2 锁定**：悬停有效敌人按F2，移开鼠标仍显示该敌人；对地面再按F2不改变目标。
6. **实际普攻刷新**：在 UI 外正常攻击锁定敌人，每次伤害后面板 Health 通过委托实时更新；检查原受击和死亡行为。
7. **按目标移除**：玩家添加增益，F2切敌人并添加增益；F1回玩家移除，敌人效果仍在；再F2回敌人可以移除它自己的增益。
8. **死亡/销毁**：锁定敌人被杀，尸体存在时显示0；Corpse Lifetime 到期后安全切回玩家，无空指针日志。
9. **输入隔离**：点击四个按钮、面板空白处、双击背景都不启动地面移动/攻击；点击后F1/F2仍有效。面板外LMB、原普攻键、WASD保持原操作。世界中按住鼠标后移入面板再松开，不出现残留寻路。
10. **网络与清理**：两客户端PIE，确认各自默认看自己的玩家，客户端按钮由服务器修改且复制可见；销毁/重新Possess本地Pawn不重置属性；调用 Remove Attribute Debug Panel 后恢复纯游戏区域，测试增益清理、没有残留回调。

自动化入口：Session Frontend → Automation → Umbra → Attributes，运行 Lifecycle 与 DebugOperations。
DebugOperations 使用真实 ASC、原生 GE 和项目现有 Controller 蓝图，在独立 Game World 调用服务器操作入口；它不模拟真实网络传输或鼠标 UI 点击。

## 11. 本次验证记录（2026-09-14）

2026-09-17 C++/蓝图边界与 Add Effect 叠加补充：UE 5.8.2 `UmbraEditor / Win64 / Development` 构建成功；`Umbra.Attributes.DebugInputAndWidget`、`Umbra.Attributes.DebugOperations` 运行结果均为 Success（2/2、0 warning、0 error）。C++ 本轮未改写 WBP 资产，故不包含 `Apply Attribute Debug State`、按钮连线、窗口布局或 PIE 视觉验证。

| 项目 | 结果与范围 |
| --- | --- |
| UE 5.8.2 UmbraEditor / Win64 / Development | 完整编译成功，包含 UHT、新 Widget 类、RPC、原生 GE 和测试 |
| Rider 项目文件刷新 | UBT -ProjectFiles -Game -Rider 成功；安装版引擎的部分 Program 目标产生不支持提示，最终生成结果为 Succeeded |
| Umbra.Attributes.Lifecycle | Success，既有属性边界/初始化/伤害回归通过 |
| Umbra.Attributes.DebugOperations | Success，服务器开关、固定伤害、15项统一增益、比例值按0.2、多层叠加、暴击率封顶、按目标清除全部自有层、其他来源同类效果保留、治疗封顶、伤害不重复及目标销毁清理 |
| 自动化进程 | 退出码 0；两个测试的 BeginEvents/EndEvents 内无错误 |
| git diff --check | 通过；未提交或推送；未改写二进制资产 |

测试日志在 Saved/Logs/UmbraAttributeDebugTests.log（生成文件，不提交）。
编译有现存 MSVC 非首选版本与引擎 GetMovementBase 弃用警告；编辑器启动注册测试时仍有引擎 Condition failed 日志，两个 Umbra 测试自身结果均为 Success。

**尚未验证/尚需手动完成：** 四个新资产的创建与控制器引用配置、WBP 实际视觉布局、Slate 鼠标点击/输入穿透、F1/F2 点击 UI 后的真实路由、地图中的普攻/移动/死亡演示、UI 的目标销毁自动回退与监听清理实测、双客户端网络传输/复制、打包后的 Shipping/Test 运行。代码中的开发构建保护已实现，但本次没有执行 Shipping/Test 打包运行。

本次没有运行真实地图 PIE；以上自动化成功不能代替第10节的编辑器验收清单。

**后续 F1/F2 修复验证补充：** 用户已完成四个资产的创建，控制器引用读取检查正确。历史 DebugInputAndWidget 测试曾覆盖实际 WBP 文字；2026-09-17 C++/蓝图边界改为原始状态事件后，测试只覆盖状态刷新和监听清理，新的 WBP 事件、布局、真实硬件按键/鼠标射线、视觉画面和网络均需重新验收。准确操作与根因见 AttributeDebugF1F2.md。
