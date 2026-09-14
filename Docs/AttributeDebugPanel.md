# Umbra 最小属性调试面板：UE 5.8 配置与验收

本指南对应 `UUmbraAttributeDebugPanel` 和 `AUmbraPlayerController` 的当前实现。
面板只观察属性，不负责初始化、重置或填满属性。操作由本地控制器发送至服务器执行。

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

1. 保存编辑器中自己的资产修改，然后关闭 Unreal Editor。此次新增了 UCLASS、BindWidget 属性、RPC 和模块依赖，首次加载使用完整编译，不依赖 Live Coding 热替换。
2. 用 UE 5.8.2 打开项目对应的 Rider 工程。目标选择 **UmbraEditor / Win64 / Development** 并 Build。
3. 本次已经执行过 Rider 项目文件刷新；如果 Rider 的新文件/依赖未更新，右键 Umbra.uproject → Generate project files，或从 Rider 重新加载 .uproject。
4. 完整编译成功后，重新打开 Umbra.uproject。
5. Content Browser 的 Settings 中开启 Show C++ Classes。C++ Classes → Umbra → UI 下应能看到 UmbraAttributeDebugPanel。
6. 创建 Widget Blueprint 时展开 All Classes，搜索 **UmbraAttributeDebugPanel** 并选择。该类是抽象的 C++ 基类，供 WBP 继承，不直接放入关卡。
7. 如果已经误建为普通 UserWidget，可在该 WBP 的 File → Reparent Blueprint 中选择 UmbraAttributeDebugPanel，然后按下面名称创建控件并 Compile。

若父类不存在，先看 Rider 是否真正编译了 UmbraEditor，而不是只有 Umbra 游戏目标；再检查 Output Log 的模块加载/UHT 错误。不要在缺类时把父类改回 UserWidget 来绕过。

## 3. 创建 WBP 与完整控件层级

在 /Game/UI/Debug 创建 WBP_AttributeDebugPanel。Designer 中删除默认 Canvas Panel（若存在），改用 **Border 作为根控件**。
不创建覆盖全屏的透明 Canvas/Overlay/Border，也不另加屏幕捕获控件。

准确层级如下；缩进表示父子关系：

```text
PanelBackground                 Border（根）
└─ PanelColumn                  Vertical Box
   ├─ TargetNameText            Text Block
   ├─ AttributesText            Text Block
   ├─ ActionsSpacer             Spacer
   ├─ ActionsColumn             Vertical Box
   │  ├─ AddEffectButton        Button
   │  │  └─ AddEffectLabel      Text Block
   │  ├─ RemoveEffectButton     Button
   │  │  └─ RemoveEffectLabel   Text Block
   │  ├─ DamageButton           Button
   │  │  └─ DamageLabel         Text Block
   │  └─ HealButton             Button
   │     └─ HealLabel           Text Block
   ├─ FooterSpacer              Spacer
   └─ HintText                  Text Block
```

**必须勾选 Is Variable 且名称逐字匹配 BindWidget 的七个控件：**
`TargetNameText`、`AttributesText`、`HintText`、`AddEffectButton`、`RemoveEffectButton`、`DamageButton`、`HealButton`。
它们的控件类型也必须正确；不能把 Text Block 换成 Rich Text Block。
其余控件不参与 C++ 绑定，按下面名字命名便于核对，Is Variable 不勾选即可。

| 从 Palette 拖入 | 准确名称 | Is Variable | 具体设置 |
| --- | --- | --- | --- |
| Border | PanelBackground | 否 | 根；Visibility=Visible；Brush Color RGBA=(0.025,0.03,0.04,0.85)；Render Opacity=1；Padding 四边 16；Horizontal/Vertical Alignment=Fill |
| Vertical Box | PanelColumn | 否 | 拖入 Border；Border Slot 横纵 Fill；Visibility=Not Hit-Testable (Self Only) |
| Text Block | TargetNameText | **是** | Vertical Box Slot=Auto；Padding Bottom=8；字体 18，浅白色；Auto Wrap Text 开启；初始文本“当前目标” |
| Text Block | AttributesText | **是** | Slot=Auto；字体 15，浅白色；Line Height Percentage=1.15；Auto Wrap Text 关闭，避免数值长行挤成两行；初始文本“等待属性…” |
| Spacer | ActionsSpacer | 否 | Size Y=10；Slot=Auto |
| Vertical Box | ActionsColumn | 否 | Slot=Auto；横向 Fill；Visibility=Not Hit-Testable (Self Only) |
| Button | AddEffectButton | **是** | Slot=Auto，横向 Fill，Padding=(0,3,0,3)；Content Padding=(8,6,8,6)；**Is Focusable 关闭**；Click Method=Down And Up |
| Text Block | AddEffectLabel | 否 | 放进对应 Button；文本“添加测试效果”；字体 14；居中 |
| Button | RemoveEffectButton | **是** | 同 AddEffectButton 的布局/交互设置，Is Focusable 关闭 |
| Text Block | RemoveEffectLabel | 否 | 文本“移除测试效果”；字体 14；居中 |
| Button | DamageButton | **是** | 同 AddEffectButton 的布局/交互设置，Is Focusable 关闭 |
| Text Block | DamageLabel | 否 | 文本“受到10点伤害”；字体 14；居中 |
| Button | HealButton | **是** | 同 AddEffectButton 的布局/交互设置，Is Focusable 关闭 |
| Text Block | HealLabel | 否 | 文本“恢复10点生命”；字体 14；居中 |
| Spacer | FooterSpacer | 否 | Vertical Box Slot Size=Fill（1）；最小 Size Y=8 |
| Text Block | HintText | **是** | Slot=Auto；字体 12，灰白色；Auto Wrap Text 开启；初始文本随意，运行时由 C++ 设置 |

