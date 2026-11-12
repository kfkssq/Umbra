# 导入资产随机装备 V1

2026-11-03后续：[全类型V2及特殊装备规则](RandomEquipmentExpansion.md)已接入剩余150件；默认生产入口和原型敌人现使用V2，本文保留V1历史配置。V1的430件按源清单实际为146武器+284防具，下文汇总已纠正。

2026-11-03。基于现有实例、公共词条池和 GAS 装备链实现。旧生成配置与测试夹具保留原算法；本规则仅对 `bRarityFromAffixCount` 配置生效。

## 已实现的规则

- 随机抽取 1–4 条词条，映射普通/魔法/稀有/史诗。传奇威能尚未实现，正式配置不生成传说；独特必须等待专门设计入口，不通过增加词条产生。六品质展示和旧测试继续可用。
- `AffixCountWeights` 默认 50/30/15/5；恰好四项、非负有限、至少一项正权重。Common–Epic 展示行必须齐全。新模式忽略展示行的 Weight/AffixCount；旧模式继续读取它们。
- 词条资格仍按分类、武器类型、槽位、装备/来源 Tags 和等级筛选；品质限制在此模式表示由数量得出的基础品质。加权无放回选择保留目标去重/互斥，并排除会阻止凑满目标数量的选择；不能凑满则整件失败，不降品质、不隐式重抽类型。
- 基础攻击/防御/重量/补正/需求不计入词条数量。等级决定可用最高梯度，数值从已有离散候选中等概率选择；没有改成连续区间采样，也没有对高品质额外放大数值。
- 随机结果仍完整存入实例。装备走原 GE 和伤害快照，拾取、穿脱、UI 不重新计算或重掷。无需变更实例 SchemaVersion；生成规则版本为 3。旧实例不因规则改变被重新归类。

## 类型掉率

`UUmbraLootTable.bUseTypeGroups` 启用三层加权：Category → TypeGroup → Template。各层按稳定 ID/枚举排序；每件独立选择。旧 Entries/NoDropWeight 在新模式中不参与选择，原有平面模式不变。

| 参数 | V1 配置 | 来源/覆盖顺序 |
| --- | --- | --- |
| 每次击杀装备机会 | 100%，1 件，方便原型验证 | Table.EquipmentDropChance / MinEquipmentCount / MaxEquipmentCount；不是最终平衡值 |
| 大类 | 武器 40、防具 60 | Table.Categories.Weight |
| 武器类型 | 10 种近战类型各权重 1 | TypeGroup.Weight；单种总掉率为全部装备的 4% |
| 防具部位 | 头/胸/手/腿各权重 1 | TypeGroup.Weight；单部位总掉率为全部装备的 15% |
| 类型内模板 | 默认每件权重 1 | Template.Weight，仅改变类型内部概率 |
| 词条数量 | 50/30/15/5 | GenerationProfile.AffixCountWeights |
| 等级 | 来源等级 + Profile.LevelOffset，夹到 Profile 范围 | 敌人 LootItemLevel/SourceTags；不读玩家等级，不覆盖 RequiredLevel |

类型数量和类型内资产数量均不从文件数量推导权重。扩大一种类型的模板集合不会改变类型选择序列。正权重空类型、重复身份、分类/部位不匹配、无武器伤害配置或无有效生成配置会阻断整表；运行时记录错误，不转抽其他类型。当前表直接引用全部定义，未实现大规模分区流送或冷加载性能优化。

## 正式资产与需求保护

输出位于 `/Game/Items/GeneratedEquipment/V1`，引用 Library 原模型和图标，不复制资源、不修改 Library 草稿。430 件：146 武器、284 防具；另 150 件现有草稿类型不在 V1 调参范围，明确排除。资源导入失败项没有草稿，因此不在这 580 件扫描范围内。

| 武器类型 | 件数 | 防具部位 | 件数 |
| --- | ---: | --- | ---: |
| 单手剑 / 双手剑 | 23 / 29 | 头 | 82 |
| 匕首 / 刺剑 | 13 / 6 | 胸 | 64 |
| 单手斧 / 双手斧 | 12 / 9 | 手 | 73 |
| 单手锤 / 双手锤 | 12 / 13 | 腿 | 65 |
| 矛 / 戟 | 16 / 13 | | |

名称、说明、图标、分类、模型、显示变换、RequiredLevel 和四维 Requirements 原样读取当前草稿。**武器需求已由现有导入流程写入 ItemDefinition.Requirements，本流程不重新从 CSV 计算，不重置为零。** 源补正系数仍为参考资料，不冒充 Umbra 每点伤害系数。

基础战斗参数来自 [random_equipment_v1.json](../Scripts/EquipmentImport/random_equipment_v1.json)，为明确的 Umbra 原型调参，不是导入游戏原数值。武器目前按类型配置单一物理通道、基础伤害区间和重量，无新增补正曲线；防具按部位配置固有护甲/魔抗和重量。名称/外形不自动决定隐藏属性或传奇威能，分类也不表示已经完成对应武器动作、投射物或角色穿戴外观。

