# 装备源资料目录

本层读取法环 CSV、Smithbox 中文文本 JSON、现有 icon 和 parts，生成可复查的物品资源关联。它是资料整理工具，不创建或修改 ItemDefinition，也不自动运行全库模型导入。原始战斗数值只保存在 raw_params，不影响 Umbra 游戏规则。

## 使用

在项目根目录执行：

```powershell
& .\Scripts\EquipmentImport\BuildItemCatalog.ps1
```

默认使用 [本机配置示例](../Scripts/EquipmentImport/item_catalog.local.example.json) 和已有 Saved/EquipmentImport/dcx_tools.json 的 UE Python。路径变化时复制配置并传入 `-Config <配置绝对路径>`。mapping 相对路径以项目根目录为基准。纯 Python CLI 同样应从项目根目录运行。

每次输出到新的 Saved/EquipmentImport/Catalog/时间戳目录，不覆盖旧报告：

- catalog.csv：快速检查名称、原始类型、Umbra 分类建议、模型和 icon 编号、异常。
- catalog.json：完整记录、文本来源、全部原始参数、资源去重索引和输入 SHA256。
- issues.json：按源物品 ID 列出的异常及复核事项。
- report.json：计数与验证边界。

CSV 是查看输出，JSON 是后续工具接口；不要把手工物品配置写进生成文件，重跑不会读取旧输出。重复运行不操作 UE、不覆盖原始文件，每次重新扫描路径。输入内容版本目前无法从 CSV 证明，明确标记 unverified。

## 关联规则

### 资源入选硬门槛（2026-11-01）

只有模型文件存在、对应明确，以及所需 icon 按编号唯一匹配且文件存在，才进入 import_candidates.json。当前防具同时存在多个男女模型时先排除，等待显式选择变体；不随机挑选。缺少任一 icon、重复编号或文件已消失均排除。筛选时重新检查文件，后续真正执行导入前也必须再次调用 resource_gate 或重新构建，不可信任旧清单。完整 catalog 保留全部资料，rejected_resources.json 记录排除原因；CSV 新增 ResourceEligible 和 ExclusionReasons。

最新实测输出 Saved/EquipmentImport/Catalog/20261101T143649541734Z：3391 条通过资源门槛，1083 条排除。仍为源记录数，包含共用模型的派生记录。入选只证明源文件存在且编号匹配，不证明 DCX 转换成功、icon 视觉一致、名称有效或类别已配置；这些待复核项保留在 remaining_review 中，不能直接据此自动创建正式 ItemDefinition。

6 项测试通过，含源文件消失及歧义匹配必须拒绝的测试。本次没有执行 UE 导入。

身份使用 `ER:EquipParamWeapon:行ID` / `ER:EquipParamProtector:行ID`。同编号跨表不混用；派生记录单独保留，共用资源只在 resources 中列一次。没有直接名称的记录保留 name_missing，不猜编号偏移，不删除占位数据，不自动视为可用物品。

名称、摘要、描述分别来自 Weapon/Protector 的 Name、Info、Caption FMG。读取 Base 和 DLC 表，忽略空值、[ERROR]、dummy。不同表对相同条目给出不同有效文本时报告 conflict，不猜覆盖顺序。

武器 `wepType` 标签来自用户安装工具的 Assets/PARAM/ER/Param Enums/WEP_TYPE.json，并记录哈希。分类建议来自 [映射配置](../Scripts/EquipmentImport/item_category_mapping.json)，对齐 Source/Umbra/Items/UmbraItemClassification.h。直剑到 OneHandedSword、大剑到 TwoHandedSword 等仅为待复核的展示分类建议，不选择 WeaponProfile，不写 AllowedSlots，不改变游戏行为。盾、拳套等没有现成对应类型时保留原类型并报告 unmapped；未知原始枚举也保留数字。

模型按 equipModelId、equipModelCategory 和防具部位索引精确文件名，排除 `_l`。武器候选为 wp_a，防具为 hd/bd/am/lg 的 m/f 文件；gender 原始字段完整保留，但候选存在不表示男女适配正确，不自动挑选。防具标为 WearableSource；武器标为 WorldDrop。目录关联不等于骨骼兼容、材质转换成功或物理可用。防具作为世界掉落资源必须走独立配置流程。

icon 仅接受 `menu_knowledge_<编号>out.png` 与 `menu_knowledge_<编号>out.obj.png`，前导零归一化。多个文件同编号报告 ambiguous，零个 missing，不模糊匹配。`matched_by_id` 不是视觉验收，visual_verified 始终为 false。iconIdM/iconIdF 分开保留。

原始 CSV 数值以字符串原样保留，避免浮点精度损失。需要映射 Umbra 数值时另建明确转换规则；不将法环攻击力/防御力直接写入 Umbra。

## 本机实测（2026-11-01）

输出：Saved/EquipmentImport/Catalog/20261101T143328858110Z。

| 项目 | 结果 |
|---|---:|
| 武器记录 | 3636 |
| 防具记录 | 838 |
| 名称匹配 | 4151 |
| 有模型候选 | 4308 |
| 所需 icon 均按编号找到 | 3623 |
| 唯一关联资源路径 | 2380 |
| 名称缺失 | 323 |
| Umbra 分类未映射 | 1887 |

这些是源记录数，包含派生和占位记录，不能当作独立可用装备数量。分类未映射包含大量现有 Umbra 枚举尚未覆盖的类型，不是读取失败。

抽查：Weapon 2000000 → 长剑 → Straight Sword → wp_a_0200；Protector 100200 → 旅行手套 → am_m_1140；Protector 270200 → 大树守卫臂甲 → am_m_1730。Weapon 100200 名称无效，仍保留，未借用同模型的长剑名称。

5 项自动测试通过：文本冲突、占位过滤、重复 icon 不猜选、部位隔离及排除低模、CSV 缺列拒绝。全量输入完成实际关联；未做全库 icon 视觉确认、CSV/文本游戏版本一致性确认、UE 导入或 PIE。本次无 C++/二进制资产修改，因此没有运行编译。

## 后续接入

2026-11-01 更新：[小批物品导入入口已实现并验证](EquipmentItemImport.md)，支持显式选择的 WorldDrop 模型、icon 和展示草稿。下面的全库选择、穿戴及正式配置补全仍待后续。

先从目录按源类型筛选有名称、唯一 icon、明确模型的记录；核对小批次 icon 后，再提供按 SourceID 选择生成导入计划的入口，调用 [现有 DCX 管线](EquipmentDcxImport.md)。防具先明确男女及 WorldDrop/Wearable 用途。随后才增加 UE icon 导入与独立源资料资产，最后增加显式选择字段的 ItemDefinition 草稿/补全步骤；手工配置与源数据刷新分开。

小批 WorldDrop 写入步骤以新版入口文档为准；尚未自动创建随机装备、角色骨骼适配或全库材质规则。
