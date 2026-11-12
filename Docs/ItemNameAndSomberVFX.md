# 物品错误名称与失色石品质光效

后续已确认：用户所指为石头等级，需求改为按等级固定品质，已按 [固定品质与19种展示](StoneGradesAndStartup.md) 实施。下文保留最初随机实例诊断的历史证据；原六档强制随机测试入口已转向新的19种测试。

2026-11-06。

## 名称修正

通过 UE 5.8.2 编辑器修正13份 ItemDefinition 的 DisplayName，移除来源文本的 `[ERROR]` 标记：纠缠脐带，以及勇士额带、勇士腕套、勇士皮盔、勇士战袍（轻装）的 Library/V1/V2 定义。定义ID、引用路径和原始来源记录保持不变。Native/Random 导入配方中的显示字段同步更新，源文本与哈希仍保留用于追溯。

导入显示名由 `Scripts/EquipmentImport/item_catalog.py::display_name` 清理；空名称或清理后仍含 error 的名称拒绝导入。原始目录数据不改写。迁移入口为 [fix_error_names.py](../Scripts/EquipmentImport/fix_error_names.py)，设置 `UMBRA_NAME_FIX_READBACK=1` 时仅检查、不保存。

已验证：保存13份资产；独立编辑器进程回读1835份定义，显示名称不再包含 error；既有 catalog 单元测试6项及显示名清理5个检查通过。报告位于 `Saved/ItemNameFix`。

## 失色石光效核查

用户报告最高品质失色石仍冒白光。当前保存资产中的 Material 六档展示配置与 V2 Armor 一致；掉落统一通过 BP_WorldItem 的 LootVFXStyles 按实例 Rarity 选取系统，没有材料类型的白色特例。材料不显示传奇战斗效果提示的规则与 Niagara 光效无关。

[smoke_somber_rarity_pie.py](../Scripts/smoke_somber_rarity_pie.py) 在 L_Prototype 的真实 PIE 中，经敌人死亡生成六种实例品质的失色石，检查对应系统、激活状态、六个实际颜色参数的色相比值与拾取清理。测试临时覆盖抽取权重，高两档还临时覆盖编号石的 SourceRarity，生成后恢复；不保存物品或地图，也不改变正式品质概率。白/绿/蓝/紫/红/金继续沿用 [LootVFX](LootVFX.md) 的配置规则。

注意：失色锻造石【1–9】是材料种类编号，不是实例品质枚举。当前 SourceRarity=1 的编号石仍可生成普通品质，普通实例应发白光。不得将编号9直接当作 Unique，或在未确认需求时改写品质概率。

本轮已执行该回归脚本，六档系统、激活状态、36个颜色参数色相比值、拾取清理均通过；报告 `Saved/SomberVFX/pie.json`。已查看隔离截图 `Saved/Screenshots/WindowsEditor/SomberRarity.png`，从左到右白、绿、蓝、紫、红、金可区分。

本轮未修改 C++ 或 VFX资产，尚未确认用户现场白光的根因；不能把测试场景正确等同于原问题已修复。未验证用户原运行实例、多人、Cook/打包、声音；未运行 C++ 构建或完整自动化套件。
