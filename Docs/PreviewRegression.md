# Equipment Preview 蓝灰色回归调查（2026-09-29）

本调查将“静止”和“实时 RT 蓝灰”分开。以下以源码、保存资产读取、实际运行日志为证据；未验证项目明确列出。不根据 Z=-100000 推断根因，不改 RT、UI 材质、相机或正常工作的属性/槽位。

## 版本证据与边界

- Git HEAD 为 `12fc338`（2026-09-21，英雄动态头像HUD）。Equipment、CharacterMenu、CharacterPreview 的相关 C++ 和全身预览资产尚未提交；这些路径的 Git 历史为空。因此目前无法从 Git 指认“最后全身预览正常版本”。HEAD 不是该基线。
- `Saved/EquipmentBuildValidation-20260928` 是重构前源码参考副本，目录名称不是版本保证：部分文件在 9 月 29 日更新过。其渲染正常与否没有验收证据，不能替代用户所述最后正常画面。
- 对照该副本，当前 `UmbraEquipmentMenu.h/.cpp`、`UmbraCharacterMenu.h/.cpp`、`UmbraCharacterPreviewComponent.h/.cpp`、`UmbraPlayerController.cpp` 全部 SHA256 相同。与 `Saved/CharacterMenuStatsValidation-20260929` 对照亦相同。结果保存在 `Saved/PreviewRegressionHashes.json`。
- UI 目录实际不同的只有 `UmbraCharacterStatsPanel.h/.cpp`、`UmbraStatEntry.h/.cpp`、`Equipment/UmbraEquipmentSlotWidget.h/.cpp`。ControllerDebug 的不同为 `OnRep_PlayerState` 递归通知 HUD/CharacterMenu 中属性面板更新上下文，没有新增 Preview 调用或 Actor 材质/可见性修改。
- 当前 Git 工作区还有其它未提交变更。`git diff HEAD` 混合了多次开发，不能将所有差异归为最近 StatEntry 重构；未提交 `.uasset` 的修改者及修改时点也不能由文件名推断。
- 找到 Preview 自动保存：9 月 29 日 10:52 `BP_CharacterFullBodyPreview_Auto3.uasset`、17:54 `..._Auto1.uasset`；不覆盖原资产，保存时间本身不能证明画面正常。

## 已确认的静止问题

`Saved/PreviewStoppedInspection.json` 记录 17:32 根菜单保存的父类是 `UserWidget`。Controller 的 `ToggleCharacterMenu` 只有转换为 `UUmbraCharacterMenu` 成功才调用 `SetMenuOpen(true)`，故该配置下自动预览链路被跳过。17:39 报告记录父类已改为 `UmbraCharacterMenu`，用户确认 Idle 恢复。这支持静止根因为根菜单父类不匹配；不能用此解释剩余蓝灰色。

## Reparent 后恢复执行的实际链路

`ToggleCharacterMenu → SetMenuOpen(true) → RefreshEquipmentPages/VisitWidget → Equipment.SetPageActive(true) → CreatePreview`：

1. Spawn 已配置 `BP_CharacterFullBodyPreview`；获取现有 Mesh/Capture。
2. 停止 Capture 自身每帧更新，初始化现有 PreviewComponent。
3. `InitializePreview` 保留蓝图镜头/Idle/专用 RT（Blueprint configuration 模式），但仍设置 ShowOnly 模式、动画刷新、物理状态。
4. `SetSourceCharacter → RefreshAppearance` 把玩家 SkeletalMesh 复制到 Preview，清空材质覆盖，再逐槽复制玩家材质；恢复预览 Idle。
5. `RefreshEquipmentVisuals → PreparePreviewPrimitives` 设置仅 Capture 可见、无碰撞/阴影，重建 ShowOnly。
6. UI 创建 MID 指向同一专用 RT；`SetPageActive` 再刷新来源并激活 Preview，恢复灯光、Mesh Tick，按现有定时器捕获。

以上步骤不是 StatEntry 重构新增，但根菜单父类错误时没有由 Controller 启动。尤其“Use Blueprint Configuration”不表示 C++ 完全不覆盖 Mesh 材质或 Capture 控制状态。

## 诊断方法与状态

所有临时 C++ 仅位于 `Saved/PreviewRegression-20260929`，生产 `Source` 未修改。诊断副本通过 UE 5.8.2 的真实游戏启动路径打开原关卡、原 Widget、原 Preview Actor；使用真实 RHI 离屏渲染，不使用 NullRHI 判定颜色。

- Mode 0：原逻辑，只加阶段日志与 RT RGB 导出。
- Mode 1（A）：只跳过 `EmptyOverrideMaterials` 和逐槽 `SetMaterial`，其余自动 Spawn、Mesh/动画、Capture 流程保留。
- Mode 2（B，仅 A 未恢复时运行）：在 A 基础上再跳过 C++ 对 Capture 的启停、模式、ShowOnly 与更新控制，保留蓝图保存的 Capture 设置。仍使用现有 Actor 和 RT。
- 导出 PNG 为 RT RGB 的可读副本，输出 alpha 强制不透明用于观察原始颜色；不改 RT 资产或 UI 材质，不代表最终 UI 合成效果。

## 实际结果

诊断副本构建成功（UE 5.8.2 / Win64 Development）。三次独立离屏游戏运行均成功结束、找到一个有效 Preview、导出实际 RT；不是推测或仅检查缩略图。

| 运行 | 材质覆盖数 | ShowOnlyComponents | 每帧捕获 | 结果 |
| --- | --- | --- | --- | --- |
| 原流程 Mode 0 | 17 | 3 | false，原定时器捕获 | 蓝灰剪影 |
| A / Mode 1 | 0 | 3 | false，原定时器捕获 | 仍蓝灰 |
| B / Mode 2 | 0 | 1（蓝图运行后列表） | true（蓝图配置） | 仍蓝灰 |

