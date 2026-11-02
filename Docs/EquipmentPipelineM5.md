# M5 装备资产清单管线

2026-10-30。入口为 [equipment_pipeline.py](../Scripts/equipment_pipeline.py)，可执行清单契约是 `load_manifest`，示例为 [samples.json](../Scripts/EquipmentPipeline/samples.json)。目前交付静态掉落样品生产与受控更新；不自动认定盔甲可穿戴，不改变正式掉落表。

## 输入与边界

- schema_version=1，manifest_id 标识批次所有者，output_root 必须位于 `/Game/Items/Tests/` 或 `/Game/Items/Staging/` 的独立子目录。每项有唯一稳定 id、asset_path、fields、managed_fields、locked_fields、mesh、icon、provenance，可选 fit_physics。
- mesh.kind 必须显式 static/skeletal。新静态资源支持 OBJ/FBX，骨骼资源仅 FBX 且必须指定已有 Skeleton；不自动创建/替换骨架，不导入动画或 PhysicsAsset。materials 是槽位名到已有 MaterialInterface 包路径的映射；自动材质/贴图导入关闭。
- mesh/icon 的 source_file 相对清单目录，禁止越界，sha256 必须与源文件一致。已有引用可省略 source_file；带源文件的新导入仅允许落在 output_root。重复导入必须与资产内保存的导入配方相同；源文件或导入设置变更需使用新目标路径，禁止覆盖重导入。
- provenance 必须包含 author/license/record/status。record 指向可读的相对来源记录，status 仅允许 project_authored 或 verified。第三方授权仍需人工确认，工具只检查记录齐备，无法验证法律授权真伪。
- 装备必须有分类、合法槽位、GenerationProfile；武器需 MainHand 和 WeaponProfile。引用类型、生成规则使用 UE 原生验证器检查。图标必填，PNG/TGA 导入 Texture2D UI 组，保留透明度。
- fields 可配置分类、槽位、品质、等级、属性需求/四维加成、护甲、抗性、重量、武器/生成配置、显示文本、掉落变换等。白名单见脚本 FIELDS。主数值来自清单；被锁字段保留编辑器实际值；运行时仍按 Definition→实例生成→GAS 既有链路计算。
- fit_physics 复用 `TryFitWorldDropPhysics`，单位厘米。失败返回 false，保留代理字段；不保存失败定义。自动拟合不能与手填/锁定物理字段共存。模型无法自动通过尺寸/中心限制时由人工修源，不偷偷缩放。

## 执行

以仓库根目录启动 PowerShell。`$ueRoot` 应设为 Umbra.uproject 关联的本机 UE 根目录；本轮验证为 `C:\Program Files\Epic Games\UE_5.8`（5.8.2）。

```powershell
$project = (Resolve-Path .\Umbra.uproject).Path
$pipeline = (Resolve-Path .\Scripts\equipment_pipeline.py).Path
& "$ueRoot\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" $project -run=pythonscript "-script=$pipeline" -unattended -nop4 -nullrhi -nosound
```

默认读取 samples.json，**默认 dry-run**，预检所有选中行，不创建、改动或保存目标资产。输出 `Saved/EquipmentPipeline/report.json`；模型导入及实际拟合结果只能在 apply 后验证。脚本路径必须绝对路径，UE 的进程目录不是项目目录。

参数：`-UmbraManifest="绝对清单路径"`，`-UmbraPipelineReport="绝对报告路径"`，`-UmbraPipelineIds=PipelineM5.Sword01,PipelineM5.Chest01`，`-UmbraPipelineMode=CreateMissingOnly|UpdateManagedFields`。确认报告后加 `-UmbraPipelineApply` 执行。任何一行预检失败，全批不写入。

- CreateMissingOnly 是默认模式：只创建缺失定义，已有定义检查类型/稳定 ID 并报告差异，保留所有已有字段。
- UpdateManagedFields 比较期望值、当前值和资产包内 `UmbraPipeline.Baseline`。只有当前值等于旧基线的字段才允许自动更新；人工改动冲突阻止全批。locked_fields 保留手填值且不推进这些字段的基线，解锁后仍检测冲突。无匹配所有者基线的定义不可接管。
- 每个导入包和定义包分别保存；定义与其新基线同包保存。失败后重跑复用已完成包，无跨包事务/自动回滚。若在编辑器交互进程中遇到保存前失败，该进程可能留有未保存对象，应退出且不保存失败对象后重跑 commandlet。外部 JSON 仅报告，不作为恢复状态。
- 无字段/拟合变更时不刷新元数据、不保存。报告包括配方 hash、动作、差异/冲突、错误、人工核对项、已保存包和用时。生产者应保存每次报告供追溯。

