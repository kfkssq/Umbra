# 物品掉落与拾取：第一阶段

2026-10-21：[标签与模型悬停修正](WorldItemLabels.md) 覆盖下文骨骼模型 NoCollision 的旧描述：骨骼模型现在具有仅 Visibility 的三角形查询，物理仍只由代理负责。可见标签矩形优先选择物品，标签屏幕避让不改变实际落点。

2026-10-21 物理修订：取消正常超时瞬移，接地后渐增阻尼，连续低速后保持当前姿态。支持逐物品有向盒和编辑器拟合按钮，现有6件定义已配置；异常掉出/长期无支撑才回收。见 [DropPhysics](DropPhysics.md)。

2026-10-21 更新：[短时掉落物理](DropPhysics.md) 替代默认三段曲线。Actor 根改为物理盒体，拾取球不再是根；服务器模拟、同步姿态、落稳固定。下文旧“仅视觉运动、无物理”描述仅适用于 DropPhysics.Enabled=false；库存/身份/RPC 与模型 Hover 契约继续有效。

2026-10-20：[1730 防具扩展](Armor1730.md) 增加可选 SkeletalMesh 显示与独立测试生成器。以下仅 StaticMesh 的历史说明由新文档补充；拾取权威、GUID 和库存链路不变。


2026-10-10。源码实现；Content 未改动，所有下述资产父类、Graph、引用、碰撞和地图导航均待编辑器确认。当前编译结果与未验证边界见 [Progress](Progress.md)。

## 现有接口与数据流

- `FUmbraItemInstance::Create` 已提供权威实例工厂，复用 AffixRollConfigs、BuildCandidates、重复 AffixId/候选值校验。本轮直接调用，不改生成规则，也不临时加入背包。
- Enemy 的 `Die` 已检查 Authority/bIsDead；先设置 bIsDead，再调用 SpawnDeathLoot，随后执行原 ApplyDeathState。Health 重复通知、OnRep_IsDead、动画和 OnDeathStarted 均不另发掉落。尸体继续禁碰撞、冻结姿势、按原寿命销毁。
- `LootTable.Select` → 找可达地面 → 每件创建一次 GUID 与完整实例 → deferred spawn WorldItem、初始化实例、FinishSpawning。物品无敌人 Owner/Attach，不随尸体销毁。
- WorldItem 复制完整 ItemInstance 与拾取状态，名称/品质/图标来自 Definition。客户端不采样。GetItemInstance 保留完整候选、SelectedTier、Value，未来可转成现有 TooltipDataBuilder 输入；本阶段不新建 Tooltip 格式化规则。
- `PlayerController.ServerPickupWorldItem(目标引用)` → WorldItem.TryPickup → PlayerState.AcceptLootInstance → Inventory.AddInstance → 原 OwnerOnly Snapshot / OnInventoryChanged → InventoryMenu 刷新。背包/装备 Tooltip 接口不变。

## 掉落规则及配置来源

| 参数 | 配置位置与覆盖顺序 | 单位/运行时规则 |
| --- | --- | --- |
| Entries / NoDropWeight | UUmbraLootTable DataAsset | 每次死亡只加权选一个条目；无掉落权重作为另一个结果 |
| Weight / DropChance | 表内条目 | 有效正权重参与；选中后再判断 0..1 概率，失败不重选；默认概率 1 |
| MinCount / MaxCount | 表内条目 | 闭区间均匀整数；每件独立实例，不堆叠；两端硬钳制到 16，最多 16 Actor/敌人 |
| bEnableLoot / LootTable / WorldItemClass | C++ 默认 → 敌人 BP → 场景实例 | 默认启用但空表不掉；空 Actor 类使用原生 WorldItem |
| LootScatterRadius | C++ 140 → 敌人 BP → 场景实例 | cm，硬钳制 0..1000；每件最多 12 次找点，相隔至少 48cm |
| LootGroundOffset | C++ 20 → 敌人 BP → 场景实例 | cm，硬钳制 0..100 |
| PickupRange | C++ 150 → Controller BP Class Defaults | cm，Pawn 中心到物品中心的三维距离；硬钳制 100..500，服务端独立取自身配置 |
| PickupApproachTimeout | C++ 15 → Controller BP Class Defaults | 游戏秒，硬钳制 1..60；每 0.05 秒有限检查，仅待接近期间存在 |
| 名称/品质色/图标 | ItemDefinition.Presentation | 直接使用 DisplayName/RarityColor/Icon，不按 Rarity 再覆盖已配置颜色 |
| 地面模型/标签 | ItemDefinition.Presentation / BP_WorldItem.Label | WorldDropMesh/Scale/Rotation/Offset 控制模型；Label 相对位置作为包围盒顶部偏移，无模型时固定高度 |
| 客户端导航 | DefaultEngine.ini 的 NavigationSystemV1 | bAllowClientSideNavigation=True；使用地图导航数据，不授予客户端背包修改权限 |

