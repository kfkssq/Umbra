# 徒手、武器配置与 AD/AP 派生（第二阶段）

第四阶段现已提供[TypedDamage](TypedDamage.md)可选类型结算，武器继承模式直接使用本组件的类型明细。下文“类型结算尚未实现”属于第二阶段历史边界；默认Legacy继续原公式。

后续第三阶段已加入[最小装备组件](EquipmentFoundation.md)：开启装备模式后，槽位/需求结果接管武器输入，并在类型明细中应用基础/补正惩罚；本页描述的是装备功能关闭时的独立武器配置流程。类型伤害Execution仍沿用旧规则。

## 范围与当前兼容边界

玩家 PlayerState 与敌人 Character 新增 `UUmbraDerivedStatsComponent`。默认 `bUseWeaponDerivedPower=false`，原有属性初始化、伤害公式和蓝图资产继续工作。组件不 Tick，监听四主属性并将结果写入原 AttackPower / AbilityPower；没有重复声明四主属性。

本阶段的九类型仅是武器数据与派生明细，不是九类型伤害结算。现有普攻仍使用旧 DamageConfig 的物理/魔法选择及 AD/AP 系数；默认 AP系数0仍不继承魔法武器明细。混合武器普攻继承、类型抗性、A/X区、新暴击、装备要求/负重/背包、词缀和威能在后续阶段实现。不要把本阶段混合武器明细等同于已实现混合命中。

## 配置及公式

- [UmbraWeaponProfile](../Source/Umbra/Items/UmbraWeaponProfile.h) 是仅含伤害数据的DataAsset。Channels每个类型最多一项：Slashing、Blunt、Piercing归物理；Fire、Lightning、Cold、Radiant、Poison、Shadow归魔法。
- 每项保存 BaseDamage（伤害点）和至多四项Scaling，每个主属性最多一次。Coefficient为非负系数；PointCurve为空时用主属性点数，否则X为主属性点数、Y为补正单位。曲线外推沿用资产设置，当前输入处必须为有限非负值。
- `类型补正 = Σ Coefficient × (PointCurve(CurrentPrimary) 或 CurrentPrimary)`；`类型伤害 = BaseDamage + 类型补正`。AD为物理类型伤害之和，AP为魔法类型伤害之和。未实现任何AD/AP特殊倍率。
- 不读取旧AD/AP作为输入；每次用专用临时Instant Override GE替换绝对结果，反复重算不累计。空Channels是明确的零伤害配置，重复类型/主属性、负值、非法枚举和溢出会拒绝。
- 配置顺序：C++徒手后备（Blunt10、无补正）→ 组件UnarmedProfile覆盖 → 非空InitialWeapon/SetWeaponProfile所选配置。SetWeaponProfile(nullptr)回到徒手。DataAsset运行时视为只读；不要原地修改共享资产。
- 组件Initialize在ASC初始GE/调试覆盖、满池初始化之后调用，所以新模式最终AD/AP覆盖旧初始值。推荐新模式初始GE不填写AD/AP；原模式保持原设置。无效初始配置保留原属性但不生成有效派生快照，伤害入口拒绝该新模式来源的攻击，避免把旧值误认作派生值。

## 生命周期与数据所有权

- 服务器计算和选武器；BlueprintAuthorityOnly只是编辑器提示，C++也检查权威端。SetWeaponProfile不是装备接口，不执行等级、槽位或属性要求判定，不给客户端提供换装RPC。
- 四主属性变化（包括GE添加/移除）触发重算。相同ASC重新Initialize保留已选武器、不重置初始配置；ASC自己的初始化标志保证不回血。
- 发布期间同步改变主属性时标Dirty，至多排空8次，避免递归。出现反馈循环或计算失败将快照标无效；旧GAS值保留用于诊断，统一伤害入口不会使用它继续攻击。发布时不允许切换武器。
- AD/AP普通Modifier GE在派生模式被整项拒绝，避免一半生效。启动时若已有持续AD/AP Modifier也拒绝启用。原F1/F2 AddEffect在派生模式改用派生调试GE：其余属性不变，只移除直接AD/AP加成，让四主属性间接产生补正。
- C++直接SetNumericAttributeBase及自定义Execution写AD/AP不经过普通Modifier查询，禁止在新模式使用这些旁路。生产AD/AP只有派生组件写入；普通装备不能提供直接AD/AP。
- 组件销毁/注销时成对移除四主属性监听和GE应用查询。客户端不计算；Snapshot通过组件复制，OnRep发送OnDerivedStatsChanged。快照包含有效性、版本、徒手标识、类型基础/补正/总值及AD/AP。
- 网络属性与Snapshot不是跨对象原子复制。需要一致的类型明细和AD/AP的UI应整份读取Snapshot，不能混用不同帧的GAS值。尚未接入CombatInfo，尚未进行网络PIE。

## 手动配置（由用户执行）

1. 保存并关闭编辑器后，在原项目完整构建UmbraEditor Win64 Development，再打开项目；新增反射类型和组件不依赖Live Coding加载。若验证使用Saved中的独立副本，该副本DLL不会更新原编辑器。
2. 内容浏览器新建Data Asset，选择UmbraWeaponProfile；填写Channels。例：挥砍Base80、Strength系数2、Dexterity系数3；火焰Base20、Intelligence系数1、Faith系数0.5。
3. 玩家实际PlayerState蓝图/敌人蓝图的DerivedStatsComponent上启用Use Weapon Derived Power，配置InitialWeapon或徒手UnarmedProfile。只对测试角色开启，未开启者维持旧模式。
4. 四主属性10/20/30/40时应得到AD160、AP70。F1/F2 AddEffect将四项各加20时AD260、AP100，RemoveEffect恢复160/70。生命/移动等现有功能继续按原规则验收。
5. 在服务器调用SetWeaponProfile(nullptr)应得到徒手结果；换回武器恢复明细。重新Possess不回血、不重复添加补正。
6. 现有伤害GE无需更换：只验证它正确读取新的AD/AP。类型继承暂未实现，不以本阶段验收混合伤害或类型抗性。

## 自动化

`Umbra.Attributes.WeaponDerivedPower`：默认旧模式、徒手、混合武器算例、曲线、纯魔法AD归零、反复初始化、Buff添加/移除、拒绝旧直接AD/AP GE、无效配置、卸下/重新选择、同步回调修改主属性、半更新期间拒绝伤害、Avatar重绑不回血、销毁解绑。构建和回归结果见[Progress](Progress.md)。这些测试不替代WBP/资产接线和网络PIE。
