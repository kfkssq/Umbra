# 装备批量导入（2026-11-02）

当前目标是可重复的基础外观与物品资料导入，不是重建法环全部着色器。模型、icon必须存在且精确关联；没有证据的特殊材质继续隔离。资产全部进入独立Staging批次，源资源、现有装备定义和玩法配置不覆盖。

## 最简操作

### 一次启动全部剩余装备

关闭UE，在项目根目录运行：

```powershell
& .\Scripts\EquipmentImport\ImportAllEquipment.ps1 -Apply
```

[总队列脚本](../Scripts/EquipmentImport/import_all.py)默认使用固定队列`Equipment_All_001`。本机已Dry Run冻结857件，排除第一批已正式尝试的30件（含6件材质失败），分成29个正式批次、143组UE任务；第二批旧的单批Dry Run没有执行，不影响本队列。不要同时再运行旧单批命令。

- 自动选取模型/icon对应明确且名称匹配的装备，同一模型只选一个代表记录。默认30件一个Library批次，每个UE进程最多6件，全部顺序处理。
- 一组导入后另开UE进程回读成功项；只做结构、参数与引用检查，不逐件生成截图，不修改玩法配置。使用现有转换缓存、材质保护与来源门槛。
- 普通材质失败记录后跳过，不自动重复尝试。工具启动、预检或回读故障停止队列，避免把环境故障冒充材质不支持。所有资源受支持不作保证。
- Ctrl+C或关机后，确认残留UE进程已退出，再运行**同一条命令**续跑。已经记录成功或材质失败的项目跳过；未完成项在原批次恢复。完整跑完后再次执行不会重导入。
- 保留`Saved/EquipmentImport/Queues/Equipment_All_001/`，这是续跑记录。运行期间不要改脚本、规则、CSV、文本和配置；哈希不一致会拒绝续跑。`-Queue`只用于有意创建新的清单，不用于恢复旧任务。
- 默认不带`-Apply`仅生成/检查计划，不调用UE。`-BatchSize`、`-ChunkSize`仅在首次生成该队列时生效。

正式资源写入`/Game/EquipmentImport/Library/Equipment_All_001_001`至后续编号目录，不依赖Staging。

报告位于`Saved/EquipmentImport/Queues/Equipment_All_001/`：`summary.md`简表、`summary.json`状态和保存数、`failures.json`失败明细、`Retries/`按原批次划分的失败清单。`Attempts/`包含每组原始导入和回读结果；顶层日志保存子进程输出。中断后再次运行会先读取子进程检查点恢复最新进度。退出码0表示全部成功，1表示队列处理完但有资源失败，2表示故障停止，130表示手动中断。

本次验证：51项Python回归通过，包含模拟子进程在部分成功后中断、恢复只处理未完成项、完成后零启动，以及引擎故障与材质失败区分；正式PowerShell入口Dry Run确认857件/29批/143组。尚未通过总队列执行857件真实导入；现有单批UE实测证据见进度文档。

### 单批入口

正式批次使用`-Destination Library`，保存到`/Game/EquipmentImport/Library/<Batch>`，可纳入Git/LFS；默认Staging继续作为忽略的测试区。保存位置写入selection.json，之后复跑该清单无需重复指定。Library中的定义仍为展示草稿，不自动启用战斗或投放配置。

```powershell
# 正式批次：先计划，确认报告后对其selection.json加-Apply执行。
& .\Scripts\EquipmentImport\ImportEquipment.ps1 -Batch Equipment_001 -Destination Library -Limit 30 -ChunkSize 6
```

后续批次可用`-ExcludeArchives '<JSON路径>'`排除此前已尝试的模型；文件内容是archive名称数组，例如`["wp_a_0100","am_m_1010"]`。已失败项单独通过原批次retry.json重试，避免混入每个新批次。

本机已配置parts、CSV、中文文本、icon、Blender、WitchyBND、UE和共享材质索引。关闭UE，在项目根目录执行：

```powershell
# 先看计划；无UE资产写入，输出selection.json固定本次物品身份。
& .\Scripts\EquipmentImport\ImportEquipment.ps1 -Batch Equipment_001 -Limit 30

# 使用上一步报告目录里的selection.json，一次执行完整流程。
& .\Scripts\EquipmentImport\ImportEquipment.ps1 -Selection '<报告目录>\selection.json' -Apply

# 修正规则/补齐资源后，仅重试失败项。
& .\Scripts\EquipmentImport\ImportEquipment.ps1 -Selection '<报告目录>\retry.json' -Apply
```