空表/全部无效条目/零总权重不掉落。无效 Definition、非有限或非正 Weight、超界/非有限概率、负数量或逆序数量的条目不参与。无掉落权重为负或非有限时视作 0。数量可为 0；不支持按品质抽装备模板。

找点使用现有 NavMesh 的 reachable query，静态地面射线核对高度/坡度并避开静态障碍，不使用物理抛射。缺 NavMesh、无可用地面、散布空间不足则跳过并记录 Warning；半径 0 通常只能容纳一件。实例配置无效或 Spawn 失败记录日志并跳过，不补抽、不入包、不重试生成；本阶段没有持久化掉落保证。

## 点击、权威及生命周期

保留原鼠标敌人选择优先级：可攻击敌人 → Visibility 命中的 WorldItem → 原地面点按/长按移动。地面物品用 QueryOnly / WorldDynamic / Block Visibility，其余通道 Ignore；Mesh、Widget 无碰撞且不影响导航。死亡胶囊与 Mesh 原本已关闭碰撞。原生标签 HitTestInvisible，鼠标操作命中 Actor，不通过 UMG 按钮发物品数据。

点击物品不会进入长按移动。近处立即请求；远处复用 StartAutoMoveToLocation / UpdateAutoMove / MovePawnToward，拒绝无完整路径。攻击中采用原有单条 queued command 和 RequestPrimaryAttackMovementCancellation，取消后执行 Pickup；新移动/攻击/停止命令取代旧拾取。进入菜单、切换 Pawn、EndPlay 清理。目标消失、Health<=0、Dead/Stunned/Attacking、禁止移动或超时终止接近；没有背包/物品数据 Tick。

客户端只发送目标 Actor 引用。服务端重新验证 Actor 未销毁/已拾取、完整实例合法、角色/ASC Avatar/Health/状态/移动模式、三维距离、PlayerState、组件就绪与容量，以及背包和已装备 GUID。AcceptLootInstance 复用 PlayerState 的 bTransferInProgress，WorldItem 自身也在 AddInstance 同步发布事件前设置重入保护；两个玩家的请求在服务端游戏线程串行完成，只有第一个成功。

AddInstance 成功才设置 bPickedUp、关闭显示/碰撞并 Destroy；失败保留同一物品、GUID 和 RolledAffixes。成功依据 AddInstance 返回值：该接口先写入快照，再同步 Publish，不重新 Create。InventoryFull、DuplicateInstance（含已知 GUID 的 payload 冲突）、InvalidItem、NotReady、OutOfRange、Unavailable、NotAllowed 返回 ClientPickupResult；本地另有 NoPath/Cancelled/TimedOut。所有结果记录 LogUmbra，Controller 的 BP_PickupResult 可显示反馈；失败不会自动不断重试，玩家可稍后再次点击。

已实现远程客户端最小拾取 RPC 和客户端导航配置；尚未进行多人 PIE 验证。客户端到达后服务端位置尚未同步时可能返回 OutOfRange，此时地面实例保留，重新点击即可。WorldItem payload 对相关客户端复制；InventorySnapshot 的 COND_OwnerOnly 未改变。断线/换角不留下服务端待执行拾取计时器，服务器只在收到最终请求时同步转移。

## 编辑器手工配置

