# 最小装备组件（第三阶段）

2026-10-04 后续已完成[背包与装备 UI 交互](InventoryEquipmentInteraction.md)：背包双击经PlayerState穿戴，右键返还原GUID到背包；初始TestSword/TestMace在背包中。下文“未来Inventory/菜单尚未绑定”及直接Equip测试步骤保留为历史阶段说明，不是当前生产UI入口。

2026-10-04 已扩展[固定词缀与装备效果](EquipmentAffixes.md)：原生GE由六个扩展为二十个SetByCaller Modifier，复用已有属性，A/X由同一GE组件读取ItemDefinition。下文六Modifier和未接入增伤的描述是第三阶段历史边界。

后续第四阶段已实现可选[九类型伤害结算](TypedDamage.md)。下文旧伤害算例和“下一阶段”记录第三阶段边界；开启Typed武器继承后，完整继承已应用装备惩罚的类型明细，默认Legacy仍保持原行为。

## 范围与数据流

玩家PlayerState和敌人Character各持有一个默认关闭的[EquipmentComponent](../Source/Umbra/Equipment/UmbraEquipmentComponent.h)。启用要求同一个Owner的DerivedStats已激活；初始化顺序为ASC初始属性 → DerivedStats → Equipment。未启用时，第二阶段InitialWeapon/SetWeaponProfile的行为不变。

`ItemDefinition → 每实例一个Infinite EquipmentEffect → GAS四主属性/Armor/MagicResistance → Equipment需求判定 → DerivedStats类型明细 → GAS AD/AP → 原伤害入口`。

- [ItemDefinition](../Source/Umbra/Items/UmbraItemDefinition.h)：允许槽位、等级/四维需求、无条件四维词缀、Weight、基础Armor/MagicResistance、可选WeaponProfile。资产运行时只读；不能原地编辑已穿戴共享定义。
- [EquipmentEffect](../Source/Umbra/Equipment/UmbraEquipmentEffect.h)：原生、可复制的Infinite GE，六个SetByCaller加法Modifier；普通词缀不含直接AD/AP。组件独占各实例效果句柄，卸下/替换/注销时只移除自己的效果。
- EquipmentSnapshot：有效性、版本、槽位/实例GUID/定义/需求满足状态、实际负重/容量/负重分档；服务器计算，属性复制，OnEquipmentChanged通知。客户端不能从占位快照推断结果，必须检查bValid。
- DerivedStatsSnapshot继续提供各类型基础值、补正值、总值与AD/AP；装备模式下这些是应用需求惩罚后的值。
- 原EUmbraEquipmentSlot抽到[共享槽位头文件](../Source/Umbra/Equipment/UmbraEquipmentSlot.h)，名称、枚举顺序和脚本包不变；原UI头继续include它，Gameplay不依赖UI目录。

没有Tick。主属性委托驱动需求/负重/派生重算；应用/移除装备GE期间暂缓AD/AP发布，双方伤害入口拒绝使用更新中的装备状态。通知回调中不允许再次换装；主属性反馈最多重算8轮，无法稳定时标无效并阻止结算。

## 规则与配置来源

所有组件参数按“C++默认值 → 所属PlayerState/Enemy BP组件默认值”覆盖，启动后固定；物品参数来自ItemDefinition，武器伤害来自它引用的WeaponProfile，曲线规则沿用[第二阶段](WeaponDerivedPower.md)。当前没有进度等级系统或威能。

| 参数 | C++原型默认 | 语义 |
|---|---:|---|
| bEnableEquipment | false | 启动选择模式；启用后空主手使用徒手，取代InitialWeapon |
| CharacterLevel | 1 | 当前阶段固定角色等级，仅穿戴时检查 |
| RequiredLevel | 1 | 等级不足拒绝穿戴，不是惩罚装备 |
| UnmetWeaponBaseMultiplier | 0.5 | 四维不足时，逐类型基础伤害乘此值 |
| UnmetWeaponScalingMultiplier | 0 | 四维不足时，逐类型补正乘此值 |
| UnmetDefenseMultiplier | 0.5 | 四维不足时，装备自身Armor/MagicResistance乘此值 |
| BaseCapacity | 40 | 基础负重容量，重量单位由项目统一约定 |
| CapacityPerStrength | 0.5 | 每点当前Strength增加的容量 |
| LightThreshold / MediumThreshold | 0.3 / 0.7 | 容量占比上限，含边界；Heavy至1，超过1为Overweight |

这些是可调原型默认，不代表最终平衡数值。组件拒绝无效/非有限数值或颠倒的阈值。

