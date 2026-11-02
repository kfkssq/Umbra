# 小批物品一站式导入

2026-11-02新增[正式分批入口与材质定义解析](EquipmentBulkImport.md)，自动筛选、失败隔离、retry清单和同一FLVER世界掉落静态部件合并。下文为早期小批用法；最新能力/限制以新文档和实测报告为准。

最新默认法线规则见 [匕首验证](DaggerNormalValidation.md)：新测试用item_normal_smoke.json / ItemNormalAuto01。下文ItemBridgeSmoke01为历史候选规则测试，不覆盖历史配方。

入口 [ImportItems.ps1](../Scripts/EquipmentImport/ImportItems.ps1) 将源资料关联、DCX 转换、UE 模型材质导入、UI icon 导入和物品展示草稿串联起来。源文件只读，全部新资产位于 `/Game/EquipmentImport/Staging/<Batch>`。

## 操作

关闭 UE 编辑器，在项目根目录运行：

```powershell
# 默认 Dry Run：重建源关联并验证选择，只写计划，不转换或写 UE。
.\Scripts\EquipmentImport\ImportItems.ps1 -Rules .\Scripts\EquipmentImport\dcx_rules_1140_review.json

# 实际导入本次两件样品，1140 明确选用已审计的候选材质规则。
.\Scripts\EquipmentImport\ImportItems.ps1 -Apply -Rules .\Scripts\EquipmentImport\dcx_rules_1140_review.json
```

默认选择 [item_smoke.json](../Scripts/EquipmentImport/item_smoke.json)：

| 源身份 | 名称 | 分类 | 主 FLVER | icon |
|---|---|---|---|---:|
| EquipParamWeapon:1000000 | 匕首 | Weapon / Dagger | WP_A_0100 | 10000 |
| EquipParamProtector:100200 | 旅行手套 | Armor / Hands 建议 | AM_M_1140 | 13072 |

以后复制选择 JSON，修改 batch 和 items，用 `-Selection <路径>` 指定。每条明确 source_id、primary_model、icon_field、usage。来源配置使用 `-Config`，默认沿用资料目录的本机路径。改变源文件/规则/分类时使用新的 batch；同一 batch 的已有资产禁止覆盖。普通装备使用默认 dcx_rules.json；1140 候选规则不是所有装备法线转换规则。

## 硬门槛与流程

1. 每次从原始 CSV/文本/目录重建关联，不信任过期候选清单。资源必须通过 [模型+icon门槛](EquipmentItemCatalog.md)，本阶段草稿还要求名称有效、分类有映射。
2. 哈希固定选中的原 DCX 和 icon；转换前后及 UE 导入前再次检查。型号不明确、文件缺失、重复选择等直接拒绝。
3. 调用现有 DCX 转换器，只转换所选物品引用的去重模型包。失败保留报告，不创建草稿。
4. 显式选择主 FLVER，若该 FLVER 导入后产生多个 Mesh，拒绝猜选。匕首包的 WP_A_0100_1 附属模型仍独立导入，但不自动绑定为主掉落模型。
5. 原模型材质导入器创建资产；UI icon 配置 sRGB、TC_EDITOR_ICON、UI TextureGroup、NoMipmaps。
6. 创建现有 UUmbraItemDefinition 类的独立草稿，填写 DefinitionId、DisplayName、FlavorText、ItemCategory、WeaponType（武器）、Icon 和 WorldDropMesh/WorldDropSkeletalMesh。
7. 原始参数和名称/类别/模型/icon来源保存到草稿 `UmbraItemImport.Source` 编辑器元数据，同时保留 Saved 下完整计划 JSON。`UmbraItemImport.State` 标明是展示草稿。原始数据不是运行时依赖，不需要新增 C++ 类型。

防具的 Hands 只是源资料中的建议部位，尚不写 AllowedSlots。草稿不填写 WeaponProfile、GenerationProfile、攻击力、防御力、重量、需求属性，不加入掉落表或装备库。需在 UE 中按 Umbra 规则进一步配置后才能作为正式游戏物品。现有 equipment_pipeline.py 要求战斗/生成配置，因此本桥接没有强行填入该工具必填项，也没有修改它。

本入口当前只接受显式 WorldDrop，用作世界掉落展示测试，防具保持独立 Skeleton。角色穿戴适配未实现，不将本次 WorldDrop 资源当成已适配穿戴模型。

## 保存与重跑

- icon/草稿保存源配方元数据。已有同配方资产只保留，不重写手工改动；配方不同或资产非本工具创建则报错，使用新 batch。
- 旧模型材质的创建保护继续生效。每次有独立 Saved/EquipmentImport/Items/<Batch>_<随机ID> 报告。
- report.json 汇总；plan.json 保留选中物品完整原始资料；meshes_report.json 记录材质/模型告警；editor.json 记录最终 UE 路径和创建/保留状态。
- 转换或 UE 出错可能留下已经成功保存的独立资产，不执行自动删除或回滚；修复后同配方重跑可复用。返回非零退出码表示不完整。
- 目前仍需要每件选择主 FLVER，后续可按已验证的规则生成选择清单；未支持全库无人工判断直接创建正式物品。

## 验证（2026-11-01）

成功报告：Saved/EquipmentImport/Items/ItemBridgeSmoke01_e635ca8a/report.json。

UE 输出 `/Game/EquipmentImport/Staging/ItemBridgeSmoke01`：14 个模型/材质/纹理/骨架包，2 个 UI icon，2 个 ItemDefinition 草稿，总计 18 包；.uasset 使用已有 Git LFS 规则。

草稿位置：

- Drafts/DA_Source_EquipParamWeapon_1000000
- Drafts/DA_Source_EquipParamProtector_100200

实测通过：

- 完整模型材质导入、材质槽绑定、真实 D3D12/SM6 编译，无材质编译错误；匕首 3 个、手套 4 个纹理采样器。
- 新 UE 进程只读回读：中文名、描述、分类、icon 配置与尺寸、模型引用、骨架、原始参数完整性；未自动填入游戏战斗/生成配置。验收结果在同目录 acceptance.json。
- 同选择、同规则重跑（ItemBridgeSmoke01_f5b578b0）：缓存复用，模型、icon、草稿均零保存；测试资产、Content/Items 和 Test01 合计 85 个包哈希未变。
- 27 项已有及新增 Python 测试通过；额外真实资料负向检查拒绝重复选择、非法主模型名、未实现的 Wearable 用途和缺资源记录。
- 已查看两张源 icon，内容分别为匕首和手套；这不代表全库 icon 视觉验收。

限制：匕首 `_n` 编码仍未确认，保持断开并报告；1140 法线/粗糙度使用显式候选规则，绿色方向和最终外观仍需视觉验收。未运行 PIE、碰撞/掉落物理测试、角色穿戴测试或全库导入。没有 C++ 修改，没有声称本次进行了 C++ 构建。UE 日志仅有既存的两项 Python 同名属性警告。
