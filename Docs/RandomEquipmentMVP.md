# 随机装备 M1–M3 实施记录

2026-10-31：[六品质修复](SixRarityRepair.md)覆盖本文历史三品质范围：生成器支持Common至Unique，RandomMVP配置已扩为六品质、规则版本2；颜色由实际品质行提供。

2026-10-30后续更新：公共Damage池与拥有者请求已进入 [M4实现](RandomEquipmentM4.md)。本文保留M1–M3交付范围，后续验证以Progress为准。

2026-10-29。范围为单机 Primary/Attribute 随机装备 MVP；公共 Damage 池、远端穿脱请求、完整存档及批量生产属于后续里程碑。原规划见 [路线图](RandomEquipmentRoadmap.md)，本次验证见 [Progress](Progress.md)。

## 实例契约（M1）

`FUmbraItemInstance` 的 SchemaVersion=1 包含 Definition/DefinitionId、InstanceId、ItemLevel、Rarity、GenerationSeed、GenerationRulesVersion 和完整 RolledAffixes。`AffixTierId` 标记内容档，`SelectedTier` 仍是 CandidateValues 下标。背包/装备的 `Instance` 是完整数据；旧 Definition/InstanceId/RolledAffixes 字段保留为 Blueprint 兼容投影，由 SetInstance 一次写入。GetInstance 支持旧投影，不删除旧 Blueprint 字段。

版本0可调用 UpgradeLegacy：保留 GUID 和全部掷值，从旧定义复制等级与品质，规则版本0表示旧工厂。旧定义未填写 DefinitionId 时暂用资源路径；这是兼容键，重命名后不能当作长期存档 ID。新随机模板必须填写稳定 DefinitionId。未知结构版本拒绝；Profile 模板不能通过旧 Create/Equip 或版本0实例绕过生成。固定装备的旧 Equip 入口也升级到版本1，避免同 GUID 新旧结构冲突。

`FUmbraItemInstanceRecord::Capture/Restore` 提供内存 DTO 边界，保存 ID/版本/掷值，Restore 由调用者提供匹配定义，不保存 Definition 对象或 GE handle。Damage magnitude 写成实际标量。已测试反射序列化内存往返，未实现存档槽、资源注册表、跨版本内容迁移或网络请求。

## 配置来源与优先级（M2）

| 参数 | 配置入口 | 规则 |
|---|---|---|
| opt-in、稳定 ID、分类/槽位/装备 Tags | ItemDefinition | GenerationProfile 为空时保留旧工厂；不允许同时填旧 AffixRollConfigs |
| 来源等级、来源 Tags | 敌人 LootItemLevel/LootSourceTags；测试生成器 GenerationContext | 来源等级必须≥1，不读取玩家等级，不影响 RequiredLevel 穿戴门槛 |
| 最终物品等级 | GenerationProfile | clamp(SourceLevel + LevelOffset, MinimumLevel, MaximumLevel) |
| 品质权重、词缀数量、颜色 | Profile.Rarities | 仅 Common/Magic/Rare；权重0不抽；不能全0；数量0–16 |
| 词缀权重 | AffixPool.Entries | 稳定按 AffixId 排序后加权无放回抽取 |
| 资格 | AffixDefinition | 分类、武器类型、允许槽位交集、品质、装备必需/禁止 Tags、来源必需 Tags、等级上限 |
| 内容档位 | AffixDefinition.Tiers | 取 MinimumLevel≤物品等级的最高档；档 ID/起始等级不重复，各档语义目标一致 |
| 候选值 | Tier.Values | ExplicitValues 优先，否则均匀5/6档；常量为1档；整数取整有重复则拒绝 |
| 最终值 | 生成时的独立 FRandomStream(seed) | 保存完整候选、下标和值；UI、转移与装备不再抽取 |

同一件实例不重复 AffixId、Primary/Attribute 语义目标或非空 ExclusiveGroup。显式固定 PrimaryBonuses/AttributeBonuses 不得与池目标重复；固有 Armor 等基础值仍是独立基础通道。生成不足会返回原因并丢弃整次结果，不减少词缀数量，不隐式重抽。Validate 校验结构与候选；实际上下文过滤/互斥后数量不足由 Generate 检出，不宣称已穷举所有未来内容组合。

量纲沿用 GAS：主属性、生命、护甲为原生加法单位；攻击速度是倍率增量，暴击率是概率增量（0.01=1个百分点）。公共池拒绝 Damage；旧逐条 A/X 工厂与生效链保留。

编辑器对 ItemDefinition 执行 Validate Assets 会调用 IsDataValid，并显示资源路径和规则原因。Profile/pool 独立资产通过引用它们的模板校验。配置变更应提升 RulesVersion；确定性保证要求同一配置、上下文和 seed，不能仅凭旧版本号在修改后的资产上重建旧掷值。

## 展示与掉落（M3）

敌人死亡保持原掉落表选择/数量语义，选到 opt-in 模板才调用生成器。测试生成器用固定批次 seed 派生各物品 seed，并为每件创建新 GUID；日志输出 ID、seed、等级、品质、词缀数。LootTable 增加显式 FRandomStream 重载。

Tooltip、背包/装备格子、地面标签/光效和拾取/穿脱/选择/拖拽音频上下文读取实例。新随机品质框使用现有默认框按 Profile 颜色染色；不新增品质美术。旧定义品质框保持原配置。基础属性、武器 profile 与美术引用继续读取定义；本次未做全部模板数据快照。

## 独立资产与复现

11个新资产位于 `/Game/Items/Tests/RandomMVP`：5个词缀（Strength、Dexterity、Health、Armor、CriticalChance）、公共池、三品质 Profile、Sword/Chest 两个模板、掉落表和 BP_LootTest_RandomMVP。两个模板由现有剑/胸甲复制，复用原模型及物理配置，清空复制来的固定奖励和旧随机配置，设置独立 ID 与 Profile。源资产未保存或覆盖。

Profile 默认三品质等权，词缀数0/1/2；两档起始等级1/10，第二档范围为第一档两倍。测试生成器等级10、批次 seed12345、两模板各6件。正式地图和敌人掉落表未改。

- 在 UE 5.8.2 中执行 [create_random_mvp.py](../Scripts/create_random_mvp.py) 可创建缺失资产；已有目标一律保留，不自动更新。
- 将 BP_LootTest_RandomMVP 放入临时测试关卡有地面的区域，开始 PIE；生成器每100cm排列一件，需要足够地面。
- [smoke_random_mvp_pie.py](../Scripts/smoke_random_mvp_pie.py) 在 L_Prototype 中临时放置生成器，执行真实 TryPickup/EquipFromInventory/UnequipToInventory，逐件比较版本、ID、等级、品质、seed、档位/候选/实际值，结束后销毁临时编辑器 Actor，不保存地图。
- 原生自动化入口 `Umbra.Items.Generation.Rules`、`Umbra.Items.Generation.Lifecycle`，加上 `Umbra.Items.RandomBaseline.*` 和现有转移/UI/伤害测试。M0回滚测试继续反射比较整个实例。

人工待验：真实渲染下颜色/品质框/Tooltip，鼠标导航拾取、菜单重开和拖拽听感；双端网络和打包。headless PIE 不代替这些结果。
