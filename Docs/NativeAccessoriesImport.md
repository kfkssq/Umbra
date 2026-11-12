# 原生护符与锻造石

**当前材料规则（2026-11-06）**：锻造石品质按等级固定，不再随机；开局展示改为19种各一件。下文的材料随机品质与单件开局展示为历史记录，已由 [固定品质与19种展示](StoneGradesAndStartup.md) 替代；护符随机规则保持。

2026-11-06 名称清理与失色石六档掉落光效核查见 [专项记录](ItemNameAndSomberVFX.md)。

2026-11-04。用户确认本批只引进护符与锻造石，随机品质沿用现有 SourceRarity 限制和晋升规则。栏位布局继续由用户配置；左侧头胸手腿、右侧双护符与双戒指/遗物，本批不新增戒指/遗物素材。

## 来源与导入结果

- 来源目录 `C:/Users/kfkssq/Desktop/EldenRingAsset`：EquipParamAccessory.csv、EquipParamGoods.csv 与 msg/itemtext_chinese.json。原图位于配置文件指定的本地法环图标目录。
- Accessory 157行中：116件名称/图标精确匹配，39件缺图标排除，2件无有效名称占位排除。缺图列表保存在 `Saved/NativeAccessories/plan.json`，不猜图、不借用其他物品图标。
- Goods 只允许19个显式锻造石ID：普通[1–8]及古龙岩、失色[1–9]及失色古龙岩。排除铃珠、铃兰及其他消耗品。
- 新增135个独立透明图标及135份物品定义，路径 `/Game/Items/NativeAccessoriesV1`。表名+行ID形成稳定身份，保留原始参数、中文名称/描述、文件SHA256和来源信息。源文件没有嵌入可核验游戏版本，因此标记版本未确认。继续遵守 [资源来源规则](EditorSetup.md)，本地提取素材不推定发行许可。
- WorldDropMesh 共用 Engine BasicShapes/Sphere，缩放0.12（约12cm直径），按模型包围盒配置掉落物理尺寸。它是可替换的通用占位物，不是法环独立模型或角色佩戴展示。

## 游戏规则与调参位置

| 参数 | 配置资产/规则 |
|---|---|
| 敌人掉落表 | BP_Enemy_Melee_01 的 LootTable；L_Prototype 同类实例覆盖同步到 DA_Loot_Prototype |
| 分类权重 | DA_Loot_Prototype：武器32、防具48、护符10、材料10；武器:防具保留2:3，新增两类暂各10%，不是法环原版掉率 |
| 模板权重 | 护符/材料各一个类型组，组内每个定义权重1；武器/防具继续引用原V2类型和模板 |
| 原版 rarity | 各 ItemDefinition.SourceRarity，直接来自对应CSV；护符106件源2、9件源3、1件源1；锻造石17件源1、2件源3 |
| 品质概率 | DA_Generation_Accessory / DA_Generation_Material，复制现有 V2 Armor 的品质权重、颜色、背景、边框和25%晋升概率；规则版本7 |
| 护符词条 | DA_Pool_Amulet：从现有防具公共/防御词条梯度复制，限制为Accessory；原V2词条资产不修改 |
| 材料 | bMaterialRarityOnly=true，复用品质选择，不抽词条、不装备、不产生高品质战斗特效预留文案 |

品质范围：源0普通/魔法/稀有（50:30:15归一化）；源1普通/魔法/稀有/史诗（50/30/15/5）；源2史诗75%、传奇25%；源3传奇/独特各50%。锻造石的具体种类/编号不因随机品质改变，当前品质用于实例和表现，不代表已实现强化收益差异。

配置覆盖顺序：导入JSON定义初始资源与分类权重；导入时读取当前V2 Armor配置生成独立Profile；运行时敌人实例覆盖高于蓝图默认，掉落表选择模板，模板SourceRarity限制Profile品质候选，生成结果写入物品实例。已有实例保留品质与词条快照，后续修改Profile不重掷。新Profile与V2是导入时复制关系，之后不会自动同步。

最新规则：护符纳入遗物，允许Amulet、Amulet2、Ring1、Ring2全部四栏，见[四遗物栏与布局](RelicSlots.md)。同一实例不可占多个栏；不同实例可同时装备，未实现原作accessoryGroup的同族互斥。原作专属护符效果未实现，不将原始描述展示为已生效技能；实际增益由Umbra现有随机词条/GAS提供。后续已按用户要求实现[锻造石堆叠](SmithingStoneStacks.md)：同种、同品质合并并显示数量；尚不实现拆分、消耗或强化。

## 可重复执行流程

### 开局试玩掉落

L_Prototype 的 PlayerStart 附近额外放置 `BP_StartupAmulet` 和 `BP_StartupSmithingStone`（`/Game/Items/Tests`）。每次PIE各从116护符/19种锻造石中等概率抽取一件，随后走正常SourceRarity品质生成；等级为1，便于初始角色直接装备护符。它们各生成一个地面掉落物，拾取后进入背包；保留原有六品质装备测试生成器。

Blueprint/关卡实例中的 Items 决定候选池，bRandomSingleItem控制只抽一件，bRandomizeSeedEachPlay控制每次重抽，GenerationContext.SourceLevel决定物品等级；实例覆盖优先于BP默认。六品质模式优先于单件模式，旧生成器默认行为不变。这是现有开发测试生成器的扩展，Shipping/Test构建不生成开局测试物品。

`Scripts/configure_native_startup.py`通过UE保存两个BP和地图，重复运行不增加重复生成器；`Scripts/smoke_native_startup_pie.py`从保存地图启动两次PIE，验证每类恰好一件、品质门槛、等级、VFX、拾取快照及种子更新。两次随机结果允许出现同一种物品或同品质。

### 批量导入

1. 修改 `Scripts/EquipmentImport/native_accessories.json` 的本地来源路径。用UE自带Python运行 `native_accessory_catalog.py`，生成 `Saved/NativeAccessories/plan.json`；检查counts、excluded与哈希。
2. 先构建匹配版本UmbraEditor。通过该编辑器 `-run=pythonscript -script=<绝对路径>/import_native_accessories.py` 导入，首次可设 `UMBRA_NATIVE_LIMIT=12`（两类各6件）；全量设0。
3. 独立进程设 `UMBRA_NATIVE_MODE=readback` 运行同一脚本，逐件回读保存结果，零保存。脚本遇到来源变化、配方变化或手工改动的已存在输出时拒绝覆盖；需审核差异后迁移，不应删除用户调参绕过检查。
4. 运行 `Umbra.Items.Generation` 自动化。然后设置 `UMBRA_RANDOM_CONFIG=<绝对路径>/native_accessories.json`、`UMBRA_RANDOM_ACTIVATE=1`，通过编辑器执行 `activate_random_equipment.py`，将已验证表绑定到蓝图和原型关卡敌人。默认不设ACTIVATE时只审计。
5. 用真实编辑器 `-ExecutePythonScript=<绝对路径>/Scripts/smoke_native_accessories_pie.py` 验证保存后的敌人绑定、实际死亡/掉落/拾取、两个护符同时装备/卸下、材料拒绝装备且保留背包。测试不保存临时敌人或测试表。

所有uasset/umap均由UE编辑器保存，使用既有Git LFS规则。报告位于Saved（不提交）；脚本、配置和文档可复核。当前实测结果与未验证项见 [Progress](Progress.md)。
