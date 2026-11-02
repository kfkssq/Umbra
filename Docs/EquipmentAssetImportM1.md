# Equipment Asset Import — M1

2026-11-01 后续：[DCX文件夹入口](EquipmentDcxImport.md) 已按用户追加要求接通WitchyBND和Blender，包含DDS→PNG和FLVER→FBX。下文M1边界与历史验收仍指FBX/PNG输入阶段；当前DCX能力、限制和命令以新文档为准。

## 审计与实施规划（2026-10-30，实施前）

现有 `Scripts/equipment_pipeline.py` 是 ItemDefinition 作者工具；它已有来源记录、增量保护、显式槽映射和 Fit Physics 接口，但关闭自动材质导入，骨骼导入要求已有 Skeleton。不更改其契约。`Scripts/EquipmentPipeline/blender_export.py` 支持单 blend 集合导出，不支持文件夹批处理。`create_armor_1730.py` 提供四件防具路径；`fit_drop_physics.py` 会扫描并保存定义，不能直接作为本次导入步骤调用。

本轮通过关联的 UE 5.8.2 Python commandlet 只读检查：Head/head_01、Body/body、Arm/arm、Legs/legs 均为 SkeletalMesh、各有独立 Skeleton；longsword/SM_longsword 与 MorningStar/MorningStar 为 StaticMesh。六件都有可读原始 FBX。长剑槽为 `材质`，其余为 `Material_0`。胸甲源目录实际为 bd_m_1731。头部有多套纹理，不能猜测分配。原材质均 Opaque；NullRHI 的 used-textures 查询为空，**不表示原材质没有纹理节点**；实际接线和视感待编辑器确认。原资源来自既有外部素材，保留原来源/许可边界，不复制进脚本目录。

### M1 架构

独立 `Scripts/EquipmentImport/`，不改 C++、ItemDefinition、原模型/材质。

1. 输入：FBX+PNG 文件夹约定或 manifest；可选 Blender 预导出。
2. 纯 Python：验证配置、路径、重名、PNG 头、法线声明，计算输入 SHA256，生成逐件计划。
3. UE 原生 Python：AssetImportTask/FbxFactory（禁用 Interchange FBX，固定旧 FBX importer）、TextureFactory、MaterialEditingLibrary。顺序处理，每件结束释放 Python 引用并 GC，不一次加载全库。
4. 输出仅 `/Game/EquipmentImport/Staging/<批次>/WorldDrop|Wearable/<id>`；不合并 Static Mesh 节点，不跨装备合并 Skeletal Mesh；每个 FBX 应属于一件装备/一个骨架族。
5. 三个共用 Master：Opaque、Masked（BaseColor alpha→OpacityMask）、Cloth（M1 为高粗糙度 DefaultLit 布料基础，不含专用织物 BSDF）。实例按配置接 BaseColor/标准 DX Normal，Metallic/Roughness 来自预设再由显式值覆盖。
6. 默认 Dry Run；Apply 仅创建缺失包。配方元数据证明来源，同配方已存在对象只检查/复用、不改手填。输入改变要求新 id/批次。失败逐项报告；成功包保留，可修复失败项后重试。无跨包事务；失败进程中的未保存对象不能手动 Save All。

### 阶段边界与扩展

本次仅 M1。DDS 转换、像素级特殊法线识别归 M2；现阶段 DDS/`_m`/未知编码报告并跳过。Icon、Definition、WorldDropMesh/WorldDropSkeletalMesh 与 Fit Physics 后续消费成功资源清单，不在导入器内修改游戏数据。随机装备沿用现有生成系统；Wearable 独立目录不代表已经完成骨骼适配。Special、专用 Cloth 着色、穿戴适配、生产规模性能验收均后续实施。