也可直接 `ImportEquipment.ps1 -Batch Equipment_001 -Limit 30 -Apply`。默认每次UE进程处理10件，`-ChunkSize 6`可降低峰值加载量。类别轮询选样，同一模型只选一个源记录；未入选的共享模型、资源门槛失败项和超过数量限制的记录保留在not_selected.json。不是把所有强化/变体参数行当成独立模型。

重复导入应使用固定selection.json或相同Batch。每次省略Batch且重新自动选样会创建新批次。报告包含成功/保留/仅资源/失败项；失败不会阻止其余分批执行，总体退出码仍为非零。空retry清单正常退出。规则改变后，如果目标已存在旧配方或部分资产，工具会保留它们并拒绝覆盖；使用`-Batch Equipment_002`在新目录验证，不能把规则升级当成授权重写手工材质。

## 实际数据流

1. CSV与中文FMG资料建立物品身份、原始类别、模型编号、icon编号；精确检查所有文件。
2. DCX复制到Saved缓存，WitchyBND解包；common_body.tpf.dcx只转换一次，按完整纹理名共享使用。
3. FLVER明确纹理路径优先，空路径查询同名MATBIN定义。绝不按相近编号猜贴图。定义同名冲突直接隔离。
4. Blender保留RGBA并验证DDS→PNG通道误差；导出主POSITION/UV通道和原始权重。布料POSITION1不再被错误追加成无权重顶点，额外语义记录在conversion.json。
5. UE创建基础PBR材质实例：主BaseColor、主RG法线、已有参数预设；支持源AlphaRef.x/255裁切阈值及双面设置。共享/细节法线不再被误当作主法线冲突。
6. 物品桥接默认把**同一主FLVER内、用于WorldDrop的静态部件**合成一个Static Mesh，保留各材质槽。不同FLVER、不同装备部位及Skeletal Mesh不合并。原始FBX仍保留；selection可设`combine_world_drop_components:false`保留多个UE静态部件，此时不猜选主部件，不创建单Mesh草稿。
7. icon使用UI纹理设置。已有Umbra分类可映射时创建展示草稿，名称/描述/原始参数带来源；无对应分类只导入资源并保留source_plan/plan.json，不扩大项目枚举，不设置战斗数值、随机生成配置、穿戴槽或物理。

## 材质定义初始化

仅parts DCX并不包含所有材质默认引用。实测皮革手套引用`AM_M_1010_Gauntlet_a`，绳索还存在没有主Albedo、仅靠常量/细节层的材质。通用编号规则不能完整恢复这些关系。

本机已从安装游戏只读导出共享材质定义，建立20377条索引，7条同名冲突被隔离。后续常规导入读取缓存，不反复扫描游戏。若游戏版本变化或换机器，可执行：

```powershell
& .\Scripts\EquipmentImport\BuildMaterialIndex.ps1 -GameRoot '<法环Game目录>' -SmithboxRoot '<现有解包工具目录>'
```

这个一次性工具复用已安装Smithbox的Andre/SoulsFormats库和游戏自带解压DLL，使用本机.NET 10 SDK；不安装UE/Blender新插件，不改游戏。生成文件全部在Saved，保留来源包和MATBIN哈希，最后更新Saved/EquipmentImport/dcx_tools.json的material_index。没有共享索引时仍可用原保守模板路径，但覆盖范围较小。

## 已知边界

- 当前支持明确列出的DetailBlend、DetailBlend S2、AMSN基础着色器族。Fur、Crystal、Rich、Glow等未实现的着色器仍阻断；不能通过套metal或忽略Alpha冒充支持。
- `_m`、法线B/A、细节混合、常量/细节层独立材质、颜色混合与动态效果尚未复原。未分类但已支持着色器使用明确标记的neutral预览参数，不能视为已确定原材质类型。
- AlphaRef的第一分量按基础裁切阈值转换，第二分量/原游戏动态处理未实现；仍需检查剪裁边缘。支持基础裁切不等于复原整套布料着色器。
- 物品草稿不是可直接投放游戏的完整装备。未验证角色适配、世界掉落物理和PIE；WorldDrop与Wearable目录及用途保持区分。
- 当前CSV/文本/parts来源版本仍需用户资料确认；材质索引有文件哈希，但不自动宣称与所有历史源文件版本一致。

## 维护与验证

### Git管理范围

`Content/EquipmentImport/Staging/`及旧管线的`Content/Items/Tests/PipelineM5/`仅作为本机导入测试区，已加入.gitignore。正式采用的资源应通过UE编辑器迁移到项目正式资产目录并检查引用后再提交，不能让正式装备依赖仅本机存在的Staging资产。