三者 Mesh 均为 Greystone、17 个实际材质槽；原流程 Preview 与玩家每个 `GetMaterial(Index)` 路径相同，未复制纯色替代材质或临时 MID。Preview 的 AnimationMode=SingleNode、AnimAsset=Idle、AnimClass=None。CaptureSource=SceneColorHDR、目标为原 RT_CharacterFullBody，Lighting=1、Materials=1；最终 KeyLight/FillLight 可见且强度均3000。完整逐槽名称及 OverrideMaterials 已写入各日志。

证据位于 `Saved/PreviewRegression-20260929/Saved/`：`Baseline.log`、`ExperimentA.log`、`ExperimentB.log`，对应 `PreviewMode0.png`、`PreviewMode1.png`、`PreviewMode2.png`。A 排除了“仅取消运行时材质复制即可恢复”的假设，B 排除了“仅取消这段 C++ Capture 覆盖即可恢复”的假设。不能据此排除所有场景、组件或蓝图运行时问题。

资产保存的 PrimitiveRenderMode=RenderScenePrimitives；但是 `SpawnActor` 返回、C++ Capture 修改之前日志已为 UseShowOnlyList，列表1项。B 保留的也是此状态。说明 Details 中的选项受蓝图/组件初始化影响，之前手改模式不能代表最终运行值。已读取蓝图节点清单，但 Python 未能读取节点函数/引脚，未将具体节点归因为罪魁祸首。

## 新找到的实际历史变化

`Saved/Logs/Umbra-backup-2026.09.29-06.30.50.log:5287` 明确记录 UTC 05:57:58（北京时间 **13:57:58**）删除 `BP_CharacterFullBodyPreview_C` 关卡实例。13:18、13:33、13:46 三份地图自动保存都包含它，根组件保存位置均为 `(-222,-643,0)`；14:01 自动保存与当前地图中均没有它。

`Saved/Logs/Umbra-backup-2026.09.29-09.43.50.log:2072` 明确记录北京时间 **17:37:02**，WBP_CharacterMenu 从 UserWidget 改为 UmbraCharacterMenu。当前实际自动 Spawn 的 Preview 位置为 `(0,0,-100000)`。因此“关卡放置实例 → 删除实例 → 修正父类后菜单自动 Spawn”是有记录的链路变化，不能只检查 StatEntry 是否直接调用 Preview。

用户再次确认最后正常画面发生在 PIE 打开装备菜单后，不能将用户正常状态当作仅缩略图正常。现存历史记录仍不足以证明上述关卡实例就是最后正常时的 RT 写入者，也不足以证明所述 Z=-100000 正常状态对应哪个实际实例。地图只读加载未注册组件时 `GetActorLocation` 返回0，所以本报告使用根组件保存的 RelativeLocation；历史地图引用当前蓝图资产，也不是完整历史工程快照。

早期 Preview 蓝图自动保存（10:52）仍引用 RT_HeroPortrait、头像镜头，无法作为“同一全身 RT 最后正常配置”的基线。17:54 保存和当前资产的 Greystone/Idle/材质覆盖一致；当前的原始 PrimitiveRenderMode 发生过修改。检查的地图雾参数未发现差异，不能据此确认或排除渲染机制。完整记录在 `Saved/PreviewHistoryInspection.json`。

## 结论、未验证项与代码处置

- 静止根因有父类记录、调用链和用户恢复反馈支持；删除关卡实例也解释了为什么此前错误父类可能被一个独立 RT 写入者掩盖，但该因果连接尚缺最后正常时的运行日志。
- 蓝灰色具体根因、责任行、最小正式修复 **尚未确认**。现有证据不支持修改 `RefreshAppearance` 的材质复制，也不支持删除 C++ Capture 初始化作为正式修复。
- 正式 `Source`、Config、Content 本轮均未修改；没有回滚、保存资产或改相机/FOV。实验代码仅存在被 Git 忽略的 Saved 诊断副本，未合入正式系统。
- 本轮修改文档：本报告、BlueprintCapturePreview、Progress。需要的后续证据是最后正常时完整工程/实际 RT 写入实例，或针对已确认实例来源变化的进一步受控验证；不能凭未提交源码或某份自动保存宣称找到了最后正常 Git 版本。
- 验证范围：完整编译、三次真实 RHI 游戏 RT 导出、只读资产/历史地图、Git 与源码哈希对比。未完成原编辑器交互式 PIE 修复验收；根因未确认，不存在已验证修复。运行日志另含旧 Greystone 输入映射、动画除零、GameplayCue 路径等警告；没有将整体环境标成无警告。

## 重现已完成的 A/B

只使用独立诊断副本，不把诊断 Source 复制回正式工程。停止另一个诊断进程后，在 PowerShell 运行（`$mode` 为0、1或2）：

```powershell
$mode = 0
$diag = 'C:/Users/kfkssq/Documents/Unreal Projects/Umbra/Saved/PreviewRegression-20260929'
& 'C:/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe' "$diag/Umbra.uproject" /Game/Maps/L_Prototype -game -RenderOffscreen -unattended -nosplash -nosound -windowed -ResX=800 -ResY=600 '-ExecCmds=umbra.PreviewRegression' "-UmbraPreviewExperiment=$mode" "-abslog=$diag/Saved/RepeatMode$mode.log"
```

进程进入游戏后自动打开菜单、记录 Spawn/同步后/最终状态、导出 `Saved/PreviewMode<mode>.png` 并退出。该命令会覆盖同模式导出图；原始三次实验日志另有独立名称。诊断副本 Content 指向原项目，命令不执行资产保存。