所有 Text Block 的 Visibility 可设为 Not Hit-Testable (Self & All Children)，让文字不单独参与鼠标命中。按钮保持 Visible；不要把 ActionsColumn 设为 Self & All Children，否则按钮也无法点击。

**定位与尺寸由控制器 C++ 设置，WBP 不需要 Canvas 锚点：**

- AddToPlayerScreen，ZOrder=20。
- Viewport Anchors=(0,0)，Alignment=(0,0)，左上角位置=(16,64)。
- Viewport Desired Size=(360,640)，单位为 DPI 缩放前的 UI 布局单位；边框内可用宽度为 328。
- Designer 的预览尺寸可设 Custom 360×640，便于检查文本和按钮。
- 若窗口低于约 720 个布局单位高，面板可能超出窗口；验收先用足够高的 PIE 窗口。此最小版本不实现响应式折叠。

Class Defaults 中将 UserWidget 的 Is Focusable 关闭，Tick Frequency 可设 Never。本面板没有 Event Tick、属性轮询或 UMG Text Bind。
完成后 Compile、Save；BindWidget 缺失会在蓝图编译时报错。

## 4. 文本刷新与按钮事件

**以下全部由 C++ 自动完成，不要在 WBP Graph 重复实现：**

- NativeConstruct 自动绑定四个 Button 的 OnClicked。
- 绑定目标时立即读取全部常驻属性，并设置 TargetNameText、AttributesText。
- 15 个常驻属性分别监听 ASC 的属性变化委托；任意变化时重读完整快照。
- 当前/最大生命合并一行，当前/最大资源合并一行，所以 AttributesText 共 13 行，仍覆盖全部 15 个常驻属性。
- HealthRegen/ResourceRegen 显示“点/秒”；AttackSpeedBonus、CriticalChance 乘 100 显示百分比；CriticalDamageMultiplier=2 显示 **200.0%**，表示总伤害两倍；MoveSpeed 显示“厘米/秒”；AbilityHaste 显示普通数值。
- IncomingDamage 不显示，也不注册 UI 监听。
- HintText 设置为两行：“F1 查看玩家”“悬停敌人后按 F2 锁定查看”。
- 尚未就绪时显示等待文本并禁用四个按钮。
- Pawn 变化、PlayerState 复制到达、ASC ActorInfo 初始化会触发重新尝试绑定，没有重试 Tick/计时轮询。
- 切换目标、目标 EndPlay、ASC ClearActorInfo/注销、Widget Destruct 时移除相应监听。
- 按钮点击后把键盘焦点交回游戏 Viewport，配合按钮不获取焦点，允许继续使用 F1/F2。

**WBP 蓝图只负责上节的布局、字体、颜色和四个按钮标签。**
不要添加按钮 OnClicked 蓝图节点、Create Widget、Add to Viewport、GAS 初始化、Set Attribute、Apply Gameplay Effect、F1/F2 键事件，或给 Text 属性点 Bind。

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
| UmbraDebugAttributeEffect | Infinite | UmbraAttributeSet.AttackPower | Additive | 20 |
| 同一 UmbraDebugAttributeEffect 的第二个 Modifier | Infinite | UmbraAttributeSet.MaxHealth | Additive | 100 |
| UmbraDebugDamageEffect | Instant | UmbraAttributeSet.IncomingDamage | Additive | 10 |
| UmbraDebugHealEffect | Instant | UmbraAttributeSet.Health | Additive | 10 |

所有 GE 的 Period 都为 0，无周期恢复。测试增益持续到点击移除或面板/控制器清理。
没有伤害公式、AttackPower 伤害绑定、护甲计算或暴击随机计算。

