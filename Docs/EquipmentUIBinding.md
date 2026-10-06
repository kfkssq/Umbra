# 装备槽位与组件实时绑定

2026-10-05 Tooltip 正式接入：EquipmentMenu.ItemTooltipClass 选择已有 WBP_ItemTooltip。Slot 仅广播 Hover；Menu 依据最新真实 EquipmentSnapshot、Slot、GUID、Definition 调用 FromEquipment，原生 SetToolTip 管理显示。SetPageActive(false)、无效快照、身份消失、PS/ASC 失效和 Destruct 清空。装备/四主属性事件触发当前 Hover 的只读刷新；查询在刷新保护期间暂不可用时，先失效再通过单次 GameThread 任务恢复。InventoryFull 失败不清除有效 Tooltip。详见 [ItemTooltipUI](ItemTooltipUI.md)，下文“未做完整 Tooltip”为历史阶段范围。

2026-10-04：接续StatsPanel/CombatInfo一致性修复，按用户选择实现装备槽位UI与组件对接。保留现有EquipmentMenu/SlotWidget、十个槽位和人物预览，未修改Content。

## 数据流

PlayerState的EquipmentComponent → 复制Snapshot / OnEquipmentChanged → EquipmentMenu → 已有SlotWidget。

EquipmentMenu默认启用Bind Equipment Data。Construct发现现有槽位、订阅所属玩家组件并立即读快照；穿戴、替换、卸下、需求状态变化后更新。隐藏页继续接收事件，重开同步；Destruct解绑并清空快照显示。PlayerState、Pawn和ASC生命周期变化重绑；组件注销额外广播无效快照，防止旧物品留在界面。没有Tick或背包遍历。

用Slot枚举匹配，不依赖数组顺序。ItemDefinition是物品定义，InstanceId才是实例身份；两枚戒指可共用定义却有不同GUID。同实例刷新保留选择，替换/清空会清除旧选择；槽位锁定与需求不满足是两件事，不自动将惩罚装备锁住。重复SlotType仍警告并禁用对应映射，避免填错槽。

## 展示契约

ItemDefinition新增Presentation字段：
- DisplayName：物品名称，留空后备为资产名称。
- Icon：纹理引用，未配置时仍视为已装备，但没有物品图像，不显示空槽图标伪装未装备。
- RarityColor：展示颜色，默认白色；不代表已有稀有度生成系统。
- SlotBackgroundTexture：该物品品质的槽位背景贴图，装备槽和背包槽共用。留空恢复槽位Designer默认背景。
- RarityFrameTexture：该物品品质的边框贴图。有贴图时按原图白色Tint显示，不再叠乘RarityColor；留空兼容原槽位默认边框及RarityColor。

EquipmentItemDisplay包含InstanceId、From Equipment Snapshot、Requirements Met和上述表现数据，由C++从组件投影。两个槽位共用FromDefinition映射和ItemSlotVisual表现规则，品质由ItemDefinition配置的图像决定，不从槽位类型推断。原BP_RefreshVisual可读取数据来画交互装饰；C++在该事件之后写入ItemIcon、SlotBackground、RarityFrame，避免旧空槽图再次隐藏真实物品。SlotWidget.GetItemDisplay供既有选择事件读取详情，UI不重算等级/四维需求。

EquipmentMenu.Equipment Data Ready区分“有效空装备”与“未就绪/组件禁用/无效快照”；数据不可用时清空旧物品显示。BP_EquipmentDataChanged在所有槽位更新后触发，可用于蓝图未就绪提示和详情面板刷新。引用尚未就绪或实例身份无效也视为不可用。

绑定期间SetSlotItem返回false，避免旧假数据覆盖真实快照。独立展示原型可在Class Defaults关闭Bind Equipment Data，继续使用原手动设置接口。直接在槽位调用SetItem仍是纯视图API，正式页面不应自行调用它冒充穿戴。

## UE编辑器手动操作

1. 使用最新原项目DLL打开编辑器（具体构建结果见Progress）。保持WBP_EquipmentMenu父类UmbraEquipmentMenu、槽位父类UmbraEquipmentSlotWidget及现有十个SlotType。无需新增布局或输入事件。
2. EquipmentMenu Class Defaults → Equipment → Data确认Bind Equipment Data开启。PlayerState的DerivedStats/Equipment仍需按前阶段启用。
3. 在实际穿戴的DA_Item_TestSword等ItemDefinition资产的Presentation配置Icon、Slot Background Texture和Rarity Frame Texture。不是WeaponProfile的配置，也不是给主手槽单独指定一张物品图。老资产新增字段默认空，不会修改已有装备属性。
4. WBP_EquipmentSlot使用UImage控件并准确命名为ItemIcon、SlotBackground、RarityFrame（勾选Is Variable），主手实例SlotType=MainHand。布局层级为背景→空槽/物品图→品质框→交互框；品质框图片中心应透明。删除旧Construct/Tick手填SetItem及这些Image上的Brush/Visibility绑定，不再在BP_RefreshVisual重复写入三层物品图；事件保留hover/selected/locked装饰和文字。也不要让控件父容器Collapsed或RenderOpacity=0。
5. 若需要显示“需求不足”，仅在From Equipment Snapshot=true且Requirements Met=false时显示惩罚提示；不要把它当作禁止装备。图标和提示的位置颜色由蓝图配置。
6. 可实现Equipment Data Changed事件，根据Ready显示/隐藏“装备数据未就绪”提示。现有槽位选择事件可用GetItemDisplay取得定义及InstanceId，更新详情。

