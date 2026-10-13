# 物品掉落与拾取：第一阶段

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
| 地面外观/标签 | WorldItem BP 组件默认值 | 可选 Mesh、Label.WidgetClass；不配置仍有可点击 Collision 和真实实例 |
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
2. 创建 `BP_WorldItem`，父类 `UmbraWorldItem`。Collision 保持 QueryOnly、WorldDynamic、Visibility Block、Pawn Ignore；半径默认 24cm。通用 StaticMesh 可选，不使用装备模型；Mesh NoCollision，不硬编码资源路径。可配置 CustomDepth Stencil，但轮廓仍需要已有兼容后处理材质。
3. 创建 `WBP_WorldItemLabel`，父类 `UmbraWorldItemLabel`。Designer 添加必需 `ItemName`（TextBlock），可选 `ItemIcon`（Image）、`ItemBackground`（Border），这些控件勾选 Is Variable；布局可用 SizeBox → Border → HorizontalBox → Image/TextBlock。根控件设为 **Not Hit-Testable (Self & All Children)**。无需任何 Graph 或属性绑定：C++ 从当前实例 Definition 读取 DisplayName/RarityColor/Icon，更新名称/品质色/图标；空定义清除旧文字和图标，缺图标仅折叠 Image。
4. `BP_WorldItem.Label` 的 Widget Class 设为上述 WBP；Screen 空间、Draw At Desired Size 已在 C++ 配置。敌人设置 bEnableLoot、LootTable、WorldItemClass、散布与高度。未配置 Mesh/WBP 时仍可通过球形 Collision 点击，但没有可见标签，验收建议至少配置标签。
5. ItemDefinition.RarityColor 按需要配置六色：Common 浅灰、Magic 蓝、Rare 黄、Epic 紫、Legendary 橙、Unique 暗金。沿用现有实际颜色，原默认 White 不会由代码猜测并替换。
6. Controller 保留既有 PrimaryAction / PrimaryAttackAction 及左键映射，可设置 PickupRange/Timeout，BP_PickupResult 按枚举显示失败提示。不要在蓝图再次 AddItem/AddInstance。
7. 地图准备覆盖敌人和玩家区域的 NavMesh、静态地面碰撞；按 P 检查可达区域并构建导航。网络地图必须给客户端加载可用导航数据。Content 真实配置本轮均待编辑器确认。

### 地面标签原生刷新与 Designer 高亮

- `ItemName` 使用 BindWidget；另两个控件使用 BindWidgetOptional，省略时安全。字体、字号、Padding、Brush 资源、尺寸及布局继续由 Designer 配置；名称颜色由物品 RarityColor 控制。图标替换不匹配纹理原始尺寸，保留 Designer 尺寸和 Tint。
- WBP Class Defaults → World Item / Highlight：`bEnableHoverHighlight` 默认 true；`HoverBackgroundColor` 默认线性 RGBA `(0.3, 0.3, 0.3, 0.9)`，可覆盖。普通状态恢复 Designer Border Brush 与 BrushColor；Hover 只替换 BrushColor，不改背景资源、文字字体或尺寸。无 Border 时不显示背景高亮；需要可见背景时在 Designer 配置能绘制的 Brush。
- 首次 NativeConstruct 缓存背景默认 Brush/Tint，同一控件重 Construct 不重新取已修改颜色；每次刷新从该缓存恢复后再应用当前 Hover，NativeDestruct 也恢复。无效 Definition 不保留旧 Hover 背景。没有 Tick 或新增委托。
- WorldItem 原有 InitializeInstance / BeginPlay / OnRep / SetHighlighted → RefreshPresentation → Label.InitWidget → SetItem 路径保留。构造前收到的实例先保存，NativeConstruct 完成后显示；之后 SetItem 立即刷新。更换 BP_WorldItem 的默认 WidgetClass 后，新标签沿相同路径接收数据；WidgetClass 为空时仍正常掉落/拾取。Screen Space / Draw at Desired Size 不变。
- `BP_RefreshItem` 保留为原生刷新后的可选纯视觉扩展，不承担基础数据绑定，也不需要实现。事件后原生再次保证标签整树鼠标穿透；扩展不得改动物品实例或重新随机。既有资产的 Graph 是否存在、是否会覆盖原生显示，仍待编辑器确认；本轮不编辑/保存资产。
- 待人工验证：无需 Graph 的标签显示不同名称/品质/有无图标；Hover 多次进出恢复 Designer 背景；禁用高亮/省略可选控件正常；复用标签传空 Definition 清旧内容；重 Construct、客户端复制和更换默认 WidgetClass 后显示正确。这些不是已执行的 PIE 结果。

## 单人 PIE 最小验收（待执行）

1. 编译并配置上述表/标签/敌人，进入 PIE，打开 Output Log。杀死一名敌人，确认只出现两件分散物品；尸体销毁后仍在。再试空表、bEnableLoot=false、DropChance=0 和 NoDropWeight，确认允许无掉落。
2. Hover 标签强调；近距离左键拾取，检查地面 Actor 消失、背包增加一个格子、开着的背包自动刷新。远距离单击确认沿现有路径接近后入包，长按物品不转成地面拖行。
3. 在 WorldItem 蓝图调试器记录 GetItemInstance 的 GUID/完整 RolledAffixes，和拾取后的 Inventory Snapshot 逐项比较；Hover、多次点击、开关背包不应改变候选/档位/值，现有 Tooltip 展示相同实际结果。
4. 先填满背包：拾取返回 InventoryFull、地面物品保留；释放一个格子再拾取，GUID 与词缀仍相同。未就绪组件、无效实例/重复 GUID 用受控调试场景验证，禁止把客户端提交 payload 当作测试入口。
5. 远处点击后改点地面/敌人、手动移动、开菜单、换 Pawn、令 Health=0/施加 Stunned，确认取消；途中销毁物品，确认无残留自动拾取。阻挡移动直到超时，确认停止且不会无限请求。
6. 回归点击敌人攻击、攻击中改点物品、普通地面点按及长按。多人另开 Listen Server + 远程客户端，验证近/远拾取、OwnerOnly 和两人抢同一物品仅一人成功；这些网络步骤不属于已完成验证。

本阶段不实现装备外观、按品质生成词缀数量、堆叠/整理、自动过滤、存档、传奇威能、比较 Tooltip、物理弹跳或完整地面属性 Tooltip。