`Saved/`中的解包缓存、材质索引、报告、预览和本机工具配置继续忽略。`Scripts/EquipmentPipeline/Source/`与`samples.json`由`generate_samples.py`重建，不提交生成副本；生成器与PROVENANCE记录保留。MaterialIndexTool的bin/obj亦忽略。

当前`item_catalog.local.example.json`实际包含本机绝对路径，虽名为example也按本机配置忽略；另支持忽略EquipmentImport目录的`*.local.json`。换机器需重新提供CSV、文本、parts、icon等路径配置。管线脚本、通用规则、类别映射、测试代码和可复用测试清单仍需提交。此次检查上述忽略目录没有已跟踪文件，无需移除Git索引；忽略不会删除本地文件。

入口：[ImportEquipment.ps1](../Scripts/EquipmentImport/ImportEquipment.ps1)、[bulk_import.py](../Scripts/EquipmentImport/bulk_import.py)。
规则：[matbin_rules.py](../Scripts/EquipmentImport/matbin_rules.py)、[normal_rules.py](../Scripts/EquipmentImport/normal_rules.py)、[flver_layout.py](../Scripts/EquipmentImport/flver_layout.py)。
回读：[validate_bulk.py](../Scripts/EquipmentImport/validate_bulk.py)、[verify_bulk_editor.py](../Scripts/EquipmentImport/verify_bulk_editor.py)。

外部接口依据：[Smithbox BinderArchive](https://github.com/vawser/Smithbox/blob/main/src/Andre/Andre.Formats/BinderArchive.cs)、[Smithbox FLVER材质默认纹理查询](https://github.com/vawser/Smithbox/blob/main/src/Smithbox.Program/Renderer/Resource%20Types/FlverResource.cs)。复用本机二进制接口，没有把第三方库或游戏材质数据加入仓库。

历史基线见[18件首次测试](EquipmentBatchCoverage18.md)。

## 本轮实测结果

最终批次为`/Game/EquipmentImport/Staging/BatchReady42_03`，使用UE 5.8.2执行42件跨类型样本，每进程6件。它是覆盖性选样，不代表全库随机抽样成功率。

| 项目 | 结果 |
| --- | --- |
| 模型、材质、icon与源资料导入 | 33件成功，保存311个资产包 |
| 可映射现有Umbra分类 | 23份ItemDefinition展示草稿 |
| 尚无现有分类映射 | 10件仅资源，保留原始类别及参数，不擅自创建玩法类型 |
| 异常隔离 | 9件；其余成功项正常完成，整体报告保留失败状态 |
| 新UE进程回读 | 33件通过：模型/材质槽/纹理设置/参数/icon/草稿引用 |
| 代表预览 | 7件、14张高清图，预览进程正常退出 |
| 成功项原批次复跑 | 23份草稿保留、10件仅资源，失败0、保存0 |
| 文件保护 | 6938个既有资产包、42件源模型/icon、311个本批资产包哈希无变化 |
| Python回归 | 最终源码44项通过 |
| 材质索引工具 | 正式PowerShell入口构建通过，0警告/0错误；重建索引哈希一致 |

隔离项：辉石杖、诺克斯流体剑（Crystal）；凯丹头盔/铠甲（Fur）；鳞甲（Rich）；黑王大剑（Glow）；调香师长袍、皮革长裤、铁头盔（没有主Albedo的常量/细节材质）。这些不会自动套用错误材质放行，修正对应规则后使用retry.json增量重试。

本机证据：

- [逐项导入结果](../Saved/EquipmentImport/Bulk/BatchReady42_03_b7f8b0fc/report.md)
- [新进程回读](../Saved/EquipmentImport/Bulk/BatchReady42_03_b7f8b0fc/verification.json)
- [预览记录](../Saved/EquipmentImport/Bulk/BatchReady42_03_b7f8b0fc/PreviewReview/acceptance.json)
- [复跑报告](../Saved/EquipmentImport/Bulk/Bulk_d74e70ec/report.json)
- [保护检查](../Saved/EquipmentImport/Bulk/BatchReady42_03_b7f8b0fc/preservation.json)

Saved证据仅在本机保留，不提交仓库。预览确认了基础外观、部件与贴图可呈现；部分金属偏暗、箭羽高光偏亮，现有粗糙度/金属预设及细节层仍不是原游戏还原验收。未运行PIE、掉落物理、穿戴骨架适配或Unreal C++构建。预览日志在Python启动前仍出现13条`LogAutomationTest: Error: Condition failed`及既有Python名称警告，未在本任务定位，因此不能声称整个项目日志或玩法自动化测试干净。
