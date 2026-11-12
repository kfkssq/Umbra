# 四遗物栏与装备布局

2026-11-04。用户本轮明确要求由代码/编辑器修正布局，覆盖此前“栏位位置由用户自行配置”的安排。

| 列 | 从上到下的槽位 | 空栏背景 |
|---|---|---|
| 左 | Head、Chest、Hands、Legs | 头、胸、手套、裤腿 |
| 右 | Amulet、Amulet2、Ring1、Ring2 | 护符、护符、戒指、戒指 |

主手和副手保留在各列底部。通过UE重新排列现有WBP_Equipment的VerticalBox子控件，保留控件名称/绑定和已有布局参数，不重新创建控件。原名Feet的控件已经绑定Amulet2，内部名称保留以避免破坏Blueprint引用；不恢复鞋子玩法槽位。背景从当前SlotType对应EmptySlotIcons读取。

右侧四格在玩法上都是遗物栏。护符/戒指背景只是外观，不限制遗物种类。保留原枚举编号与C++名称，Accessory=3的中文展示改为“遗物”，无需重编号或改已有物品身份。

## 规则与配置优先级

- ItemDefinition.ItemCategory=Accessory定义遗物；GetAllowedEquipmentSlots固定返回四个右侧槽位，CanEquipInSlot只允许这四格。此类别规则高于旧AllowedSlots配置，因此新增或旧护符/戒指遗物都能进入任一遗物栏。Unknown及其他分类继续使用原AllowedSlots，未分类的测试物品不会自动变成遗物。
- 116件已导入护符的AllowedSlots及配方也同步四格，使编辑器数据与玩法一致；今后导入脚本默认配置四格。物品名称、图标、原版rarity限制、词条、等级和身份不变，遗物没有堆叠行为。
- 服务器装备校验、背包显式目标/空位选择、Tooltip目标和装备快照校验、生成词条槽位筛选共用以上接口。素材分类原本只作展示，本轮Accessory新增了这条明确的穿戴规则；其他类别行为不因此变化。
- 四格可同时装备四个不同实例；同一实例不能重复占格。等级、能力需求、已占用槽位和所有权规则继续有效。盔甲按头胸手腿各自槽位，锻造石仍不可装备。
- 自动穿戴继续原有行为：多于一个空遗物格时请求选择目标；拖到指定格时按该格校验。本轮未改变装备互换或自动替换规则。

## 工具与验证

`Scripts/audit_relic_layout.py`回读原布局；`Scripts/configure_relic_slots.py`通过UE保存控件顺序及116个导入配方，`UMBRA_RELIC_READBACK=1`时只检查、不保存。`Scripts/smoke_relic_slots_pie.py`检查保存布局、实际空栏画刷、四个护符经背包UI同时装备到四格、目标Tooltip、卸下快照并截图。

自动化`Umbra.Equipment.FourRelicSlots`验证旧护符与戒指配置均适配四格、四实例同时穿戴和非遗物槽位拒绝；`Umbra.Items.Generation.NativeAccessoryAssets`扩展为逐件测试全部四个遗物槽。结果与未验证项见 [Progress](Progress.md)。
