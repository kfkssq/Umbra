# 开局六品质随机掉落测试器

`/Game/Items/Tests/BP_SixRarityStartup` 继承 `AUmbraLootTestSpawner`，放置在 `/Game/Maps/L_Prototype` 的 PlayerStart 附近，标签为 `Six Rarity Startup - Random Test Loot`。每次重新 Play/载入关卡，服务端仅生成一批六件：普通、魔法、稀有、史诗、传说、独特各一件。重生玩家不会重复生成。Shipping/Test 构建不执行该测试器。

## 调整

选中地图中的该 Actor，在 `Loot Test` 分类调整：

- `Enabled`：关闭即可停止开局生成，无需改正式掉落表。
- `One Of Each Rarity`：新夹具开启；已有测试夹具默认关闭，仍按原 Items 列表生成。
- `Randomize Seed Each Play`：开启时每次开局新种子；关闭后使用 `Generation Seed` 重现模板和词条抽取（实例 GUID 仍新建）。
- `Generation Context.Source Level`：默认10，仍由各正式 Profile 的等级偏移及上下限处理。
- `Spacing`：默认150厘米，沿 Actor 的局部右方向排列；移动/旋转此 Actor 可调整生成位置/方向。各点上下500厘米查静态地面，法线 Z 至少0.65，离地2厘米；无有效地面时该位置跳过并记警告。
- `Items`：配置脚本从 V2 正式掉落表提取去重模板。这里是各模板等概率的测试采样，不代表敌人的分类/类型掉落概率。正式表新增模板后可重新运行配置脚本。

## 生成边界

始终使用既有 `UmbraItemGeneration::Generate`，保留源稀有度限制、词条数量、互斥、等级和数值规则。保留每个品质第一次抽到的真实实例，集齐后按品质排序生成；不强改已生成品质、不克隆临时定义、不修改正式 Profile 或掉落概率。所有物品都使用正式 BP_WorldItem，可拾取并显示现有六档光效。

最多抽样4096次，避免缺少某档候选时卡住开局。当前正式模板池通过验证；如果将来禁用某档概率或删掉候选，未集齐时整批不生成并记录错误，不能凭空补造非法品质。

配置顺序为 C++ 默认 → BP 默认 → 地图实例覆盖。`Scripts/configure_six_rarity_spawner.py` 通过匹配 UE 编辑器创建/配置 BP 并放置唯一地图实例；重复运行不重复放置，不重置已有实例位置。

## 验证

- `Umbra.Loot.SixRarityStartup`：24个种子各产出六档、相同种子可重现、不同种子变化、空池拒绝。
- `Scripts/smoke_startup_loot_pie.py`：连续两次真实 PIE，检查持久化地图实例、每次六档各一、新随机模板/种子、光效与拾取。
- 本轮结果和未验证边界见 [Progress](Progress.md)。
# 当前临时展示覆盖（2026-11-06）

用户要求暂时关闭六件装备开局展示，改为 [19种锻造石固定品质展示](StoneGradesAndStartup.md)。原生成器保留，Enabled关闭；本文其余内容描述原展示。
