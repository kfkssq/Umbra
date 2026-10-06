# 背包↔装备权威转移

2026-10-04。PlayerState统一协调背包与装备转移。后续[UI交互闭环](InventoryEquipmentInteraction.md)已接入背包双击和装备页右键返还；不含拖拽、客户端请求RPC、整理/分类或存档。

## 接口

- EquipFromInventory(Slot, InstanceId)：按实例GUID在背包中找到物品，装备到空槽位。装备成功后才消费背包；任一步失败都不触碰两侧。目标槽位已有物品时返回SlotOccupied，要求先卸装再穿戴，避免静默销毁旧装备。
- UnequipToInventory(Slot, ExpectedInstanceId, OutReturnedItem)：卸下装备并放回背包。要求背包有空位，且该GUID不在背包中，否则返回InventoryFull或DuplicateInstance。OutReturnedItem返回放回的物品定义、GUID与槽位索引。
- GetInventoryComponent/GetEquipmentComponent：只读访问两个组件，供蓝图测试事件或后续UI调用。
- 结果统一为EUmbraTransferResult，覆盖权限、未就绪、非法槽位/GUID/物品、等级、未找到、槽位占用、背包满、重复实例、效果被拒、已应用但快照无效、回滚失败等分支。

## 原子性与回滚

- 装备方向：先调用Equipment.Equip，成功后再Inventory.RemoveItem。若移除意外失败，则回滚卸下刚装备的效果，返回TransferFailed，不让同一物品同时存在于两侧。
- 卸装方向：先TryUnequipInstance，成功后再Inventory.AddItem。若放回意外失败，则用原定义和GUID重新穿戴回原槽位，返回TransferFailed；重穿失败仅记录错误日志并上报，避免静默丢物品。
- 所有分支都在同一权威端同步执行，无跨帧状态；前置校验（槽位占用、背包满、重复GUID、身份有效）在修改前完成，正常路径不会触发回滚。回滚只作为防御，不承诺网络或外部并发下的补偿。

## 编辑器接入

1. 保存并关闭编辑器，Rider构建后重启（本阶段已构建主项目并更新DLL）。
2. 单机PIE临时测试事件（权威端，非Tick）：Get PlayerState → Get Inventory Component → AddItem（Definition选实际ItemDefinition资产，传入保存的New Guid），再调用 EquipFromInventory（Slot=MainHand，传入同一GUID）。装备页主手应出现该物品，背包对应格消失；容量文字变化。
3. 已消费的同一GUID再次EquipFromInventory返回NotFound；背包中另一实例尝试同一已占槽才返回SlotOccupied。调用UnequipToInventory传入当前装备GUID，装备槽清空、背包恢复该GUID。
4. 背包满时UnequipToInventory返回InventoryFull且装备保留；传入不存在GUID返回NotFound，空GUID返回InvalidIdentity。
5. 当前装备页右键和背包双击已接入这两个接口，真实WBP与PIE仍由用户验收。成功反馈事件禁止再次AddItem。

## 边界

- 只在已装备系统启用（bEnableEquipment与武器派生开启）时可用；否则返回NotReady。背包存储本身不依赖装备系统。
- 同定义可有多实例（不同GUID）；转移保持GUID不变，不重新生成。替换/卸装不涉及等级需求重算之外的额外逻辑。
- 远程客户端本阶段仍不发RPC；AddItem/EquipFromInventory/UnequipToInventory均为AuthorityOnly，UI只读快照。
- 自动化：Umbra.Inventory.Transfer覆盖未知GUID、非法身份、成功装备消费、槽位占用、错误槽位、成功卸装返还、过期GUID、重复实例拦截、背包满拦截。构建与回归结果见[Progress](Progress.md)。