服务器按 ASC 保存 ActiveGameplayEffectHandle：同一个控制器对同一目标重复添加会直接返回；切换目标不会移除或遗失句柄；只按本控制器存储的句柄移除，不按效果类别/Tag 批量删除其他来源的效果。
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
| WBP 编译报 BindWidget 错 | 七个必需名称的大小写、控件类型、Is Variable；确认父类；不要在 Graph 新建同名普通变量代替 Designer 控件 |
| 没有面板 | 实际 GameMode/Controller 类；Enable Attribute Debug Panel；Widget Class；Output Log 搜索“Attribute debug”；不要用 Shipping/Test |
| 面板存在但长时间等待 | Pawn/PlayerState 是否为现有 GAS 角色；玩家 PossessedBy/OnRep_PlayerState 是否调用初始化入口；ASC 是否有 UmbraAttributeSet；不要通过 UI 重初始化来掩盖问题 |
| 文本始终不变化 | 删除 Text 属性上的 Bind；删除蓝图中覆盖 SetText 的逻辑；确认使用目标的 ASC 和服务器 GE；客户端检查复制及 Actor 网络相关性 |
| F1/F2 无反应 | 两个 Action 是 Digital bool；三个 Input 资产引用完整；只加入专用映射属性；按钮 Is Focusable 关闭；PIE 窗口有焦点 |
| 点击 UI 同时移动/攻击 | 根是局部 Border，Visible；按钮 Visible；不要全树设为 Not Hit-Testable；不要绕过现有控制器在蓝图另绑 LMB 攻击/移动 |
| UI 外完全不能操作 | 删除全屏 Canvas/Border；不要用 Set Input Mode UI Only；检查 WBP 是否自行 AddToViewport 全屏；保留 C++ 的 360×640 局部槽位 |
| 按钮可点但属性不变 | 服务器 Controller 开关是否开启；目标 ASC 就绪；敌人是否在 10000cm 范围；Output Log 是否有“Attribute debug rejected target”；等待复制 |
| F2 无法选中高亮敌人 | 鼠标是否位于身体碰撞范围，是否被更近的 Visibility 障碍物遮挡，是否仍 CanBeAttacked；看面板反馈及 Output Log 的 Attribute debug: F2 行 |
| F1/F2 使画面变色/线框 | 关闭并重启编辑器加载新的 DefaultInput.ini；PIE 中 F3 或控制台 viewmode lit 恢复正常光照。不要在编辑器未 Play 的场景视口中执行游戏调试键 |
| 数值被加两次 | 删除蓝图 OnClicked/F1/F2 的重复逻辑；确认没有在另一控制器窗口也加了独立增益 |
| 普攻旧行为异常 | 确认未改旧 DamageEffectClass、IMC_Default 或角色 Ability Input Actions；本调试功能没有重接原攻击 GE |

Output Log 打开方式：Window → Developer Tools → Output Log，或底部 Output Log 标签。

## 10. 按顺序验收

下面是待执行的编辑器验收清单，不代表已经实测通过：

1. **初始玩家**：PIE 后默认玩家名称正确，生命/资源与原初始数据一致，暴击总倍率 2 显示 200%；重复 F1 不改变任何属性。
2. **添加/移除无累积**：记下原 AttackPower 和 MaxHealth；连点添加三次，只增加 20/100；连点移除三次只恢复一次。重复整个循环三遍，数值不漂移。
3. **上限不回血**：先受到10点伤害再添加增益，Health 不增加、MaxHealth 增加100；治疗到大于原上限，移除增益后 Health 裁剪到原上限。
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

| 项目 | 结果与范围 |
| --- | --- |
| UE 5.8.2 UmbraEditor / Win64 / Development | 完整编译成功，包含 UHT、新 Widget 类、RPC、原生 GE 和测试 |
| Rider 项目文件刷新 | UBT -ProjectFiles -Game -Rider 成功；安装版引擎的部分 Program 目标产生不支持提示，最终生成结果为 Succeeded |
| Umbra.Attributes.Lifecycle | Success，既有属性边界/初始化/伤害回归通过 |
| Umbra.Attributes.DebugOperations | Success，服务器开关、固定伤害、重复添加防叠加、多目标句柄、同类其他来源效果保留、治疗封顶、最大生命裁剪、伤害不重复、目标销毁后的效果清理、百分比格式通过 |
| 自动化进程 | 退出码 0；两个测试的 BeginEvents/EndEvents 内无错误 |
| git diff --check | 通过；未提交或推送；未改写二进制资产 |

测试日志在 Saved/Logs/UmbraAttributeDebugTests.log（生成文件，不提交）。
编译有现存 MSVC 非首选版本与引擎 GetMovementBase 弃用警告；编辑器启动注册测试时仍有引擎 Condition failed 日志，两个 Umbra 测试自身结果均为 Success。

**尚未验证/尚需手动完成：** 四个新资产的创建与控制器引用配置、WBP 实际视觉布局、Slate 鼠标点击/输入穿透、F1/F2 点击 UI 后的真实路由、地图中的普攻/移动/死亡演示、UI 的目标销毁自动回退与监听清理实测、双客户端网络传输/复制、打包后的 Shipping/Test 运行。代码中的开发构建保护已实现，但本次没有执行 Shipping/Test 打包运行。

本次没有运行真实地图 PIE；以上自动化成功不能代替第10节的编辑器验收清单。

**后续 F1/F2 修复验证补充：** 用户已完成四个资产的创建，控制器引用读取检查正确。新增 DebugInputAndWidget 测试已通过，覆盖有效输入配置、实际 WBP 敌人显示、生命委托更新及销毁回退；前述“资产尚未创建”和“UI 回退未实测”的初次交付状态由此更新。真实硬件按键/鼠标射线、视觉画面和网络仍未手动验证。准确操作与根因见 AttributeDebugF1F2.md。
