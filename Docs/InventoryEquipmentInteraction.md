# 背包与装备 UI 交互闭环

2026-10-04。保留现有 WBP、ScrollBox、InventoryGrid、格子模板、品质表现与人物预览；本轮不修改 Content。实际资产父类、事件图和绑定均待编辑器确认。

## 审查与配置来源

- InventoryComponent 已有权威 AddItem/RemoveItem、固定槽位、稳定 InstanceId、OwnerOnly 快照和变化/注册通知。InitialCapacity 首次注册时限制在 0..512，原生默认 96；组件快照覆盖 UI 的预览容量。Columns、InventorySlotClass 仍由现有 WBP 配置。
- EquipmentComponent 已有 Equip、TryUnequipInstance、GE 管理和装备快照。直接 Equip 允许替换，因此 UI 必须使用 PlayerState.EquipFromInventory；直接卸装不会返还，因此 UI 使用 UnequipToInventory。
- PlayerState 已有两向转移；本轮补充转移重入保护。装备/背包通知同步发生，观察者不可把中间通知理解为已完成两侧转移，更不可在回调里发物品。
- ItemDefinition 的 Presentation 是属性分类，包含 DisplayName、Icon、SlotBackgroundTexture、RarityFrameTexture、RarityColor，并非另一个嵌套结构。两个 UI 共用 FromDefinition 与 ItemSlotVisual，不生成 GUID，不重算 GAS 属性。

## 数据与选择

InventoryViewState 区分 NotReady、Ready、InvalidSnapshot。Ready 且无物品才是有效空背包；不可用/无效时清除格子并显示 —/—。无效容量、槽号、定义、GUID 或重复身份/槽号均拒绝投影。

Construct/Destruct 成对订阅；关闭时只保留选中 GUID，移除显示数据，重开后用当前快照核实选择。同 GUID 移到另一格也能恢复；实例消失、替换玩家或无效数据清除选择。菜单可见性、Pawn、PlayerState 复制和组件注册事件触发刷新，无 Tick、定时器或 Text Binding。背包展示不依赖 ASC；穿戴需装备系统已初始化。

## 穿戴

左键只选择；NativeOnMouseButtonDoubleClick 仅左键提交一次请求，空格不提交。不增加拖放逻辑。

RequestEquipItem(InstanceId) 从所属玩家的当前背包重新验证实例，再读取 AllowedSlots 与装备快照：唯一合法槽直接请求；多个合法槽仅在唯一空槽时自动选择；多个空槽返回 SlotSelectionRequired；全满返回 SlotOccupied。RequestEquipInSlot(InstanceId, TargetSlot) 是明确选择后的入口，仍校验所属玩家、定义允许槽位和权威转移结果。当前不制作槽位选择弹窗。

LastEquipResult/LastEquipMessage 与 BP_EquipFinished 提供可本地化反馈；成功只由组件通知更新格子并清除已消费 GUID 的选择。失败保留物品。

## 右键返还与兼容反馈

现有 RequestUnequip → RequestUnequipSlot 保留活动页、所属玩家、锁定、空槽、快照来源检查，随后调用 PlayerState.UnequipToInventory(Slot, ExpectedInstanceId)。返回前验证容量与实例身份；满背包不撤销 GE、不清格子；过期 GUID 不能卸下后来替换的物品。两侧展示都由组件通知刷新。

保留 BP_UnequipFinished 的旧签名及 EUmbraUnequipResult，避免破坏现有资产引脚：Success/AppliedInvalid 的 RemovedItem 已经进入背包，事件只做反馈，禁止再次 AddItem。新增 LastUnequipTransferResult 暴露准确的 InventoryFull、DuplicateInstance 等转移结果；这些失败在兼容旧枚举中为 Failed，Message 给出具体原因。LastUnequipTransferResult 在未进入转移时为 NotReady，应同时看原 Result 区分 Locked/EmptySlot 等 UI 拒绝。

修复旧 UnequipToInventory 在临时 GetSnapshot() 数组中查找后持有悬空指针的问题，改为保留快照再复制 OutReturnedItem。

## 初始测试物品

按本轮追加要求，Config/DefaultGame.ini 的 `/Script/Umbra.UmbraPlayerState` 节配置 InitialInventoryItems 为 `/Game/Items/Weapons/DA_Item_TestSword.DA_Item_TestSword` 与 `/Game/Items/Weapons/DA_Item_TestMace.DA_Item_TestMace`。这里引用物品定义，不是 DA_Weapon_* 武器数值资产。

PlayerState.BeginPlay → InitializeInventory 在权威端每个 PlayerState 只执行一次，通过 Inventory.AddItem 放进背包，不穿戴。每项仅在创建时生成 GUID；初始化、菜单开关、组件重注册均不补发已经移走的物品。新 PIE/新 PlayerState 会创建新实例，不提供跨局持久化。

参数优先级：原生空清单 → DefaultGame.ini → PlayerState Blueprint 显式覆盖。容量仍来自 InventoryComponent.InitialCapacity；资产加载失败或容量不足记录 Warning 并跳过，启动发放不会轮询或在之后自动补发。可在 BP_UmbraPlayerState 的 Inventory 分类调整 InitialInventoryItems；本轮没有修改该资产。实际加载与继承值由 Umbra.Inventory.InitialTestItems 检查。

## 权威与边界

本轮针对单机及 Listen Server 主机的权威路径。远程客户端只显示复制快照，交互返回 AuthorityRequired；没有 PlayerController 请求 RPC。UI 不直接修改组件，不更改伤害公式或 GAS 语义。

不含掉落/拾取、堆叠、整理/筛选/排序、随机词缀、重铸、传奇生成、存档、拖拽、外观换装或完整 Tooltip。选槽 UI 和消息显示样式保留给现有 WBP 的明确扩展点。