1. 四维不足仍允许装备，普通四维词缀始终生效。需求值通过GAS重新求值并忽略本装备效果句柄，不能用“最终属性减去本装备词缀”。这样外部乘法Buff也正确；其它装备与外部Buff可以满足需求。
2. 当前只支持MainHand武器，OffHand可以放无WeaponProfile的副手防具。带Weapon的定义允许槽位只能是MainHand，避免静默忽略副手武器。非武器定义可以按AllowedSlots配置；物品不隐含互斥/双手规则。
3. `类型伤害 = BaseDamage × 基础惩罚倍率 + 原四维补正 × 补正惩罚倍率`；正常需求时两倍率均1。补正使用完整当前四维，包括该装备自己的无条件词缀；只有需求判断排除自身。
4. Armor/MagicResistance是当前装备定义的基础防御，经需求倍率后通过GAS加法GE叠加其它来源；不覆盖初始防御/Buff。尚无九类型抗性和防御词缀。
5. `EquipLoad = Σ已穿戴Weight`；`MaxEquipLoad = BaseCapacity + CurrentStrength × CapacityPerStrength`。零容量且零重量算Light；零容量有重量算Overweight。无移速、闪避、翻滚或耐力惩罚，无背包重量。特殊容量加成接口留到威能阶段。

## 接口与生命周期

- 服务器`Equip(Slot, Definition, InstanceId)`：GUID必须有效且在当前已穿戴物品中唯一。多个实例可共享同一Definition；GUID由未来Inventory提供。当前API信任权威调用者，不实现物品拥有权检查、客户端RPC、复制背包或存档。
- 返回Success代表已穿戴且计算有效。NotReady/InvalidItem/WrongSlot/LevelTooLow/DuplicateInstance在修改前拒绝。EffectRejected表示新GE未接受，原槽位/句柄保留。AppliedInvalid明确表示已经穿戴但最终快照失效，不能当作未消费物品重复创建；需修正输入或卸下恢复。
- 同槽替换先确认新GE接受，再撤销旧GE；更新期间不会使用临时叠加的属性进行攻击。重复Initialize保留穿戴内容，不增加效果、不恢复Health/Resource。
- `Unequip(Slot)`移除实例及自己的GE，空主手回到UnarmedProfile。已有装备GE被外部系统误移除时快照无效，不把残缺贡献当真实结果；通过卸下/重新穿戴恢复。未来驱散规则需明确排除装备持有效果。
- 装备模式期间禁止直接调用SetWeaponProfile绕过槽位校验。退出/注销清理监听和自己的GE；功能启用开关不支持运行中切换。
- Equipment与Derived快照、GAS属性各自复制，尚未保证客户端三者同一网络帧到达。UI观察快照各自版本，不在客户端拼装权威伤害。

## 编辑器手动接入与验收

本阶段没有修改Content资产或操作编辑器。用户负责以下配置与PIE：

1. 加载本轮完整编译的项目。在BP_UmbraPlayerState的DerivedStatsComponent保留`Use Weapon Derived Power=true`，EquipmentComponent设置`Enable Equipment=true`。仅启用装备后，InitialWeapon不会自动变成已穿戴物品；空主手显示徒手10，属于预期。
2. 创建Data Asset，类选UmbraItemDefinition，例如DA_Item_TestSword。AllowedSlots只填MainHand，Weapon引用现有DA_Weapon_TestSword；根据测试填写需求、四维词缀、重量。不要改原WeaponProfile的类或手工操作uasset二进制。
3. 从PlayerState取得EquipmentComponent，在ASC初始化完成后的临时测试输入/事件中，Authority分支调用Equip。先生成并保存New Guid作为这件测试物品的实例ID，再传Definition和MainHand；读取返回枚举。重复用同GUID穿戴会返回DuplicateInstance，需要先卸下或换另一真实测试实例。
4. 用单通道Blunt基础100、Strength补正系数2的测试武器，角色基础Strength10，物品Strength需求15、自带Strength+10：应允许穿戴，当前Strength20，需求不满足，AD50。再戴一个Strength+5的戒指：当前Strength25，武器需求满足，AD150。卸戒指回AD50；卸武器回徒手10且Strength10。
5. 防具基础Armor100/MagicResistance40、需求Strength30、自带Strength+20：确认自身+20不能满足自己，未满足时贡献50/20；用其它来源满足后贡献100/40；卸下无残留。测试错误槽位、等级不足、同GUID重复、反复换装与角色重绑不回血。
6. 读取EquipmentSnapshot检查只计已穿戴Weight、容量、Light/Medium/Heavy/Overweight；MoveSpeed保持原逻辑。已有角色菜单槽位与CombatInfo仍需后续C++订阅接线，本阶段没有把占位显示自动换成真实值。
7. 普攻继续使用原GE/技能系数。此前AD160/AP70、AD系数1/AP系数0.2的伤害仍为174（零护甲、非暴击）。本阶段不擅自把AP系数改0，也没有实现混合类型逐段伤害。

## 自动化与后续

新增`Umbra.Equipment.RequirementsAndLifecycle`覆盖槽位/等级/身份、排除自身且含乘法Buff、其它装备满足需求、基础伤害与补正惩罚、防御恢复、只计已装备负重、无移动惩罚、GE拒绝保留旧装备、反复替换/初始化、更新中拒绝伤害、重绑和注销清理。当前构建/运行证据记录在[Progress](Progress.md)，不要将旧阶段测试结果当成本轮通过。

下一阶段是九类型伤害结算与混合普攻继承；之后再做A/X区、新暴击、威能、完整Inventory与装备UI/CombatInfo真实绑定。当前仅提供可复用服务器API和变化快照。