## 样品与验证入口

[generate_samples.py](../Scripts/EquipmentPipeline/generate_samples.py) 生成 5 把方块剑和 5 件刚性胸甲的 OBJ、128×128 透明 PNG、清单和哈希。图标是同一几何的固定正交投影。来源见 [PROVENANCE](../Scripts/EquipmentPipeline/PROVENANCE.md)，无第三方几何。它只允许创建缺失文件或复用内容完全相同的文件，不覆盖手改源。

30 个正式清单测试包位于 `/Game/Items/Tests/PipelineM5`（10定义+10模型+10图标）；独立验收另建一个 BP_LootTest_PipelineM5。样品复用已存在的测试武器逻辑与 RandomMVP 生成池，不能据此推断那些外部资源已完成来源核验。模型使用默认材质，未提供正式 UV/材质贴图/LOD/骨架权重。

- 独立进程执行 [verify_samples.py](../Scripts/EquipmentPipeline/verify_samples.py)：重载、两种模式重复运行零保存、包 hash 不变、原生验证、物理参数和注册表依赖审计。临时手改重量验证冲突保护，再按单ID修改显示名并通过管线恢复，最后检查零保存。产生 audit.json 和各操作报告，然后按缺失创建测试生成器。仅对本管线测试样品运行此验收脚本。
- [smoke_random_mvp_pie.py](../Scripts/smoke_random_mvp_pie.py) 用 `-UmbraPipelineM5` 切到这10件；在 L_Prototype 临时放置生成器，固定单机 PIE、结束还原播放设置，不保存地图。报告 `Saved/EquipmentM5PIESmoke.json`。
- 标准库测试：UE 内置 python.exe `-B -m unittest discover -s Scripts/tests -p test_equipment_pipeline.py -v`。覆盖 dry-run、零写入、手改冲突、锁、选择 ID、错误引用预检、源哈希/来源/路径、提交后中断恢复。模拟后端测试不替代 UE 导入验收。
- [blender_export.py](../Scripts/EquipmentPipeline/blender_export.py) 是可选批导出入口：显式 source_blend/collection/output_fbx/report；拒绝未应用缩放和已有输出，不修改源 blend；报告 UV、材质、绑定和单位。未安装 Blender，实际执行未验证。网格方向、权重、UV 和抓握位置仍需人工核对。

## 加载治理与验收限制

预检只加载选中清单的定义/直接引用，Registry 审计只查询这些包；未增加运行时全库扫描或 LoadSynchronous 循环。保留既有软网格引用、硬武器/生成配置引用。本批数据不足以证明应迁移所有引用为异步或接入全库 AssetManager，因此没有做未经测量的运行时重构。

audit.json 的计时是**预检之后的暖加载+验证**，不是冷启动、帧耗时或打包性能。最初30包合计218682字节（未含生成器/传递依赖），独立验证进程峰值物理内存1734MB来自 commandlet 日志，包含整个编辑器基线，不能作为装备本身内存。

Cook 验证命令：`UnrealEditor-Cmd.exe <project> -run=Cook -TargetPlatform=Windows -CookDir="<项目>/Content/Items/Tests/PipelineM5" -NoDefaultMaps -NoGameAlwaysCook -unattended -nop4 -nullrhi -nosound`。测试目录只在显式 CookDir 中包含，未新增正式打包目录规则。Cook 会重建 Saved/Cooked 缓存；其依赖闭包仍可能包含被引用的公共资产。Cook 结果与本轮实际测试状态见 [Progress](Progress.md)。Cook 成功也不代表已跑打包客户端。

原生注册表导出命令为同一编辑器执行 `-run=AssetRegistryDump -Input="<项目>/Saved/Cooked/Windows/Umbra/Metadata/DevelopmentAssetRegistry.bin" -OutDir="<项目>/Saved/EquipmentPipeline/CookRegistryFirst"`。然后运行 [verify_cook.py](../Scripts/EquipmentPipeline/verify_cook.py) 的 `--dump <OutDir>/AssetRegistry --report <报告.json>`，检查31个样品包及直接配置引用有非零包数据；只在依赖列表出现名称不足以判定成功。本轮验证通过，Cook共614包成功、7包平台跳过，334.41秒；最终编辑器验收包计数31、244531字节。

人工未验收：图标实际UI可读性、网格视感/法线/正式材质、鼠标拾取与音效、骨骼FBX及材质映射分支、Blender、打包独立客户端冷加载、万级资产规模。这里的 chest 仅证明掉落和数值装备闭环，不能用于 M6 穿戴外观验收。