## 编辑器检查与单人 PIE

1. 使用本轮完整构建的 UE 5.8.2 项目。确认 BP_UmbraPlayerState 的 DerivedStatsComponent.UseWeaponDerivedPower 与 EquipmentComponent.EnableEquipment 开启，InventoryComponent.InitialCapacity 按需配置。
2. WBP_Inventory 父类为 UmbraInventoryMenu，BindInventoryData=true；原 InventoryGrid/CapacityText、Columns、InventorySlotClass 保持。模板父类为 UmbraInventorySlot。删除旧演示赋值及 CapacityText 的 Text Binding，勿重建布局。装饰层不能截获鼠标；勿把素材包窗口拖拽事件接入菜单。
3. 现有装备页继续使用 UmbraEquipmentMenu/UmbraEquipmentSlotWidget，十个 SlotType 唯一；BindEquipmentData=true，CharacterMenu 仍通知活动状态。结果事件只显示反馈，不能再次 AddItem、Unequip 或 ClearItem。
4. 在现有提示区域显示 BP_EquipFinished 的 Message；没有提示区域时可先用临时 Print Text 验证。本轮不自动改蓝图。双戒指都空时应提示选择槽位；测试事件显式调用 RequestEquipInSlot，或以后制作选槽 UI。
5. 单人 PIE 出生后背包应已有 TestSword/TestMace 两件初始物品，装备保持初始空槽。打开背包检查 2/N、图标/品质；单击只选中，双击单槽装备后对应格清空、容量减少，装备页显示原 GUID。需要同定义多实例时，权威临时测试事件 AddItem，另生成并保存一个 GUID；不要每次菜单打开都发物品。
6. 对照测试物品的 Weapon、Armor/MagicResistance、PrimaryBonuses、AttributeBonuses 和 Weight，检查 AD/AP、防御、词缀与负重随装备变化；不在 UI 自行算公式。等级不足、错误槽位和全满槽位请求均不得消费物品。
7. 关闭/重开，确认未穿戴物品的选择保留；移除选中实例再开应清除。反复开关后一次双击仅一次转移。检查 hover、ScrollBox 滚轮、属性分类折叠和人物预览。
8. 右键装备，原 GUID 回背包，容量恢复、装备恢复空槽且属性/负重回退。用权威 AddItem 填满背包再右键，需返回 InventoryFull、装备及 GE 保留；腾空一格再右键应成功。旧显示 GUID 不能移除已替换实例。
9. 本文的步骤是待执行验收，不能当成已通过的 PIE/网络证据。实际构建和自动化结果见 [Progress](Progress.md)。

## 本轮文件清单

清单仅列本轮触及的文件，不代表这些文件的所有未提交内容都由本轮新增。

| 操作 | 文件 | 用途 |
|---|---|---|
| 修改 | [DefaultGame.ini](../Config/DefaultGame.ini) | 两个初始物品清单 |
| 修改 | [UmbraPlayerState.h](../Source/Umbra/Player/UmbraPlayerState.h)、[cpp](../Source/Umbra/Player/UmbraPlayerState.cpp) | 启动发物品、转移重入校验、返回快照生命周期 |
| 修改 | [UmbraInventoryComponent.h](../Source/Umbra/Inventory/UmbraInventoryComponent.h) | 暴露只读广播状态，拒绝通知中转移 |
| 修改 | [UmbraInventoryMenu.h](../Source/Umbra/UI/Inventory/UmbraInventoryMenu.h)、[cpp](../Source/Umbra/UI/Inventory/UmbraInventoryMenu.cpp) | 数据状态、选择恢复、自动/显式选槽、转移反馈 |
| 修改 | [UmbraInventorySlot.h](../Source/Umbra/UI/Inventory/UmbraInventorySlot.h)、[cpp](../Source/Umbra/UI/Inventory/UmbraInventorySlot.cpp) | 原生双击请求 |
| 修改 | [UmbraEquipmentMenu.h](../Source/Umbra/UI/Equipment/UmbraEquipmentMenu.h)、[cpp](../Source/Umbra/UI/Equipment/UmbraEquipmentMenu.cpp) | 右键返还、兼容反馈、选择恢复 |
| 新增 | [UmbraTransferFeedback.h](../Source/Umbra/UI/Items/UmbraTransferFeedback.h)、[cpp](../Source/Umbra/UI/Items/UmbraTransferFeedback.cpp) | 共享可本地化转移消息 |
| 修改 | [UmbraPlayerControllerDebug.cpp](../Source/Umbra/UmbraPlayerControllerDebug.cpp) | PlayerState复制变化通知背包 |
| 新增 | [UmbraInventoryInteractionTests.cpp](../Source/Umbra/Tests/UmbraInventoryInteractionTests.cpp) | 鼠标穿戴/返还闭环、容量、GAS及真实初始物品 |
| 修改 | [UmbraInventoryTestTypes.h](../Source/Umbra/Tests/UmbraInventoryTestTypes.h)、[UmbraInventoryLiveTests.cpp](../Source/Umbra/Tests/UmbraInventoryLiveTests.cpp)、[UmbraEquipmentLiveUITests.cpp](../Source/Umbra/Tests/UmbraEquipmentLiveUITests.cpp) | 原生输入入口及选择/返还回归 |
| 新增/修改文档 | 本文、[Progress](Progress.md)、[Architecture](Architecture.md)、[EquipmentFoundation](EquipmentFoundation.md)、[EquipmentUIBinding](EquipmentUIBinding.md)、[InventoryFoundation](InventoryFoundation.md)、[InventoryTransfer](InventoryTransfer.md) | 当前接口、边界、证据与手动验收 |

Content未编辑/保存；没有新增模块或Target，不需因此刷新Rider工程文件。