## 最小验收

- 打开装备页，用已有测试事件Equip TestSword，主手自动出现对应名称/图标/颜色；切页再返回仍正确。卸下后恢复原空槽图标。
- 同定义换新实例，GUID更新、旧选择清除；两个戒指槽独立显示，卸下一个不清空另一个。
- 四维不足装备仍显示为装备，Requirements Met=false；外部增加四维后同步为true，属性面板按战斗派生结果变化。
- 重复开关菜单/切角色不重复订阅；未就绪或组件被注销时不残留旧装备；重开读取最新快照。
- StatsPanel与CombatInfo对照AD/AP、护甲/魔抗、攻速、暴击率、移速、急速，数值及格式一致。

穿戴由背包双击经PlayerState.EquipFromInventory执行；右键卸装见下节。尚未做背包拖拽、客户端卸装RPC、完整物品Tooltip、装备模型换装或随机词缀。人物预览沿用现有来源，不由物品图标推导外观。不要通过UI构造随机GUID来替代未来Inventory实例身份。

## 装备页卸装交互（2026-10-04）

槽位右键调用RequestUnequip → OnUnequipRequested → EquipmentMenu.RequestUnequipSlot → PlayerState.UnequipToInventory → EquipmentComponent.TryUnequipInstance / InventoryComponent.AddItem。左键选择和hover不变，无Tick、拖拽或鼠标捕获；也可从现有右键菜单按钮调用RequestUnequip。装备页必须处于活动状态、绑定当前PlayerState且数据有效；外来/重复槽位、锁定槽位、手工预览数据、过期GUID、未就绪及无权威权限都不会卸装。组件在修改前比较Slot和ExpectedInstanceId，防止旧UI卸掉替换后的装备。

成功复用原Unequip清理实例GE、重新派生并广播快照，UI只响应快照更新，不提前清空。BP_UnequipFinished返回Result、RemovedItem（原Definition/InstanceId/Slot）与可本地化Message；LastUnequipMessage保留最近反馈。Success表示已返还背包，AppliedInvalid表示已返还但派生快照无效，不得当作未执行再次补偿或返还。其他结果的RemovedItem为空。Locked是UI交互限制，组件可信API不将视觉锁当作物品所有权规则。

当前已接入[UI交互闭环](InventoryEquipmentInteraction.md)，PlayerState统一验证容量和GUID并返还。LastUnequipTransferResult给出InventoryFull等准确失败原因；兼容旧Result映射为Failed。BP回调禁止再次补发物品。远程客户端本阶段不发RPC，不允许本地修改属性。

编辑器操作及验收：

1. 保存并关闭编辑器，构建原项目后重新打开。现有EquipmentMenu/Slot父类和布局不变；原生右键无需新增按键事件。页面仍通过CharacterMenu调用SetPageActive。
2. 可在WBP_EquipmentMenu实现BP_UnequipFinished，把Message接入既有提示区；可按Result自行本地化样式。没有提示区时不强制增加控件，LastUnequipMessage可供调试查看。不要在事件内再次调用Unequip或ClearItem，不要再次AddItem；成功消息已表示“返还背包”。
3. 单机PIE装备TestSword后右键主手，确认槽位清空、属性与负重按组件更新；再次右键得到EmptySlot且数值不再变化。关闭重开仍为空，另一个戒指槽不受影响。
4. 左键选择、悬停Tooltip和其他按钮继续工作；如果已有子控件/蓝图覆盖鼠标事件并吞掉右键，应将其显式接到RequestUnequip且避免重复调用。实际WBP输入路由与视觉仍待用户PIE验证。

2026-10-04背包槽补充：UmbraInventorySlot的SetItemDefinition(Definition, InstanceId)、ClearItem、HasItem及GetItemDisplay使用同一物品表现规则。此API只负责显示，不能替代InventoryComponent或穿戴操作；现由[InventoryMenu实时绑定](InventoryFoundation.md)提供既有GUID与真实容量计数，堆叠数和EquippedMarker仍未实现。开启实时绑定时不要手工覆盖格子。清空恢复默认背景并隐藏Icon/品质框，换到缺少品质贴图的物品不会沿用上一件贴图。

当前验证证据见[Progress](Progress.md)；数据与UI职责见[EquipmentFoundation](EquipmentFoundation.md)、[CombatInfoBinding](CombatInfoBinding.md)。