1. 创建 Data Asset，选择 `UmbraLootTable`，命名如 `DA_Loot_Prototype`。Entries 加入现有有效 ItemDefinition，Weight=1、DropChance=1、MinCount=MaxCount=2，NoDropWeight=0；先用确定性掉落验收。
2. 创建 `BP_WorldItem`，父类 `UmbraWorldItem`。Collision 保持 QueryOnly、WorldDynamic、Visibility Block、Pawn Ignore；半径默认 24cm。Mesh 由 ItemDefinition 的 WorldDropMesh 配置；运行时强制 NoCollision、无物理模拟、不影响导航，不硬编码资源路径。
3. 创建 `WBP_WorldItemLabel`，父类 `UmbraWorldItemLabel`。Designer 添加必需 `ItemName`（TextBlock），可选 `ItemIcon`（Image），控件勾选 Is Variable；删除 ItemBackground Border 或清空其 Brush，透明布局可用 SizeBox → ItemName。根控件设为 **Not Hit-Testable (Self & All Children)**。无需任何 Graph 或属性绑定：C++ 从当前实例 Definition 读取 DisplayName/RarityColor/Icon，更新名称/品质色/图标；空定义清除旧文字和图标，缺图标仅折叠 Image。
4. `BP_WorldItem.Label` 的 Widget Class 设为上述 WBP；Screen 空间、Draw At Desired Size 已在 C++ 配置。敌人设置 bEnableLoot、LootTable、WorldItemClass、散布与高度。未配置 Mesh/WBP 时仍可通过球形 Collision 点击，但没有可见标签，验收建议至少配置标签。
5. ItemDefinition.RarityColor 按需要配置六色：Common 浅灰、Magic 蓝、Rare 黄、Epic 紫、Legendary 橙、Unique 暗金。沿用现有实际颜色，原默认 White 不会由代码猜测并替换。
6. Controller 保留既有 PrimaryAction / PrimaryAttackAction 及左键映射，可设置 PickupRange/Timeout，BP_PickupResult 按枚举显示失败提示。不要在蓝图再次 AddItem/AddInstance。
7. 地图准备覆盖敌人和玩家区域的 NavMesh、静态地面碰撞；按 P 检查可达区域并构建导航。网络地图必须给客户端加载可用导航数据。Content 真实配置本轮均待编辑器确认。

### 地面标签原生刷新与 Designer 高亮

- ItemName 必需，ItemIcon 可选；C++ 不再绑定或修改 ItemBackground，不创建背景。旧 WBP 背景需在 Designer 删除或清空 Brush。字体、字号与阴影仍由 Designer 配置，整树保持 Not Hit-Testable (Self & All Children)，无需 Graph。
- 名称直接使用 RarityColor；透明（A<=0）、非有限或负 RGB 视为无效并回退 Designer 默认文字颜色。默认 White 与不透明黑色仍是有效配置，不修改 Rarity/颜色数据。
- WBP Class Defaults：bEnableHoverHighlight 默认 true，HoverBrightnessMultiplier 默认 1.25（范围 1..4）。从基础颜色重新乘 RGB，Alpha 不变；离开恢复原色，多次 Hover 不累计。饱和颜色提亮可能不明显；阴影不由 C++ 修改。NativeDestruct 恢复默认文字颜色。
- NameWrapWidth 默认 240 Slate 单位，名称按需调整宽度并在上限换行；0 保留 Designer 换行设置。不要在 SizeBox 强制过长宽度。
- DA_Item_TestSword.Presentation 指定已导入 StaticMesh：WorldDropMesh 为软引用；WorldDropScale 默认 (1,1,1)，WorldDropRotation 默认 (0,0,0) 度，WorldDropOffset 默认 (0,0,0) cm。按模型轴向配置横放/斜放，通过 Offset 补偿枢轴、陷地或既有掉落点离地高度。每种物品独立配置，运行时覆盖 BP Mesh 外观；不改变 Actor 位置/找地规则。
- BP_WorldItem.Label.WidgetClass 继续指向该 WBP。Screen / Draw At Desired Size 保留。复用 Label 组件相对位置（C++ 默认 → BP 默认 → 场景实例），默认 (0,0,35) cm；有模型时加在变换后包围盒顶面中心，无模型时使用固定位置，不增加重复位置字段。
- InitializeInstance / BeginPlay / OnRep 通过 StreamableManager 异步加载，弱 Actor 回调、请求版本及取消 Handle 防止过期结果。无模型/失败先隐藏 Mesh，保留标签及拾取球。加载完成后应用已配置的局部变换、更新模型可见性和标签位置。Mesh 路径改变、拾取、EndPlay 清加载请求/Mesh，拾取及 EndPlay 清标签数据；相同路径刷新复用加载。Actor 无 Tick，视觉组件仅动画期间 Tick。
- SetHighlighted 同步刷新文字与 VisualComponent 描边，不重新加载模型或重置动画。BP_RefreshItem 保留为可选视觉扩展，之后强制鼠标穿透；无需 Graph，既有 Graph 是否覆盖显示待编辑器确认。本轮未编辑或保存 Content。
- 待 PIE：模型姿态、缩放/Offset 与长名称；无模型及失败；反复 Hover、禁用高亮与无效品质色；近远拾取/背包满/多人竞争；加载中销毁及复制更新无残留；GUID/词缀保持、模型不阻挡角色/导航/Visibility。上述未执行。