词条池：

- 通用：力量、敏捷、智力、信仰、生命上限。前两项及生命沿用现有 MVP 两档数值；智力/信仰复用同一主属性梯度。
- 防具：通用 + 护甲，沿用现有 Armor 两档。没有向武器开放防御词条。
- 武器：通用 + 暴击率 + 匹配该类型基础物理通道的普通攻击 A 区增伤。增伤沿用 M4 SlashA 的现有梯度，不自动加倍或当作传奇威能；暴击沿用 MVP 梯度。
- A 和暴击为比例增量；护甲、生命、主属性为原生加法单位。实际 A 生效依赖攻击原有 `bUseDamageBuckets` 开关。

源定义已有 Weapon/生成配置/固定奖励时拒绝自动合并。同身份或同模型不会重复加入。世界物理代理通过 UE 的 `TryFitWorldDropPhysics` 拟合，不合格项明确隔离。430 件均通过本轮边界拟合，但不等于每个模型人工落地外观已验收。

## 工具与修改方式

关闭 UE 并完成 UmbraEditor 构建后，在项目根目录运行：

```powershell
# 只读审计
& ./Scripts/EquipmentImport/PrepareRandomEquipment.ps1
# 创建缺失目标并接入原型敌人；已有目标字段漂移会报错保留
& ./Scripts/EquipmentImport/PrepareRandomEquipment.ps1 -Apply -Activate
```

入口会核对引擎关联并拒绝与正在运行的编辑器并行写资产；可用 `-EngineRoot` 显式指定安装位置。

[create_random_equipment.py](../Scripts/EquipmentImport/create_random_equipment.py) 在 UE Python 中执行：默认只读审计；环境变量 `UMBRA_RANDOM_APPLY=1` 创建缺失目标。配置与预期字段保存为托管元数据，重复执行检查已有字段；配方变化或手改冲突保留已有资产并报错，不覆盖。已创建的独立包可续跑，整体错误仍必须修正后重新验证，不将部分成功视为完成。

配置入口有两个用途：JSON 是可重复创建的默认值；正式 DataAsset 是运行时权威值。编辑器手改目标不会被创建器覆盖，但后续校验会报告漂移。需要批量更换配方时使用新的 destination/规则版本并重新验证后切换掉落表；当前没有自动覆盖已发布实例或全库迁移按钮。

[activate_random_equipment.py](../Scripts/EquipmentImport/activate_random_equipment.py) 默认审计；`UMBRA_RANDOM_ACTIVATE=1` 将 `/Game/Blueprints/Enemies/BP_Enemy_Melee_01` 及 L_Prototype 中该类的显式实例覆盖接入新表，只保存发生变化的目标。保留世界物品类、来源等级和散落配置。检查 `Saved/RandomEquipment/activation.json` 获取实际写入对象与旧引用；测试生成器不改。

引擎路径从本机 `Saved/EquipmentImport/dcx_tools.json.ue_root` 读取，核对 `Umbra.uproject` 的 5.8 关联与 Build.version，禁止猜测路径。脚本通过 `UnrealEditor-Cmd -run=pythonscript -script=<绝对路径> -EnablePlugins=PythonScriptPlugin,EditorScriptingUtilities` 执行；PIE 脚本使用编辑器 `-ExecutePythonScript` 入口。所有资产保存都通过 UE 5.8.2；本轮没有修改模块/Target，无需重新生成 Rider 项目。

## 验证

- `Umbra.Items.Generation.ProductionLoot`：1–4 条与品质、确定性、类别过滤、互斥死路、空类型拒绝、零掉落机会、2000 次固定种子类型选择；将剑模板扩到 29 件而匕首仅 1 件后，类型选择序列完全相同。
- `Umbra.Items.Generation.ProductionAssets`：真实 430 件定义，每件 4 种数量 × 4 个等级边界 × 8 个种子，共 55040 次生成；词条数量、品质及类别限制通过。
- [smoke_random_equipment_pie.py](../Scripts/smoke_random_equipment_pie.py)：14 个类型各一件强制数量覆盖 + 8 次正常表选择，走真实 Health 归零、死亡、掉落、拾取、穿戴、卸下及 Tooltip；逐次检查完整实例不变，不保存测试地图。
- PIE 测试夹具使用 400cm 散落半径，并分散到原地图敌人位置。首次沿用原位置/半径时 22 次死亡中 1 次无法找到合法落点；正式落点算法及半径未改，不保证狭窄/阻挡区域每次都能放置掉落物。
- 最新实际结果及未验证范围见 [Progress](Progress.md)。Saved 报告为本机证据，不入 Git。
