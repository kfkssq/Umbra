# 辉石魔砾：源资产提取与动画转换

当前状态（2026-11-07）：可释放的玩法适配版与原版三阶段声音已接入；**原FXR视觉还原、法阵接入及动作自然度修复尚未完成**。本文按时间保留各阶段记录，旧阶段结论不能作为当前验证。最新FXR审计见文末。

## 已核实的源数据

本地源目录 `C:/Users/kfkssq/Desktop/EldenRingAsset`；用户补充解包了 sfx/sound。原文件只读。来源版本以实际 regulation 内部版本 `11730034` 记录，不推断游戏营销版本；素材许可/再分发授权未核实。

前一轮读取的 Magic 4000：mp=7、refId1=10400000、refId2=10400099；castSfxId=523000、fireSfxId=523001、effectSfxId=510010。Bullet 10400000：atkId_Bullet=40000、sfxId_Bullet=523002、sfxId_Hit=523003、life=1.12、initVellocity=13、maxVellocity=25、accelInRange=40、accelTime=0.32、homingAngle=35。这些是源参数原值，尚未解释所有引擎语义/单位或映射到 Umbra，不把它称为纯直线弹。

Magic/Bullet 的初步读取快照位于 `Saved/SkillImport/GlintstonePebble/Magic.json` 和 `Bullet.json`，正式参数映射仍需版本/布局复核。提取器本轮不依赖这些临时 JSON 重新计算技能伤害。

## 具体资产与证据

- `c0000.anibnd.dcx` 内提取 `Skeleton.hkx`（150 骨）和 `a401.tae`（12 动作记录）。`c0000.chrbnd.dcx` 内提取原 `c0000.flver`。
- **实际 HKX 在 `c0000_a9x.anibnd.dcx`，并非 a4x**。已提取 `a9x.compendium`，以及 a401_045000/045010/045020/045030、045100/045110/045120/045130、145000/145010/145020/145030 共12段。各段是施法组候选，左右手/连发/骑乘等用途尚待事件和视觉核对，不只凭编号认定。
- 提取 FXR 与资源表：510010、523000、523001、523002、523003。清单引用的贴图/模型从基础 common 包提取；命中依赖 `s00002_m` 不在该包，从 `sfxbnd_commoneffects_dlc02.ffxbnd.dcx` 补齐。资源表闭包收齐不等于 FXR 行为、嵌套效果或音频依赖都已还原。
- 已产出25张 DDS、3个特效 FLVER；没有伪造 Niagara 或以现有技能特效冒充法环表现。模型/贴图与 FXR 的渲染映射待处理。
- TAE 当前 event bank=37；已安装模板无法直接适用。保留事件类型、起止秒数与原始参数字节，不强制套用其他 bank。源事件命名/释放/声音映射尚未验证。

所有产物位于 `Saved/SkillImport/GlintstonePebble`，属于本地生成数据，不提交。

## 可重复执行工具

[ExtractPebble.cs](../Scripts/SkillImport/ExtractPebble.cs) 使用本机 Smithbox 的 `Andre.SoulsFormats.dll` 与本机 Oodle，只读指定源包，输出选定资源、SHA256 和来源报告。已有同名中间资源内容不符即拒绝覆盖；JSON 报告可重新生成。工具是这个技能的专用提取器，不是通用技能管线。

以项目根目录运行，`$skillTools` 指向实际 Smithbox 目录：

```powershell
$skillTools = 'C:/Users/kfkssq/Desktop/EldenRingExtractionTools'
$skillBuild = Join-Path (Get-Location) 'Saved/SkillImport/ExtractorBuild'
dotnet build Scripts/SkillImport/ExtractPebble.csproj "-p:SmithboxRoot=$skillTools" "-p:BaseIntermediateOutputPath=$skillBuild/obj/" "-p:OutputPath=$skillBuild/bin/" -p:NuGetAudit=false
dotnet "$skillBuild/bin/ExtractPebble.dll" 'C:/Users/kfkssq/Desktop/EldenRingAsset' $skillTools "$((Get-Location).Path)/Saved/SkillImport/GlintstonePebble"
```