## 单人 PIE 最小验收（待执行）

1. 编译并配置上述表/标签/敌人，进入 PIE，打开 Output Log。杀死一名敌人，确认只出现两件分散物品；尸体销毁后仍在。再试空表、bEnableLoot=false、DropChance=0 和 NoDropWeight，确认允许无掉落。
2. Hover 标签强调；近距离左键拾取，检查地面 Actor 消失、背包增加一个格子、开着的背包自动刷新。远距离单击确认沿现有路径接近后入包，长按物品不转成地面拖行。
3. 在 WorldItem 蓝图调试器记录 GetItemInstance 的 GUID/完整 RolledAffixes，和拾取后的 Inventory Snapshot 逐项比较；Hover、多次点击、开关背包不应改变候选/档位/值，现有 Tooltip 展示相同实际结果。
4. 先填满背包：拾取返回 InventoryFull、地面物品保留；释放一个格子再拾取，GUID 与词缀仍相同。未就绪组件、无效实例/重复 GUID 用受控调试场景验证，禁止把客户端提交 payload 当作测试入口。
5. 远处点击后改点地面/敌人、手动移动、开菜单、换 Pawn、令 Health=0/施加 Stunned，确认取消；途中销毁物品，确认无残留自动拾取。阻挡移动直到超时，确认停止且不会无限请求。
6. 回归点击敌人攻击、攻击中改点物品、普通地面点按及长按。多人另开 Listen Server + 远程客户端，验证近/远拾取、OwnerOnly 和两人抢同一物品仅一人成功；这些网络步骤不属于已完成验证。

本阶段不实现装备外观、按品质生成词缀数量、堆叠/整理、自动过滤、存档、传奇威能、比较 Tooltip、物理弹跳或完整地面属性 Tooltip。

## 地面掉落动画与模型 Hover（2026-10-17）