API 依据：[Epic MaterialEditingLibrary](https://dev.epicgames.com/documentation/en-us/unreal-engine/python-api/class/MaterialEditingLibrary)。实际行为以本机 UE 5.8.2 验证为准。

## 最简操作

已有 FBX + PNG 时无需 Blender。首次将 [示例清单](../Scripts/EquipmentImport/manifest.example.json) 复制到自己的源目录，填写 `source_root`、模型和槽映射。以下命令从项目根目录运行：

```powershell
# 默认 Dry Run：写报告，不保存 Content。
.\Scripts\EquipmentImport\Run.ps1 -Manifest 'D:\EquipmentSource\manifest.json'
# 保存并关闭交互编辑器后实际导入。
.\Scripts\EquipmentImport\Run.ps1 -Manifest 'D:\EquipmentSource\manifest.json' -Apply
# 失败项修复后重试，多个 ID 用逗号分隔。
.\Scripts\EquipmentImport\Run.ps1 -Manifest 'D:\EquipmentSource\manifest.json' -Apply -Ids Sword01,Armor01
```

脚本从 Epic Launcher 安装记录解析 `.uproject` 的引擎关联；自定义安装用 `-EngineRoot`，仍检查主次版本。仅本次进程启用 PythonScriptPlugin/EditorScriptingUtilities，不改 uproject。报告和引擎日志默认位于 `Saved/EquipmentImport/<时间戳>.json[.log]`，含失败 ID、输入哈希、拟执行动作、已保存包和引擎告警；退出非零表示有失败。报告中的 `ok` 表示自动步骤完成，不表示视觉/权重验收通过。

### 文件夹一键批处理

目录约定：`<源目录>/<WorldDrop或Wearable>/<static或skeletal>/<装备ID>/`。每件放一个 FBX 和 `<精确材质槽名>_a.png`、可选 `<精确材质槽名>_n.png`。大小写/名称按实际 FBX 槽名匹配；同目录多个 FBX 会产生 ID 冲突，不会强行合并。

```powershell
.\Scripts\EquipmentImport\ImportFolder.ps1 -SourceRoot 'D:\EquipmentSource' -Batch Batch01
.\Scripts\EquipmentImport\ImportFolder.ps1 -SourceRoot 'D:\EquipmentSource' -Batch Batch01 -Apply
```

扫描器生成清单到 Saved，再调用同一入口。非约定路径、DDS、blend、未知 PNG 在 `scan_warnings` 中列出；这些文件没有被导入。特殊项目可以在装备目录放 `item.json` 覆盖扫描行的 `materials` 等字段。覆盖顺序：扫描默认 → item.json；材质数值：preset → 显式 metallic/roughness。`materials` 整体替换，不做模糊合并。

- `kind` 必填 static/skeletal；不根据扩展名猜测骨骼。
- `usage` 必填 WorldDrop/Wearable，二者隔离。Wearable 不等于已适配角色。
- `scale` 默认1，无量纲；FBX 单位转换到 UE cm，不自动纠正未知比例/朝向。
- `preset`：metal=(1,.35)、leather=(0,.65)、cloth=(0,.85)，依次为 Metallic、Roughness，允许0–1显式覆盖。默认 Opaque/leather；Cloth 默认 cloth。
- `normal_encoding=standard_dx` 才连接 PNG 法线，sRGB=false、TC_NORMALMAP、Green不翻转。未确认编码的法线报告后不连接，不尝试修复法环特殊编码。
- BaseColor：sRGB=true、TC_DEFAULT；PNG存在 alpha/tRNS 时必须声明 `alpha_policy=ignore|mask`。`mask` 必须搭配 `surface=Masked`，使用 BaseColor Alpha，双面开启。这里只检测通道存在，未进行透明像素/特殊法线语义分析。
- `provenance` 记录原来源和使用范围；工具检查填写，不验证授权真实性。保留外部原始资源/许可文件。
- `icon_asset` 可保留已有图标路径，仅透传到清单，不加载、不创建、不匹配或修改 Icon。

### Blender 可选预导出

使用 [批导出脚本](../Scripts/EquipmentImport/blender_batch.py) 和 [示例 jobs](../Scripts/EquipmentImport/blender_jobs.example.json)：

```text
blender --background --python-exit-code 1 --python <绝对路径>/blender_batch.py -- <绝对路径>/jobs.json
```

输入为 blend/FBX，**不直接支持 partsbnd/dcx/FLVER**，仍需外部模型插件先读取并保存 blend。按文件或指定 collection 输出，保留 Mesh 分离和 Armature，不改源文件、不保存 blend、不覆盖已有输出。缺失材质、未应用缩放、多骨架族直接报错；材质槽按 `material_names` 显式重命名，不重新排序面索引。不烘焙动画/修改器，不自动填补缺失权重。当前机器未发现 Blender 安装，脚本未实际验收。

## 保护与恢复边界

每个导入包写入来源配方元数据；输出存在且配方一致时只读复用，手调材质参数/绑定不重置。无归属元数据、错误类型或配方改变会阻止该项；更换批次/id后重新导入。保留原资产，不自动重导入。不会创建 ItemDefinition、调用 Fit Physics、保存地图或触碰装备运行逻辑。

每件按纹理→MI→网格保存；失败后已经成功保存的纹理/MI可在修正槽映射后复用。网格保存前检查全部槽映射、材质引用和骨架。多输出包不是事务；若进程在多个网格包保存中途终止，组完整性检查阻止静默成功，需检查失败目录并选择新id/批次，不能宣称任意中断都可自动恢复。不要在失败交互进程里 Save All。当前脚本为 commandlet 使用设计。

原 FBX 导入时的槽名可能与既有 UE 人工配置不同。实际本轮四件防具为 `None`，晨星锤为 `WorldGridMaterial`，长剑为 `材质`，都用显式映射处理；不会把“只有一个材质”作为自动绑定理由。

## 本轮验证（2026-10-31）

- 标准库测试13项通过：路径越界、保护目录、ID冲突、缺失/损坏贴图、DDS拒绝、alpha策略、普通/未知法线、数值边界、哈希变更和扫描槽映射。
- UE 5.8.2 实际导入四件1730防具+长剑+晨星锤成功：`/Game/EquipmentImport/Staging/M1Validated`，23包（6网格、4独立骨架、6纹理、6实例、1 Master）。证据：`Saved/EquipmentImport/validated_apply.json` 及同名 `.log`。
- 三种材质探针使用明确生成的RGB/Alpha/标准法线PNG和长剑FBX：`M1Probes` 共15包。Opaque、Masked、Cloth基础分支实际执行成功，`probes_apply.json`。
- 独立 UE 进程 [verify_editor.py](../Scripts/EquipmentImport/verify_editor.py) 重载并回读 BaseColor、Normal、sRGB、压缩、父材质、粗糙度；六样品+三探针重复运行零保存；相关包SHA256不变；MI内存手改被保留且不保存；配置变更被拒绝。报告 `Saved/EquipmentImport/acceptance.json`。
- 初次试验暴露旧UE槽名与FBX槽名不同，以及SkeletalMaterial数组迭代复制问题，已修正并在新批次完整复验。`M1Samples` 是早期诊断输出，含失败/未正确绑定样品，**不要作为最终资产使用**；保留现场，没有删除用户文件。最终验收以 `M1Validated` 和 `M1Probes` 为准。
- 已检查新 `.uasset` 命中 Git LFS 规则；未提交/推送。

**未验证/已知异常：** 使用NullRHI，不代表GPU着色编译、真实渲染或PIE视觉验收。原始FBX有缺失平滑组及绑定姿势中缺骨骼警告，需要DCC/编辑器检查权重、朝向、比例和法线。1730贴图映射是明确的基础测试映射，不是原材质完整还原；头部树枝/毛发拆槽、透明和特殊法线仍待确认。Cloth只是DefaultLit高粗糙度基础实例，不含专用织物BSDF。未进行Blender实跑、多Mesh复杂FBX/多槽真实素材、角色穿戴、打包客户端、百件规模或C++重新编译；本次没有C++/模块变更。

人工最小验收：打开最终样品的Mesh Editor检查各槽引用及纹理；查看四件骨骼参考姿势和模型尺寸；在临时关卡分别摆放六个模型与Masked探针检查光照、透明和背面，不保存正式地图。确认后再将资产路径接入现有Definition工具；该接入不在本次M1执行范围。
