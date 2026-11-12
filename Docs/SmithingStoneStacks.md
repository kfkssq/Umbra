# 锻造石背包堆叠

2026-11-06：新生成石头的品质由等级固定，见 [固定品质与19种展示](StoneGradesAndStartup.md)。同种同品质堆叠键不变；不同历史品质实例仍分堆。

2026-11-04：同种、同品质的锻造石自动合并为一个格子，格子显示总数量（单件显示1）。不同种类或品质分格；武器、防具、护符保持逐实例存放。

## 规则与数据

- ItemDefinition.bStackableMaterial为显式开关，仅NativeAccessoriesV1的19件EquipParamGoods定义启用。必须是Material、不可装备、无装备栏/武器、无随机词条、未强化的实例才能堆叠。随机品质生成与原版rarity限制不变。
- FUmbraItemInstance.Quantity是权威数量，默认1，进入背包快照与实例记录Capture/Restore；旧数据缺少此字段时仍为1。当前没有磁盘存档系统。
- 合并键为同一物品定义和实例品质。物品等级/随机种子不影响锻造石用途，不参与分堆；合并保留目标堆的GUID、位置、等级等代表信息，只增加数量，不重掷。
- 拾取优先合并，再检查空格：满背包仍能接收匹配的锻造石。没有匹配且无空格时，拾取失败、地面物品保留。显式TargetSlot优先于自动找堆；指定空格可用于受信任的整堆转移。
- 拖动到空格移动整堆；拖到同类同品质堆上合并并清空源格；其他物品继续交换格子。当前不提供拆分堆叠、使用/消耗或强化功能。
- 无人为99/999上限；数量以正int32表示，上限2,147,483,647。超过上限时整笔拒绝，不部分吞入；复用Full结果。数量为0、负数或非堆叠装备数量大于1会被拒绝。
- 被吸收的GUID在该背包组件生命周期内退休，不能重复提交增加数量；原目标GUID的KnownInstances同步最新数量。RemoveItem移除整堆并返回完整数量，受信任返回接口可恢复整堆。没有新增客户端授权入口。

## UI与网络

服务器修改背包，沿用COND_OwnerOnly快照复制及Revision通知。FUmbraEquipmentItemDisplay投影Quantity/bShowQuantity，WBP_InventorySlot现有StackCountText由C++填充并显示，空格和非堆叠物品清空隐藏。Widget不计算合并、不推算数量；拖动沿用已有请求及身份验证。

## 配置与验证

`Scripts/EquipmentImport/enable_smithing_stacks.py`通过匹配UE编辑器迁移19件定义、说明文本和导入配方，不改变来源参数/概率。`import_native_accessories.py`已同步新字段，独立readback仍检查所有输出。运行配置顺序：定义开关约束实例可堆叠性；服务器数量决定快照；UI仅读取快照投影，无蓝图数量覆盖。

自动化 `Umbra.Inventory.SmithingStacks`覆盖品质/种类隔离、不同生成等级合并、满包、重复身份、整堆移动/合并/交换、记录恢复、上限/零数量和真实WBP文字/可见性清理。`Scripts/smoke_smithing_stacks_pie.py`验证实际敌人生成4颗石头、拾取后3格（2+1+1）、已打开菜单实时数量变化及整堆移动。执行结果与未验证项见 [Progress](Progress.md)。