- WorldItem 创建 UUmbraWorldItemVisualComponent，复用原 Mesh。仅修改 Mesh Relative Transform；Actor、Root、拾取球及转移规则不变。WorldItem 管理实例、异步加载和复制，组件管理局部动画与 CustomDepth；标签字体/布局仍由原 Label 管理。
- 配置来源：组件 C++ 默认值 → BP_WorldItem 继承的 VisualComponent 默认值（EditDefaultsOnly）；最终姿态始终从 ItemDefinition 的 Presentation 分类下 WorldDropOffset/Rotation/Scale 读取，临时变换在其上叠加，不写回定义。标签按静止包围盒定位。
- BP_WorldItem 选择 VisualComponent：Enable Drop Animation=true；Rise/Fall/Bounce Duration 默认 0.18/0.40/0.22 秒，总计 0.8 秒；Rise/Bounce Height 默认 70/8 cm；Initial/Peak Scale Multiplier 默认 0.65/1.1；Additional Air Rotation 默认 (Pitch=0,Yaw=180,Roll=45) 度，为顶点附加姿态，落地归零。Rise 减速上升、Fall 加速下落、Bounce 抛物线回弹。时长 0 跳过阶段，全部为 0 直接静止。
- 可选 AnimationCurve 为阶段插值重映射：横纵轴 0..1，端点应为 (0,0)/(1,1)，用于高度/缩放及旋转进度，不是世界高度曲线。不配置时用原生插值。动画结束精确恢复定义 Transform 并关 Tick；静止时无 Tick，无物理。
- InitializeInstance 在服务端记录生成世界时间，按配置总时长用一次性 Timer 关闭动画窗口；时间和窗口状态复制，Mesh Transform 不复制。Mesh 就绪后由 GameState 服务器时钟求已过时间，只播剩余部分；超时/窗口关闭/无同步 GameState 直接静止。窗口关闭状态额外保护迟加入或重进相关范围的旧物品。实例 ID 只读取用于本地一次性播放标记，不修改 GUID；OnRep、Widget、Hover 不重播。Mesh 替换/清理保留标记，同实例替换后静止。
- 无 Mesh 或加载失败保持标签和拾取。拾取、失效、EndPlay 清动画/描边，EndPlay 取消加载和服务器 Timer。专服不播放视觉。
- Enable Hover Outline=true、CustomStencilValue=2（允许 2..255，1 留给敌人）。Hover 同步原标签提亮并开启 Mesh CustomDepth；退出关闭。播放中可描边。GetRarityHighlightColor 返回 Definition.RarityColor，供后续美术扩展；它不会自动为后处理传递 RGB，也不创建动态后处理材质实例。
- **手工材质步骤**：在 M_PP_EnemyOutline 保留现有 CustomStencil==1 的敌人掩码/颜色，另做 CustomStencil==2 的物品掩码。复用原描边的邻域采样/边缘计算，对中心及相邻采样一致使用物品掩码，生成物品边缘后乘单独的 WorldItemOutlineColor 参数，再与敌人分支及原场景合成。不可仅把总条件改为 ==2，否则敌人描边消失。确认项目 Custom Depth-Stencil Pass 为 Enabled with Stencil，后处理体积/相机启用该材质；统一物品颜色在后处理材质/现有实例配置。自定义 ID 时材质同步匹配。本轮未修改资产，真实节点/引用待编辑器确认。
- 待人工 PIE：三段动画、最终姿态/标签位置、多类武器；动画中快速 Hover 和拾取/销毁；无 Mesh/加载失败/延迟加载；禁用动画/描边、零时长；Listen Server 与客户端延迟/迟加入/重进相关范围；OnRep 不重播、静止组件 Tick 关闭；敌人 Stencil 1 与物品 2 共存，近远拾取与碰撞保持。本轮不新增或运行自动化测试。

## 模型表面鼠标命中（2026-10-20）

- 修正长武器（例如 mace 锤头）超出根部默认24cm球体后无法 Hover 的代码缺口：WorldItem.Mesh 改为 QueryOnly，仅阻挡 Visibility，仍无物理模拟/Overlap/导航影响，忽略 Pawn。Controller 的 GetWorldItemUnderCursor 优先复杂 Visibility 射线，未命中物品时保留原简单查询；Hover 和点击共享此入口，服务端拾取距离、RPC、库存转移不变。原文中的 Mesh NoCollision 契约由本节替代。
- 沿用 StaticMesh 的碰撞数据，不改导入资产。精确轮廓要求 Mesh 提供复杂碰撞（通常为 Project Default / Simple And Complex）；Use Simple Collision As Complex 会继续使用简化碰撞。真实 mace 资产设置待编辑器确认。根球仍是宽容兜底，因此并非所有可点击区域都严格等于像素轮廓。
- 待 PIE：分别悬停锤头/柄/根部、空白区域和被墙遮挡的物品；描边与标签同步，点击目标与 Hover 一致；无模型仍可拾取，角色通行/导航不变。动画中命中跟随 Mesh。未改材质、Stencil ID、VisualComponent 或 Content；未新增/运行自动化测试。
