# 背包实例与实时UI：最小基础阶段

2026-10-05 Tooltip 正式接入：InventoryMenu.ItemTooltipClass 在 WBP Class Defaults 配置。Slot 原生 Hover 通知 Menu，Menu 使用当前真实快照 + 槽索引/GUID/Definition + 当前装备上下文调用 FromInventory（不选目标槽），惰性复用一个原生 UMG ToolTip。SetItemDefinition 的手动数据不标记为快照来源；正式投影由菜单设置来源标记。快照变化立即刷新/清空当前 Hover，四主属性/装备事件合并刷新需求但不重建网格；CharacterMenu 页面关闭与 Destruct 清除 Tooltip。完整生命周期、等级通知边界、用户配置与人工验收见 [ItemTooltipUI](ItemTooltipUI.md)。无 Content 修改。

2026-10-04。此阶段完成内存中的非堆叠背包和既有网格的数据绑定，不含装备转移、背包右键穿戴、整理/分类、拾取、随机掉落、存档或客户端请求RPC。

## 数据与配置

- PlayerState原生创建UmbraInventoryComponent，与ASC、Equipment并列。背包不依赖GAS初始化，不生成GE，也不改变角色负重；EquipLoad仍仅表示已装备重量。
- InitialCapacity在PlayerState蓝图的InventoryComponent默认值配置，默认96，首次注册时限制为0..512。当前不支持运行中扩容；卸载/重新注册组件保留本次会话内容，销毁PlayerState不保证跨关卡保存。
- Snapshot包含Ready、Capacity和Items。每条Item保存SlotIndex、InstanceId、ItemDefinition；一件物品占一格，无堆叠数量。相同定义可以有多个不同GUID；同背包内重复GUID拒绝，满容量拒绝且不覆盖已有物品。移除后其他槽位索引不变，添加复用最前面的空格。
- AddItem(Definition, InstanceId)是可信权威端授予/导入接口；GUID在创建实例时生成一次。RemoveItem按GUID返回被移除实例。无所有者/非权威端拒绝；未注册或正在广播时拒绝重入。它们不负责销毁掉落Actor、验证跨装备/背包的所有权或执行交易，不能当作面向客户端的通用RPC。
- Snapshot使用OwnerOnly复制；本轮未进行多人网络测试。客户端UI只读副本，定义必须是可在客户端解析的资产引用。测试使用的临时NewObject定义不代表网络资产验证通过。

## UI契约

UmbraInventoryMenu默认Bind Inventory Data=true，自动查找所属PlayerState的InventoryComponent，监听内容与注册生命周期；Pawn切换、菜单重新打开时重绑，也提供NotifyPlayerContextChanged供自定义玩家上下文切换调用。Construct/Destruct成对订阅，不使用Tick。关闭再打开读取当前数据，注销组件清除旧内容，重新注册恢复。

运行时格数由组件Capacity决定；原WBP的InventoryCapacity只用于Designer及关闭Bind Inventory Data后的独立展示原型。Columns和InventorySlotClass继续由WBP配置。CapacityText显示真实占用/容量，未就绪显示“—/—”，Ready的空背包才显示0/N。BP_InventoryDataChanged在刷新后提供Ready，样式由蓝图处理。

同GUID刷新保留选择，移除/更换实例清除旧选择；各格调用已有SetItemDefinition/ClearItem，复用品质背景、图标和框。StackCountText、EquippedMarker仍隐藏。分类按钮、整理按钮没有在本轮获得实际行为；网格布局和其他菜单输入不改动。

## 编辑器接入与验收

1. 保存并关闭编辑器，在Rider构建原项目后重启。PlayerState应出现InventoryComponent；InitialCapacity按设计配置，例如96。当前验证在独立副本完成，原项目DLL需自行重编译。
2. WBP_Inventory继续继承UmbraInventoryMenu，开启Bind Inventory Data。保留InventoryGrid、CapacityText、InventorySlotClass和Columns配置；删除容量文字的旧绑定/固定赋值，避免覆盖原生值。InventoryGrid仍只能放运行时生成的格子。
3. 单机PIE临时测试事件：权威端Get PlayerState → Get Component by Class(UmbraInventoryComponent) → AddItem，Definition选择实际ItemDefinition资产，传入保存的New Guid。仅物品创建时生成GUID；重复调用相同GUID应返回DuplicateInstance。不要从UI Tick添加。
4. 打开菜单应出现物品及其品质背景/框，容量显示1/N；同定义不同GUID可占两个格。RemoveItem传入第一个GUID后仅对应格清空，第二格不前移；再添加新GUID复用空格。满背包再添加返回Full，旧内容不变。
5. 关闭重开、Pawn切换、组件注销/重新注册后不残留旧人物物品；检查左键选择、hover与原界面布局。真实WBP和PIE由用户验收。

## 当前交互边界

权威背包↔装备转移已实现，见[InventoryTransfer](InventoryTransfer.md)。当前背包双击和装备右键已改为调用PlayerState.EquipFromInventory/UnequipToInventory，自动消费/返还原GUID，详见[UI交互闭环](InventoryEquipmentInteraction.md)。不要在BP_UnequipFinished直接调用AddItem拼接生产流程，否则会绕过原子性和跨模块身份检查。

自动化：Umbra.UI.Inventory.LiveStorage覆盖存储拒绝分支、满容量、同定义多实例、稳定槽位、组件重注册、延迟玩家上下文、实时计数/内容、选择生命周期、关闭重开及换PlayerState。构建与回归结果见[Progress](Progress.md)。