本轮另在 Saved 中隔离使用官方 [Soulstruct Blender 3.2.1](https://github.com/Grimrukh/soulstruct-blender/releases/tag/v3.2.1)，没有修改已安装插件。下载 zip 的 SHA256：`C6D012BA65E1406405AD70C353273FA7432D619A498D3175B45F1CDF98909B60`。扩展清单标识 GPL-3.0-or-later，随包保留许可；依赖各自许可保留。第三方源码/二进制均未加入项目提交。

`Saved/SkillImport/SoulstructDependencies` 是发行包自带 wheels 在本地隔离解包的依赖，需 Blender 兼容 Python 版本。本机 Blender 5.2.3 LTS 实测可读该样本。[export_pebble_animation.py](../Scripts/SkillImport/export_pebble_animation.py) 使用这些依赖，在原 HKX 骨架上重建动作、转换坐标并导出12份 FBX 和一个含动作的 blend；它不会重定向至 Greystone。30 fps 的采样时间按实际源 duration 分配。

```powershell
& 'C:/Program Files (x86)/Steam/steamapps/common/Blender/blender.exe' --background --factory-startup --python-exit-code 1 --python Scripts/SkillImport/export_pebble_animation.py
```

脚本逐帧验证 Blender 骨骼矩阵与源姿态计算一致。报告保留单独 root-motion 样本，不自动应用未知的根运动轨迹；本批12段均未包含 extractedMotion 样本。FBX 在 `Converted/`，此阶段只供后续导入/重定向使用。

## 源文件转换阶段验证与限制（重定向前）

- .NET 提取器编译及实际源包提取通过；读取原始包，不修改源文件。`extraction-report.json` 记录输入包 hash、输出资源及缺失列表。
- Blender 12段转换通过，每段150骨、42动画轨道；逐帧矩阵误差低于0.001。`animation-report.json` 记录各段帧数、时长和误差。
- 独立 Blender 进程重导12份 FBX，全部检查通过：单个 Armature、150骨、存在 Action、时长与源误差小于一帧。证据 `Converted/fbx-readback.json`、`fbx-readback.log`。该检查不等于 FBX 姿态逐帧往返验证，也不等于实际角色动作视觉验收。
- 保存 blend 时系统缩略图写入 `~/.thumbnails` 被拒绝，blend 本身保存成功；不宣称整份日志零错误。此前 WitchyBND 启动失败，改用本地库只读提取已成功。
- sound 导出仅7个文件，主要为索引、事件信息和两个小 PCK。实际游戏 `Game/sd` 档案仍需按需读取；不能称音频齐备。
- 本轮未运行 UmbraEditor C++ 构建、Automation、UE 导入/回读、PIE、声音试听、多人或 Cook。未更改 Content、角色/技能栏、GAS 或玩法代码。

## 2026-11-06 UE 动画导入与 Greystone 重定向样本

已通过 UE 5.8.2 编辑器创建并保存第一段候选 `a401_045010`（1.5 秒），当前有效候选位于：

- `/Game/Skills/Staging/GlintstonePebble/SourceV4`：`SK_ER_SourceBoneProxy`、自动生成的 Skeleton、`AS_ER_Cast_a401_045010`。
- `/Game/Skills/Staging/GlintstonePebble/RetargetV4`：源/目标 IK Rig、`RTG_ER_Pebble_Greystone`、`AS_Greystone_Pebble_Cast_a401_045010`。目标为现有 Greystone Skeleton，20 条显式重定向链覆盖躯干、四肢和手指。

`c0000.flver` 实际不含网格，因此源预览使用本项目生成的150骨诊断几何（900顶点/1200三角形），不是法环角色模型。Blender 中直接将骨骼与动画平移换算为厘米，统一 FBX 轴向；UE 导入倍率为1。原参考姿态与动画实际骨轴差异导致早期候选整体翻转，最终使用候选动作首帧作为源导入参考姿态，再对齐目标。这一选择是重定向配置，不代表原技能已逐帧还原。

`Source`、`SourceV2/V3` 和 `Retarget`、`RetargetV2/V3` 是失败/诊断候选，不能接入玩法；V2 存在倍率问题，V1/V3 的目标站立姿态检查失败。保留用于追踪，不作为最终资产。所有新二进制资产均由匹配编辑器生成，LFS 属性已检查。

执行工具依次为 [源预览导出](../Scripts/SkillImport/export_source_preview.py)、[UE 导入](../Scripts/SkillImport/import_pebble_source.py)、[重定向](../Scripts/SkillImport/retarget_pebble.py)、[独立回读](../Scripts/SkillImport/readback_pebble_retarget.py)、[PIE 预览](../Scripts/SkillImport/preview_pebble_pie.py)。先完成前节动画转换，再用 Blender 运行源预览导出。UE 前三个脚本通过 `UnrealEditor-Cmd.exe <项目> -run=pythonscript -script=<脚本绝对路径> -unattended -nop4 -nullrhi -nosound` 执行；PIE 脚本通过普通编辑器 `-ExecutePythonScript=<脚本绝对路径> -RenderOffscreen -unattended -nop4 -nosound` 执行。重定向脚本发现已有输出会拒绝覆盖；复核已有候选只需回读脚本。

本阶段验证：

- 原生导入/重定向完成；独立编辑器重新加载后，源和目标 Skeleton、1.5秒时长、动作变化及0/0.5/1/1.45秒的头部—骨盆—脚部高度关系检查通过（`readback-v4.log`）。源转换的12段逐帧矩阵误差仍低于0.001；新版 FBX 未重复前阶段的全部12段 Blender 往返检查。
- 独立启动带渲染 PIE，真实玩家 Mesh 播放并采样该动画，4张1280×720截图生成，`retarget-pie.json` 为 passed。已查看全部4张，整体站立、抬手与收手变化正常，没有早期整体翻转。截图保留既有剑盾和场景掉落特效；它们不是本技能导入内容。镜头未完整覆盖脚底，不据此宣称脚底贴地、手指或武器对位完全通过。
- 日志保留 Python 同名暴露、动画依赖加载和无曲线等告警，不宣称全日志零告警。此前预览截图 API 引发回调重入，改为先推进采样状态再请求截图，最终4样本通过。
- 未修改正式角色/地图、技能定义、技能栏、GAS或C++。本阶段未运行本任务的C++构建或Automation，未测试技能释放、碰撞、伤害、音效、多人或Cook。尚未制作 Montage/释放事件、法杖挂点、Niagara或声音资产，尚未与原游戏画面逐帧比较。

证据在 `Saved/SkillImport/GlintstonePebble/`：`pipeline-v4.log`、`ue-source-import.json`、`ue-retarget.json`、`ue-retarget-readback.json`、`retarget-pie.json`、`retarget-pose-0..3.png`。这些报告记录不同验收边界，导出成功不等于完整技能完成。

## 继续实施顺序

1. 核实12段动作的用途和 TAE 释放/连发/声音事件，明确首版采用的单发动作及前后摇。
2. 在已保存的 V4 重定向样本上完成原动作对照、脚底、手指与法杖挂点验收，确定 Montage 和释放时刻。
3. 依据原 FXR 和贴图/模型重建施法、弹体、命中效果，定位 sd 中的对应声音。原效果无法直接转换的部分逐项记录重建差异。
4. 接入现有 GAS 的服务器施法与投射物逻辑，验证费用、释放、碰撞、命中、取消和表现清理，再完成真实 PIE 验收。此次先完成单个参考技能，不同时开发变异系统。

## 2026-11-06 可释放版本（覆盖上述未接入状态）

当前已实现完整的单发技能玩法闭环，但属于 **Umbra 适配版**，不是原游戏的无损复刻。在 `L_Prototype` 点击 Play，鼠标指向目标方向，按主键盘 **5**；技能栏第七格显示辉石魔砾图标，消耗7资源。没有增加法杖装备要求：右手施法，保留当前剑盾。按移动命令可取消动作，已消耗资源不退还。

正式资产目录 `/Game/Skills/GlintstonePebble`。`DA_GlintstonePebble` 是本技能参数与表现引用的配置源，初始发放仍由 `BP_UmbraPlayerState.InitialSkillSlots[6]` 控制；`IMC_Default` 的 `IA_GlintstonePebble` → 数字5，角色输入绑定到 `Input.Skill.Seven`。保留原六技能和UI布局，仅补第七格的Designer预览。不要使用 Staging 的旧候选代替正式动画。

| 参数 | 当前值与来源 |
|---|---|
| 动画 | V4输出复制为 `A_PebbleCast`，`AM_PebbleCast` 使用现有 UpperBody 槽，时长1.5秒 |
| 释放 | StrikeTime=0.35秒、PlayRate=1；人工配置，尚未解码确认原TAE释放事件 |
| 消耗/冷却 | 7资源、1.8秒；7参考源mp，冷却为Umbra配置。现有 `umbra.Skills.IgnoreCooldowns` 为最终原型开关，本轮未改变用户原来的默认值 |
| 飞行 | 初速1300、上限2500 cm/s；0.32秒后以4000 cm/s²加速；寿命1.12秒，碰撞半径10cm |
| 瞄准/追踪 | 输入时朝鼠标地面方向；释放时在约12°锥体内选择最近可攻击且可见目标，按35°/秒有限转向。源参数单位/语义未全部核实，这是明确的UE配置 |
| 伤害 | 现有 ExplicitChannels/GAS链：Cold通道24基础值＋0.7 AbilityPower，允许暴击和既有伤害桶；没有新增一套伤害公式。Cold映射是本项目适配，并非声称原游戏魔力伤害等于寒冷伤害 |

执行流程：沿用 `UUmbraSweepAbility` 的费用、冷却、动作锁和取消机制，新增 `Projectile` 执行分支。到释放时刻由服务器生成 `AUmbraSkillProjectile`；投射物以最多1/120秒的子步扫掠 WorldStatic/WorldDynamic/Pawn，忽略施法者，首次碰撞就锁定命中并通过 `UmbraDamage::Apply` 结算一次。角色中心到发射点另做扫掠，防止发射点跨墙。正常完成或取消都清理施法粒子；弹体寿命独立，已发射后取消动作仍可命中。命中状态与位置复制给客户端，表现不参与伤害判定。

### 表现与资源来源

- 已提取的25张DDS使用隔离 Pillow 13.0.0 解码成PNG；正式材质使用原 `s10011_a` 透明辉光与 `s34050_a` 能量纹理。转换脚本为 `convert_pebble_textures.py`；没有改源DDS。依赖位于Saved，不加入源码或Content。
- `P_Pebble_Cast/Flight/Impact/Release` 是通过匹配UE编辑器创建的 **Cascade粒子重建**，不是 Niagara，也不是FXR转换。施法辉光附着 `hand_r`；飞行包含亮核、世界空间拖尾与光点；命中包含闪光、能量云与散射光点。未导入原特效FLVER网格、完整FXR曲线或子效果。
- `S_Pebble_Cast/Release/Impact` 是 `create_pebble_audio.py` 可重复生成的自制PCM短音效（0.32/0.22/0.48秒），不是法环原声。原sd音频定位仍未完成。声音随施法、释放、命中触发；已发出的短音效在取消时自然结束。
- 图标来自用户本地 `技能/menu_knowledge_06000out.obj.png`；本地EquipParamGoods 4000及名称表对应辉石魔砾，SHA256为 `942b860643ad4d43374b3cb3fe09e6221fa8975631177dd32187655464fa71de`。贴图/动画沿用前述法环来源与授权未核实状态。
- `configure_pebble_skill.py` 创建材质、粒子、声音、动画Montage、数据资产和输入/HUD引用；C++编辑器工具 `ConfigurePebbleParticles` 负责可编辑粒子模块。它会重建本技能的粒子设置，不用于覆盖其他特效。

### 本阶段验证

- `UmbraEditor Win64 Development` 构建通过，日志 `skill-build.log`；未变更模块依赖/Target。首次编译发现指针推导与向量类型错误，修正后通过。
- `Umbra.Skills.CooldownView`、`GreystoneExpansion`、`PebbleDefinition`、`SweepSector` 共4项Automation通过，报告 `Automation/` 与 `skill-automation.log`。
- 独立带渲染、启用音频的PIE，通过真实 Enhanced Input 注入测试6条路径，`skill-pie.json` 为passed：正常命中一次24伤害；墙体阻挡0伤害；释放前取消无弹体/0伤害；释放后取消已发弹体仍命中一次24伤害；空放超时清理；资源不足不激活/不扣费。成功施法均只扣7，重复输入不重复扣费，冷却结束恢复可用，结束后无弹体或活动施法粒子残留。测试角色AP为0，24伤害不是所有角色固定最终伤害。
- 单独慢速PIE补查视觉，`skill-visual-pie.json` 为passed；慢速只作用于临时PIE世界，不改正式参数。已查看 `skill-cast.png`、`skill-flight.png`、`skill-impact.png` 及 `Saved/Screenshots/WindowsEditor/ScreenShot00080.png`：可见蓝色飞行亮核/拖尾、命中闪光，第七格图标、键5、消耗7、真实冷却与资源93/100。场景紫色掉落光柱不是本技能。
- 未执行实体键盘输入或人工声音试听；验证声音资产已导入且播放路径接线，不声称听感还原。未测试多人、丢包、Cook、移动目标追踪或全套项目Automation；也未完成原游戏逐帧视觉/音频一致性验收。启动阶段仍有既有 `LogAutomationTest: Condition failed` 信息；上述专用测试与回归测试通过，不宣称整份日志零错误。
- 所有新uasset由UE5.8.2编辑器保存，LFS属性已确认。未保存测试场景、未提交或推送，保留工作区原有改动。

后续保真工作是原FXR材质/曲线与网格映射、法杖挂点以及原动作对照；这些与当前“能释放并造成伤害”的玩法闭环分开验收。尚未扩展多弹、反弹、穿透等变异。原sd声音定位已由下面的阶段完成。

## 2026-11-07：替换原版发射、飞行、命中音效

现有VFX只用了 `s10011_a`、`s34050_a` 两张源纹理，通过自建UE材质和Cascade发射器实现亮核、拖尾、闪光。并没有把原FXR中的层级、网格、发射规律、颜色/缩放曲线、UV动画、混合模式与各层时序逐项映射。因此此前效果只能算玩法原型的近似表现，差异大不能归因于贴图导入；本轮没有更改这部分，也没有完成原版视觉验收。

声音来自用户本机原版游戏基础 `sd/sd.bdt`，银行路径 `/enus/cs_main.bnk`。解包索引8650条全部有路径映射；仅提取所需bank。FXR f000523001–3的action75分别引用523001–3；对应Wwise事件和HIRC容器解析如下，不按文件名猜声音。

| 阶段 | 原事件 | WEM | 默认事件增益 |
|---|---|---|---|
| 发射 | Play_s000523001 | 524617251 | -3 dB |
| 飞行 | Play_s000523002 | 1040010023 | -15 dB |
| 命中 | Play_s000523003 | 82601330 / 164447618 / 123415567 | -3 dB |

使用官方wwiser v20260808解析事件、vgmstream r2168解码WEM。无合成、变调或重采样；导入UE后用PCM避免再次有损编码。飞行源44100Hz，[0,18687)为循环区间，裁成约0.423741秒的独立循环SoundWave；完整解码与原WEM留在Saved。发射约2秒，命中变体约1.94–2.01秒。

DA中 `CastSound=None`，`StrikeSound=S_ER_Pebble_Release_524617251`，`ProjectileSound=S_ER_Pebble_Flight_1040010023`，`ImpactSound=SC_ER_Pebble_Impact`。三种命中波形均接入Cue；旧合成资产不删除，但技能不引用。已知原施法事件523000包含前奏及局部循环，其启动/停止时序尚未核实，本轮不冒充已接入。重新运行bootstrap会保留原声配置。

`FlightAudio`属于投射物，跟随根组件移动；BeginPlay启动，命中与EndPlay停止，专用服务器不播放。原版事件增益存于SoundWave.Volume；Cue及播放调用不再额外降低命中音量。`ATT_Pebble`空间化开启，球形内半径400cm、衰减距离2400cm，是UE适配参数而非原游戏推导值。UE命中随机袋一轮不重复，与原容器仅避开上次的调度不完全相同；也未移植原游戏总线、环境混响及动态RTPC。

可复现工具及步骤见 [OriginalAudioExtractor](../Scripts/SkillImport/OriginalAudioExtractor/README.md)。本地证据在 `Saved/SkillImport/GlintstonePebble/OriginalAudio/manifest.json`（bank/WEM/WAV SHA256、数据偏移、采样区间与解码元数据）、`events-original/`（事件链）、`configuration.json`（UE引用回读）。资源仅来自用户本机，不因转换而改变原版权或获得再分发许可。

本轮验证：UmbraEditor Win64 Development构建通过；五项Umbra.Skills Automation通过，包括新增PebbleOriginalAudio（实际重新加载资产验证波形ID、循环长度、事件音量、三种命中节点）。启用音频的实际Enhanced Input PIE六路径通过：命中39帧飞行/90帧停止、墙体9/90、释放后取消39/90、空放134帧飞行后弹体清理；释放前取消与资源不足均无弹体。正常命中一次24测试伤害、扣费7。常规测试不截图，以免截图卡帧跨过短飞行阶段。导入时曾遇到Cue图引脚数量断言，补建随机节点引脚后重新导入与测试通过。

未验证：人工试听、原游戏同步A/B、多人复制及丢包、Cook、全项目Automation、原版视觉一致性。空放清理通过Actor销毁及EndPlay停止实现，没有做扬声器录音或音频设备输出泄漏测量。启动日志仍有历史Condition failed，不声明日志整体零错误。

## 2026-11-07：FXR结构审计与动作诊断（不是完整重建交付）

用户明确要求源结构、材质和时序还原，不接受继续用近似效果替代。本阶段没有新建替代粒子或用通用特效填补未知节点。现有cast/flight/impact仍是旧近似版，保持待替换状态；本阶段唯一正式表现修正是移除没有源依据的独立释放闪光，原版发射音保留。

隔离下载并固定 `@cccode/fxr 32.1.3`，官方来源 [EvenTorset/fxr](https://github.com/EvenTorset/fxr)，发行包在Saved，未修改全局Node环境。语义分析工具 [analyze_pebble_fxr.mjs](../Scripts/SkillImport/analyze_pebble_fxr.mjs) 显式指定EldenRing读取，保留完整曲线/修饰器/未知字段。每份源FXR经过解析→内存重新编码→再解析，语义对象深比较通过；这仅证明当前解码器往返一致，不证明其逆向语义或UE渲染正确。

| 源FXR | 节点配置数 | 主要内容 |
|---|---:|---|
| 510010 | 4 | 两层MultiTextureBillboardEx法阵＋点光；两层引用s60130纹章 |
| 523000 | 17 | 施法光、Billboard、PointSprite、Tracer、多纹理层；0.5秒后引用440 |
| 523001 | 1 | NodeSound 523001；appearance为空，没有独立可见发射器 |
| 523002 | 19 | 飞行网格/RichModel、轨迹、精灵、多纹理层、Distortion、点光 |
| 523003 | 11 | 命中网格、QuadLine、精灵、多纹理层、点光；另引用430 |
| 430 / 440 | 各1 | WindForce风场；不是法阵 |

提取器补齐430、440，七份文件的嵌套引用已遍历闭合，资源表对应DDS/FLVER均存在。源文件SHA256、资源SHA256、节点路径、曲线类型及随机/ExternalValue修饰器保存在 `Saved/SkillImport/GlintstonePebble/FxrAudit/manifest.json`；逐FXR完整语义保存在同目录。报告 `uePromotionAllowed=false` 是当前未通过UE视觉转换验收的明确状态，不是已实现的通用发布系统。

法阵源纹章为 `Textures/s60130_a.png`，已实际查看。510010两个层使用不同混合模式、UV扰动/滚动、颜色/亮度/alpha阈值曲线，不能简化为贴图淡入淡出后声称还原。510010父节点duration=1秒，其主纹章粒子duration=0.6秒；这些是FXR局部时间，**不是已确认的技能全局施法时间**。523000中0.5秒延迟的引用属于风场，也不能用来断言法阵在0.5秒出现。

当前0.35秒释放是历史人工适配值；a401_045000长约0.833秒，a401_045010长1.5秒，但不能直接拼接两段时长当作真实施法总时长。实际切换受TAE/行为状态及施法速度等影响，现有TAE bank37尚无验证通过的事件布局。源法杖dummy-poly挂点与当前hand_r也不是等价变换。缺少这部分证据时不修改正式施法时间，不冒充原版。

动作诊断 [audit_pebble_pose.py](../Scripts/SkillImport/audit_pebble_pose.py) 已通过UE5.8.2独立加载执行，保存源/目标7个时刻的骨骼变换和retarget pose偏移到 `pose-diagnostic.json`。现有V4使用施法首帧作为源参考姿态并自动对齐，左右手臂offset显著不同；这本身也可能反映单手施法姿态，**不单凭Euler角或左右不对称断言根因**。旧独立单动画截图已可见低头/倾斜，故不能只归咎于游戏中UpperBody混合；仍需分离参考姿态、颈/头链、锁骨/手臂及实际AnimBP混合/武器姿势逐项对照。旧高度关系测试不包含朝向、左肩或姿态自然度，不能当作视觉验收。本阶段没有宣称修复歪头或左半身。

正式资产变更通过UE编辑器执行 [apply_pebble_fxr_verified_fixes.py](../Scripts/SkillImport/apply_pebble_fxr_verified_fixes.py)：DA.StrikeEffect设为空，StrikeSound仍为原WEM524617251；bootstrap同步该规则，防止下次覆盖回自制闪光。修正记录 `FxrAudit/verified-fixes.json`。未删旧粒子资产、未改变伤害/输入/原声音源。

下一步完整还原的验收顺序：源动作/挂点/时序核实 → 法阵两层材质与曲线 → 飞行网格与轨迹 → 命中层次 → 固定曝光/相机/速度的多帧对照。没有验证的渲染基元、材质公式和外部参数必须保留为未实现，不能静默降级为普通精灵。当前审计工具不是FXR到Niagara转换器。

长期方向建议：把辉石魔砾保真转换保留为有边界的研究样本，正式技能优先同一角色/同一骨架或兼容骨架的UE动画包，加可编辑Niagara。UE原生并不自动保证自然：参考姿态、根运动、武器握持、左右手IK、UpperBody混合仍需检查；Niagara也要统一比例、亮度、时序与性能。建议先以同一角色做一个原生施法样本，与法环版本按修改耗时、动画自然度、技能可读性和扩展难度对照，避免直接批量重定向法环全套动作。

技能规则继续由C++/GAS负责；Montage/动画通知与DA定义施法、释放、飞行、命中、取消等表现接口，Niagara接收颜色/大小/强度/轨迹等参数。多弹、反弹、穿透由投射物逻辑决定，每次生成/碰撞驱动表现，不能用粒子数代替伤害判定。官方依据：[IK Retargeting](https://dev.epicgames.com/documentation/unreal-engine/ik-rig-animation-retargeting-in-unreal-engine)、[Retargeting Operation Stack](https://dev.epicgames.com/documentation/unreal-engine/retargeting-operation-stack-in-unreal-engine-5-8)、[Niagara模块参考](https://dev.epicgames.com/documentation/en-us/unreal-engine/system-and-emitter-module-reference-for-niagara-effects-in-unreal-engine)。这是开发路线建议，本阶段没有擅自更换动画素材或扩大技能系统。
