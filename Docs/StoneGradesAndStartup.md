# 锻造石固定品质与19种开局展示

2026-11-06，用户确认：锻造石与失色锻造石不使用随机品质，品质随石头等级固定。

| 材料等级 | 品质 | 地面光效 |
|---|---|---|
| 1–2 | 普通 | 白 |
| 3–4 | 魔法 | 绿 |
| 5–6 | 稀有 | 蓝 |
| 7–8 | 史诗 | 紫 |
| 9（仅失色） | 传奇 | 红 |
| 古龙岩（两类） | 独特 | 金 |

## 配置来源与规则

- 显式源ID映射在 `Scripts/EquipmentImport/stone_grades.py`。普通石为10100–10107，古龙岩10140；失色1–8为10160–10167，失色9为10200，古龙岩失色为10168。不可按ID连续性猜测后两件的顺序，也不从中文名称解析规则。
- 导入器和 `apply_stone_grades.py` 将映射写入19份 ItemDefinition.Rarity，以及对应颜色、背景、边框。展示资源取自现有 DA_Generation_Material 的对应品质行。
- `bMaterialRarityOnly` 现在表示固定材料品质：C++生成器只选择 Definition.Rarity 对应的展示行，不受 SourceRarity、随机种子、词条数量权重或晋升概率影响；不产生战斗词条。缺少对应品质行时拒绝生成。SourceRarity保留为原始来源信息，不覆盖材料品质。
- Material Profile版本升为8，关闭 `bRarityFromAffixCount`。装备/护符仍沿用原有随机品质规则。掉落种类概率、材料强化/消耗功能没有改变。
- 运行时实例保存固定品质快照，UI和地面Niagara均读取同一实例品质。Niagara继续使用现有六档系统及高档红/金覆盖色。旧实例不自动重掷；重新开始PIE会按新规则生成。
- 资产调参优先于导入默认映射：运行时读取Definition.Rarity，而不是硬编码源ID。重新导入遇到手工改动仍按现有配方检查拒绝覆盖。原始文本、参数与哈希来源信息保留。

## 开局展示与恢复

2026-11-06 当前状态：按用户要求暂时关闭两排19种石头展示。`BP_StartupAllSmithingStones`、`BP_StartupAllSomberStones`、旧单石生成器 `BP_StartupSmithingStone` 的 Blueprint 默认和 L_Prototype 实例均关闭 Enabled；保留物品列表、位置和固定品质规则。执行 `Scripts/disable_startup_stones.py` 可重复应用，设置 `UMBRA_STONE_DISABLED_READBACK=1` 只读回查。恢复19种展示可执行原 `configure_stone_rarity_startup.py`，或开启两排生成器的 Enabled。

L_Prototype使用两排原生测试生成器：

- `BP_StartupAllSmithingStones`：锻造石1–8、古龙岩，共9件。
- `BP_StartupAllSomberStones`：失色锻造石1–9、古龙岩失色，共10件。

启用展示时，每次开局每种一件，按编号从左到右排列，古龙岩位于末端。相邻物品间距140cm，两排相距250cm。复用原生生成器的逐定义模式，等级1，品质固定，实例GUID每次更新；没有新增Blueprint图表逻辑。

原六件装备生成器 `BP_SixRarityStartup` 和单件随机石头生成器 `BP_StartupSmithingStone` 的Enabled暂时关闭（BP默认及地图实例）；护符生成器保持现状。要恢复旧展示，在地图关闭两排新生成器，并重新开启上述两个旧生成器；BP默认也可同步恢复。此操作只改变展示，不恢复随机材料品质。配置脚本 `Scripts/configure_stone_rarity_startup.py` 会重新启用19种展示。

## 验证入口

- 构建 UmbraEditor / Win64 / Development。
- `Umbra.Items.Generation.MaterialRarity`：六档固定品质×四种来源品质×32个随机种子，无词条、记录恢复、非法配置拒绝。
- `Umbra.Items.Generation.NativeAccessoryAssets`：19份真实材料/116份护符、生成/背包Tooltip/装备拒绝与混合掉落表。
- `Umbra.Inventory.SmithingStacks`：同种同品质堆叠、不同类型/历史品质分堆，数量与记录保存。
- `apply_stone_grades.py` 加 `UMBRA_STONE_READBACK=1`：独立进程仅回读配置与配方，不保存。
- `Scripts/smoke_stone_startup_pie.py`：从已保存地图连续两次PIE，检查9+10件、旧生成器停用、固定品质、实际粒子系统/颜色、拾取及跨开局稳定性；不保存测试状态。

本轮实际结果见 [Progress](Progress.md)。
