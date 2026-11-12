## 2026-11-12 可控制Manny与原生冲击弹实验场

- 已保存`/Game/SkillLab/Maps/L_MannySkillLab`，地图级GameMode绑定独立BP_MannyLab/PlayerState；WASD/方向键移动、鼠标方向+数字5释放第一格技能，三个静止Quinn靶子和阻挡墙供PIE操作。新增原生动画实例提供移动状态，编辑器创建可编辑ABP的待机/跑步混合及全身Slot，原生MM_ChargedAttack制作Montage。正式Greystone、项目默认GameMode及辉石魔砾配置未替换。入口与参数见[路线图](SkillImportRoadmap.md)。
- DA_MannyArcBolt连接四阶段Niagara：右手施法、0.55秒释放、飞行、命中；7资源、1.8秒冷却、24点Cold伤害沿用GAS。实验PlayerState资源回复10/秒，伤害无暴击/AP系数；当前使用引擎测试模板，无配置音效，是可玩技术样本，非最终法术动作/特效美术。
- 本轮UE5.8.2 UmbraEditor Win64 Development构建成功，Rider工程文件刷新成功，Umbra.Skills六项Automation成功（`NativeBaseline/PlayableAutomation/index.json`）。最终带渲染PIE通过真实Enhanced Input注入移动与技能：移动约276.5cm、回到待机、头脚垂直差151.9cm；六条命中/撞墙/前后取消/空放/资源不足路径成功，观察实际Montage、施法Niagara、飞行启停、一次24伤害/一次7费用与短效清理。报告`Saved/SkillImport/NativeBaseline/manny-pie.json`。
- 独立编辑器回读地图、角色/动画/技能引用、三个靶子、WASD/数字5映射与正式资产隔离成功（`manny-audit.json`）。慢速带渲染PIE及三阶段截图通过（`manny-visual.json`、`manny-*.png`）；发现并修正Python位置式Rotator参数造成的初始朝向问题，新增明确Pitch/Yaw/Roll与站立高度检查，已查看修正后角色和施法画面。
- 未进行实体键盘/鼠标人工操作、完整动作美术验收、多人、Cook或性能压测；无NavMesh场景的左键寻路不在验收范围。存在历史启动Condition failed，不声明整份日志零错误。相关资产LFS属性与文本空白检查通过；未提交/推送，保留其他工作区改动。

## 2026-11-12 UE原生角色与Niagara工程基准（前一阶段）

- 用户选择Manny／Quinn兼容角色、免费素材优先。通过匹配UE5.8.2编辑器导入128份Characters模板资产（97段动画序列，不是97段施法动作），建立隔离检查地图和两份引擎Niagara测试系统。来源、分工、入口及后续八类样本见 [路线图](SkillImportRoadmap.md)。用户报告Niagara Examples Pack与5.8兼容提示，该外部包尚未导入/验证，改为可选项。
- C++增加施法/释放/飞行/命中四阶段Niagara配置，禁止同阶段与Cascade双配；沿用GAS伤害/资源/冷却，补齐取消、命中和结束清理。释放与命中资产必须自行有限结束；不自动判定任意Niagara图的生命周期。
- 本轮UmbraEditor Win64 Development构建成功；Umbra.Skills六项Automation成功。带渲染PIE在Niagara测试配置与原有正式配置下各六条路径成功：命中、墙阻挡、释放前/后取消、空放、资源不足。Niagara另采样飞行启停、施法组件清理和短效系统8秒内结束；测试通过真实Enhanced Input注入，不是实体键盘操作。报告分别为 `Saved/SkillImport/NativeBaseline/niagara-pie.json` 与 `Saved/SkillImport/GlintstonePebble/skill-pie.json`。
- 独立编辑器回读确认两个人体共享骨架/所需骨骼存在、十段原生动作结构有效；渲染三组姿态截图，已查看蓄力姿态的双角色正面斜视截图。这是基础结构与静帧检查，未验收完整动作自然度。回读确认正式Player仍为Greystone，辉石魔砾四个新增Niagara引用均为空，临时测试未保存到正式定义。
- 尚未制作正式Manny玩家动画蓝图、施法动作库、八个完整技能或最终美术；未完成法环FXR复现。未验证多人、Cook、性能预算或原版逐帧对比。日志仍有既有启动Condition failed，不声明全日志零错误。新资产LFS属性通过；保留原脏工作区，未提交推送。

## 2026-11-07 辉石魔砾FXR审计（完整视觉与姿态修复未完成）

- 固定官方@cccode/fxr32.1.3，解析510010/523000–3及嵌套430/440；七份语义往返比较通过，源资源依赖齐全，报告 `Saved/SkillImport/GlintstonePebble/FxrAudit/manifest.json`。法阵位于510010，两层引用s60130纹章；430/440为风场。飞行19节点配置、施法17、命中11，不等价于之前的少量Cascade发射器。
- 源523001仅有声音而无appearance，已通过UE编辑器移除DA的自制释放闪光并保留原声；bootstrap同步修正。其他旧视觉仍待替换，未把审计结果冒充已还原特效。UE原生材质/渲染基元映射、TAE37/行为切换、法杖挂点、多帧原版对照尚未验收。
- 独立UE动作诊断完成，保存参考姿态offset及源/目标七个时间点变换。旧测试只检查高度/时长，不覆盖歪头或左右协调；当前未修复姿态，也未新增法阵。详细证据、局限和UE原生素材发展路线见 [导入记录](GlintstonePebbleImport.md)。
- 提取器.NET构建与补提取通过；FXR语义检查通过；实际编辑器资产保存/回读通过；UmbraEditor Win64 Development构建检查通过（0编译动作，`fxr-audit-build.log`）；本轮带渲染和音频的六路径PIE通过（`fxr-verified-pie.log`、`skill-pie.json`）。未重新跑Automation、未进行原版视觉/音频A/B、多人/Cook验证。未删除旧资产、未提交或推送。

## 2026-11-07 辉石魔砾原版三阶段音效

- 从本机游戏 `sd/sd.bdt` 的 `/enus/cs_main.bnk` 提取原媒体；FXR action75 → Wwise事件/容器 → WEM对应已核实。发射524617251、飞行1040010023、命中82601330/164447618/123415567。正式DA已换原声，移除合成施法音；旧合成资产保留但不被技能引用。
- 飞行音按原44100Hz的[0,18687)采样区间循环，AudioComponent附着弹体，命中停止，EndPlay停止。命中Cue接三种原变体；事件默认音量保留，UE空间衰减/随机袋/混音属于适配，未移植原Wwise全部RTPC及环境总线。
- 本轮UE5.8.2 Win64 Development构建通过；Umbra.Skills五项Automation全部通过，新增PebbleOriginalAudio验证来源、循环、音量、三变体。带音频PIE六路径全部通过，观察到飞行播放及命中停止。报告 `Saved/SkillImport/GlintstonePebble/OriginalAudioAutomation/index.json`、`skill-pie.json`、`OriginalAudio/configuration.json`。常规生命周期测试关闭耗时截图，避免短飞行阶段被截图卡帧跳过。
- 未做人工试听、原游戏同步A/B、多人/Cook/全套Automation。本轮没有修复VFX保真：现有Cascade仅复用两张原贴图，其发射器、曲线、时序是人工搭建，不能称原FXR复现。细节见 [导入记录](GlintstonePebbleImport.md)。未提交/推送，保留其他工作区变更。

## 2026-11-06 辉石魔砾可释放适配版（历史记录，音效已被上文替换）

- 已接入第七技能格/数字5，鼠标方向施法；7资源，0.35秒释放，1.8秒冷却（服从既有全局开关）。原动画V4制作正式Montage，原图标与两张原DDS贴图导入，重建施法/释放/飞行/命中Cascade，自制三段声音。不是FXR无损导入或原声还原，保留当前武器、无新增法杖要求；参数、来源及差异见 [完整记录](GlintstonePebbleImport.md)。
- C++新增服务器单次命中投射物，扫掠阻挡、有限追踪、寿命清理；伤害走既有GAS ExplicitChannels（Cold 24＋0.7 AP）。沿用既有技能执行器与UI状态，不新增独立伤害计算或技能系统。
- UE5.8.2 UmbraEditor Win64 Development构建通过；Umbra.Skills四项Automation通过。真实Enhanced Input注入的6条PIE路径通过：命中、墙阻挡、释放前取消、释放后取消、空放、资源不足；正常命中仅一次24伤害，消耗7且不重复扣费，冷却/状态/粒子/弹体清理通过。慢速视觉PIE另通过，已查看飞行拖尾、命中闪光及第七格HUD截图。
- 未测试实体键盘、人工声音试听、多人/丢包、移动目标追踪、Cook或全套Automation；原版视觉/音频保真未验收。启动日志仍有既有Condition failed，不声明整份日志零错误。新资产LFS已确认，测试地图未保存，未提交推送。

## 2026-11-06 辉石魔砾首段动画导入与重定向

- UE 5.8.2 已保存候选 a401_045010（1.5秒）及 Greystone 重定向输出；当前候选为 `/Game/Skills/Staging/GlintstonePebble/SourceV4`、`RetargetV4`。使用厘米数据和动画首帧参考姿态解决早期倍率/整体翻转，20条重定向链。源 c0000 FLVER 无网格，诊断代理不是角色美术。脚本、旧候选状态与后续步骤见 [导入记录](GlintstonePebbleImport.md)。
- 源/目标骨架、时长、动作变化及四采样时刻的站立高度检查通过；独立带渲染PIE四样本/四截图通过，已查看整体站立、抬手和收手变化。保留现有剑盾；法杖对位、手指、脚底贴地及原游戏逐帧一致性未验收，不能称辉石魔砾已完整复现。
- 未接入正式技能/地图/角色配置，没有本任务C++改动或构建/Automation；尚未实现释放事件、投射物、Niagara、原声音，未测伤害、多人或Cook。资产通过编辑器生成，LFS属性已检查，原工作区变更保留，未提交或推送。

## 2026-11-06 辉石魔砾源资源提取与动画转换

- 用户选择辉石魔砾并补齐 sfx/sound 导出。已读取实际源包，提取150骨玩家骨架、a401事件、a9x包内12段候选动作及 compendium、5组 FXR/资源表、25贴图与3模型；缺少的 s00002_m 从 DLC02 公共包补齐。脚本、来源、报告和下一步见 [辉石魔砾导入记录](GlintstonePebbleImport.md)。
- 隔离使用官方 Soulstruct 3.2.1 与 Blender 5.2.3，12段动作转换到原骨架 FBX；逐帧矩阵检查通过，独立进程重导12份FBX的骨数/Action/时长检查通过。尚未做源动作视觉比较或 UE 重定向，不能称技能已复现。
- TAE bank37 尚缺适配模板，事件保留原始数据；实际音频主体仍在独立 sd 档案中，sound 目录的7个文件不足以证明音效已齐备。未执行 UE 构建、Automation、导入、PIE、试听、多人或Cook，未修改Content及玩法。原有工作区变更保留。

## 2026-11-06 技能导入、自定义与变异规划

- 新增 [技能导入路线图](SkillImportRoadmap.md)：单个法环参考投射物技能盘点与复现 → 动画/逻辑/表现解耦 → 原创“幽焰裂矢” → 多弹、穿透、墙反射及敌间跳弹组合。明确清单管线、强化来源/覆盖顺序、服务器权威、去重、取消清理和分阶段验收。
- 状态为规划，下一步从 M0 单技能源资源与依赖盘点开始。现有图标导入不代表动画/VFX/SFX已经可用；原始技能资源与转换工具未核实，资产内部待编辑器确认。
- 本轮对照源码与现有文档，检查文档链接/空白；没有编译、Automation、导入、PIE、试听、多人或 Cook。未修改 C++ 或 Content，保留已有工作区改动。

## 2026-11-06 法环图标导入与技能UI接入

- 检查本地战技联系表和两张原PNG，选择08314近身剑姿给Q横扫、08309宽挥斩轨迹给E广域横扫。UE编辑器导入为 `/Game/UI/Icons/EldenRing/T_Skill_Sweep_ER08314`、`T_Skill_WideSweep_ER08309`，1024×1024，UI/EditorIcon/sRGB/NoMipmaps/NeverStream。来源、SHA256、选择依据与未核实的版本/许可见 [HUD图标说明](SkillHUDLayout.md)。
- 替换DA_Sweep/DA_WideSweep.Icon及技能栏前两格、技能格默认Designer PreviewIcon。编辑前后布局参数比较通过，技能半径/角度/伤害/费用/冷却/动画/VFX快照一致。经验条、血蓝条、两侧框和其他资产未写入；原PNG未修改。工具为Scripts/import_skill_icons_er.py，报告Saved/SkillHUD/er-icon-import.json。
- 独立编辑器重新加载后，数据定义、WBP预览引用和纹理设置回读通过。带渲染PIE确认Q/E实际材质IconTexture分别引用两张新纹理；真实ASC横扫激活、资源100→85、目标10000→9985、冷却文本出现与结束隐藏通过（er-icons-pie.json）。已查看ScreenShot00022/00023.png，图标在既有边框内，冷却与经验条正常显示。
- 本轮只改资产引用、导入与报告脚本、文档，无C++改动，因此未重编译C++或运行全套Automation。技能通过ASC测试，未重新执行实体键盘Q/E输入；未验证多人或Cook。导入命令let零错误、两条既有Python命名告警；新uasset的LFS属性和文本空白检查通过，未提交推送。

## 2026-11-06 紫色经验条显示占位

- 新增普通UserWidget `WBP_PlayerExperienceBar`，复制现有资源条的控件树和进度条子WBP；高度14、无数值文字，材质使用Violet的项目子实例 `MI_ExperienceBar_Preview`，固定50%填充。放在蓝条下方，Vitals容器高80→84；保留用户最新1332总宽、800中列宽及201.33中列高度、两侧框、技能栏与原条Slot配置。没有经验/等级/GAS绑定。配置入口见 [HUD布局](SkillHUDLayout.md)。
- UE5.8.2 UmbraEditor Win64 Development构建成功（Saved/SkillHUD/experience-build.log）。创建与回读完成（experience-created.json），事件图只有未连接的默认事件。最初材质设置的断言依赖UE5.8.2始终返回false的setter，已改为实际参数回读验证；最终保存成功。
- 带渲染PIE脚本通过：唯一经验条实例、位置层级正确、实际动态材质Value持续0.5、ValueText折叠（experience-pie.json）。已查看ScreenShot00021.png：紫色细条位于蓝条下方，半满显示，两侧框及现有布局保留。没有执行技能按键、经验增减、升级、多人或Cook测试，也未运行全套Automation。日志启动阶段存在LogAutomationTest Condition failed信息（脚本执行前），不宣称整份日志零错误；经验条脚本及运行阶段没有发现新增蓝图报错。
- 新增uasset的LFS属性已确认，未提交或推送。修改仅涉及经验条相关资产、HUD挂载、编辑器创建工具、脚本和文档。

## 2026-11-05 ActionbarDemo 技能格与键位

- 检查样品W_Window_ActionbarDemo / W_Window_Actionbar / W_Actionbar_Item_Slot实际层级与画刷，技能格使用fg4_slotBrown，键位底框fg4_slotTooltip1；沿用现有同系列石纹连续背景。WBP_SkillBar左对齐，子格间距0；WBP_SkillSlot布局宽64、画框宽72，以透明边重叠实现可见边缘贴合，Q/E位于图框下方。保留现有图标、成本、冷却及用户此前移除的名称/状态控件。
- UE5.8.2编辑器保存成功；带渲染PIE通过ASC激活横扫、单次命中、资源扣15、冷却文字显示及结束隐藏（Saved/SkillHUD/pie.json）。截图ScreenShot00015.png已查看，框从左紧密排列、键位底框可见。离屏几何返回零、未做运行时坐标断言；本轮未验证按键输入、多人、打包或声音。未修改C++，未提交推送。

## 2026-11-05 Fantasy GUI 4 技能栏风格

- 按用户参考图通过UE编辑器将WBP_SkillBar的连续框改为 fantasy_gui_4/textures/borders/fg4_borders_stone，Box画刷Margin 0.04；WBP_SkillSlot的底框改为 fantasy_gui_4/textures/quality/fg4_slotGreyLight，原Img_Frame折叠避免遮挡图标。保留血条上方原位置、两技能格、图标、按键与GAS冷却连接。素材和人工调整入口见 [连续框技能栏](SkillHUDLayout.md)。
- UE5.8.2保存成功，数据记录Saved/SkillHUD/fantasy-style.json。带渲染PIE验证单个嵌入栏、一次横扫与冷却文字出现/结束隐藏，资源100→85、目标生命10000→9985；通过ASC接口激活，不是键盘输入。已人工查看Saved/Screenshots/WindowsEditor/ScreenShot00011.png：石纹连续底框、方形技能框和冷却可见。离屏缓存几何仍为零，未做坐标断言；未验证多人、Cook或声音。仅编辑WBP画刷及文档，没有新增C++编译需求。

## 2026-11-05 连续框技能栏与本地图标盘点

- 用户要求由我们先制作UI再自行修正：UE编辑器保存WBP_PlayerHUDGroup、WBP_SkillBar、WBP_SkillSlot与控制器配置。连续skill_bar现有纹理作为500×112整框，两个92×96技能格居中在框内；嵌于原500宽生命/资源条列上方，独立SkillBarClass清空。现有头像、生命条、资源条、右侧属性面板及其内部布局保留。结构、参数、人工调整入口见 [连续框技能栏](SkillHUDLayout.md)。
- 本地只读盘点173张6000段魔法/祷告与91张8300–8499动作候选；08309为横挥候选，08303更接近旋转斩；Goods CSV+中文文本可核实6000辉石魔砾、6014辉石弯弧、6050卡利亚大剑、6052卡利亚贯刺。缺少Gem/SwordArts参数表，动作图标不猜正式名称；源版本与许可未核实。没有导入或切换法环图标，继续使用现有warrior_12/48。来源、SHA256及联系表见Saved/SkillHUD。
- 本轮UE5.8.2 UmbraEditor Win64 Development构建通过；编辑器回读确认完整树与独立挂载关闭（Saved/SkillHUD/configuration.json）。首次保存出现新增控件缺少GUID的编辑器ensure，已补注册并重编译重保存，最终migration-final.log无该错误。两项Umbra.Skills自动化通过（Saved/SkillHUD/Automation/index.json）。
- 带渲染PIE通过单实例、所属玩家、ASC真实激活、资源100→85、目标10000→9985、真实冷却数字出现并结束隐藏（Saved/SkillHUD/pie.json）。截图ScreenShot00009.png已人工查看：连续框位于血条上方，两格在框内，显示3.3秒冷却。离屏CachedGeometry返回零值，未通过程序化坐标断言；实际对齐以截图观察及Designer配置为准。本轮通过ASC接口激活，不冒充键盘验证；此前键盘测试见上一条记录。未验证多人、Cook打包和声音听感。
- 保留现有图标及相关项目改动，未提交或推送；用户可按指南调整位置、框体和图标。

## 2026-11-05 横扫技能管线与技能栏接口

- 用户最终明确：WBP配置/接线由用户完成，视觉以用户HUD为准。已提供 [完整手工接线指南](SkillBarBlueprintGuide.md)：HUD → WBP_SkillBar → WBP_SkillIcon，自定义控件名、OnSkillViewUpdated各引脚、冷却遮罩/数字/状态、输入和排错。未替用户创建新命名WBP_SkillIcon或完成其最终HUD。此前演示WBP_SkillSlot/WBP_SkillBar保留；用户接入HUD前需把BP_UmbraPlayerController.SkillBarClass清空，防止独立演示栏重复挂载。
- 新增UUmbraSweepDefinition和一个UUmbraSweepAbility模板。DA_Sweep/DA_WideSweep共享独立AM_Sweep；Q/E，消耗15/25，冷却4/6秒，250cm/120°与350cm/170°，武器通道倍率1.5/1.0。服务器GAS扣费/冷却与现有伤害链、单次扇形/高度/遮挡、动作完成/移动与死亡眩晕中断；未接法环技能资产。
- Greystone动画和Cascade资产通过编辑器加载确认，同骨架PrimaryB序列1.4秒；技能Montage从PrimaryB_Montage复制并去除原Notify。初期独立AnimationEditor打开曾触发访问冲突，后续使用PIE观察角色动作。现有warrior_12/48与ClassicUI灰框用于演示，不更改第三方原资产。
- UUmbraSkillSlotWidget提供ReadViewState/OnSkillViewUpdated，每0.05秒读取当前玩家ASC真实Active GE冷却，没有UI独立倒计时。可选BindWidgetOptional自动表现与用户自定义蓝图表现两种接法详见指南；新增UmbraEditorTools仅供编辑器创建演示资产/临时测试，不属于游戏运行模块。
- 本轮UE5.8.2 UmbraEditor Win64 Development最终构建成功（Saved/SkillPrototype/build-final.log）；项目文件已刷新。实际PIE发现SetPositionInViewport重置Anchor，已修正顺序并再次构建。Independent readback通过：Q/E唯一正确映射、技能定义费用/冷却/icon、角色输入引用和已保存WBP层级；Saved/SkillPrototype/readback.json。5.8读取DefaultKeyMappings而非弃用Mappings，FKey使用自定义token导入，已修正配置脚本并通过回读。
- 自动化：Umbra.Combat.Maintenance、Umbra.Damage.EntryCompatibility、Umbra.Damage.TypedChannels在本轮automation.log中通过；初次CooldownView测试错误尝试实例化abstract控制器导致测试崩溃，修为普通APlayerController后，Umbra.Skills.CooldownView和SweepSector重跑2/2无警告通过（Saved/SkillPrototype/SkillRetest/index.json）。这是本轮执行结果，非引用历史通过。
- 实际键盘PIE记录：首个Q资源100→85、目标生命10000→9985；首个E资源85→60、生命9985→9975，每次Start后约0.4秒各一次Strike且targets=1。真实控件读回冷却3.9、3.8…及另一格约6秒递减；观察到演示栏在现有生命/资源条上方。保存Saved/SkillPrototype/keyboard-first-pass.json与keyboard-first-pass.log。随后还有额外空挥测试，不能把最终资源5当作第一次施放成本。首两次冷却结束/动作非Active状态在连续记录中可见。
- 截图HighresScreenshot00001/00002包含动作场景但不包含UMG，不能作为冷却UI视觉截图。已经查看初始技能栏画面，冷却文字通过真实控件读回；连续特效表现/声音听感、多人、Cook打包和用户最终手工接线的HUD未验收。用户提出自行接WBP后停止代改UI；配置脚本已移除UI创建与控制器挂载写入，该脚本调整后未再执行资产写入。
- 免费icon推荐已核对CraftPix两款官方页面/Freebie许可与Game-icons的CC BY页面，见指南；未下载导入。文本空白与新资产LFS属性检查通过，未提交或推送，保留其他已有改动。

## 2026-11-04 左侧防具与右侧四遗物栏

- 按用户新要求直接修正WBP_Equipment布局：左侧头/胸/手/腿，右侧护符/护符/戒指/戒指，主副手仍在两列底部。通过UE重排现有VerticalBox子控件，保留名称/引用和原有间距；SlotType及空栏背景逐格核对。新要求覆盖此前保留用户自行排版的安排。规则与脚本见 [四遗物栏与布局](RelicSlots.md)。
- Accessory类别统一显示为“遗物”，运行时GetAllowedEquipmentSlots/CanEquipInSlot允许全部四个右侧栏；服务器装备、背包指定格/空位选择、Tooltip和词条槽位过滤同步。116件护符及导入配方同步四栏。类别以外的装备仍用原AllowedSlots；同实例不能占多格，品质、词条、等级要求和堆叠规则不变。
- 本轮UE5.8.2 UmbraEditor Win64 Development构建通过。通过UE保存WBP与116件定义，独立回读布局/背景/116件配置通过，Saved/RelicSlots/configuration.json与readback.json；完整135件导入readback零保存通过，Saved/RelicSlotsImportReadback.log。
- 本轮27项自动化全部通过（24无警告、3带警告），包括Equipment、UI.Equipment、Inventory、UI.Inventory、Items.Generation、Items.Tooltip.Classification；新增FourRelicSlots验证旧护符/戒指配置均映射四格、四件同时装备及防具/武器槽拒绝，NativeAccessoryAssets逐件覆盖四栏。报告Saved/RelicSlots/Automation/index.json、日志RelicSlotsAutomation.log。
- 真实PIE通过：加载保存布局、核对两列子控件顺序和实际空栏画刷；四件敌人掉落护符经背包UI分别装备四个遗物栏，目标Tooltip有效且类型为遗物；四件同时生效，全部卸下保持实例快照。报告Saved/RelicSlots/pie.json，最终日志Saved/RelicSlotsPIE-final.log。已查看穿戴截图ScreenShot00003.png及布局稳定后的空栏截图ScreenShot00006.png（均在Saved/Screenshots/WindowsEditor），确认左侧头胸手腿、右侧双护符双戒指。
- Python语法、文本diff空白和资产LFS属性检查通过。首次配置脚本调用模板运行时视觉事件产生未构建HighlightFrame警告，后改为直接设置编辑器属性避免后续重跑触发；真实PIE未见此绑定问题。保留既有启动Condition failed、Paragon输入/动画及导航告警；未验证多人、声音、Cook打包。未提交或推送，保留已有改动。

## 2026-11-04 锻造石同种同品质堆叠

- 19种原生锻造石启用显式bStackableMaterial，服务器按物品定义+实例品质合并；不同种/品质分格。Quantity默认1，随实例记录和OwnerOnly背包快照保留；满包先尝试合并，合并退休源GUID以拒绝重复提交。拖动整堆保留数量，拖入兼容堆合并，其他格继续交换。规则、int32上限、配置与接口边界见 [锻造石堆叠](SmithingStoneStacks.md)。
- WBP_InventorySlot使用已有StackCountText显示数量，包括单件1；空格/非堆叠物品清空隐藏。仅C++投影实例数量，无蓝图合并逻辑。通过UE5.8.2保存19件定义及配方/说明迁移，更新可重复导入脚本；完整135件独立readback通过且零保存，Saved/SmithingStacks/migration.json、Saved/SmithingStacksReadback.log。
- 本轮UmbraEditor Win64 Development最终编译通过。18项相关自动化最终全部通过：首轮17通过（15无警告、2带警告），RequestProtocol因旧测试用1级角色装备10级物品失败；仅补齐该测试的CharacterLevel前置条件并重新编译，单独重跑1/1通过，不改变生产等级规则。报告Saved/SmithingStacks/Automation/index.json及ProtocolRetest/index.json。覆盖Inventory/Inventory UI、Generation、Presentation；新增SmithingStacks含满包合并、类别/品质隔离、不同等级合并、重复GUID、移除返回、移动合并/交换、溢出拒绝、记录恢复及真实WBP数量显示/清理。
- 真实PIE最终通过：实际敌人生成4颗测试石头，经拾取成为3格（2+1+1）；已打开菜单即时显示数量；整堆移动后数量2保留、源格数字清除，正常EndPIE。Saved/SmithingStacks/pie.json与SmithingStacksPIE-final.log；首轮玩法检查已通过，仅截图接口名错误，修正后完整重跑成功。离屏渲染截图Saved/Screenshots/WindowsEditor/ScreenShot00001.png已查看，当前可见的单件数量显示；数量2移动到第90格，文字由真实控件读回验证。
- Python语法、文本diff空白、文档链接和uasset LFS属性检查通过。日志保留既有启动Condition failed、Paragon输入/动画等信息；未验证双人网络、声音或Cook打包，也未实现拆堆/消耗/强化/磁盘存档。未提交或推送，保留先前用户改动。

## 2026-11-04 开局额外随机护符与锻造石

- L_Prototype出生点附近新增BP_StartupAmulet和BP_StartupSmithingStone，每次开始各等概率选一件，再沿用SourceRarity品质限制和晋升规则；物品等级1，可拾取。保留原六品质测试装备。配置入口、实例覆盖顺序及开发构建边界见 [开局试玩掉落](NativeAccessoriesImport.md#开局试玩掉落)。
- AUmbraLootTestSpawner新增bRandomSingleItem，默认false，不改变原批量或六品质模式；六品质模式优先。Blueprint和地图由UE5.8.2编辑器保存，脚本configure_native_startup.py可重复配置而不增加重复Actor。记录Saved/NativeStartup/configuration.json。
- 本轮UmbraEditor Win64 Development构建通过；真实两次PIE离屏渲染通过：每次各一件、种子/身份刷新、等级1、品质限制、正确词条数、Niagara激活和拾取快照，原六件仍存在。两次抽到不同护符及锻造石，重复种类本身也属于合法随机结果。报告Saved/NativeStartup/pie.json、日志Saved/NativeStartupPIE-render.log。
- 首轮NullRHI模式的VFX激活断言失败，改用RenderOffscreen后完整重跑通过。检查脚本语法、C++文本diff空白、资产LFS属性；日志仍有既有Paragon/启动警告。未运行完整Automation、多人、Cook打包或人工视觉/声音验收；未提交或推送。

## 2026-11-04 原生护符与锻造石导入

- 用户补齐Accessory/Goods两表后，按本地CSV的iconId精确匹配图标：导入116护符、19种锻造石及135独立图标；39件缺图护符与2件无名占位排除。独立目录NativeAccessoriesV1共280个资产，保留源行、中文文本、原始rarity、源文件/图标哈希及版本未确认信息。流程与调参入口见 [原生护符与锻造石](NativeAccessoriesImport.md)。
- 两类均沿用现有SourceRarity品质限制/晋升规则；护符使用独立复制的公共/防御词条池，可装备Amulet/Amulet2；材料复用品质抽取但无词条、无装备槽、无战斗特效预留文案。共用Engine Sphere缩放0.12作掉落占位。未实现原作护符专属效果、同族互斥、材料堆叠/消耗/强化，未引进其他消耗品或戒指/遗物。
- DA_Loot_Prototype引用原V2的580模板，加135件新物品。分类权重暂设武器32/防具48/护符10/材料10，保留旧武器防具2:3比例，不宣称是法环掉率。通过UE保存BP_Enemy_Melee_01的新默认表；关卡7个同类敌人检查继承新表，无需重存地图，未移动任何UI栏位。
- 本轮UE5.8.2 UmbraEditor Win64 Development构建通过（包含新增测试）。小批量12件导入、全量导入和独立进程135件回读均通过，回读零保存；报告 `Saved/NativeAccessories/import_smoke.json`、`import.json`、`readback.json`，激活记录 `activation.json`。所有二进制由匹配编辑器保存。
- 本轮自动化9/9通过（7无警告、2带警告）：DamageSnapshot、Lifecycle、MaterialRarity、NativeAccessoryAssets、ProductionAssets、ProductionLoot、Rules、SixRarities、StagedEquipment。新增测试覆盖源0..3各1000种子的品质限制/记录恢复、135件各24种子的生成、物品背包Tooltip、双护符槽装备和2000次分类抽取；原V2生成测试同时回归。报告 `Saved/NativeAccessories/Automation/index.json`，日志 `Saved/NativeAccessoriesAutomation.log`。
- 真实单机PIE最终通过：原型敌人/新生成敌人的已保存掉落表引用；两护符和两锻造石经实际死亡掉落/拾取/Tooltip/实例快照检查；双护符同时穿戴再卸下；材料空栏装备拒绝且背包不变；打开/关闭菜单并EndPIE。记录 `Saved/NativeAccessories/pie.json`、`Saved/NativeAccessoriesPIE-final.log`。首轮测试在已占用护符栏请求装备材料，返回SlotOccupied导致断言失败；修正为先测试空栏，未改变生产拒绝规则，重跑通过。
- 检查135张源图联系表、脚本语法、文本diff空白和新增uasset的LFS属性。日志仍含原有Paragon输入轴/动画除零、GameplayCue路径、启动Condition failed等警告/错误文本；测试报告中的警告为无上下文测试World销毁和HTTP探测超时，不声称日志完全干净。PIE使用NullRHI/nosound，未人工验收渲染/声音、多人、Cook打包；未提交或推送，保留原工作区改动。

## 2026-11-04 鞋子栏替换为第二护符栏

- 用户确认保持十栏：Legs 覆盖裤靴，Feet 隐藏停用，新增 Amulet2；保留旧序列化编号，防止旧鞋子自动变成护符。有效槽位检查同步装备、背包转移、Tooltip、防具掉落类型与菜单。
- UE 编辑器已保存 WBP_Equipment 和 WBP_EquipmentSlot，原鞋子控件 SlotType 映射到 Amulet2，使用已有 neck_background 图标；未移动控件或修改布局坐标，最终位置由用户在 Designer 配置。回读十个唯一槽位通过，记录 `Saved/SecondAmulet/assets.json`，迁移日志 `Saved/SecondAmuletMigration.log`。隐藏旧枚举的迁移由编辑器专用桥处理。
- 本轮 UmbraEditor Win64 Development 最终构建通过；10项自动化全部通过（9项无警告、1项带警告），覆盖 RequirementsAndLifecycle 中双护符独立穿脱/同实例拒绝重复占槽/Feet拒绝，以及 FixedAffixesAndDamage、EmptySlotIcons、LiveSnapshot、PageVisibility、PreviewLifecycle、SlotBindings、SlotContract、EquipInteraction、TooltipHover。报告 `Saved/SecondAmulet/Automation/index.json`，日志 `Saved/SecondAmuletAutomation.log`。不是仅引用历史验证。
- 本轮未导入护符、戒指/遗物、消耗品或配置其公共占位模型：本机缺少 EquipParamAccessory.csv 与 EquipParamGoods.csv，用户确认暂无；现有文本不能替代 iconId 映射。外部图标仅核查产品资料，未购买、未逐图验收。素材比较、消耗品优先级与导入边界见 [双护符与素材计划](AccessoriesAndConsumables.md)。
- 真实 L_Prototype PIE 通过：独立进程加载保存后的 WBP，打开角色菜单，回读十个唯一有效槽位及两护符空栏图标，关闭菜单并正常 EndPIE。记录 `Saved/SecondAmulet/pie.json`、`Saved/SecondAmuletPIE.log`。脚本初次两轮因 Python 接口差异失败，修正为 GameplayStatics/ObjectIterator 后重跑通过；不视为玩法失败。
- 已检查日志：PageVisibility 测试的空测试菜单有0/10槽位警告；编辑器启动阶段仍有 LogAutomationTest 的 Condition failed 文本及 Paragon 旧输入轴警告，不能称全日志零警告/零错误。指定十项测试报告均通过，真实菜单未出现缺槽警告。未进行双人网络或人工视觉验收。

## 2026-11-03 高品质统一红框

- 按用户要求，五套V2 Profile的Legendary/Unique边框统一为ClassicRPGUI/SpecialFrames/Frame_red；通过UE5.8.2编辑器保存，同步rarity_palette.py六档映射和生产配方。颜色、背景和概率不变。
- 独立编辑器进程回读全部5套×2档引用通过：Saved/HighRarityFrames/verified.json，保存明细assets.json。diff空白检查与既有LFS规则通过。
- 仅资产配置/脚本调整，无C++修改，未重新编译、运行自动化或PIE，未人工确认渲染；无提交或推送，保留既有改动。
## 2026-11-03 紫色与金色品质明度

- 史诗紫6B30B7→B070FF，独特金A88732→FFD34D（sRGB）；五套V2 Profile的Color、生产配色脚本及无Profile词条默认色同步。品质标签与对应梯度词条共用新色；背景、边框、掉落概率不改，已有物品可读取新色。Scripts/brighten_rarity_colors.py通过编辑器保存五个资产并同步配方。
- UE5.8.2 UmbraEditor Win64 Development构建通过（Saved/RarityBrightness/build.log）；独立进程回读五套两档颜色通过（verified.json），真实WBP AffixRollColor自动化1/1通过（Automation/index.json）。diff空白检查通过。
- 未人工检查游戏内渲染效果，未重跑战斗PIE、多人或打包；本轮使用nullrhi/nosound。保留既有修改，无提交或推送。
## 2026-11-03 品质晋升概率与高品质预留标记

- 规则版本6：源0仍普通/魔法/稀有；源1普通/魔法/稀有/史诗权重50/30/15/5；源2按用户最终指定为史诗75%、传奇25%；源3传奇/独特各50%。SourceTwoLegendaryChance独立配置源2概率，五套Profile均保存0.25；未将源3的1:1权重误用为源2概率。高档仍4条普通词条。
- 实例通过HasSpecialEffect()/IsUnique()从持久化品质派生预留身份，Tooltip公开bHasSpecialEffect/bIsUnique；传奇显示“特殊效果待实现”，独特另显示“独特物品”。不新增GE、传奇威能或独特技能；切回低品质清除标记。已有实例保留原品质，高品质已有实例也能显示标记。
- UE5.8.2 UmbraEditor Win64 Development最终构建通过：Saved/SourceRarityPromotion/build-final.log。用户在初次编译期间调整为25%，已重新完成最终编译。Python修改脚本语法和diff空白检查通过。
- 自动化12/12通过（10成功、2带警告）：Saved/SourceRarityPromotion/Automation/index.json。580模板×各允许品质×4等级×8种子，共60736次正式生成；另验证源1/2各2000种子的5%/25%概率、0/1边界、非法概率、品质标记记录恢复与UI重用清除。原有标题颜色测试不在本次范围，不宣称已修复。
- 真实单机PIE40/40通过：Saved/SourceRarityPromotion/pie.json、pie.log。含14件传奇、4件独特，已检查死亡/掉落/拾取/Tooltip标记、可用装备穿脱和等级/未开放类型拒绝。源3/2/1分别14/19/7件。未人工验证渲染/声音、多人或Cook打包，本轮使用nullrhi/nosound；保留既有启动警告。
- 独立进程回读580件来源及五套Profile的25%概率通过：Saved/SourceRarityPromotion/verified.json；完整生产管线重复执行580件、零保存、无冲突：Saved/RandomEquipmentV2/source-rarity-promotion-repeat.json。
- 通过匹配编辑器保存V2配置/配方版本，未修改Library；不重掷已有实例，不提交或推送，保留先前未提交改动。配置来源和覆盖顺序见RandomEquipmentExpansion.md、EditorSetup.md。
## 2026-11-03 原始稀有度约束正式随机掉落

- V2 580 件定义导入 SourceRarity：369件原始1、190件原始2、21件原始3，无原始0模板。源0/1仅普通/魔法/稀有，源2固定史诗，源3仅传奇/独特；服务器在抽品质前过滤，零合法权重拒绝生成，不降级。-1保留旧规则。来源逐件来自 Library 的 UmbraItemImport.Source.item.raw_params.rarity，未修改 Library。
- 低档权重50:30:15在合法集合内归一化，传奇/独特默认1:1，两者各4条普通随机词条；史诗4条，普通至稀有1/2/3条。未新增传奇威能/独特效果，也不改类型和模板选择概率。V2规则版本5，来源与覆盖顺序见 RandomEquipmentExpansion.md。已有实例保持生成快照，新掉落生效。
- UE5.8.2 UmbraEditor Win64 Development最终构建通过：Saved/SourceRarity/build-final.log。编辑器保存625个V2包的规则/配方版本，后续清理580个配方中的已废弃required_level引用；独立回读580件源值与5个高档配置通过（verified.json）。物品等级继续采用既有实例等级。
- 自动化8/8通过（7成功、1带警告）：Saved/SourceRarity/AutomationFinal/index.json。ProductionAssets覆盖580个模板、1339个允许品质组合，每组合4等级×8种子，共42848次生成；ProductionLoot另覆盖源0..3、禁止品质、全零合法权重、完整词条与旧规则。首轮两项生命周期/GAS测试被角色默认等级1拦截，已将测试角色等级设置为生成物品等级后重跑通过，未放宽生产等级门槛或原有断言。
- 生产管线重复执行通过：Saved/RandomEquipmentV2/source-rarity-repeat.json，580件符合条件、零保存，verify-final.log无Python错误。首次重复执行暴露旧required_level导入引用，移除过时字段并迁移配方后完整重跑通过。
- 真实单机PIE 40/40通过：Saved/SourceRarity/pie.json、pie.log，32类型定向+8正常表掉落，源3/2/1分别14/20/6件。覆盖死亡/掉落/拾取/Tooltip、可用装备穿脱和等级/未开放类型拒绝，全部满足源品质限制。
- 未验证人工视觉/声音、多人或Cook打包；本次PIE与自动化为nullrhi/nosound，仍有既有启动警告，不宣称全项目日志无错误。diff空白检查及uasset LFS规则通过；保留先前改动，无提交或推送。
## 2026-11-03 词条六档稀有度着色

- 六候选值词条的 SelectedTier 0..5 映射白/绿/蓝/紫/粉/金，词条名、数值、区间同时着色，独立于物品品质。优先使用物品 GenerationProfile.Rarities.Color，缺项回退既有六色。固定词条、预览、无效索引及非六候选区间为白色；生成和 GAS 规则不变。已有实例直接读取保存的梯度，无需重新掉落。
- 新增 RollColor 展示字段，真实 WBP 与原生回退行统一使用；复用时清除旧色。配置来源与覆盖顺序见 ItemTooltipUI.md；无需改 WBP 资产。
- UE5.8.2 UmbraEditor Win64 Development 构建通过：Saved/AffixRollColor/build-final.log。首次测试代码使用了不存在的 Cyan 常量，已改用显式颜色并重新构建通过。
- 自动化 8/8 通过（7 成功、1 带警告）：Saved/AffixRollColor/Automation/index.json。新增 AffixRollColor 验证六档、独立于物品品质、不等距数值、配置覆盖、真实 WBP 三文本控件及复用清色；同时运行全部 Items.Tooltip、TooltipLifecycle 和 TooltipLayoutStability。
- diff 空白检查通过，保留既有未提交改动。未运行人工渲染/交互 PIE、多人、全量测试或打包；本轮自动化使用 nullrhi/nosound。既有 TooltipTitleColor 测试不在本轮范围，不宣称该问题已修复。无提交或推送。
## 2026-11-03 武器伤害明细恢复原字号

- 新增可选 DamageKeyValueEntryClass，仅武器 DamageRows 使用；未配置则回退 KeyValueEntryClass。UE 编辑器创建 WBP_TooltipDamageEntry 并将词条名、数值恢复 24；基础伤害汇总行与防具主要属性保留 30。脚本 Scripts/configure_damage_row_font.py 可重复应用，字体仍由 WBP Designer 配置。
- UE5.8.2 UmbraEditor Win64 Development 构建通过（Saved/DamageRowFont/build.log）。首次编译发现重复属性声明，修正后重建通过。独立编辑器回读验证伤害明细 24/24、防具 30/30、汇总 30/30（verified.json）。
- TooltipLayoutStability、TooltipLifecycle、TooltipViewportFit 自动化 3/3 通过（2 成功、1 带警告），Saved/DamageRowFont/Automation/index.json。未再次运行战斗 PIE、全量测试或人工渲染检查；本次使用 nullrhi/nosound。原有标题颜色测试不在本次范围，不能据此宣称已修复。
- diff 空白检查通过，新 WBP 符合 LFS 规则；保留既有改动，无提交或推送。
## 2026-11-03 物理词缀、初始背景与主属性字号

- V2 斩击/打击/穿刺三个增伤资产全部层级改为 AttackSource=Any，生成脚本同步；按伤害类型匹配普攻和技能。已有实例保留生成时快照，新生成物品使用新配置。
- 初始 DA_Item_TestSword、DA_Item_TestMace 补齐三种背景引用，普通品质使用 fg4_glowGreyLight，关闭二次染色。WBP_ItemTooltip 的基础伤害标题/总值和 WBP_TooltipKeyValueEntry 的词条名/数值字号从 24 增至 30；后者共同服务武器分项和防具主属性。
- 特殊效果栏不再插入需求失败或惩罚提示；需求行与等级标记继续反馈不足，实际惩罚规则不变。脚本 Scripts/fix_item_presentation.py 通过 UE 编辑器保存资产，未手改二进制。
- **UE5.8.2 UmbraEditor Win64 Development 构建通过**：Saved/ItemPresentationFix/build.log。独立编辑器进程回读 3 个物理词缀资产、2 件初始装备、4 个字号通过：verified.json；资产保存明细 assets.json、assets-final.log。
- **自动化 10/11 通过**（9 成功、1 带警告、1 失败）：Saved/ItemPresentationFix/Automation/index.json。包含 Damage.BucketsAndMigration、TypedChannels 和全部 UI.Items；唯一失败仍为已知 TooltipTitleColor（白色实现与品质色断言不一致），未放宽断言。初次将回读脚本与自动化合跑时脚本结束使编辑器提前退出，已单独重跑并取得上述完整报告。
- **真实单机 PIE 40/40 通过**：Saved/ItemPresentationFix/pie.json、pie.log；32 类型定向 + 8 次普通掉落，死亡/拾取/Tooltip/可用装备穿脱，以及等级或未开放类型拒绝穿戴；本次生成的 5 条物理增伤全部验证来源 Any。
- Source/Scripts/Docs diff 空白检查通过，所改二进制沿用 LFS。未验证人工渲染效果、声音、多人或打包；本轮 PIE 使用 nullrhi/nosound，启动仍有既有警告/条件错误，不宣称项目全日志无错误。保留既有改动，无提交或推送。
## Tooltip WBP 配套 C++、需求等级与强化名称（2026-11-03）

- 接入用户新增 DefenseSection / DefenseRows，防御值与基础伤害区彻底分离。要求栏只显示非零属性需求，不足追加红色“不满足”，无需求整栏折叠；等级不足改为上方红色“物品等级 N ×”，复用时恢复原始颜色。未开放穿戴/惩罚说明转入特殊说明区。风味隔断由用户 WBP 控制，本轮未保存/修改 WBP 二进制。
- 需求等级统一为实例 GetItemLevel：实际 EquipInstance、需求查询、快照刷新与Tooltip一致。旧 Definition.RequiredLevel 仅保留序列化兼容，停止编辑和运行判定；不会将旧值写入物品等级。CharacterLevel仍是原型固定配置，新增所属客户端复制。新增非负实例 UpgradeLevel，默认0，只用于名称 +N，记录往返保留；Tooltip、装备显示和地面标签共用 GetDisplayName。未实现强化玩法/数值加成。
- **构建通过**：UE5.8.2 UmbraEditor Win64 Development，Saved/TooltipCompactFix/build-final.log。**自动化19/20通过**（18成功、1带警告），AutomationFinal/index.json；真实用户 WBP 的 TooltipCompactContract、防御分栏、精简行、红色/×恢复、实例等级拒绝/允许穿戴、记录强化字段往返、背包转移均通过。唯一失败为既有 TooltipTitleColor（测试期望品质色，现有标题白色），未放宽该断言。
- **真实单机 PIE 两组各40/40通过**：原型等级组 pie.json（24等级不足拒绝、9未开放类型拒绝、7穿脱成功），临时高等级默认值组 pie-high.json（达标装备穿脱、未开放类型拒绝）。均验证死亡/掉落/拾取/需求文本/实例等级一致/基础防御行及实例保留。脚本不保存地图或角色资产，高等级测试结束恢复默认值。首次旧脚本假定全可穿戴失败，第二次尝试编辑实例 EditDefaultsOnly 属性被编辑器拒绝；最终脚本按实际等级分支，另用临时CDO默认值验证达标组，没有放宽游戏规则。
- **未验证**：nullrhi/nosound，无人工渲染/声音验收、多人等级复制实测、Cook/打包；启动日志保留已有条件错误/第三方警告，退出有编辑器服务通信错误，不宣称全日志无错误。源文件/文档/脚本 diff 空白检查通过。用户 WBP SHA256 保持 BCBF72F29A829A0B5F54CCAEBA0D2E7D2FA648703CE4B31764B8A631383B7EB8。无提交/推送，保留原有工作区修改。
## 六档品质 icon 背景配置（2026-11-03）

- 已按用户所示素材，将 V2 Armor/Weapon/Shield/Ranged/Catalyst 五个生成配置的全部六档背景保存为 fg4_glowGreyLight/Green/Blue/Violet/Pink/Yellow；共30行、90个背景引用。背包格、Tooltip icon、Tooltip标题背景同步使用原色光晕，TintBackgrounds=false。边框引用不变；本节覆盖此前“背景留给用户配置”的待办。
- 已验证：UE5.8.2 编辑器脚本保存成功，并在独立进程回读全部30行、90个引用与关闭染色状态；Saved/TooltipDefenseFix/backgrounds.json、backgrounds-verified.json、backgrounds.log、backgrounds-verify.log。命令0错误，2条既有Python暴露字段同名警告。脚本/文档 diff 空白检查通过，.uasset沿用Git LFS。
- 未验证：本次没有重新构建C++、运行自动化或PIE人工视觉检查；这是贴图引用配置修改，先前测试结果不算本轮验证。未提交/推送，保留原有工作区修改。
## Tooltip 基础防御与品质素材修正（2026-11-03）

- 基础护甲/魔抗单独投影至 DefenseEntries，显示在独立“主要属性”区；固定/随机防御词缀仍在物品属性，保留实际值与范围。原始 Stats 与 GAS 数值未改变，复用现有 WBP 主数值区，无 WBP 二进制修改。
- UE5.8.2 编辑器 Python 修正 V2 五个 DA_Generation_* 的魔法/稀有/史诗边框引用，使用 Frame_green/blue/purple 原色；普通保留 Frame_grey。脚本仅修复灰色默认引用，保留其他手工配置；后续新建默认配置也使用对应贴图。证据 Saved/TooltipDefenseFix/frames.json、frames.log。
- 用户要求**全部六档**均配置：五个 Profile 已各有完整六行。传奇粉色与独特金色的同系列透明边框尚未找到，待用户指定资产；不以带不透明底板的 fg4_slotPink/Yellow 覆盖图标。没有改动掉落品质概率。
- 已核对：所有六档当前 SlotBackgroundTexture=Background_grey，TooltipIconBackgroundTexture/TooltipTitleBackgroundTexture=fg4_glowGreyLight，TintBackgrounds=true。按用户要求留给用户手动配置彩色背景，不宣称背景配置完成。路径与开关见 [ItemTooltipUI](ItemTooltipUI.md)。
- **最终 UmbraEditor Win64 Development 构建通过**：Saved/TooltipDefenseFix/build-final.log。**自动化 12/13 通过**：AutomationFinal/index.json，DefenseSeparation、TooltipLifecycle、Presentation、Hover、LayoutStability 等通过；唯一失败为已有 TooltipTitleColor（期望品质色，现有标题固定白色），未放宽该断言。保留工具链非首选版本与引擎弃用警告。
- **真实单机 PIE 40/40 通过**：Saved/TooltipDefenseFix/pie.json、pie.log。32 类型定向 + 8 正常掉落，检查死亡/掉落/拾取/Tooltip、基础防御行数量与不混入词缀、可用装备穿脱、未开放类型拒绝穿戴及实例保留。nullrhi/nosound，未验证人工视觉、悬停高亮遮挡、声音、多人或打包；日志仍有已知启动 AutomationTest 条件错误，不能称全日志无错误。
- Source/Docs/Scripts 的 diff 空白检查通过；所改 Profile 沿用 .uasset LFS 规则。保留先前未提交代码/资产，无提交/推送。
# Umbra 进度与验证状态

## 剩余150件接入与特殊装备规则 V2（2026-11-03）

- [全类型掉率、词条与盾/远程/施法规则](RandomEquipmentExpansion.md)：580件全部投放（204近战、62盾、24远程、6施法器、284防具）。新增150件中58件近战可走现有穿脱/GAS，另外92件可掉落拾取且显示词条，按“先规划战斗规则”暂不开放穿戴；服务端门禁在库存/GE修改前拒绝并返回明确反馈。
- 武器40%/防具60%不变。武器内部近战70%、盾15%、远程10%、施法器5%，28个具体武器类型逐项设计权重，4防具部位各占总装备15%；不从资产数量推导类型概率。名称、分类、Requirements/RequiredLevel原样保留。纠正此前V1汇总笔误：430件实际146武器+284防具，而非153+277。
- UE5.8.2创建V2独立625包（580定义及规则/战斗配置），位于`/Game/Items/GeneratedEquipment/V2`；首次清单`Saved/RandomEquipmentV2/initial-apply.json`。新进程回读580件及重复生产零保存，`apply.json`。近战敌人BP已改用V2，L_Prototype四个敌人均继承正确表，本轮激活仅保存BP，没有再次保存地图。
- **UmbraEditor Win64 Development构建通过**，`Saved/RandomEquipmentV2/build.log`。**自动化27/28通过**（25成功、2带警告、1失败），`Automation/index.json`；ProductionAssets验证580模板每件128次，共74240次生成；ProductionLoot验证资产数量不影响类型选择；新增StagedEquipment验证拒绝穿戴不创建GE、不改变背包Revision及明确Tooltip状态。唯一失败仍是原有TooltipTitleColor白色实现与品质色断言不一致，未放宽断言。
- **单机PIE 40/40通过**：32类型定向覆盖+8次正常随机，真实Health归零/死亡/掉落/拾取；可用装备穿脱、完整实例和Tooltip保持，特殊类型穿戴拒绝且物品留包。证据`Saved/RandomEquipmentV2/pie.json`、`pie.log`。测试夹具使用临时400cm散落区域，不保存地图，不改变正式落点参数。
- 生产脚本默认V2，显式`-Config`可选择V1；原Library与V1共9900个包SHA256保持不变，`preservation.json`。新资产符合既有LFS规则，原有未提交修改保留，没有提交/推送。
- **未验证/未实现**：格挡、远程投射物/装填、施法器技能行为仍仅为文档规则，专属动作/攻击距离/双手占用未实现；本轮未做人工渲染/声音、多人PIE、Cook/打包或大规模加载性能验证。PIE使用nullrhi/nosound，启动日志既有AutomationTest条件错误/第三方蓝图输入警告仍存在，不宣称整个项目日志无错误。

## 导入资产随机装备 V1（2026-11-03）

- [规则、配置与工具](RandomEquipmentProduction.md)：新模式按 1/2/3/4 条词条确定普通/魔法/稀有/史诗；默认数量权重 50/30/15/5。保留旧品质先行算法，传奇威能和独特专用生成尚未实现，正式表不投放这两类。
- 分层掉落：武器40%/防具60%，10种近战武器各占总装备4%，头/胸/手/腿各15%；类型内模板等权，新增模型不增加类型总概率。当前原型每次死亡尝试掉1件，落点失败仍可能没有实物。数量、权重和基础战斗值均有独立配置来源。
- UE5.8.2实际创建430件独立玩法定义（153武器、277防具），总计455个新资产包，位于`/Game/Items/GeneratedEquipment/V1`。读取现有名称/分类/图标/模型，四维Requirements与RequiredLevel原样保留；原型基础伤害/防御另配，不导入原游戏攻击数值或补正系数。150件尚无V1玩法调参的草稿明确排除。
- 已将`BP_Enemy_Melee_01`及L_Prototype里两个原先显式使用Armor1730表的敌人接入新表；通过编辑器保存BP和地图，其他测试夹具不改。写入前引用见`Saved/RandomEquipment/initial-activation.json`。
- **构建通过**：UmbraEditor Win64 Development，`Saved/RandomEquipmentBuild.log`；VS工具链非首选版本警告仍存在。初次受限运行因本机构建/缓存权限退出，使用允许访问引擎缓存的执行方式后完成构建及编辑器验证。
- **相关自动化26/27通过**（24成功、2带警告、1失败）：`Saved/RandomEquipment/Automation/index.json`。范围Umbra.Items、Inventory、Equipment、UI.Items、Loot.DropPhysics。新ProductionLoot与ProductionAssets均通过，430模板每件128次，共55040次生成；29:1模板数量下2000次类型选择序列不变。旧TooltipTitleColor仍失败：现有代码固定白色，而断言期待品质色；本轮未修改该UI代码或放宽断言。
- **真实单机PIE 22/22通过**：14类型定向覆盖+8次正常表选择，Health归零→死亡→分层掉落→拾取→穿戴→卸下，完整实例与Tooltip品质校验通过。`Saved/RandomEquipment/pie.json`、`pie-final.log`。首次同位置/原半径22次中1次无合法落点，测试改用分散位置/400cm临时散落半径后通过；正式落点算法与半径未改。
- **独立进程回读及重复执行零保存**：`Saved/RandomEquipment/repeat.json`；正式`PrepareRandomEquipment.ps1 -Apply -Activate`入口再次完成，创建/激活均零保存（最新apply.json/activation.json），首次455包清单保留于initial-apply.json。原有9445个Library包SHA256全不变，`library-preservation.json`；新资产使用既有Git LFS规则。本轮文档本地链接及diff空白检查通过。原先未提交的分类、属性需求脚本/代码/资产修改全部保留，没有提交或推送。
- **未验证**：430件逐项人工材质/图标/落地外观、鼠标交互和耳听、角色穿戴外观、对应武器动作/投射物、多人PIE/独立客户端、Cook/打包与大规模加载性能。本轮PIE为nullrhi/nosound；启动日志仍有既有AutomationTest条件错误、第三方蓝图输入警告，不能称整个项目日志无错误。传奇/独特、连续数值采样、技能/存档不在本轮实现范围。

## 导入武器正式属性需求及补正来源（2026-11-03）

- 296份Library武器定义采用源力量/敏捷/智力/信仰需求作为正式Requirements；不引入感应属性，不配置伤害系数或曲线。完整补正原值和相关参数引用另存SourceScaling元数据，供未来曲线制作。新导入与分类补建入口同步采用规则。
- 预检无手工冲突，295份数值更新、1份数值一致，296份保存需求基线和来源元数据。已有手工需求与基线不一致时保留并报冲突；等级、模型、材质、战斗配置、装备槽不改。
- 55项Python测试通过，包含需求与补正字段区分、弹药排除、手工保护和补正原值/引用保真。实际UE执行报告在`Saved/EquipmentImport/WeaponRequirements/`。本次没有C++更改，未重编译或运行PIE；不宣称新增玩法链路完成验收。
- 独立UE进程回读296份全部通过，重复执行零保存。打刀实测Requirements为力量11/敏捷15/智力0/信仰0，SourceScaling保留力量36/敏捷55及引用编号，未赋Umbra伤害曲线。

## 装备分类补齐与弹药排除（2026-11-03）

- 追加23种展示武器类型及中文名，旧枚举0–14不变；补齐对应法环类别映射。箭/大箭/弩箭/弩炮箭不属于本项目装备定义范围，显式排除，12件已有弹药模型/icon保留。详见[批量导入分类说明](EquipmentBulkImport.md)。
- 通过UE补建127份缺失分类草稿，复用已有模型/icon，不设置战斗、穿戴槽或生成配置。新进程127件回读与复跑通过、零保存；正式库现有580份展示草稿。报告`Saved/EquipmentImport/ClassificationBackfill/`。
- UmbraEditor Win64 Development（UE5.8.2）构建成功；52项Python回归通过。构建暴露既有Unity文件内Nonnegative辅助函数重名，将装备模块局部函数改为IsEquipmentNonnegative，逻辑不变。保留编译器非首选版本及引擎弃用警告。未做PIE、掉落物理或穿戴测试。
- 最终UE自动化`Umbra.Items.ImportedClassification`和`Umbra.Items.Tooltip.Classification`均通过（2成功、0失败），报告`Saved/Automation/ImportedClassificationFinal/index.json`；覆盖新增中文类型、旧数值兼容、无效类型回退及分类不生成战斗配置。

## 装备全量队列入口（2026-11-02）

- [ImportAllEquipment.ps1](../Scripts/EquipmentImport/ImportAllEquipment.ps1)提供一次启动、固定清单、顺序分组导入/回读和断点续跑。失败材质单独记录、不循环重试；环境故障停止并保留待处理项。细节见[批量操作教程](EquipmentBulkImport.md)。
- 51项Python回归通过（含模拟中断/恢复/完成后零启动）；实际PowerShell Dry Run固定剩余857件，29批/143组。默认队列Equipment_All_001，清单与状态位于Saved，不入Git；正式目标为Library。
- 本次只交付脚本并生成计划，未启动全量UE导入，未运行C++构建或PIE。此前Equipment_001真实导入验证继续有效，但不等同于新总队列已全量验收。

## 首批正式装备资源导入（2026-11-02）

- 新增显式Library目标，默认Staging行为不变；模型、材质、icon与展示草稿统一使用清单中的目标位置。Library可进入Git/LFS，不依赖被忽略的测试目录。正式入口见[批量导入](EquipmentBulkImport.md)。
- Equipment_001按每进程6件处理30件：24件资源成功、214个资产包，其中13份展示草稿、11件现有类别尚未映射的仅资源；6件特殊材质隔离（鳞甲、黑王大剑、诺克斯流体剑、铁头盔、蒙格温圣矛、皮革长裤）。资产位于`/Game/EquipmentImport/Library/Equipment_001`。
- 最终45项Python测试通过；独立UE进程24件回读全部通过，检查模型、骨架、材质/父材质/贴图、icon和草稿引用均在本批目录，以及实际纹理设置和参数。未做本批逐件视觉验收、PIE、掉落物理、穿戴验证或C++编译；没有提交或推送。
- 本机结果：`Saved/EquipmentImport/Bulk/Bulk_9e73885b/report.json`、`report.md`、`verification.json`、6项`retry.json`。排除本批所有已尝试模型后，第二批30件计划已生成于`Saved/EquipmentImport/Bulk/Equipment_002_39cc5630/selection.json`，尚未执行；续跑使用该固定清单，不能重新从第一条自动选样。

## 装备批量管线修正（2026-11-02）

- [正式入口、边界与实测](EquipmentBulkImport.md)：一键选样/Dry Run、分块执行、失败隔离、缓存与增量重试；补齐MATBIN精确默认引用、共享贴图、主/细节法线区分、裁切/双面规则，修复布料次级POSITION误追加及多静态部件WorldDrop导入。
- 最终BatchReady42_03实际42件：33件资源成功（23份展示草稿、10件分类待映射的仅资源），9件特殊/常量材质隔离；保存311包。新UE进程33件回读通过，7件14张高清预览生成、进程正常退出。基础外观不等于完整原版着色器还原。
- 成功33件复跑保存0、失败0；6938个既有资产包、42件源模型/icon、311个本批资产包哈希不变。最终44项Python回归通过；材质索引辅助C#工具正式入口构建成功且索引重建一致。
- 未验证PIE、穿戴、掉落物理或Unreal C++构建。预览启动阶段仍有13条AutomationTest条件错误（在Python运行前），未定位；不宣称项目整体日志无错误。详见上述实测文档和本机Saved报告。

## 18件跨类型导入测试（2026-11-01）

- [逐项结果与限制](EquipmentBatchCoverage18.md)：18件均满足模型/icon存在门槛；7件成功导入，6份草稿+1件盾资源，保存68包。11件因模板/Alpha/detail法线/贴图命名被拒绝；不能声称当前规则覆盖大多数装备。
- 两个新UE进程回读7件通过，生成28张光照/BaseColor预览；33项Python回归通过；6870个既有资产包导入后哈希零变化。未运行PIE/穿戴/原版外观还原验收。

## 普通模板直接法线（2026-11-01）

- [匕首验证与默认规则](DaggerNormalValidation.md)：_n原PNG以Normalmap/sRGB=false接入，真实帧渲染对比观察到细节响应。普通metal/leather/fabric/belt模板采用RG约定与引用/缩放/保真检查；未知B/A及_m不接线。
- ItemNormalAuto01默认一站式成功19包，三处法线绑定回读通过，重跑零保存；33项Python测试通过。未声称原版逐像素还原或全库逐件视觉验收。

## 小批物品一站式导入（2026-11-01）

- [入口与验证](EquipmentItemImport.md)：ImportItems.ps1 接通源资料门槛、DCX/Blender、UE 模型材质、UI icon、独立 ItemDefinition 展示草稿。匕首+旅行手套共18包成功，既有配置不覆盖，不自动赋战斗/生成配置。
- 实际RHI材质编译与新进程回读通过；重跑零保存，85个包哈希不变。27项Python测试及4项实际输入负向检查通过。
- 未验收：最终材质外观、匕首特殊法线、PIE/掉落物理/穿戴适配；1140仍为限定候选规则，不能推广全库。

## 装备源资料关联（2026-11-01）

- 新增模型+icon同时存在且对应明确的资源入选门槛：3391条入选、1083条排除；6项测试通过。多模型变体待选择、缺失或重复icon不进入候选清单。只验证源文件与编号，未执行UE导入。

- [使用与验证](EquipmentItemCatalog.md)：BuildItemCatalog.ps1 只读两张 CSV、中文 JSON、icon 和 parts，输出源物品目录、资源去重索引及异常报告；不写 UE/ItemDefinition。
- 实测 4474 条记录，4151 条匹配名称、4308 条找到模型候选、3623 条所需 icon 按编号找到。5 项边界测试通过；全量重复构建结果一致、输入哈希未变、建议枚举与当前头文件一致。
- 待确认：数据版本一致性、icon 视觉对应、男女/骨骼适配；UE icon/资料导入及 ItemDefinition 补全尚未实现。本次未编译 C++ 或运行 PIE。

## AM_M_1140 材质通道审计（2026-11-01）

- [证据、候选规则与复验](Material1140Audit.md)：读取实际Test01确认仅BaseColor接线；DDS仅_a/_n，没有_m。新增单模板+源哈希限定的RG法线重建及1-B粗糙度候选，输出到Material1140Review，默认规则和Test01不改。
- Blender通道转换7测试、纯Python回归21测试通过；实际候选7包导入、回读配置、重复零保存与Test01哈希保护通过。视觉、绿色方向、Alpha用途和原版材质还原未验收，不能推广为所有法环贴图规则。
- 显式AllowCommandletRendering后D3D12/SM6实际编译通过，0编译错误、4采样器；原Test01和候选包哈希保持不变。该结果不是人工视觉验收。

## DCX源资源导入入口（2026-11-01）

- [流程与当前限制](EquipmentDcxImport.md)：ImportDcx.ps1接通原DCX→Witchy→Blender插件→FBX/PNG→UE，默认只列计划，Apply执行，缓存与失败报告在Saved。
- 新武器wp_a_0100（两个模型）与新防具am_m_1130端到端成功；后者重复运行缓存复用、UE零保存；19项Python测试通过。am_m_1140仅转换通过；四种贴图/Alpha/特殊材质异常真实触发并保留失败报告。
- 未运行全部3046包、GPU/PIE视觉或穿戴验收。未知法线/遮罩不连接，Hair/crystal等特殊模板不自动猜测；规则扩展仍需验证。未接Icon/params或修改Definition。

## 模型与材质批导入 M1（2026-10-31）

- [流程、配置和验收](EquipmentAssetImportM1.md)：独立 EquipmentImport 脚本，FBX/PNG、三类基础Master/MI、精确槽绑定、隔离输出与增量保护；不改Icon/Definition。
- UE 5.8.2六件真实样品导入23包、三个材质探针15包；独立重载与重复零写入、手改保留、冲突拒绝通过；标准库13测试通过。最终目录M1Validated/M1Probes，早期M1Samples失败诊断包不作交付样品。
- NullRHI验证不代表视觉验收；源FBX绑定姿势/平滑组有告警。未执行Blender、PIE视觉、穿戴适配、GPU着色或生产规模测试；未改C++，未重新编译。本次止于M1，DDS/特殊编码转换和Icon参数匹配未实现。

## 六品质、掉落音效与客户端Tooltip修复（2026-10-31）

- [修复契约和复验步骤](SixRarityRepair.md)：Common白、Magic绿、Rare蓝、Epic深紫、Legendary玫红、Unique暗金。生成校验支持全部六品质；RandomMVP扩为六行、规则版本2。实际实例品质驱动标题、品质文字、槽位背景/边框和Tooltip背景；物品图标本体不染色。
- UE保存6个固定定义和2个测试生成配置，清单在 `Saved/SixRarityMigration.json`；音效库迁移前后SHA256相同。保留既有地图、音效和其他未提交修改，本轮未保存地图、未提交/推送。
- 修复客户端时钟暂不可用时过早消耗生成音、落稳状态导致新物品静音、不同品质共享全局冷却互相压制。仍保留同品质冷却和超过1秒历史掉落不补播规则。
- **UE 5.8.2 UmbraEditor Win64 Development编译通过**，`Saved/Logs/SixRarityBuild.log`。**相关自动化22/22通过**（20成功、2带警告、0失败），`Saved/Automation/SixRarityFinal/index.json`；范围 Umbra.Audio、Umbra.Items、Umbra.UI.Items、Umbra.Loot.DropPhysics。旧标题固定白色断言按本轮品质色要求更新后重跑通过。未重跑旧Armor1730Definitions，不宣称历史差异已解决。
- **真实Health归零→敌人死亡→掉落单机PIE通过**：六品质各1件，逐件拾取和Tooltip品质/颜色/背景开关校验通过，`Saved/SixRarityDeathPIE.json`。`Saved/SixRarityAudioDispatch.json`按Actor逐件匹配日志，六件各有且仅有1次LootSpawn，分别使用库中六种正确音源；同帧掉落均触发。脚本首次受UE Python接口限制及Guid包装对象比较影响，修正测试适配后最终通过；以最终报告为准。
- **专用服务器+客户端网络PIE通过**：3个World、100ms延迟/10%丢包；远端背包和装备Tooltip均有效，详细需求保持未知，已装备需求状态来自服务器。复制、穿戴/卸装/移动RPC、实际GE伤害快照同轮通过。证据 `Saved/TooltipNetworkDedicated.json`、`Saved/Logs/TooltipAfter.log`；修复前 `TooltipBefore.log`实际复现 ContextUnavailable。
- **未验证**：真实鼠标悬停视觉、六色屏幕观感、耳听音量/设备输出、打包独立客户端。死亡PIE启用音频但使用nullrhi，日志只证明播放API请求；网络PIE使用nullrhi/nosound。当前测试不替代人工视听验收。

## M5 批量资产管线：核心实现与样品闭环已验证（2026-10-30）

- [管线契约与运行方法](EquipmentPipelineM5.md)：清单驱动来源/hash/类型/生成池校验，默认 dry-run/CreateMissingOnly；UpdateManagedFields 以同包基线进行三方比较，保留锁定字段，冲突全批停止，支持选中ID、失败报告和已保存包续跑。未新增运行时全库加载。
- 本轮 UE 5.8.2 UmbraEditor Win64 Development **编译通过**，日志 EquipmentM5Build.log；未改模块/Target。新增 TryFitWorldDropPhysics 可观测拟合成功/失败，保留原 Blueprint void 入口。首次编译发现不可达返回语句，已修复。
- **9/9 Python测试通过**：源hash/路径/来源、预检不写、重复零写、手改/锁/选中ID、提交后恢复。**Umbra.Loot.DropPhysics 1/1通过，无警告**，证据 Saved/Automation/EquipmentM5/index.json。本轮未重跑M4全套37项；旧 Armor1730Definitions 差异没有修改，不宣称已解决。
- UE 实际导入保存 10定义+10静态网格+10透明图标，另一个测试生成器，共31包：/Game/Items/Tests/PipelineM5。全部 .uasset 和 PNG 使用既有 LFS 属性；OBJ 是自有文本几何且已从编译器 .obj 忽略规则中精确豁免。来源 Scripts/EquipmentPipeline/PROVENANCE.md；样品不是正式美术。
- 初次导入遇到 DataAssetFactory Python 属性访问错误，仅保存2个依赖包；修正后新进程复用依赖并完成其余28包。Saved/EquipmentPipeline/apply.json 与 apply-resumed.json 记录这次真实恢复。未覆盖已有导入。
- 独立编辑器重载、原生DataValidation、物理拟合参数和注册表依赖通过；两种模式再次执行0保存且包哈希不变。Saved/EquipmentPipeline/audit.json、CreateMissingOnly.json、UpdateManagedFields.json；Saved/Logs/EquipmentM5Verify.log。最初30包218682字节，首次独立验证进程峰值物理内存1734MB含整个编辑器，暖加载计时不能当作冷加载或运行时性能。
- **单机PIE 10/10模板通过**：Saved/EquipmentM5PIESmoke.json、Saved/Logs/EquipmentM5PIE.log。真实拾取→穿戴→卸装，完整实例字段保持；显式设单机并恢复原播放设置，干净EndPIE。仅临时放置Actor，不保存地图。运行使用nullrhi/nosound，未验收真实视觉/音效。
- 最终编辑器适配器验收通过：临时手改重量触发冲突且0保存；单ID修改显示名仅保存1包，通过管线恢复原值后全清单再次0保存。证据 manual-conflict.json、selected-update.json、selected-restore.json、final-noop.json。最终 audit.json 包含31包244531字节，验证进程峰值1702MB；这些文件位于 Saved/EquipmentPipeline。
- **Windows Cook通过，退出0、0错误/0警告**：Saved/Logs/EquipmentM5Cook.log，334.41秒；614包烘焙、7包平台跳过，共621包，包含引擎/插件公共资源；峰值物理内存4227MB。原生 AssetRegistryDump 解码 DevelopmentAssetRegistry.bin 后验证31个样品包及武器/生成配置直接引用均有非零Cook包数据，Saved/EquipmentPipeline/cook-audit.json 与 CookRegistryFirst/AssetRegistry。不是只Cook30包，也不是打包客户端运行验收。Cook后进行过显示名更新/恢复测试，内容恢复但编辑器包序列化字节不保证与Cook前完全相同。
- 未验证：正式美术质量/真实UI可读性、骨骼FBX/材质槽分支、Blender实际运行（未安装）、穿戴外观、打包独立客户端冷加载和大规模性能。未更改正式地图/掉落表；未提交/推送，用户已有改动保留。M6尚未开始。

## 随机装备 M4：实现、真实网络 PIE 核心闭环已验证（2026-10-30）

- 公共池支持 Damage A/X，沿用原条件过滤和 TypedExecution。装备 GE 使用可复制 `FUmbraEquipmentEffectContext` 保存伤害快照；旧内嵌随机伤害和公共池伤害统一读取，不再根据当前模板重建随机条件。每件装备仍只持有一个 GE，卸下准确撤销。配置/序列化边界见 [M4实施说明](RandomEquipmentM4.md)。
- 拥有者 Controller 新增穿戴/卸装/移动 Reliable RPC。请求仅含 GUID、槽位、操作和预期 Revision；服务端固定操作自身 PlayerState，校验请求序号、所有权与快照版本，复用原事务。客户端一次一个挂起请求，按请求号处理反馈，不预测修改背包；菜单与拖拽已接入。
- **最终编译通过**：UE 5.8.2 UmbraEditor Win64 Development，`Saved/Logs/RandomM4Build.log`；保留既有引擎弃用/非首选编译器警告。未改模块依赖或 Target。
- **最终相关自动化36/37通过**：32无警告、4带警告、1失败；`Saved/Automation/RandomM4Verified/index.json`、`Saved/Logs/RandomM4Verified.log`。新增 `Umbra.Items.Generation.DamageSnapshot`、`Umbra.Inventory.RequestProtocol` 通过，覆盖位流往返（含条件TagQuery）、深拷贝、真实扣血、模板变更隔离、单句柄/撤回、重复/过期/越权请求。旧内嵌 A/X、转移失败回滚与UI回归通过。
- **唯一保留失败仍为 `Umbra.Loot.Armor1730Definitions`**：和M0/M1–M3相同的5条资产固定值差异；未覆盖用户资产或放宽旧断言。
- **Listen Server + 远端客户端通过**：`Saved/RandomM4NetworkListen.json`、`Saved/Logs/RandomM4NetworkListen.log`，2个PIE世界。实际 `PktLag=100`、`PktLoss=10` 设置有引擎日志；服务器拾取后完整实例复制到拥有者，远端穿戴RPC成功，客户端实际活动GE收到2条伤害快照；远端卸装后快照移除，移动至第23格并保持全部实例字段。
- **编辑器内专用服务器 + 客户端通过**：`Saved/RandomM4NetworkDedicated.json`、`Saved/Logs/RandomM4NetworkDedicated.log`，3个PIE世界，同样100ms延迟/10%丢包，以上闭环通过。属于同进程多World网络PIE，不能称为打包独立服务器或跨机器测试。
- 网络脚本为 [smoke_random_m4_network.py](../Scripts/smoke_random_m4_network.py)。首次运行发现编辑器设置未导出到Python，且Python执行保护将Actor RPC强制本地；新增Tests下适配器恢复真实网络callspace并在结束还原播放设置。生产权限检查未放宽。仅最终两个JSON作为通过证据。
- **6个独立M4资产**：`/Game/Items/Tests/RandomM4`，UE保存、LFS属性确认。创建日志 `Saved/Logs/RandomM4Assets.log`；ItemDefinition原生DataValidation最终退出0，`Saved/Logs/RandomM4Validation.log`。原模板、正式地图、正式掉落表与Config无新增改动；用户已有音频及拖拽修改保留。未提交/推送。
- 自动审批曾因服务用量限制中断一次最终回归启动，该命令当时未执行；用户要求继续后审批成功，最终回归和专用服务器PIE已完成。
- **未验证/待人工验收**：真实视听和鼠标菜单操作、双玩家实际拾取RPC竞争、网络满包失败场景、断线重连/晚相关、打包独立服务器与跨机器。原生测试覆盖事务满包和同物品重复拾取，但不能替代以上网络场景。运行采用 `-nullrhi -nosound`；M4核心实现已交付，路线图人工验收未全部关闭，M5当前状态见顶部条目。

## 随机装备 M1–M3：实现与单机自动验证已交付，人工验收待完成（2026-10-30）

- M1：版本化实例携带稳定定义 ID、等级、品质、seed、规则版本及完整词缀；背包/装备统一 SetInstance/GetInstance；保留 Blueprint 旧字段投影、版本0适配与旧固定装备入口。内存 DTO 序列化往返已验证，不包含完整存档系统。
- M2：新增 AffixDefinition、AffixPool、GenerationProfile、Context 和确定性生成器。Primary/Attribute 公共池支持等级档、三品质权重、分类/槽位/品质/Tags 过滤、语义去重、互斥组、候选值和失败原子性；编辑器模板校验已接入。详见 [配置与实施契约](RandomEquipmentMVP.md)。
- M3：接入敌人/测试生成器；Tooltip、世界展示、背包/装备与音频上下文读取实例品质。UE 5.8.2 创建并保存 `/Game/Items/Tests/RandomMVP` 下11个独立资产，复用剑/胸甲模型，不改原模板、地图或正式掉落表。测试资产使用 LFS 规则；未提交或推送，用户原有音频/拖拽等改动保留。
- **编译通过**：UmbraEditor Win64 Development，`Saved/Logs/RandomM123Build.log`。仍有既有引擎弃用/编译器非首选警告。
- **自动化34/35通过**：29无警告通过、5带警告通过、1失败。报告 `Saved/Automation/RandomM123Verified/index.json`，日志 `Saved/Logs/RandomM123Verified.log`。范围与M0相同，新增 `Umbra.Items.Generation.Rules`、`Umbra.Items.Generation.Lifecycle` 均通过，覆盖固定seed/排序不变、权重/过滤/互斥错误、DTO往返、真实TryPickup、GAS应用/撤回、完整metadata转移及展示。不将退出码0当作全绿。
- **唯一保留失败**：`Umbra.Loot.Armor1730Definitions`，仍为M0已有的5条资产固定值断言差异；未通过覆盖用户资产或放宽断言消除它。临时World无Context与现有UI警告有记录，未成为新增测试失败。
- **单机 headless PIE 通过**：`Saved/RandomMVPPIESmoke.json`、`Saved/Logs/RandomMVPPIE.log`。真实 L_Prototype 临时生成12件（6 Common、5 Magic、1 Rare；剑/胸甲各6件），逐件 TryPickup→穿戴→卸装，逐字段保持 ID/版本/等级/品质/seed/档位/候选/掷值，GUID唯一，正常 EndPIE，未保存地图。脚本 [smoke_random_mvp_pie.py](../Scripts/smoke_random_mvp_pie.py)。
- **原生资产校验通过**：DataValidation 对8个注册 ItemDefinition 执行校验，包含两个新模板及其池/Profile；命令结果0，日志 `Saved/Logs/RandomMVPValidation.log`。资产创建日志 `Saved/Logs/RandomMVPAssets.log`。
- 首轮验证修复了旧固定 Equip 版本0与背包版本1冲突，以及测试代码数组自引用、Python GUID比较问题；以上最终报告已重跑，不采用失败轮次充当通过证据。构建/运行日志时间为执行主机2026-10-29；本条按客户端2026-10-30收尾。
- **尚未验证**：真实渲染中的品质框/Tooltip视觉、实际听感、人工鼠标导航拾取/拖拽/菜单重开、双端网络、打包。当前自动化/PIE使用 `-nullrhi -nosound`；M3人工验收不标通过。M4公共Damage池与多人装备生命周期未开始。

## 随机装备 M0：源码基线已建立，资产基线与人工验收待确认（2026-10-29）

- 新增 [UmbraRandomEquipmentBaselineTests.cpp](../Source/Umbra/Tests/UmbraRandomEquipmentBaselineTests.cpp)：`Umbra.Items.RandomBaseline.Creation`、`Umbra.Items.RandomBaseline.TransferAndEffects`。覆盖5/6档/显式候选、整数重复与非法范围、权限/GUID/重复词缀拒绝、完整payload比较、世界实例接收、Tooltip实际值、同GUID篡改拒绝、移动/穿脱/重注册不重掷、低等级/错槽/满包/GE拒绝、双向中途失败回滚、随机Primary/Attribute/A/X真实生效及卸下恢复。
- 兼容约束见 [ItemInstanceAffixes 的 M0 节](ItemInstanceAffixes.md#m0-基线契约2026-10-29)：未来版本适配保留既有掷值；不改变SelectedTier语义。本轮未新增实例字段、词缀池、生成规则、存档或平衡数值；M1未开始。
- 修正测试与现实现的差异：Tooltip普通词缀改用AffixEntry，X单独展示；Hover测试改读菜单持有的Tooltip，并提供无界面视口容器；非法武器范围日志列为精确预期错误；DropPhysics复制夹具显式DispatchBeginPlay，使音频生命周期断言实际运行。未修改这些模块的生产玩法逻辑。
- 构建前发现已有拖拽修改使两个槽位cpp不再首先include自身头文件；本轮仅调整 `UmbraInventorySlot.cpp` / `UmbraEquipmentSlotWidget.cpp` 的前两行顺序，保留用户其他修改。
- **编译通过**：Launcher安装记录确认UE 5.8.2，UmbraEditor Win64 Development；日志 `Saved/Logs/RandomM0Build.log`。保留编译器版本非首选、引擎弃用API警告。初次沙箱构建/DDC访问失败，获准系统环境后执行成功；中途自动审批服务用量限制曾阻止一次构建，用户要求继续后重试成功。
- **最终自动化32/33通过**（29项无警告通过、3项带警告通过，1项失败、0未运行）：报告 `Saved/Automation/RandomM0Verified/index.json`，日志 `Saved/Logs/RandomM0Verified.log`。覆盖 `Umbra.Items`、`Umbra.Equipment`、`Umbra.Inventory`、`Umbra.Loot`、`Umbra.Damage`、`Umbra.UI.Items`、`Umbra.UI.Equipment`、`Umbra.Attributes.WeaponDerivedPower`。不能按进程退出码0误称全绿；以JSON测试状态为准。新随机生命周期测试有一次临时World无Context的DestroyActor警告，未发生测试错误；另外两项带警告为HoverAndLabelLayout和PageVisibility。
- **保留失败：`Umbra.Loot.Armor1730Definitions`**。UE实际只读加载了现有四件资产；其中两件不满足“No affixes”，三件某主属性需求实际为10而测试预期0，共5条失败断言。旧测试将2026-10-20的初始测试数值当固定契约，不能据此认定当前资产错误；本轮不改资产、不删除断言来掩盖差异。下一步在编辑器逐件核对Head/Chest/Hands/Legs与Hand/Hands引用，确认哪些为正式调参、哪些仍是固定测试夹具；随后另立变更将固定数值测试迁到专用夹具或更新经确认的资产契约。
- 新测试初版曾因数组自引用追加触发断言，已修正；首轮相关回归也暴露旧Tooltip强制转换崩溃，已改为真实AffixEntry并通过最终运行。此前失败日志不作为最终验证结果。
- **未验证**：人工PIE鼠标/导航拾取、真实WBP视觉/音效、双端网络、角色外观、打包和存档。此次用 `-nullrhi -nosound` 跑原生自动化；随机世界转移测试调用PlayerState接收边界，不代替完整TryPickup RPC。未创建测试地图、未执行资产保存/重导入脚本、未修改Content、未提交或推送。M0源码基线与兼容文档已交付，资产基线和路线图中的人工验收仍待完成。

## 敌人攻击 Swing（2026-10-27，配置已保存）

- UE 5.8.2 保存 `AM_Enemy_BasicAttack`：新增 Audio 轨唯一 AttackSwing Notify（0.33909455秒），使用默认库 swordswing_fast_01；原命中窗口及底层动画保持不变。备份及配置方法见 [音频指南](Audio/AudioSetupGuide.md)。
- 独立进程资产重载检查与最小 PIE 通过：真实敌人音频组件、1秒攻击蒙太奇启动、5秒 PIE 正常结束。报告 `Saved/EnemySwingPIE.json`，日志 `Saved/Logs/EnemySwingPIE.log`。目标资产 LFS 属性已确认。此次仅资产配置和脚本/文档，无新增 C++ 改动，未重新编译。
- 未验证：实际听感、视觉同步、自然 AI 攻击全过程、多人和打包。使用 -nullrhi -nosound；Output Log 存在既有输入轴、动画除零、导航等警告。前述 Hurt/CriticalVoice 工作仍保持其单独待完成状态。
## 玩家 Hurt 匹配与暴击语音（2026-10-27，待关闭编辑器完成）

- 已用 UE 5.8.2 Python commandlet 只读核实：默认库 Hurt 行要求 Player.Greystone 且已引用 Greystone_Effort_Pain，但玩家 BP AudioEvents.CharacterType=None；现有 Critical 行引用 critical_hit。日志 `Saved/Logs/InspectCharacterAudio.log`。
- C++ 已追加 CriticalVoice，基于同次 GAS 暴击结果从攻击者位置播放。准备 `Scripts/configure_character_audio.py` 修正玩家类型并配置 Greystone_Ability_ScoredCrit（每角色2秒冷却），脚本尚未执行、Content 尚未保存。配置与 PIE 验证见 [音频指南](Audio/AudioSetupGuide.md)。
- 编译未通过：初次沙箱 UBA 系统缓存写入被拒，获准系统环境重试后被活动 Live Coding 锁阻止（`Saved/Logs/HurtCriticalBuild.log`）。已请求用户保存并关闭编辑器；本轮自动化、PIE、声音听感、网络均未验证。两项目标资产的 LFS 属性已确认；未提交或推送，保留已有工作区修改。

## 掉落稀有度环境音（2026-10-26）

- LootSpawn 继续使用真实物品稀有度匹配，为六品质生成提示补充独立配置步骤；LootRarity 改为落稳后跟随物品生命周期的3D循环环境音，支持旧掉落/晚到复制，拾取及销毁清理。独立并发默认4声，配置有效音源后低频恢复被抢占的播放；不改变掉落物理或拾取权威流程。
- 素材需在编辑器设置循环并配置 DataAsset；未改用户 Content 或配置。详细步骤及 PIE 清单见 [音频指南](Audio/AudioSetupGuide.md)。新增映射和生命周期断言，但编译被 Live Coding 阻止，自动化及 PIE 尚未验证；日志 `Saved/Logs/AudioLootLoopBuild.log`。此前音频测试通过记录不代表本轮验证。

## 槽位 Hover 静音（2026-10-26）

- InventorySlot 与 EquipmentSlotWidget 均移除 Hover 音效调用，空槽及有物品槽位都静音；原有高亮、选择、Tooltip 交互和普通按钮 Hover 保留。配置说明见 [音频指南](Audio/AudioSetupGuide.md)。
- UE 5.8.2 UmbraEditor Win64 Development 编译成功，日志 `Saved/Logs/AudioSlotHoverBuild.log`；有引擎 GetMovementBase 弃用警告。未进行本次 PIE 鼠标操作或听感验证，需检查两类槽位静音及普通按钮 Hover 正常。未修改用户音频资产或配置。

## 音频框架第一阶段（2026-10-24）

2026-10-25 交付复核：音频概率/变体/Pitch 改用独立 FRandomStream，避免影响玩法全局随机序列；UmbraEditor 再次构建成功，Umbra.Audio 两项重跑通过。日志 AudioBuildReview.log / AudioAutomationReview.log；本次未重复 PIE，听感与多人验证边界保持不变。

- [实际项目审计](Audio/AudioAudit.md)、[486项音频资产清单](Audio/AudioAssetInventory.md)、[32项素材需求与来源](Audio/AudioAssetRequirements.md)、[配置/手动Notify/PIE指南](Audio/AudioSetupGuide.md)。未新增或修改 Content；素材映射默认空。
- 新增事件/上下文、DataAsset、项目设置、无 Tick 本地播放策略、复制表现组件、蓝图 UI 入口与纯表现 AnimNotify。接入现有 UI、装备最终结果、拾取结果回传、掉落新鲜实例/真实接地、GAS 正数伤害/暴击/受伤/击杀/死亡。挥动/脚步待手动配置动画；格挡/闪避/环境及完整技能只预留，不重构业务。
- UE 5.8.2 UmbraEditor Win64 Development 与 Rider 文件刷新成功；首轮7项回归通过，最终修改后音频2项和真实掉落物理1项重跑通过。精确名称/日志及警告见配置指南验证节。
- 无资源原型 PIE 五秒启停通过，调用音频接口安全静默；使用 -nullrhi -nosound，未验证实际声音、鼠标操作、多客户端音频、动画接触帧与打包。Output Log 有既有动画除零/输入/导航等诊断，不能视为全项目无错误验收。

## 世界标签稳定布局（2026-10-21，本次修复）

- 修复无历史的逐帧最近点布局导致的位置竞争：保留目标/显示位置、预留历史位置、修复候选滞回、稳定距离分桶、DeltaTime指数平滑。悬停标签优先保持实际显示位置；扫掠路径可能交叉时整批安全瞬移，命中矩形始终取显示位置。Destroy即时清理缓存和Hover。
- 本次只修改 Controller 标签布局、现有布局助手、相关测试与文档；未修改物理、资产、碰撞、拾取RPC、背包或GAS。详细根因、参数来源及PIE步骤见 [WorldItemLabels](WorldItemLabels.md)。
- UE 5.8.2 UmbraEditor Win64 Development 构建成功：`Saved/Logs/StableLabelsBuild.log`。`Umbra.Loot.HoverAndLabelLayout`、`Umbra.UI.Items.TooltipTitleColor`、`Umbra.Loot.DropPhysics`、`Umbra.Inventory.Transfer` 全部通过（4项，0失败；标签测试有既有测试控制器输入配置警告）。报告 `Saved/Automation/StableLabels/index.json`，日志 `Saved/Logs/StableLabels.log`。
- 标签测试扩展到300帧密集静态、轻微相机扰动、双标签接近、悬停保持、DPI/视口坐标变化、销毁缓存清理、平滑无重叠及帧率独立性。真实鼠标、WidgetComponent显示时序、真实DPI切换、PIE画面与网络未验证；无界面测试不能替代这些验收。

## Tooltip 白色标题、防具表面悬停与标签避让（2026-10-21）

- Tooltip ItemName 固定白色，稀有度文字及其他字段不变。WorldItem 骨骼模型启用三角形 Visibility 查询；补充明确的查询体变换同步，防止引用姿势模型的查询体在无渲染/延迟创建渲染状态时停留在旧位置。物理仍由独立盒体负责。
- 本地 Controller 按实例ID稳定顺序排列标签，使用实际 DesiredSize、DPI单位和默认4单位间距，上下/左右避让并限制视口边缘；空间不足暂时隐藏，不强行叠字。可见标签矩形优先选择对应 WorldItem，模型/标签共用高亮和原拾取逻辑，不会点击标签却攻击背后的敌人。
- 已通过 UE 5.8.2 为 `Content/Asset/Equipments/{Head/head_01,Body/body,Arm/arm,Legs/legs}.uasset` 保存 Enable Per Poly Collision 数据；修改前备份在 `Saved/ArmorCursorCollision/`，脚本 `Scripts/enable_armor_cursor_collision.py`。本轮未保存物品定义、WBP或地图，保留前序与用户工作区改动；未提交/推送。配置/手工步骤见 [WorldItemLabels](WorldItemLabels.md)。
- **最终构建已验证**：UmbraEditor Win64 Development 成功，日志 `Saved/Logs/LootLabelBuild.log`。未改变模块/Target，无需重新生成项目文件；既有 MSVC/引擎弃用警告不变。
- **最终4项针对性自动化通过、0失败**：`Umbra.Loot.HoverAndLabelLayout`、`Umbra.UI.Items.TooltipTitleColor`、`Umbra.Loot.DropPhysics`、`Umbra.Inventory.Transfer`。3项无警告，HoverAndLabelLayout 带测试控制器未配置 PrimaryAction/PrimaryAttackAction/ToggleCharacterMenuAction 的3条警告，不是实际蓝图输入验收。报告 `Saved/Automation/LootLabelsVerified/index.json`，日志 `Saved/Logs/LootLabelsVerified.log`。覆盖20名称不相交、视口边界/空间不足、标签矩形到目标/销毁清理、四件真实防具Visibility表面及拾取球外命中/描边开关、标题白色/稀有度色/复用清空、物理和库存回归。
- **初轮问题与限制**：测试最初没有执行已加载资源的延迟完成回调；测试调整回调延迟并推进物理帧。源码追查到组件开关还依赖源模型三角形数据，且引擎 per-poly 更新依赖 MeshObject，已配置资产并补显式同步。原有 `TooltipHover` 综合测试未建立有效菜单 tooltip，`TooltipLifecycle` 存在网格/行元素的旧类型强转并中止测试（`LootLabelTests.log`、`LootLabelFinalTests.log`）；只修正了行容器应使用通用 Panel 的断言，不宣称这些综合测试通过。本轮新增独立颜色测试覆盖请求行为。
- **未验证**：真实 PIE 画面/鼠标输入、相机移动中标签观感、多种DPI、极端掉落量性能、真实多人和分屏。无界面射线、布局与目标解析测试不等同于这些验收。分屏的独立标签视图未实现。

## 掉落自然收敛与逐物品有向盒（2026-10-21 修订）

- 修复3秒硬超时突然放置/复位：旧 MaximumSimulationTime 序列化名保留，编辑器显示 Settling Start Time；现在仅对有静态支撑的物品在0.8秒内渐增阻尼，离地恢复基础阻尼。自然固定保留当时位置/旋转。异常回收仅用于低于安全地面2000cm或连续30秒无静态支撑，并重新确认安全地面。
- ItemDefinition 新增 WorldDropPhysicsRotation 和编辑器 Fit World Drop Physics 按钮；按模型本地轴拟合有向盒，避免斜放武器的轴对齐盒过大，根/模型逆变换保持初始外观。单盒不能完全表达锤柄、成对手甲的空隙，仍须视觉确认。
- 已通过匹配 UE 5.8.2 Python API 保存6件定义的 Extent/Center/Rotation：TestSword、TestMace、1730 Head/Chest/Hand/Legs。保留用户已有 Chest/Legs 修改，不更改外观变换、装备数值、需求或词缀。脚本 `Scripts/fit_drop_physics.py`；初始备份 `Saved/DropPhysicsFit/20261021_123055/`，最终报告 `Saved/DropPhysicsFit/20261021_123337/report.json`。所有6件资产仍由 Git LFS 管理。未修改地图；工作期间出现的 L_Prototype.umap 工作区改动保留，未回退。
- **本轮验证**：UE 5.8.2 UmbraEditor / Win64 / Development 构建成功（`Saved/Logs/DropSettlingBuild.log`）。最终 `Umbra.Loot.DropPhysics`、`Umbra.Loot.VisualMeshCompatibility`、`Umbra.Inventory.Transfer`、`Umbra.Equipment.RequirementsAndLifecycle` 全通过，0失败。报告 `Saved/Automation/DropSettlingFinal/index.json`，日志 `Saved/Logs/DropSettlingFinalTests.log`。新增断言验证软截止不移动/冻结物品、接地渐变与离地复原、异常掉出回收、斜放薄模型的紧凑盒尺寸和外观变换保持。
- **未验证**：实际 PIE 画面、各资源接触姿态、用户鼠标拾取、真实多人/专服延迟。沿用上一轮旧防具数据契约测试的已知差异，本轮未重新运行该项。配置与用户操作见 [DropPhysics](DropPhysics.md)。未提交或推送。

## 掉落物短时物理与固定（2026-10-21）

- 已实现服务器 Chaos 盒体代理、随机弹出/翻转、静态场景/PhysicsBody 碰撞、连续静态支撑与速度检测、超时地面回收、关闭模拟后保持最终姿态。拾取球随根运动，标签保持在世界包围盒上方；复用静态/骨骼模型、描边和原拾取库存链。客户端只接收运动快照并插值，固定状态可供迟加入接收。参数/覆盖顺序/资源配置见 [DropPhysics](DropPhysics.md)。
- **编译已验证**：LauncherInstalled.dat 和 Build.version 确认 UE 5.8.2，UmbraEditor / Win64 / Development 最终构建成功；新增 PhysicsCore 私有依赖，Rider 项目文件已刷新。日志 `Saved/Logs/DropPhysicsBuild.log`、`DropPhysicsProjectFiles.log`。首次构建受沙箱 UBA 缓存权限限制；后续补齐 PhysicsCore 链接依赖，并在编辑器 DLL 占用解除后成功构建。仍有非首选 MSVC 与引擎 Character.h 弃用警告。
- **最终自动化4项通过、0失败**：`Umbra.Loot.DropPhysics`、`Umbra.Loot.VisualMeshCompatibility`、`Umbra.Inventory.Transfer`、`Umbra.Equipment.RequirementsAndLifecycle`。使用匹配版本 UnrealEditor-Cmd / NullRHI，报告 `Saved/Automation/DropPhysicsFinal/index.json`，日志 `Saved/Logs/DropPhysicsFinalTests.log`。
- 新测试在实际物理 World 中验证向上运动、拾取球跟随、自然落稳早于超时、固定后不再运动/无 Tick/无 Timer、空中低速不误判、悬崖超时返回安全姿态；直接调用客户端接收路径验证迟到最终姿态、插值结束，无地面保留拾取、禁用兼容、旧 BP 根组件/启动/销毁清理。客户端接收路径测试不等于真实网络测试。
- **另有已有资产契约差异**：初轮 `Umbra.Loot.Armor1730Definitions` 失败，当前部分真实资产有词缀或10点主属性需求，与旧测试“全部无词缀、零需求”断言不符。未修改这些资产或放宽断言。详见 `Saved/Automation/DropPhysics/index.json` 与 `Saved/Logs/DropPhysicsTests.log`；不能将该轮写成全绿。
- **未验证**：实际 PIE 渲染、每个模型盒体贴合、鼠标 Hover/标签视觉、真实 Listen Server/专服/网络延迟与迟加入。用户通过 Escape 停止了电脑界面操作，此后未再进行 UI 控制；本轮无 PIE 通过结论。未编辑/保存 Content、未提交/推送。蓝图 Graph 与真实视觉配置仍待编辑器确认。人工步骤见 DropPhysics 文档。

## 1730 四件防具世界掉落（2026-10-20）

- 已由匹配项目的 UE 5.8.2 编辑器创建 `/Game/Items/Armor/DA_Armor_1730_{Head,Chest,Hands,Legs}`，数值/槽位/零需求/无词缀按任务配置；四个真实 SkeletalMesh 及原材质引用加载确认。已创建 `/Game/Items/Loot/BP_LootTest_Armor1730`，预填四件定义和既有 BP_WorldItem；后者继承 UmbraWorldItem、Label 引用 WBP_WorldItemLabel 已确认。创建脚本和手工验收见 [Armor1730](Armor1730.md)。
- 最小 C++ 扩展：ItemDefinition 可选骨骼模型软引用；WorldItem 保留静态模型兼容，增加参考姿势组件和包围盒底部补偿；VisualComponent 共用动画/描边。测试生成器只在非 Shipping/Test 权威 BeginPlay 生成一次。不改变装备/GAS、正式敌人概率、角色默认 Mesh 或原有 Content。
- **编译已验证**：UmbraEditor Win64 Development，UHT、编译、链接全部成功；保留非首选 MSVC 14.51 和引擎 Character.h 弃用警告。首次被 Live Coding 拦截，编辑器退出后完整构建通过。未改变模块/Target，无需重新生成项目文件。
- **自动化已验证**：`Umbra.Loot.Armor1730Definitions`、`Umbra.Loot.VisualMeshCompatibility`、`Umbra.Inventory.Transfer`、`Umbra.Equipment.RequirementsAndLifecycle` 全部通过，失败0（3项无警告、1项带临时测试 World 无 context 的销毁警告）。覆盖真实定义与模型、唯一GUID、二次初始化拒绝、实际 TryPickup→PlayerState→Inventory、成功销毁、重复拾取拒绝、静态/骨骼描边切换。报告 `Saved/Automation/Armor1730/index.json`，日志 `Saved/Logs/Armor1730Tests.log`。中间测试夹具误实例化抽象 Controller 的失败已通过具体测试子类修正并复测。
- **无渲染 PIE 已验证**：L_Prototype 内临时添加测试地面和测试 Blueprint，未保存地图。PIE 检查恰好四件防具 Actor，各自正确骨骼模型已加载、组件 Visible=true；结果 `Saved/Automation/Armor1730PIE.json` passed=true，日志 `Saved/Logs/Armor1730PIE.log`。启动/结束日志还有已有 Greystone 输入映射、动画除零、GameplayCue 路径及 RecastNavMesh 提示，未在本任务修改。
- **未验证**：实际屏幕渲染、材质和描边视觉、模型比例/朝向/斜坡/动画地面穿插、真实鼠标点击/远距离导航、背包UI显示及多人竞争/复制。无渲染 PIE 不替代这些人工步骤。骨骼模型无表面碰撞，依赖原根球命中；静态武器表面查询保留。
- Git 差异空白检查通过；四件 DA 和测试 BP 均匹配 LFS。保留用户原有模型目录迁移、WBP_WorldItemLabel 和 M_PP_EnemyOutline 未提交修改。未提交、未推送；生成日志/报告仅在 Saved。


## 地面掉落动画与模型 Hover（2026-10-17）

- 新增 UmbraWorldItemVisualComponent.h/.cpp，WorldItem 持有并复用原 Mesh；仅动画 Relative Transform，三段默认 0.8 秒，结束恢复定义姿态并关闭 Tick。服务器生成时间和窗口关闭状态复制，客户端本地按已过时间插值，实例 ID 防重复；旧物品迟加入静止。
- Hover 同步原标签与 Mesh CustomDepth（默认 Stencil 2），清理/拾取/销毁关闭；敌人 1 不变。RarityColor 仅预留数据接口，实际统一物品颜色需要手工在 M_PP_EnemyOutline 增加 ==2 分支。完整契约和待 PIE 步骤见 [LootAndPickup](LootAndPickup.md)。
- 当前验证：Launcher 确认 UE 5.8.2；UmbraEditor Win64 DebugGame C++ 和 DLL/lib 链接成功（退出码 0），新增反射代码已生成；日志 Saved/Logs/WorldItemVisualDebugBuild.log。保留非首选 MSVC 和引擎 Character.h 弃用警告。Development 构建被当前编辑器 Live Coding 拦截，尚未完成，需要关闭编辑器后重编。
- 源码/文档差异空白检查通过；未新增或运行自动化测试，未执行 PIE、未修改或保存 Content。未改模块/Target。已有用户修改保留；真实 Blueprint/Material 配置及动画、Hover、无 Mesh、网络迟加入/重进相关范围和拾取回归均待编辑器确认。

## 地面模型与纯文字标签（2026-10-16）

- ItemDefinition 新增 WorldDropMesh/Scale/Rotation/Offset；WorldItem 复用 Mesh，异步加载、取消请求与弱回调/版本校验，复用 Label 相对位置为包围盒顶部偏移。Mesh 强制无碰撞/物理/导航影响，拾取球与转移逻辑不变。
- 标签移除背景绑定及背景高亮，品质色用于文字，失效颜色回退 Designer 默认；Hover 按基础色乘亮度并恢复，默认 1.25，无 Tick。NameWrapWidth 默认 240。详见 [Designer 契约](LootAndPickup.md#地面标签原生刷新与-designer-高亮)。
- 本轮验证：关联 Launcher UE 5.8.2，UmbraEditor Win64 Development 的 UHT、C++ 编译及 DLL/lib 链接成功，退出码 0。日志 Saved/Logs/WorldItemPresentationBuild.log。保留非首选 MSVC 与引擎 Character.h 弃用警告。源码/文档核对及 git diff 检查；未修改模块/Target。
- 未新增或运行自动化测试，未启动编辑器/执行 PIE，未编辑或保存 Content。模型真实引用、WBP 布局/Graph 均待编辑器确认。待 PIE 验证姿态/长名称/反复悬停/空模型及失败/加载中销毁/客户端复制/近远拾取/GUID 与词缀保持。
- 以下条目为历史记录，旧 ItemBackground/HoverBackgroundColor 契约已由本节及配置文档替代。

## 地面物品标签原生化

- 本轮仅修改 `UmbraWorldItemLabel.h/.cpp` 与相关文档。ItemName 为必需 BindWidget，ItemIcon/ItemBackground 为可选；C++ 读取实例 Definition 的名称、品质色和图标，空定义清旧内容，无图标折叠 Image；不改实例、不重新随机。
- 原生缓存首次构造的 Designer 背景 Brush/BrushColor，刷新从缓存恢复后应用可配置 Hover，离开/无效数据/析构恢复；重复 Construct 不重采高亮色。构造前 SetItem 暂存，构造后刷新；无 Tick、无新增委托。BP_RefreshItem 仅在原生刷新后作可选视觉扩展。
- 审查确认 WorldItem 原有初始化、BeginPlay、OnRep、Hover → InitWidget/SetItem 路径可复用，未修改组件、碰撞或拾取代码。WBP 只需 Designer 命名控件、Is Variable、鼠标穿透与 Widget Class 配置，不需要 Graph。详见 [Designer 契约](LootAndPickup.md#地面标签原生刷新与-designer-高亮)。
- **编译通过**：LauncherInstalled.dat 确认关联 UE 5.8.2，原项目 UmbraEditor / Win64 / Development UHT、C++、DLL/lib 链接成功，退出码 0；日志 `Saved/Logs/WorldItemLabelNativeBuild.log`。保留非首选 MSVC 和引擎 Character.h 弃用警告。未修改模块/Target。
- **验证边界**：未新增或运行自动化测试，未运行 PIE，未打开/修改/保存 Content；真实 WBP、重复 Construct、客户端复制及视觉效果仍待编辑器确认。保留已有源码、Tests、资产和文档修改，未提交/推送。

## 2026-10-10 Tooltip 抖动与剑/锤比例修复

- 原因：C++ 每件物品按自己的自然高度计算整体缩放，内容长短直接改变字体/图标比例；外层 `SScaleBox(ScaleToFit)` 又让上一帧分配区域和缩放参与文字测量，存在多帧布局反馈。
- 修复：`SUmbraTooltipViewport` 将自然布局与视口 RenderTransform 分开；`ViewportFitReferenceSize` 默认 `(400, 1440)` Slate 单位，共用预算内物品使用相同比例，超出预算仍应急缩小保留全部内容。参数来源与维护步骤见 [ItemTooltipUI](ItemTooltipUI.md)。不改玩法或数据构建器。
- 资产说明：诊断过程中曾通过 UE 5.8.2 调整 `WBP_ItemTooltip.ScaleBox_0` 的 Stretch/UserSpecifiedScale；收到用户要求后已通过同版本编辑器恢复为 `ScaleToFit / 1.0` 并重新编译保存。保留用户原有外层宽 400、内层宽 500、ScaleBox Both 和其他布局，最终修复不依赖资产属性调整。恢复记录 `Saved/Logs/TooltipAssetRestore.log`；资产经历了编辑器重存，不能宣称二进制从未变更。
- 编译：UE 5.8.2 `UmbraEditor Win64 Development` 构建成功，日志 `Saved/Logs/TooltipStabilityBuild.log`。无模块/Target 修改，无需刷新 Rider 工程。
- 定向验证：`TooltipViewportFit`、`TooltipLayoutStability`、`TooltipDesignerPreview`、`TooltipReadOnly`、`Classification`、`IdentityAndRequirements` 全部通过（5 Success + 1 SuccessWithWarnings，唯一警告为引擎 HTTP 连通性请求超时；退出码 0）。真实 WBP 的剑/锤/剑切换覆盖 5 组视口、4 档 DPI、每组 60 次 prepass/paint，检查稳定后的边界、位置尺寸和实际绘制图标大小；共 3600 次布局/绘制。报告 `Saved/Automation/TooltipLayoutVerified/index.json`，日志 `Saved/Logs/TooltipLayoutVerified.log`。
- 扩展数据回归未全通过：`DefinitionAndScaling` 因现有非法伤害范围诊断未被测试预期捕获而失败；`Formatting` 的旧统计断言期望 24 行，当前为 23 行。这两项不涉及本轮布局路径，未改动以掩盖失败。初轮报告 `Saved/Automation/TooltipStabilityFinal/index.json`（其中布局失败已在最终版本修复）。
- 未验证：已尝试打开编辑器检查，窗口随后关闭，未完成真实 PIE 悬停/鼠标、窗口拖动和网络验证。NullRHI 布局/paint 回归不能代替 GPU 视觉检查。未提交/推送，保留工作区已有修改。

## 2026-10-10 物品掉落与拾取第一阶段

- 新增 LootTable、持有完整实例的复制 WorldItem、WorldItemLabel 原生基类；敌人权威 Die 一次触发，加权选择/概率/数量上限 16，可达地面散布，无美术路径和 Content 修改。
- 点击物品复用现有导航与攻击取消/单条指令队列，有限接近计时器；Controller 最小 Server RPC 只传目标引用。WorldItem 校验状态/距离，PlayerState 事务入口复用 Inventory.AddInstance，保留 GUID/RolledAffixes，成功后才销毁。Inventory OwnerOnly/UI 事件链不变；DefaultEngine 开启客户端导航查询。
- 配置规则、失败边界、蓝图手工接线和待执行 PIE 步骤见 [LootAndPickup](LootAndPickup.md)。所有原有未提交修改保留，未提交/推送；未修改模块/Target，无需刷新 Rider 工程。
- **编译验证**：LauncherInstalled.dat 与 Build.version 确认 UE 5.8.2；原项目 UmbraEditor / Win64 / Development 完成 UHT、C++、DLL/lib 链接，Result: Succeeded。日志 `Saved/Logs/LootAndPickupBuild.log`。新增局部命名遮蔽及 GetPawn TObjectPtr 推导错误已修复；仍有非首选 MSVC 14.51 和引擎 Character.h 的既有弃用警告。
- **未验证**：按要求未新增或运行自动化测试；未运行 PIE、未打开/保存 Content、未验证真实标签/鼠标/地图导航/多人竞争与远程客户端。用户按指南配置蓝图后验收，不能把编译成功视作玩法或网络通过。

## 2026-10-09 武器伤害区间、实例离散词缀与 Tooltip AffixEntry

- 完成 WeaponProfile 兼容区间、DerivedStats 均值参考与服务端 WeaponChannels 单次采样；旧 BaseDamage、Legacy/ExplicitChannels、A/X 和单次扣血链保留。
- 新增 FUmbraItemInstance 与离散候选/实际档位，接入背包创建、完整实例转移/回滚、装备 GE 和注销恢复；Inventory/Equipment 实例快照 OwnerOnly。新增 UmbraTooltipAffixEntry，普通属性/A 增伤同区同样式，X 投影独立 SpecialEffects。配置、单位、准确绑定与待 PIE 步骤见 [ItemInstanceAffixes](ItemInstanceAffixes.md)。
- 验证：使用 LauncherInstalled.dat 确认的 UE 5.8.2，原项目 UmbraEditor / Win64 / Development 最终编译、UHT、DLL/lib 链接成功（Result: Succeeded，退出码0）；日志 Saved/Logs/ItemInstanceAffixesBuild.log。首次沙箱 UBA 缓存写入权限失败，获授权后重跑；新控件 Visibility 参数遮蔽错误已修复。仍有既有 MSVC 非首选版本和引擎 Character.h 弃用警告。
- 源码/文档 git diff --check 及受影响 Markdown 文件链接目标检查通过；Content 与 Tests 无本轮修改，原有四个文件未提交修改保留。未修改模块/Target，无需重新生成 Rider 工程。
- 按要求未新增、未运行自动化测试；未打开编辑器、未执行 PIE、未确认 WBP 资产绑定或多人网络。真实伤害范围、同 GUID 词缀稳定、装卸与失败回滚、GE 移除、OwnerOnly 网络、Tooltip 外观仍待人工验收。没有提交/推送。

## 2026-10-08 Unity 合并编译名称冲突修复

- 修复物理/类型伤害执行器的匿名命名空间 `Captures` 重名，以及 Tooltip 与 EquipmentComponent 的 `Nonnegative` / `ValidSlot` 重名。将伤害捕获函数分别命名为 `PhysicalDamageCaptures` / `TypedDamageCaptures`，Tooltip 校验函数增加 `Tooltip` 前缀，并更新调用；规则与数值计算不变。匿名命名空间不能隔离同一个 Unity 翻译单元内的同名函数。
- **构建验证**：UE 5.8.2 UmbraEditor Win64 Development 增量构建成功；随后使用 `-ForceUnity -DisableAdaptiveUnity` 构建成功，并检查生成的 Module.Umbra.1.cpp / Module.Umbra.2.cpp，确认原冲突文件确实被分别合并编译。日志为 Saved/Logs/UnityCollisionBuild.log 与 Saved/Logs/UnityCollisionBuildFullUnity.log。最初沙箱构建因 UBA 缓存目录权限失败，授权后重跑成功；仍有既有 MSVC 非首选版本和引擎 Character.h 的 GetMovementBase 弃用警告。
- **自动化验证**：NullRHI 编辑器运行 `Umbra.Damage.` 与 `Umbra.Items.Tooltip.`，8/8 Success、0 测试警告、0 失败，退出码 0。报告 Saved/Automation/UnityCollision/index.json，日志 Saved/Logs/UnityCollisionTests.log。源码/文档 `git diff --check` 通过。
- **未验证**：真实 PIE、UI 视觉/鼠标输入及多人网络；本次仅修复编译符号冲突，未修改 Content 资产或构建配置，未提交/推送。

## 2026-10-06 Tooltip 视口适配修复

- 修复仅钳制位置却未处理 tooltip 自身超高/超宽，以及桌面绝对坐标与视口局部坐标混用的问题。运行时整体等比缩小到视口内，使用缩放后尺寸定位；显示期间仅跟踪布局、槽位与视口变化，保留所有数据行和鼠标穿透。
- 修改 UmbraItemTooltip、背包/装备 Tooltip 接入及 UI 测试，新增 `Umbra.UI.Items.TooltipViewportFit`。当前契约、参数来源和 PIE 验收见 [ItemTooltipUI](ItemTooltipUI.md)。未修改 Content，未提交/推送，保留已有工作区修改。
- 验证：UE 5.8.2 UmbraEditor Win64 Development 构建成功。首次编译的 FVector2f/FVector2D 显式转换问题已修正。最终定向回归 7/7 Success、0 警告、0 失败，包含新增 TooltipViewportFit、DesignerPreview、ReadOnly 和四项数据测试；报告 Saved/Automation/TooltipViewportVerified/index.json，日志 Saved/Logs/TooltipViewportVerified.log。源码/文档 diff 空白检查通过。
- 完整 UI 回归未通过：原有 TooltipHover 仍通过 Slot.GetToolTip 读取实例，与已经使用 AddToViewport 的接入不一致；原有 TooltipLifecycle 把 ScalingRows（UniformGridPanel）强转 VerticalBox 导致中止，记录于 Saved/Logs/TooltipViewportFit.log。本轮保留这些旧测试，未宣称全套通过。未执行实际 WBP 视觉、真实 PIE 鼠标/窗口缩放或网络验证；需重启加载新 DLL，并 Compile WBP_ItemTooltip 后检查 NativeTick 和四边显示。

## 2026-10-05 Tooltip 第三阶段：品质背景与正式 Hover（验证中）

- ItemDefinition → 三个 Builder → TooltipData 新增独立 TooltipQualityBackgroundColor（默认 White）。quality_bg / SlotBackground 仅 Tint；Brush 保留，Icon 不继承背景色；名称/品质文字用 RarityColor。Designer Preview 契约保持。
- InventoryMenu / EquipmentMenu 新增 ItemTooltipClass，原生 Slot Hover → 菜单重取最新快照 → Builder → 缓存一个 Tooltip → SetToolTip。校验真实来源、所属 PS、索引/槽、GUID、Definition；不自动选择背包装备目标槽。快照、组件与页面失效立即清空；四主属性/装备事件合并到一次 GameThread 任务刷新需求，无 Tick/网格重建/游戏状态修改。
- 本轮修改文件：Items/UmbraItemDefinition.h、UmbraItemTooltipData.h/.cpp；UI/Items/UmbraItemTooltip.h/.cpp；InventoryMenu.h/.cpp、InventorySlot.h/.cpp、新增 UmbraInventoryTooltip.cpp；EquipmentMenu.h/.cpp、EquipmentSlotWidget.h/.cpp、新增 UmbraEquipmentTooltip.cpp；UI/UmbraCharacterMenu.cpp；Tests/UmbraItemTooltipUITestTypes.h、新增 UmbraItemTooltipHoverTests.cpp；Architecture、ItemTooltipData、ItemTooltipUI、InventoryFoundation、EquipmentUIBinding 与本记录。
- 当前验证状态：首轮编译发现 UWidget.Slot 遮蔽与 UObject→ItemDefinition 参数类型问题，已修复并通过相关 C++ 编译；随后原项目 DLL 链接被正在运行的 UnrealEditor 占用（Saved/Logs/TooltipHoverBuildFixed.log）。已请求用户保存关闭。后续生命周期/测试补充尚待最终构建和自动化，不能视为验证通过。
- 未修改/保存 Content，未提交/推送；真实 WBP Designer/PIE 鼠标路由、滚轮、视觉和网络仍待人工验收。配置及详细四组 PIE 清单见 [ItemTooltipUI](ItemTooltipUI.md)。

## 2026-10-05 Tooltip 最小 Designer Preview 修复

- UUmbraItemTooltip.RefreshDisplay在IsDesignTime()下仅保持自身HitTestInvisible并关闭焦点，提前返回，保留已有Designer文字/图标/静态行及区域设计显隐。不生成ItemDefinition、GUID、属性或需求，不把预览标为有效数据，不发送数据变化事件；运行时Set/Clear与无效数据Collapsed契约不变。未修改Content（包括用户新建的Content/UI/Tooltip）、未新增输入/拖动/Tick。
- 新增Umbra.UI.Items.TooltipDesignerPreview，验证Title/Legendary/285及静态行在重复PreConstruct后保留，预览数据仍无效；运行时清除占位、真实数据覆盖及Clear后折叠。首轮仅测试错误假定VerticalBox默认Visibility为Visible而失败，已改为验证其原始设计显隐值保持不变。
- UE5.8.2 UmbraEditor Win64 Development构建成功，原项目DLL已更新，日志Saved/Logs/TooltipDesignerBuildFinal.log。最终Tooltip专项及数据/生命周期回归7/7 Success、0警告/0失败，报告Saved/Automation/TooltipDesignerFinal/index.json；源码空白和文档diff检查通过。未进行实际WBP Designer视觉、真实PIE或网络验证；此前静态占位若已被旧预览实例清空，重新打开Designer检查。使用说明见[ItemTooltipUI](ItemTooltipUI.md)。

## 2026-10-04 统一物品 Tooltip 第二阶段：分类与 UI 基础

- ItemDefinition新增显示专用ItemCategory/WeaponType，默认Unknown，枚举显式稳定数值；旧资产按有效WeaponProfile/AllowedSlots/普通物品后备。类型和六级品质均本地化；分类冲突输出带资产路径的警告及ClassificationWarnings，不改资产、不改变槽位/动作/伤害规则。三个Builder入口统一携带分类和已格式化的标题/伤害/评级/需求/重量文本，原GUID、原始StatLine、失败与要求契约保留。
- 新增UUmbraItemTooltip（SetTooltipData/ClearTooltipData/GetTooltipData/BP_TooltipDataChanged）和UUmbraTooltipStatEntry（SetEntryText/Text+Style）。前者只写绑定控件、重建动态行和显隐区域，后者只显示已格式化文本及可配置状态颜色。无效数据、切换、重复Construct、Destruct清理不保留旧条目；整树HitTestInvisible、不可聚焦，无Tick/轮询/拖动/父窗口操作。默认StatEntryClass为空时使用原生TextBlock降级；所有有效物品显示零或实际重量。
- **构建成功**：实查项目关联UE5.8.2，原项目UmbraEditor / Win64 / Development；最终日志`Saved/Logs/ItemTooltipUIBuildVerified.log`，DLL已更新。首次完整构建与测试加入后的构建均通过；保留已有MSVC14.51非首选版本和引擎GetMovementBase弃用提示。
- **实际测试过程**：第一次新增UI测试未保存TakeWidget返回的Slate引用，立即触发Destruct清空后CastChecked空行导致进程中止（`ItemTooltipUITests.log`）；改为持有Slate引用后TooltipLifecycle通过。第二次TooltipReadOnly测试在已初始化的测试World中重复InitializeComponent触发引擎断言（`ItemTooltipUITestsFinal.log`）；改为HasBeenInitialized检查。两处均为测试夹具修正，没有修改生产装备/ASC规则。
- **最终自动化**：`Saved/Automation/ItemTooltipUIVerified/index.json`，日志`Saved/Logs/ItemTooltipUITestsVerified.log`；33/33 Success，32项无警告，1项既有警告，0失败/0未执行。新增`Umbra.Items.Tooltip.Classification`、`Umbra.UI.Items.TooltipLifecycle`、`Umbra.UI.Items.TooltipReadOnly`均无警告；第一阶段3项Tooltip及Inventory/Equipment/Attributes/Damage、相关UI/CombatInfo回归全部通过。唯一警告仍是`Umbra.UI.Equipment.PageVisibility`测试空菜单0/10槽位配置提示。
- **新增覆盖**：全部分类与14种具体武器类型本地化、六级品质/Epic、真实旧DA_Item_TestSword默认兼容、火焰伤害与单手剑分类独立、三个入口分类传递；有效标题/颜色/区域、None评级过滤、曲线参考说明、同一Stats列表、Unknown需求、空故事/效果折叠、零重量、无效数据清空、切换/重复刷新/构造析构、可选绑定缺失/原生行降级、行颜色复位、文本不被UI重算、命中测试不可见状态和Inventory/Equipment/GAS无副作用。
- **未验证**：实际WBP_ItemTooltip/WBP_TooltipStatEntry尚未创建，因此未执行它们的蓝图编译、素材图表审查、渲染/DPI/字体/动画、真实PIE鼠标和滚轮、正式悬停以及网络验证。原生测试使用临时WidgetTree和NullRHI，不替代完整UI验收；素材残留拖动/隐藏其他窗口逻辑需用户手动排除。
- 本轮未修改Content、Config、InventoryComponent、EquipmentComponent、PlayerState、DerivedStats或伤害规则，未提交/推送。全部文件清单、准确BindWidget名称/类型、推荐层级、Class Defaults、编辑器步骤及下一阶段Tool Tip Widget接口见[ItemTooltipUI](ItemTooltipUI.md)。下文第一阶段“不创建Widget”为历史范围。
- **静态检查**：13个本轮源码/专题文件行尾空白检查通过；入口文档git diff --check通过，五份受影响文档148个相对链接存在性检查通过（未验证锚点渲染）。已检查前后Git状态，保留既有未提交内容，生成日志/二进制仍在忽略目录。

## 2026-10-04 统一物品 Tooltip 第一阶段：数据与格式化

- 新增 FUmbraItemTooltipData 与三个 BlueprintPure 构建入口（定义、背包实例、已装备实例）；保留原GUID、完整来源/属性/单位/伤害条件，失败返回全新空数据。复用库存快照校验与槽位标题后备，不创建 Widget、不接悬停或对比、不修改Content。
- ItemDefinition 新增独立 ItemLevel=1、六级 Rarity=Common、可选 FlavorText；品质明确数值0..5，Epic=3，保留既有颜色/背景/框。WeaponProfile 四维可手动覆盖评级（可显式None）；默认按基础伤害加权系数自动S/A/B/C/D，集中Game配置阈值，曲线只标记参考强度。原伤害、GAS与DerivedStats算法不变。
- Equipment.QueryRequirements与正式Refresh共享排除自身/目标槽GE的过滤规则，区分等级禁止、主属性惩罚、满足和上下文未知；倍率读取组件配置。所有属性在统一列表保留Intrinsic/FixedAffix来源；攻速/暴击百分比、移速cm/s、类型抗性评分、A比例/X因子和双方标签/攻击来源/暴击/易伤条件统一格式化。CombatInfo沿用原渲染参数并复用提取的数字函数。
- **构建**：已实查Launcher安装记录的UE5.8.2；UmbraEditor / Win64 / Development原项目构建成功，DLL已更新，日志 `Saved/Logs/ItemTooltipBuild.log`。最初沙箱构建因ProgramData缓存权限中止；首次沙箱外链接因用户编辑器占用DLL失败，用户保存关闭后重新构建成功。保留MSVC14.51非首选版本提示，初次全量编译也有既有引擎GetMovementBase弃用警告，无本轮编译错误。
- **自动化**：`Saved/Automation/ItemTooltip/index.json`，日志 `Saved/Logs/ItemTooltipTests.log`；30/30 Success（29项无警告、1项带既有警告），0失败、0未执行。新增 `Umbra.Items.Tooltip.DefinitionAndScaling`、`Formatting`、`IdentityAndRequirements` 全部无警告，覆盖真实旧DA_Item_TestSword读取默认字段、品质序号、等级独立、九通道/总基础、加权/阈值/零值/曲线/手动覆盖、全部词缀单位与A/X条件、过滤自身GE（含倍率情形）、同定义不同GUID/非法数据/过期装备快照、构建前后Inventory/Equipment/GAS不变。
- **回归范围**：Umbra.Attributes（4）、Damage（4）、Equipment（2）、Inventory（2）、UI.Inventory（4）、UI.Equipment（6）、UI.CombatInfo（3）、UI.Items.Presentation（1）、UI.CharacterStats.PrimaryRows（1），加新增Tooltip（3），合计30。唯一警告为既有 `Umbra.UI.Equipment.PageVisibility` 的空测试菜单0/10槽配置提示；没有把它列为新增Tooltip警告。
- **未验证**：真实地图PIE、鼠标输入/渲染、网络/远程客户端；本次为NullRHI编辑器自动化，不能替代视觉验收。完整客户端穿戴前逐项需求预览未实现（现有Equipment仅权威绑定ASC），复制快照可提供聚合主属性状态。真实资产除测试读取旧默认字段外的蓝图内容及视觉配置均待编辑器确认。本轮不创建或修改Content资产，不提交/推送，保留用户原有工作区改动。
- 字段/API、完整格式表、文件清单、配置来源与覆盖顺序，以及第二阶段UUmbraItemTooltip/WBP_ItemTooltip接入见[ItemTooltipData](ItemTooltipData.md)。下文“不含Tooltip”为历史阶段记录。
- **静态检查**：本轮新增/修改源码与专题文档无行尾空白；受影响入口文档的git diff --check通过，四份文档132个相对链接存在性检查通过（未验证锚点渲染）；已复查前后Git状态，生成的构建/测试输出仍在忽略的Binaries/Intermediate/Saved目录。

## 2026-10-04 背包与装备 UI 交互闭环、初始物品

- 复用既有Inventory快照、容量、网格列数和SetItemDefinition/ClearItem；新增NotReady/Ready/InvalidSnapshot区分，背包与装备按GUID保留同实例选择，移除/换人/无效数据清除，Destruct清显示但保留可重验的选择身份。PlayerController.OnRep_PlayerState补发背包上下文通知。没有Tick、轮询、Text Binding或布局重建。
- 背包左键选择，原生双击一次提交到PlayerState.EquipFromInventory；空格不提交。单合法槽直接使用，多合法槽仅唯一空槽自动选择；多个空槽返回SlotSelectionRequired并提供RequestEquipInSlot显式接口；全部占用不替换。LastEquipResult/Message与BP_EquipFinished提供可本地化反馈。
- 装备右键经PlayerState.UnequipToInventory检查容量和快照GUID，成功返还同一实例，失败不清格子。BP_UnequipFinished保留原签名，但成功实例已在背包，禁止再次AddItem。准确转移结果在LastUnequipTransferResult，旧枚举将InventoryFull等映射为Failed。转移入口增加重入/库存广播保护；修复旧OutReturnedItem读取临时快照悬空指针。
- 按追加要求，DefaultGame.ini配置DA_Item_TestSword/DA_Item_TestMace为InitialInventoryItems。PlayerState.BeginPlay在权威端只发放一次到背包，不自动装备；重开菜单、重复初始化和组件重注册不补发。可由PlayerState BP显式覆盖清单。加载失败/容量不足记录警告并跳过，不轮询补发。
- **分步验证**：背包绑定构建通过，Umbra.UI.Inventory 3/3 Success（Saved/Automation/InventoryUIBinding）；双击穿戴构建通过，背包及Transfer 5/5 Success（Saved/Automation/InventoryEquipUI）。初次新增快照用例错误地向数组追加自身引用触发容器断言，已改为复制后追加；随后通过，不隐藏失败记录。
- **返还与初始物品首轮**：UE5.8.2 UmbraEditor Win64 Development构建通过；27/27 Success，0失败（含2项既有警告，Saved/Automation/InventoryEquipmentUI）。实际加载BP_UmbraPlayerState及两个ItemDefinition，验证配置继承、两件入包、独立GUID、初始化不复制/不补回、无自动穿戴。最后新增的重开选择用例发现Construct中绑定前空快照清除选择，已调整绑定顺序并回归通过。
- **最终验证**：UE5.8.2 UmbraEditor Win64 Development构建通过，原项目DLL已更新，日志Saved/Logs/InventoryEquipmentUIBuildFinal.log。31/31 Success，0测试错误，报告Saved/Automation/InventoryEquipmentUIFinal/index.json，日志Saved/Logs/InventoryEquipmentUITestsFinal.log。覆盖Inventory/Equipment/Attributes/Damage全部相关用例及CombatInfo、选择、表现、预览生命周期等回归；新用例无警告，Maintenance保留6条既有动画警告、PageVisibility保留1条空测试菜单警告。构建保留既有MSVC非首选版本/引擎弃用提示。git diff --check与文档相对链接检查通过。
- **未验证**：实际WBP鼠标路由/渲染、真实PIE、Listen Server运行及远程客户端网络。原生测试调用真实鼠标处理函数，不等于真实窗口输入通过；滚轮与人物预览视觉需人工确认。远程请求RPC与选槽弹窗未实现；不扩展拖放、Tooltip、掉落、存档、排序或伤害规则。
- 未修改Content资产，保留原有未提交代码/资产；未提交或推送。配置、API审查、完整文件清单与编辑器/单人PIE操作见[InventoryEquipmentInteraction](InventoryEquipmentInteraction.md)。下文同日早期“UI未接转移”等描述为历史阶段状态。

## 2026-10-04 背包↔装备权威转移

- PlayerState新增EquipFromInventory(Slot, InstanceId)与UnequipToInventory(Slot, ExpectedInstanceId, OutReturnedItem)，以及GetInventoryComponent/GetEquipmentComponent只读访问。转移统一协调两个组件：装备成功后才消费背包，卸装成功后才放回背包；槽位占用、背包满、重复GUID、未知/过期身份在修改前拒绝，意外失败走回滚并返回TransferFailed。GUID全程不变，失败不丢物品。EUmbraTransferResult覆盖全部分支。
- 本轮直接构建主项目并更新DLL；编辑器已关闭，无需再提示“关闭编辑器重编译”。未新增UI入口，装备页右键和背包点击仍沿用旧原型，尚未接转移接口。详见[InventoryTransfer](InventoryTransfer.md)。
- **构建通过**：UE5.8.2 UmbraEditor Win64 Development，主项目日志Saved/Logs/InventoryTransferBuild.log。仅保留既有非首选MSVC与引擎GetMovementBase弃用提示，无新错误。
- **自动化**：新增Umbra.Inventory.Transfer；结果见Saved/Automation/InventoryTransfer/index.json。覆盖未知GUID、非法身份、成功装备并消费背包、槽位占用、错误槽位、成功卸装返还并保留GUID、过期GUID、重复实例拦截、背包满拦截。既有装备/词缀/属性/伤害/UI回归见该报告。
- **未验证**：真实WBP、PIE与多人网络复制；由用户后续编辑器验收。未修改Content资产；未提交/推送。

## 2026-10-04 InventoryComponent与背包实时数据

- 新增PlayerState原生InventoryComponent，最小非堆叠存储包含物品定义、稳定GUID、固定槽位索引和容量。默认InitialCapacity=96，首次注册限制0..512；AddItem/RemoveItem仅允许权威端，拒绝非法/重复身份、无效定义、满容量和广播重入，移除不移动其他槽位。快照OwnerOnly复制，无客户端请求RPC、存档或装备转移。
- InventoryMenu默认绑定当前PlayerState，事件驱动物品图标、品质背景/框和真实占用容量；未就绪显示“—/—”。菜单/Pawn/组件生命周期重绑，无Tick；同实例保留选择，移除清除选择。原InventoryCapacity仅用于Designer/关闭绑定的展示原型，运行时由组件决定。接入说明见[InventoryFoundation](InventoryFoundation.md)。
- **构建通过**：UE5.8.2 UmbraEditor Win64 Development，Saved/DamageBucketsValidation独立副本，日志Saved/Logs/InventoryFoundationBuild.log。首轮重复成员声明已修正后构建成功；保留既有MSVC非首选与引擎弃用提示。原编辑器运行中，原项目DLL未替换；用户保存关闭后需重编译原项目。原Source文件与验证副本哈希一致，副本保留此前额外诊断测试源，本轮未运行该诊断。
- **28/28 Success，0测试错误**：Saved/Automation/InventoryFoundation/index.json，日志Saved/Logs/InventoryFoundationAutomation.log。新增Umbra.UI.Inventory.LiveStorage覆盖满容量/重复GUID/非法输入/未注册拒绝、固定槽位和空格复用、同定义多实例、注销重注册保留、UI延迟上下文/计数/实例/选择、重复构造、关闭重开、换PlayerState解绑。新测试无警告；Maintenance保留6条、PageVisibility保留1条既有警告，其他26项无警告。已有装备卸装、词缀、属性、伤害与UI回归通过，源码/文档空白检查通过。
- **未验证**：真实WBP接线/外观/输入、PIE与多人网络复制。未修改Content资产或布局，未提交/推送。当前卸装仍不自动返还背包；下一阶段需接权威背包↔装备转移，统一验证身份、槽位与返还空间，不能用两个独立蓝图调用模拟原子交易。

## 2026-10-04 装备页右键卸装与结果反馈

- 用户结束UI拖拽问题排查，继续下一阶段。本轮未修复拖拽资产，新增EquipmentSlot原生右键请求、EquipmentMenu活动页/归属/锁定/数据来源检查、EquipmentComponent.TryUnequipInstance槽位与GUID校验。成功复用原卸装GE清理及派生/快照广播，拒绝不提前清空UI；保留左键选择与hover，无Tick。BP_UnequipFinished返回结果、原物品定义/GUID与Message，LastUnequipMessage可读。接线和验收见[EquipmentUIBinding](EquipmentUIBinding.md)。
- 边界：只有权威端修改，未新增客户端RPC；没有InventoryComponent，不自动转移到背包或持久化卸下物品。返回的实例信息供后续权威物品转移接口接入，不能宣称已入背包。AppliedInvalid代表卸下已执行但后续快照无效，其他失败返回空物品。
- **构建通过**：UE5.8.2 UmbraEditor Win64 Development，Saved/DamageBucketsValidation独立副本，日志Saved/Logs/UnequipInteractionBuild.log。原编辑器运行中，未替换原项目DLL；用户需保存并关闭编辑器后构建原项目。所有原项目Source文件与验证副本逐文件哈希一致；副本保留此前仅用于诊断的额外测试文件，本次未运行其诊断测试。编译保留既有非首选MSVC及引擎GetMovementBase弃用提示。
- **27/27 Success，0测试错误**：报告Saved/Automation/UnequipInteraction/index.json；日志Saved/Logs/UnequipInteractionAutomation.log。扩展Umbra.UI.Equipment.LiveSnapshot覆盖原生右键到组件、空槽重复操作、锁定/外来槽位、旧GUID及手工视图拒绝、无效GUID、原实例返回、页面未激活、未通知的PlayerState切换，测试无警告。属性、伤害、固定词缀、UI与调试GE相关回归通过；Maintenance保留6条既有警告，PageVisibility保留1条既有警告。
- **未验证**：真实WBP鼠标路由、Tooltip/视觉、PIE及多人网络；由用户后续进行编辑器验收。源码/文档空白检查通过。未保存或修改Content，保留已有资产及其他工作；未提交/推送。

## 2026-10-04 槽位图标刷新与物品品质贴图

- 用户反馈主手图标不显示，并要求装备/背包槽背景和品质框由物品信息配置。ItemDefinition.Presentation新增SlotBackgroundTexture、RarityFrameTexture；共享FromDefinition投影及ItemSlotVisual负责图标/背景/品质框。贴图按白色Tint显示，背景缺失恢复Designer默认，品质框缺失兼容原默认Brush+RarityColor；替换和清空不残留上一件物品图。未引入随机品质或背包所有权逻辑。
- 原EquipmentSlot在写入Icon之后执行BP_RefreshVisual，允许旧蓝图再隐藏/覆盖图标；改为BP交互样式先执行，C++最后落实物品层。InventorySlot原先无条件隐藏物品层，现提供SetItemDefinition/ClearItem/HasItem/GetItemDisplay，同样从物品定义取图；数量、已装备标记和InventoryMenu容量计数仍未接真实背包。没有Tick，未修改Content。
- **构建通过**：UE5.8.2 UmbraEditor Win64 Development，独立副本Saved/DamageBucketsValidation，日志Saved/Logs/ItemPresentationBuild.log。原编辑器运行中，因此原项目DLL未替换；需用户保存并关闭编辑器后在Rider重编译原项目，再使用新增反射字段。保留引擎弃用和非首选MSVC提示。
- **27/27 Success，0测试错误**：前轮24项+Umbra.UI.Inventory.EmptyGridLifecycle、CharacterMenuComposition、Umbra.UI.Items.Presentation。报告Saved/Automation/ItemPresentation/index.json，日志Saved/Logs/ItemPresentationAutomation.log。Maintenance原有6条警告、PageVisibility原有1条警告，其余25项无警告。新测试验证两个槽位图标/品质资产、贴图不叠乘颜色、替换缺失贴图回退、清空及交互状态，并只读加载已保存WBP_EquipmentSlot，确认其MainHand实例ItemIcon资源及可见性正确。
- **诊断边界**：现有保存的WBP包含可用ItemIcon绑定，修复后槽位级真实蓝图测试通过；未复现当前编辑器PIE中原始完整主手接线状态，不能确认唯一原因就是BP刷新覆盖。未验证未保存的蓝图、实际主菜单到主手的PIE状态、贴图视觉/遮挡、多人网络。检查MainHand枚举、已穿戴ItemDefinition、EquipmentDataReady、父容器可见性及Brush/Visibility绑定的步骤见[EquipmentUIBinding](EquipmentUIBinding.md)。
- 保留现有用户资产改动（含WBP_EquipmentSlot及Equipment_Icon目录），无资产保存、提交或推送。后续需重编译后配置物品Presentation，并在两个槽位确认背景→物品图→透明中心品质框的层次。

## 2026-10-04 固定词缀与装备效果

- 用户暂不能进行编辑器测试，明确选择继续固定词缀与装备效果。ItemDefinition新增16种既有属性的AttributeBonuses白名单和复用FUmbraDamageBonus的DamageBonuses；四维沿用PrimaryBonuses，AD/AP仍派生，未新增GAS属性、随机生成或背包系统。配置来源、单位和验收见[EquipmentAffixes](EquipmentAffixes.md)。
- 每实例仍一个Infinite原生EquipmentEffect句柄，二十个SetByCaller Modifier承载四维、固有防御及固定属性；装备专用GE组件从Spec.Context.SourceObject读取只读ItemDefinition的A/X，统一结算和面板摘要通过GetBonuses复用数据。需求惩罚只影响固有防御，固定防御词缀完整生效；非法重复属性/数值/增伤配置在替换前拒绝。卸下、替换、注销清理同一句柄，无Tick。
- **原项目构建通过**：UE5.8.2 UmbraEditor Win64 Development，DLL已更新。最终日志Saved/Logs/EquipmentAffixesBuildFinal.log。修正了测试误调用私有Shutdown（改为销毁组件验证清理），并在无界面启动中发现且修复构造GE时AddComponent内部NewObject导致的启动断言，改用命名CreateDefaultSubobject。最终启动及测试进程退出码0；保留既有MSVC非首选工具链提示。
- **24/24 Success，0测试错误**：上一阶段23项加Umbra.Equipment.FixedAffixesAndDamage。报告Saved/Automation/EquipmentAffixesFinal/index.json，日志Saved/Logs/EquipmentAffixesAutomationFinal.log。新测试无警告；Maintenance保留6条既有动画警告，PageVisibility保留1条空测试菜单提示，其余22项无测试警告。
- 新测试验证MaxHealth增加不回血、攻速增量、固有防御惩罚与词缀隔离、真实100→132→156扣血、A摘要及X标志、同GE类不同装备资产来源、重复替换只有两个句柄、非法配置不影响旧装备、卸下回到120及组件销毁后效果/面板清理。原有四维派生、装备需求、伤害兼容、属性初始化、CombatInfo、StatsPanel及装备槽位回归均通过。源码/文档git diff空白检查通过。
- **未验证**：实际DataAsset配置、真实WBP与PIE按键/视觉、多人网络复制；仍由用户稍后验收，未将前阶段UI待测项视为已通过。未修改或保存Content资产，保留既有用户修改；未提交/推送。下一阶段可接装备页卸装交互及结果反馈。

## 2026-10-04 StatsPanel一致性修复与装备槽位实时绑定

- 先修复StatsPanel旧AD/AP占位：八个重叠属性共用UmbraCombatStats::Read，统一来源/精度/单位/“仅存储”语义；AD/AP额外监听DerivedStats快照，四主属性保留局部通知。独立修复构建通过，StatsParity自动化4/4 Success、0错误/警告，覆盖两个页面共享显示与原四主属性回归。日志Saved/Logs/StatsParityBuild.log、StatsParityAutomation.log，报告Saved/Automation/StatsParity/index.json。
- 随后按用户明确选择推进装备槽位UI对接。EquipmentMenu默认订阅所属PlayerState的Equipment快照，按Slot及InstanceId投影到现有槽位，更新名称/图标/颜色/实例GUID/需求状态；ItemDefinition新增Presentation字段，不改变属性。相同实例保留选择，替换或清空移除旧选择；重复槽位不投影。未就绪、无效引用/快照时清空数据且EquipmentDataReady=false；Construct重绑、Destruct解绑、PS/Pawn/ASC变化重绑。组件注销新增无效快照广播，避免陈旧装备显示，无Tick。
- **原项目最终构建通过**：UE5.8.2 UmbraEditor Win64 Development。发现编辑器已关闭后直接构建原项目，DLL已更新，包含本轮StatsPanel修复、装备槽位绑定及上一阶段CombatInfo代码。日志Saved/Logs/EquipmentUIBindingBuild.log、EquipmentUIBindingBuildFinal.log。首轮局部Slot遮蔽UWidget成员且auto指针无法推导TObjectPtr，已改为显式类型EquipmentSlot并重建通过；保留引擎弃用/MSVC非首选版本提示。
- **最终23/23 Success，0测试错误**：上阶段22项加Umbra.UI.Equipment.LiveSnapshot。报告Saved/Automation/EquipmentUIBinding/index.json，日志Saved/Logs/EquipmentUIBindingAutomation.log，退出码0。Maintenance保留6条既有动画警告、PageVisibility保留1条空测试菜单提示，其余21项无测试警告。新测试覆盖已有槽位填充、名字/图标/颜色、GUID、同定义双戒指、需求变化、同实例选择保持/新实例重置、卸下隔离、阻止手工假数据、重复Construct、关闭解绑/重开、ASC失效恢复、PS更换、组件注销清空。
- **未验证**：真实WBP图标/文本/布局、PIE视觉、多人网络。未实现背包拖拽、点击卸装RPC、完整物品Tooltip、模型换装或装备随机词缀。穿戴/卸下仍使用现有组件接口及测试事件；本阶段完成真实数据展示与已有选择行为对接。
- 编辑器只需给ItemDefinition配置Presentation，确认Menu.BindEquipmentData开启，移除旧手填装备数据；保留现有十槽位和预览布局。详细步骤见[EquipmentUIBinding](EquipmentUIBinding.md)，StatsPanel契约见[CharacterStatsPanel](CharacterStatsPanel.md)。本轮未修改Content，保留既有用户/前阶段修改；未提交/推送。

## 2026-10-04 CombatInfo真实数据绑定：第六阶段代码完成

- 用户确认敌人易伤测试通过，继续第六阶段。现有UmbraCombatInfo/CombatStatEntry接入GAS、DerivedStats和Equipment事件，保持原WBP布局和行实例；新增CombatStatData的43个独立行标识及Live/StoredOnly/Placeholder/Unavailable状态。TestValue仅供Designer预览，运行时不伪装真实数据。Utility三绑定可选，三分类折叠独立，关闭页面解绑、重开刷新；Controller通知PlayerState变更，ASC生命周期和Pawn变更重绑。
- AD/AP及有效基础武器伤害读取派生快照（旧模式AD/AP读GAS），负重读取装备快照；类型抗性显示rating，攻速为现有普攻周期下的理论次/秒。伤害模块新增只读A分类摘要，按有效GE等级/层数/抑制状态汇总，全伤/类型/暴击/易伤分别展示；额外条件和X不混入标量，并以*及状态字段提示。旧CriticalDamageMultiplier不进入新暴击加成。无新增GAS属性、UI伤害计算、Tick或Content修改。
- **构建通过**：UE5.8.2 UmbraEditor Win64 Development，因原编辑器仍运行而使用独立验证副本Saved/DamageBucketsValidation。日志Saved/Logs/CombatBindingBuild.log、最终Saved/Logs/CombatBindingBuildFinal.log。原项目DLL未更新，需关闭编辑器并在Rider重建原项目后使用新增反射字段。首次测试代码调用Inhibit接口与UE5.8签名不符，已改用返回句柄及三参数版本，最终编译链接成功；存在既有引擎弃用/MSVC非首选提示。
- **最终22/22 Success，0测试错误**：上一阶段19项加Umbra.UI.CombatInfo.LiveBinding、SectionLifecycle、TestTextWithoutGameplay。报告Saved/Automation/CombatBindingFinal/index.json，日志Saved/Logs/CombatBindingAutomationFinal.log，退出码0。Maintenance既有6条动画警告、PageVisibility既有1条空菜单槽位提示，其余20项无测试警告。
- 新绑定测试验证GAS变化、A分类隔离、旧暴击倍率排除、GE添加/两层叠加/单层及全部撤销/抑制恢复、条件与X提示、四维驱动AD、基础值不含补正、容量事件、PS重绑、ASC失效/恢复、关闭解绑/重开、精确实例名及显式标识优先、类型抗性单位。原分类测试扩展第三组；预览测试首轮因SetDesignerFlags只追加标记而复用Designer实例失败，改用独立运行时实例后通过，未放宽测试文本隔离要求。
- **未验证**：真实WBP逐行身份配置与布局、PIE视觉、多人网络、自然到期专项。无Modifier的A/X GE若运行中直接SetActiveGameplayEffectLevel，引擎不发通用值变化事件；本阶段按移除重施或明确调用NotifyPlayerContextChanged处理，未增加通用等级变更通知框架。跨装备/派生/GAS复制不承诺原子UI更新。
- 数据分类、43行来源、蓝图手动步骤、范围和验收见[CombatInfoBinding](CombatInfoBinding.md)。用户继续负责编辑器验收；本轮未保存Content，保留既有修改，未提交/推送。

## 2026-10-03 敌人易伤GE快捷入口

- 同一Controller开发开关新增7应用/6撤销，默认引用已有GE_TestVulnerable。7即时光标选取未被遮挡、可攻击的UmbraEnemyCharacter，从玩家PlayerState ASC创建Level1 Spec应用到敌人ASC；同目标重复不叠加，切目标撤销上一目标测试效果。保存独立敌人ASC弱引用/句柄，6不影响玩家8/9 Buff；外部同类来源保护、外部移除后可重加、EndPlay清理。无新增Tick/RPC，蓝图可显式传Target调用。配置见[DamageBuckets](DamageBuckets.md#敌人易伤快捷键)。
- **原项目构建成功**：检查时编辑器已关闭，UE5.8.2 UmbraEditor Win64 Development直接更新原项目DLL。日志`Saved/Logs/QuickVulnerableBuild.log`、最终增量`Saved/Logs/QuickVulnerableBuildFinal.log`；保留引擎弃用和MSVC非首选版本提示。
- **最终9/9 Success，0测试错误/警告**：Umbra.Debug.QuickGameplayEffect，Umbra.Attributes下4项，Umbra.Damage下4项。报告`Saved/Automation/QuickVulnerableFinal/index.json`、日志`Saved/Logs/QuickVulnerableAutomationFinal.log`，退出码0。新增断言覆盖四键绑定幂等、易伤资产类解析、敌人Level1/玩家来源、10次重复仅一份加成、非敌人拒绝保留原效果、切敌清理、撤销与玩家Buff隔离、外部来源保护和外部移除后重加。
- 首轮测试世界未InitializeActorsForPlay，导致BlueprintNativeEvent可攻击判断被跳过；补齐世界上下文/初始化后又发现手动InitializeComponent与自动初始化重复，已加HasBeenInitialized判断，最终重建重跑全部9项成功。仅修正测试场景，没有绕过运行时目标检查。
- **未验证**：PIE光标选择/遮挡/实体按键、GE_TestVulnerable资产内部标签配置与实际视觉、多人网络、自然到期/EndPlay专项。自动化用已知原生GE验证应用与句柄，实际易伤资产只确认类可加载。仍由用户编辑器验收；本轮没有编辑或保存Content，保留既有修改，源码空白检查通过；未提交/推送。

## 2026-10-03 测试GE快捷应用与撤销

- 现有Controller新增开发快捷键8/9及BlueprintCallable应用/撤销接口，默认软引用用户已有GE_TestDamageBonuses。权威端从PlayerState获取ASC，以Level1应用并保存句柄；重复按键不叠加、不刷新；按句柄撤销，不移除其它来源。同类效果已由外部应用则拒绝重复；外部移除后允许重新应用；切换PlayerState后下一次应用清理旧ASC，EndPlay清理。无Tick、无远端客户端RPC。配置、蓝图替代接线和日志见[DamageBuckets](DamageBuckets.md#测试ge快捷入口)。没有修改Content资产。
- **构建通过**：UE5.8.2 UmbraEditor Win64 Development，独立验证副本`Saved/DamageBucketsValidation`，日志`Saved/Logs/QuickGEBuild.log`。存在引擎Character.h既有弃用警告；原项目DLL未替换，用户需保存/关闭编辑器并在Rider重建原项目。验证副本全部Source与工作区逐文件哈希一致。
- **19/19 Success，0测试错误**：上一节第五阶段18项加新增`Umbra.Debug.QuickGameplayEffect`。报告`Saved/Automation/QuickGE/index.json`、日志`Saved/Logs/QuickGEAutomation.log`，进程退出码0。Maintenance保留6条动画警告、PageVisibility保留1条空菜单提示，其余17项无测试警告。新增测试覆盖输入绑定幂等、真实测试资产类可解析、Level1、连续10次应用同句柄且仅一份加成、撤销恢复、外部移除后重加、外部同类保护和PlayerState切换清理。
- **未验证**：真实PIE按键分发、GE_TestDamageBonuses内部配置与实际增伤效果、多人网络、自然到期和EndPlay清理专项测试。自动化使用原生已知GE验证应用/撤销行为；真实资产仅验证类可加载。用户负责编辑器验收；未提交或推送，保留已有代码和资产修改。

## 2026-10-03 A/X与暴击易伤：第五阶段代码完成，显式开启

- 用户确认第四阶段测试完成后，本轮接入第五阶段。规则与编辑器步骤见[DamageBuckets](DamageBuckets.md)。新增GameplayEffect内部`UUmbraDamageBonusComponent`与条件化条目，按类型、BasicAttack/Skill、统一暴击、目标易伤及双方Tag Requirements筛选。A按分数加总，X按独立倍率连乘；GAS句柄、等级、层数、移除与Inhibition负责生命周期，无额外Tick或AD/AP写回。
- Typed.UseDamageBuckets默认false，保留第四阶段总暴击倍率。显式开启后使用DamageRules的BaseCriticalMultiplier=1.5、BaseVulnerableMultiplier=1.2，CriticalDamageMultiplier不再参与新规则；额外暴击/易伤增伤仅作为满足条件的A项。新增State.Vulnerable标签，可由目标GE授予。Legacy始终保留旧公式。A/X不会自动接入现有装备生成或CombatInfo。
- 完整伤害仍逐类型计算两层防御、一次IncomingDamage和统一暴击；日志新增A/X/C/V及Buckets/Vulnerable，仍一次总飘字。新GE数据在应用和命中时校验：拒绝Instant/周期GE、负值/非有限值、无效筛选/倍率溢出。普通GE叠层A=N×数值，X=数值^N，移除与禁用后下次命中不再贡献。
- **构建成功**：UE5.8.2，UmbraEditor Win64 Development。因原编辑器仍运行，使用`Saved/DamageBucketsValidation`独立Source/Config/Intermediate/Binaries及Content目录联接，NoHotReloadFromIDE；没有替换原项目已加载DLL。完整日志`Saved/Logs/DamageBucketsBuild.log`，最终测试增量编译/链接`Saved/Logs/DamageBucketsBuildFinal.log`。最终验证副本与工作区全部Source逐文件哈希一致。用户需保存/关闭编辑器并在Rider重建原项目后使用新增反射类。
- **最终18/18 Success，全部0测试错误**：`Umbra.Attributes.DebugInputAndWidget`、`DebugOperations`、`Lifecycle`、`WeaponDerivedPower`；`Umbra.Combat.Maintenance`；`Umbra.Damage.EntryCompatibility`、`Types`、`TypedChannels`、新增`BucketsAndMigration`；`Umbra.Equipment.RequirementsAndLifecycle`；`Umbra.UI.CharacterStats.PrimaryRows`、`DamageNumberLogic`、`PlayerAttributeBarState`；`Umbra.UI.Equipment.EmptySlotIcons`、`PageVisibility`、`PreviewLifecycle`、`SlotBindings`、`SlotContract`。报告`Saved/Automation/DamageBucketsFinal/index.json`，日志`Saved/Logs/DamageBucketsAutomationFinal.log`，退出码0。Maintenance既有6条动画警告、PageVisibility空测试菜单1条槽位提示；其余16项无测试警告，包括新测试。
- 新测试覆盖四种暴击/易伤组合（185/345/258/468）、混合类型X得到603.9、来源隔离、双方标签条件、Buff撤销、两层叠加/移除一层、Inhibit/恢复、可配置基础C/V、CanCritical关闭、旧总倍率9与新基础1.5互不重复、非法GE拒绝、单次扣血及AD不变。
- 首轮旧Maintenance因原生测试PlayerState没有武器派生数据、实际GA_BasicAttack已经由用户切到WeaponChannels而失败；已仅在临时测试角色上按能力模式初始化徒手20派生数据，保留原伤害/命中去重断言，未修改能力资产。测试叠层配置曾遇UE5.8 SetStackingType未导出导致链接失败，改用带说明的兼容字段写入后最终构建通过；无游戏代码绕过检查。
- **未验证**：原编辑器加载本阶段类、增伤/易伤GE资产手动配置、真实PIE、持续时间到期专项测试、多人网络与UI显示。本阶段没有批量新增UI占位属性、完整威能/词缀、特殊AD/AP百分比或CombatInfo绑定。旧面板的CriticalDamageMultiplier仍是旧总倍率，不能当新A区暴击加成显示。
- 本轮未操作编辑器或保存Content，保留用户已有GA_BasicAttack、BP_UmbraPlayerController、BP_UmbraPlayerState、WBP_CombatInfo和Content/Items修改；源码空白/文档链接/验证副本源文件一致性检查通过。未提交/推送；原项目DLL尚未更新。下一步可接CombatInfo真实数据来源与事件订阅。

## 2026-10-03 九类型伤害：第四阶段代码完成，按能力选择模式

- 用户已确认上一阶段装备测试成功；此确认不等于多人复制或全部UI验收。本轮继续实现九类型伤害，迁移步骤、参数来源及公式见[TypedDamage](TypedDamage.md)。未代改用户的能力/装备/Controller蓝图。
- DamageConfig.Typed支持Legacy（默认）、WeaponChannels和ExplicitChannels。武器继承直接使用已补正/惩罚后的各类型明细乘WeaponMultiplier，不再叠加AD/AP；显式类型攻击使用各自Base+AD/AP系数，不隐式加入武器。统一入口选择原生Instant Typed GE，旧GE/旧公式默认保留，来源/测量标记继续独立。
- AttributeSet新增九个实际参与结算的类型抗性rating，默认0、非负、GAS复制/通知完整。新Execution按Armor或MagicResistance及对应类型抗性逐项减伤，共用一次暴击判定，汇总一次IncomingDamage。可选DamageRules资产配置一般防御K、目标等级曲线、类型转换K和默认30%减伤上限。临时等级来源是目标EquipmentComponent.CharacterLevel，无组件时1；不使用技能等级代替目标等级。
- 新模式仍用现有CriticalDamageMultiplier总倍率，未实现A/X或易伤。单个总伤害飘字用最终物理/魔法占比更大侧的现有颜色（平局物理），不扩展九类型颜色协议。伤害日志增加DamageChannel/ DamageTyped，原Health结算链不变。
- **原项目构建成功**：UE5.8.2，UmbraEditor Win64 Development；新增UHT、C++和最终链接通过，日志`Saved/Logs/TypedDamageBuild.log`。首次新增测试类名与旧测试冲突导致链接失败，已改为独立类名；首轮自动化的重复类型输入触发TArray自引用断言，已改为先复制再添加，并重新构建、重新运行全组。既有MSVC非首选版本和引擎弃用提示未扩展处理。
- **最终17/17 Success，全部0测试错误**：`Umbra.Attributes.DebugInputAndWidget`、`DebugOperations`、`Lifecycle`、`WeaponDerivedPower`；`Umbra.Combat.Maintenance`；`Umbra.Damage.EntryCompatibility`、`Types`、新增`TypedChannels`；`Umbra.Equipment.RequirementsAndLifecycle`；`Umbra.UI.CharacterStats.PrimaryRows`、`DamageNumberLogic`、`PlayerAttributeBarState`；`Umbra.UI.Equipment.EmptySlotIcons`、`PageVisibility`、`PreviewLifecycle`、`SlotBindings`、`SlotContract`。报告`Saved/Automation/TypedDamageFinal/index.json`、日志`Saved/Logs/TypedDamageAutomationFinal.log`，退出码0。Maintenance既有6条动画警告、PageVisibility空测试菜单1条槽位提示，其余15项无测试警告；启动自动化注册日志不等于用例事件。
- 新测试验证：AD160/AP70武器完整继承230；Slashing160/Fire70、Armor100/MR300、对应抗性100/25结算70；统一必暴140/禁止暴击70；武器倍率；显式Fire技能46；九属性逐个映射与其它类型抗性隔离；30%上限/负数钳制；目标等级曲线与技能等级无关；无效K/重复类型/负伤害整体拒绝；零伤害；单次Spec、Health变化、元属性清零、来源与测量/飘字标记；Legacy仍174。
- **未验证**：用户实际能力BP切换新模式、真实地图普攻/敌人攻击与受击死亡、多人网络和飘字视觉。用户继续负责编辑器验收。没有修改Content；保留用户已有BP_UmbraPlayerController、BP_UmbraPlayerState、WBP_CombatInfo与Content/Items修改。文档链接和源码空白检查通过；未提交/推送，模块/Target未修改。
- 下一阶段：A区条件筛选、X区独立倍率、新暴击/易伤与旧CriticalDamageMultiplier兼容迁移；CombatInfo真实绑定后续接入。当前不应把类型抗性rating当百分比，或把暴击总倍率当A区加成。

## 2026-10-03 最小装备组件：第三阶段代码完成，默认关闭

- 新增ItemDefinition、EquipmentComponent及原生Infinite EquipmentEffect；玩家PlayerState/敌人Character持有默认关闭组件。复用现有槽位枚举，抽到Gameplay共享头并由旧UI头转引，枚举身份和顺序不变。规则、配置来源、接口语义及人工步骤见[EquipmentFoundation](EquipmentFoundation.md)。
- 支持槽位/等级拒绝、实例GUID去重、同槽替换/卸下、无条件四维词缀、排除自身GE句柄的四维需求、武器基础/补正惩罚、自带Armor/MagicResistance惩罚、已穿戴负重/Strength容量/四档负重。普通词缀不提供直接AD/AP；未增加GAS负重属性或移动惩罚。
- 装备接管DerivedStats武器输入，主属性变化先重算需求再发布AD/AP；GE添加/移除期间暂停派生发布，伤害入口对来源和目标的装备中间态均拒绝。外部误移除装备GE时快照失效，卸下可恢复；相同ASC重绑保留装备且不回血。UI通过复制快照/事件读取，当前WBP尚未接入。
- **原项目构建通过**：UE 5.8.2，UmbraEditor / Win64 / Development，新增反射类UHT、C++和链接成功；最终日志`Saved/Logs/EquipmentBuild.log`。本轮编辑器未运行，直接更新原项目DLL，无独立验证副本。存在非首选MSVC及引擎Character.h既有弃用警告。模块/Target未修改。
- **最终自动化16/16 Success，全部0测试错误**：`Umbra.Attributes.DebugInputAndWidget`、`DebugOperations`、`Lifecycle`、`WeaponDerivedPower`；`Umbra.Damage.EntryCompatibility`、`Types`；`Umbra.Combat.Maintenance`；新增`Umbra.Equipment.RequirementsAndLifecycle`；`Umbra.UI.CharacterStats.PrimaryRows`、`DamageNumberLogic`、`PlayerAttributeBarState`；`Umbra.UI.Equipment.EmptySlotIcons`、`PageVisibility`、`PreviewLifecycle`、`SlotBindings`、`SlotContract`。报告`Saved/Automation/EquipmentFoundationFinal/index.json`，日志`Saved/Logs/EquipmentAutomationFinal.log`，命令退出码0。
- Maintenance的6条既有动画除零/播放率警告，以及PageVisibility纯C++空菜单测试的1条0/10槽位配置提示仍在；其余14项无测试警告。故障注入故意移除装备GE，捕获5次预期失效日志并断言伤害阻止及恢复。引擎启动自动化注册Condition failed不是测试用例失败，不称整个启动日志零错误。
- 新增测试覆盖自身词缀不能满足自身需求（含外部乘法Buff的反例）、其它装备支持、武器/防御惩罚及恢复、负重不影响移速、GE拒绝保留旧装备、反复穿戴/初始化不叠加、双方伤害中间态保护、重绑不回血、销毁精确清理。新增测试单独放在`UmbraEquipmentGameplayTests.cpp`，旧`UmbraEquipmentTests.cpp`原样保留。
- **未验证**：实际ItemDefinition/BP接线、真实输入/PIE、多人网络复制顺序、UMG显示。未做完整Inventory/拥有权校验、客户端换装RPC、随机词缀/威能、负重动作惩罚、九类型伤害结算或CombatInfo真实绑定。现有普攻AD/AP系数、暴击/护甲算法及Health/Resource/MoveSpeed规则不变；此前160+70×0.2=174仍为当前配置的正确结果。
- 本轮未操作编辑器或保存Content资产，保留用户已有BP_UmbraPlayerState/WBP_CombatInfo及Content/Items修改。源码空白/文档相对链接检查通过；未提交或推送。后续顺序为九类型伤害结算与混合普攻继承，再推进A/X、新暴击与UI真实绑定。

## 2026-10-02 武器与AD/AP派生：第二阶段代码完成，默认保持旧模式

- 新增 `UUmbraWeaponProfile`：九类型基础伤害及逐类型四主属性补正，支持可选FloatCurve；新增 `UUmbraDerivedStatsComponent`，玩家PlayerState与敌人Character各持有一个，默认关闭。配置/公式/覆盖顺序及人工步骤见 [WeaponDerivedPower](WeaponDerivedPower.md)。
- 服务器监听四主属性并从完整输入重算类型明细，用Instant Override GE发布AD/AP；不读取旧AD/AP累计。徒手后备Blunt10，可配置；SetWeaponProfile支持测试武器切换与回到徒手，尚不是正式装备接口。来源明确的普通AD/AP Modifier在新模式拒绝，派生调试GE仅去掉直接AD/AP加成，其余调试功能保留。
- 支持复制的有效性/版本/徒手标识/类型明细/AD/AP快照及变化事件；重入主属性更新有界排空，发布期间统一伤害入口拒绝读取半更新结果。注销/销毁移除监听与应用查询。现有DamageExecution及AttackSpeed/Health/Resource/MoveSpeed算法未修改。
- **构建通过**：关联UE 5.8.2，UmbraEditor Win64 Development，日志 `Saved/Logs/DerivedStatsBuild.log`。首次编译发现复制宏参数名错误，修正后最终完整构建成功。因用户编辑器仍在运行，构建使用 `Saved/DerivedStatsValidation` 独立Source/Config/Intermediate/Binaries，Content为只读测试使用的目录联接；使用NoHotReloadFromIDE，不触碰原项目正在加载的DLL。Source逐文件哈希与工作区一致。存在非首选MSVC/引擎弃用警告；模块/Target未改。
- **自动化10/10 Success，全部0测试错误**：`Umbra.Attributes.DebugInputAndWidget`、`DebugOperations`、`Lifecycle`、新增`WeaponDerivedPower`；`Umbra.Damage.EntryCompatibility`、`Types`；`Umbra.Combat.Maintenance`；`Umbra.UI.CharacterStats.PrimaryRows`、`DamageNumberLogic`、`PlayerAttributeBarState`。报告 `Saved/Automation/WeaponDerivedPower/index.json`，日志 `Saved/Logs/WeaponDerivedPowerAutomation.log`，退出码0。Maintenance有6条既有动画/播放率警告，其余9项0警告。引擎启动自动化注册日志不等于测试用例事件。
- 新测试覆盖：默认旧模式、徒手/混合武器/纯魔法与曲线算例、20次重算无累积、主属性Buff添加/移除、拒绝旧直接AD/AP GE、无效配置保持原选择、卸下/重选、同步回调改主属性、发布期间拒绝伤害、重绑不回血、销毁解绑；另验证派生AD180经旧Execution与Armor100结算90伤害。
- **尚未实现**：装备要求/负重/背包、特殊AD/AP倍率、A/X区、新暴击、九类型伤害结算与混合普攻继承、CombatInfo派生快照接线。不要以九类型配置或AP汇总推断默认AP系数0的普攻已经继承魔法伤害。
- **未验证**：用户原编辑器加载新类、实际DataAsset/BP配置、地图与多人PIE、快照网络时序及UI视觉。用户自行执行编辑器验收。需保存并关闭编辑器后完整构建原项目再加载新增反射类，不能把独立副本构建称为原DLL已更新。
- 本轮未操作编辑器、未保存Content资产；原有WBP_CombatInfo资产哈希保持不变。源码差异空白、文档链接及验证副本源文件一致性检查通过；未提交或推送。

## 2026-10-02 统一伤害入口：第一步代码完成，编辑器验收由用户执行

- 新增 `FUmbraDamageRequest` / `UmbraDamage::Apply`；玩家、敌人普攻显式使用 `Damage.Source.BasicAttack`，主动技能调用方可选择 `Damage.Source.Skill`。旧 `UmbraPhysicalDamage::Apply` 保留为兼容适配；旧 false 参数不推断为 Skill。玩家伤害测量标记单独保留，敌人普攻不新增该标记。详见 [统一伤害入口](UnifiedDamageEntry.md)。
- 只迁移提交入口：旧 Execution、AD/AP来源、暴击总倍率、初始化/调试 GE、生命/资源/移动、普攻前摇/周期/同击去重、受击死亡及飘字结算链未修改。尚未实现装备派生、九类型结算、A/X区或新暴击。
- **修改前基线**：关联 UE 5.8.2，UmbraEditor / Win64 / Development 构建成功（目标已是最新）；日志 `Saved/Logs/DamageEntryBaselineBuild.log`。6项自动化全部 Success，报告 `Saved/Automation/DamageEntryBaseline/index.json`；Maintenance 有6条既有警告，其余5项无警告/错误。
- **修改后构建**：原项目完整 UHT/C++/链接成功；日志 `Saved/Logs/DamageEntryBuild.log`。非首选MSVC版本及引擎 Character.h 弃用警告仍存在。模块/Target未改，无需重新生成工程。
- **修改后自动化7/7通过，全部0测试错误**：`Umbra.Attributes.Lifecycle`、`Umbra.Damage.Types`、新增 `Umbra.Damage.EntryCompatibility`、`Umbra.Combat.Maintenance`、`Umbra.UI.CharacterStats.PrimaryRows`、`Umbra.UI.DamageNumberLogic`、`Umbra.UI.PlayerAttributeBarState`。报告 `Saved/Automation/DamageEntry/index.json`，日志 `Saved/Logs/DamageEntryAutomation.log`，退出码0。Maintenance仍有6条既有动画蓝图除零/高速动画播放率警告，其他6项0警告。引擎启动另有自动化注册 Condition failed 日志，不能把整个启动日志称为零错误。
- 新增测试验证现有玩家伤害GE加载、物理/魔法及非暴击/必暴固定算例、新旧入口等价、单次提交与Health变化、IncomingDamage归零、来源互斥、玩家测量标记、Level传递、旧false语义、零伤害与非法请求拒绝。旧入口与新入口共用实现，固定预期数值断言用于防止仅比较两条相同错误路径。
- **代码复核**：双方能力仅替换伤害提交调用，既有权威检查、命中去重、前摇和清理逻辑保留；执行器和AttributeSet无差异。此后仅补文档，未再次改动已编译的源码。
- **未验证**：实际地图普攻/敌人攻击、受击死亡和飘字观感、真实输入、多人PIE。编辑器曾打开L_Prototype，但验收被用户停止，不能计为PIE通过。用户明确后续编辑器端验收自行完成；不再由代理操作编辑器，人工步骤见上述指南。
- 保留工作区原有 `WBP_CombatInfo.uasset` 修改，本轮不保存任何Content资产；未提交或推送。生成日志与报告在Saved下，不提交。

## 2026-10-01 第二页战斗属性：C++ 支持完成，蓝图由用户手动接线

- 新增 `UUmbraCombatInfo` 与 `UUmbraCombatStatEntry`，只实现分类折叠/展开和固定测试文本显示。后者复用原 StatEntry 的纯 View，不扩展 Stat 枚举；没有 GAS、AttributeSet、GameplayEffect、装备或伤害计算，没有 Tick。原菜单切页、第一页、HeroInfo、TalentInfo及原StatEntry代码不变。
- UE 5.8.2 只读确认：实际 AttributeMenu 的 WidgetSwitcher_0 为 HeroInfo滚动容器（含HeroInfo+PrimaryAttribute）、CombatInfo、TalentInfo，CombatInfo仍为索引1。其内部 ScrollBox_1 → Overlay_151 → 原纸张Image_188 + VerticalBox_1，PrimaryAttribute仅为占位实例。检查报告 `Saved/CombatInfoInspection.json`、`Saved/CombatInfoProbe.json`；最终只读检查均0 error/0 warning。未保存资产，GUI未保存状态不在报告范围。
- 用户明确选择手动编辑蓝图。完整[CombatInfoUI接线说明](CombatInfoUI.md)包含行副本/父类、六个BindWidget、16项攻击与10项防御测试值、原Overlay Slot Left→Fill、滚动、分类交互及三页PIE验收。**当前蓝图仍是占位状态，不能称UI或三页集成已完成。** Content工作区无改动，新行资产尚未创建；uasset的Git LFS规则已核对。
- **完整构建通过**：关联UE 5.8.2，原项目 UmbraEditor / Win64 / Development，`Saved/Logs/CombatInfoBuild.log`。含最终测试代码的UHT/C++/链接成功；存在非首选MSVC版本提示和引擎Character.h既有弃用警告。未改变模块或Target，无需重新生成工程。
- **自动化3/3通过，每项0 warning/0 error**：`Umbra.UI.CombatInfo.SectionLifecycle`验证初始展开、独立折叠、箭头、重复Construct不重复绑定、Destruct解绑、重开保留状态且不追加行；`Umbra.UI.CombatInfo.TestTextWithoutGameplay`验证没有World/Player/ASC时原样显示整数/百分号/小数及空值清除；既有`Umbra.UI.CharacterStats.PrimaryRows`回归通过。报告`Saved/Automation/CombatInfoUI/index.json`，日志`Saved/Logs/CombatInfoAutomation.log`，退出码0。
- 文档本地文件链接、差异空白和改动范围检查通过。**未验证**：手动接线后的WBP编译、26行实际渲染、中文长名称、鼠标折叠/滚动、三页按钮集成及真实PIE。NullRHI原生测试不代表这些已验收。未提交或推送。

## 2026-09-30 当前文字按钮基础交互

- 用户选择现成W_ButtonBrownSquare_1作为顶部五个过滤器与底部整理按钮。UE 5.8.2只读核对六个实例、模板父类UserWidget、内部Button_0/TextBlock_0、默认Text参数，以及Up/Over/Down/Disabled四种Brush与悬停/按下音效。报告 `Saved/InventoryButtonInspection.json`，命令行加载0 error/0 warning；实例变量的Instance Editable标记及完整Graph未通过此次反射读取确认，仍待Editor核对。
- 更新[Inventory按钮步骤](InventoryPhase1.md#52-当前文字按钮w_buttonbrownsquare_1后续选择以本节为准)：优先修改各实例Text；保留UMG原生Hover/Pressed反馈。无需物品、标签、父类迁移或新增C++。文字未驱动时才补已有Text到TextBlock_0的PreConstruct接线；改模板时使用项目副本，避免影响其它UI。过滤/整理暂不改变内容或保持Selected。
- 本轮未修改C++/二进制资产，未执行新构建或真实PIE。已核对资产配置、文档文件链接和差异；不能宣称实际鼠标命中、中文字体和显示已验收。

## 2026-09-30 Inventory格子压窄与行距诊断

- 已只读加载当前保存的Inventory/InventorySlot/CharacterMenu，报告 `Saved/InventoryLayoutInspection.json`，命令行检查0 error/0 warning。Slot根200×200；Inventory根1000×1500；Grid的ScrollBox Slot为Size Fill 1与双向Fill，Grid Slot Padding为0；右栏ScaleBox_2为Scale To Fit。
- 结合截图及UE 5.8 SUniformGridPanel源码：8列200格需至少1600宽，父级仅1000导致每列约125宽，格子横向压窄；Grid纵向Fill分摊多余行高，造成不是Padding定义的行距。右栏ScaleBox等比缩放不直接改变宽高比。
- [完整指南第8节](InventoryPhase1.md#8-2026-09-30-实际槽位压窄与行距诊断)给出准确层级：Grid的ScrollBox Slot改Auto/Left或Center/Top；ScrollBox本身仍Fill；通过Grid Slot Padding分别控制水平/垂直间距；保留200格/8列时增加根设计宽度，考虑整页缩放与DPI。修正了指南中遗漏ScrollBox Slot Size=Auto的步骤。
- 本次没有修改C++或二进制资产，不需新构建；用户未保存Graph与修正后的PIE画面尚未验证。已做文档链接/差异检查，保留用户全部资产改动。

## 2026-09-30 布局澄清：三栏并排与文字分类按钮

- 用户确认Attribute、Equipment、Inventory在CharacterMenu内同时显示；未来SkillTreeMenu与整个CharacterMenu在外层并列。已撤回首轮误加的CharacterPageSwitcher、Tab_Character/Equipment/Inventory和SetActiveTab；CharacterMenu的h/cpp与本次Inventory开发前版本无差异。原预览监听/启停和属性页内部Tab不改。
- 将误设的 `Umbra.UI.Inventory.CharacterMenuTabs` 测试替换为 `Umbra.UI.Inventory.CharacterMenuComposition`，检查三栏同时显示、Inventory可见性不关闭Equipment预览、整菜单关闭/重开与三栏保留。空Grid生命周期与Rarity修复继续保留。
- 更新[完整指南](InventoryPhase1.md)：右侧加入Inventory容器；从AttributeMenu实际按钮复制WBP_InventoryFilterButton文字变体，保留原Style，隐藏icon_object，Label驱动LabelText。当前只使用Hover/Pressed视觉，无过滤或切页订阅，不新增按钮C++类。同步纠正旧Equipment接入指南。
- **编译通过**：UE 5.8.2，UmbraEditor / Win64 / Development，日志 `Saved/Logs/InventoryCompositionBuild.log`。当前用户Editor保持打开，构建使用 `Saved/InventoryCompositionValidation` 独立源码副本，Source逐文件哈希与工作区一致；原项目DLL未替换。存在引擎弃用/非首选MSVC警告；加载修正需保存并关闭Editor后完整构建原项目，不使用Live Coding移除反射字段。
- **自动化最终结果7项通过**：首批 `Saved/Automation/InventoryComposition` 中EmptyGridLifecycle及五项Equipment测试成功，PageVisibility仍有空夹具0/10槽的既有警告。新增CharacterMenuComposition最初因未构造Slate而使用IsVisible查询产生3个断言失败；修正为显式UMG可见性状态断言后，重新编译并单独重跑，在 `Saved/Automation/InventoryCompositionRetest` 为Success（0 warning/0 error）。其余预览启停与三栏保留断言首轮即通过。最终Source哈希复核一致；该测试只验证原生组合契约，不渲染真实WBP。
- **资产与未验证边界**：保留用户已修改的WBP_InventorySlot、已新增的WBP_Inventory和MI_UI_InteractionHighlight_Inventory，未编辑或保存任何二进制资产。按钮实际Graph、三栏右侧布局、真实鼠标和PIE画面由用户按指南验证，不能由原生测试推断通过。文档本地文件链接和Source/Docs差异空白检查通过；未提交或推送。

## 2026-09-30 第一阶段空背包（首轮记录；布局以随后的修正为准）

- 新增 `UUmbraInventoryMenu / UUmbraInventorySlot`：40容量、8列、可配置SlotClass；C++动态UniformGrid与唯一Selected；同实例重构/重开复用格子，委托成对解绑/唯一绑定，无Tick、无物品系统或测试物品资产。容量显示0/40；所有物品内容层Collapsed，Hover/Selected只控制高亮。
- CharacterMenu增加可选的顶层Switcher/三Tab绑定，复用UmbraMenuTabButton和既有SetActiveWidgetIndex流程。预览观察/启停实现、FullBody Preview资产、GAS属性和装备功能未改。EquipmentSlot在BP视觉刷新之后强制按HasItem显示RarityFrame，空槽始终隐藏。
- 本轮只读加载已保存资产：CharacterMenu父类正确，但树仍为属性/装备HorizontalBox并排，没有顶层Switcher；InventorySlot为UserWidget父类且缺新绑定；EquipmentSlot的RarityFrame默认Visible。完整Graph、GUI未保存改动仍待编辑器确认。报告 `Saved/InventoryPhase1Inspection.json`；命令行只读检查成功，0 error/0 warning。没有改动或保存任何Content二进制资产。
- **构建通过**：本机已核对UE 5.8.2路径，原项目 `UmbraEditor / Win64 / Development` 完整UHT/C++/链接及补充Tab后的最终构建成功，日志 `Saved/Logs/InventoryBuild.log`。工具链有非首选MSVC提示及引擎Character.h的既有弃用警告。不是Live Coding。
- **自动化通过：7/7**，报告 `Saved/Automation/InventoryPhase1/index.json`，日志 `Saved/Logs/InventoryAutomation.log`。`Umbra.UI.Inventory.EmptyGridLifecycle` 验证40格/坐标、Hover与Selected、空内容层、重复Construct/Destruct及关闭/打开不追加、重绑、配置重建、非法容量/列数与缺类；`Umbra.UI.Inventory.CharacterMenuTabs` 验证三页选择、非法Index、重复打开/销毁重绑及装备预览激活通知；`Umbra.UI.Equipment.EmptySlotIcons` 增加所有空槽交互状态下RarityFrame隐藏及既有快照清空后的回归。另有Equipment的SlotContract、SlotBindings、PageVisibility、PreviewLifecycle通过。PageVisibility测试有原生空夹具0/10 unique slots的一条警告，其余六项0 warning/0 error；没有把该警告当成实际WBP缺槽。
- **交付/未验证边界**：C++已可构建且原生测试通过；WBP_Inventory、Slot迁移、Inventory MI和顶层Tab的Designer/Graph操作尚由用户按[完整Blueprint指南](InventoryPhase1.md)执行。没有实际创建/保存这些WBP或运行最终地图PIE，不声称40格画面、真实鼠标命中/滚动、材质、FullBody颜色、输入模式或多人已验收。测试用NullRHI，不能验证渲染。本轮没有提交或推送。

## 2026-09-29 Equipment Preview 回归调查更新

- 已实际完成原流程及 A/B 真实 RHI 离屏游戏验证，三次均蓝灰；取消材质复制和 C++ Capture 覆盖均未恢复。17个材质槽逐一一致，灯光与 Materials/Lighting 最终开启。蓝灰具体根因、责任行与最小修复尚未确认，正式源码/资产未改。
- Preview 核心源码与重构前保存副本哈希一致；相关文件未提交，无最后正常 Git 版本可指认。历史日志确认13:57删除关卡全身 Preview，17:37根菜单 Reparent；历史放置实例与当前自动 Spawn 实例状态不同，不能将其直接认定为颜色根因。用户确认最后正常画面来自真实 PIE。
- 完整证据、结果、验证限制与复现命令见 [Preview 回归报告](PreviewRegression.md)。下节早先优先关闭雾的建议已被本轮证据流程取代，不再作为当前修复指导。

## 2026-09-29 PIE 装备预览静止排查

- 17:41 根据用户“RT 缩略图正常、UI 材质缩略图纯色”的截图追加检查：已保存 M_UI_CharacterFullBody 的纹理 RGB → Multiply、A → OneMinus 端口正确，不能归因为 Alpha 误接颜色。缩略图可能不同步，仍需 Capture 雾效开关及 PIE 实时 RT 对照；诊断脚本成功，未修改源码或资产。

- 后续用户确认 Reparent 后人物会动，但出现蓝灰色剪影。17:39 只读复查确认根菜单现已继承 UmbraCharacterMenu，Capture 的 ShowFlagSettings 无覆盖、展示位置 Z=-100000cm，Image 白色 tint、灯光开启、材质节点来源符合预期。高度雾覆盖是优先排查项，建议仅关闭 PortraitCapture 的 Fog/Atmosphere/Volumetric Fog 后重新 PIE；未做实际渲染对照，不能宣称根因或修复已确认。证据与下一步见 [会动但纯色](BlueprintCapturePreview.md#人物会动但呈蓝灰色剪影)。本轮只读脚本0 error/0 warning，仅更新文档，无源码/资产修改。

- 用户反馈 PIE 打开装备页后人物一直不动。17:32 用关联 UE 5.8.2 命令行编辑器只读加载当前保存资产：WBP_CharacterMenu 的实际 Parent Class 是 UserWidget，WBP_Equipment 是 UmbraEquipmentMenu；Controller 指向该 WBP_CharacterMenu。按当前源码，根类不匹配导致 Controller 跳过 SetMenuOpen，已有自动预览激活链路不执行。
- BP_CharacterFullBodyPreview 的 PortraitMesh 已配置 Idle、循环/播放开启、速率1、PauseAnims/NoSkeletonUpdate关闭；Equipment 的 PreviewActorClass 指向该 BP，UseBlueprintConfiguration=true、CaptureRate=30。未发现日志中有缺 Idle 或 RT 冲突诊断。
- 修正操作指南：WBP_CharacterMenu 需 Reparent 为已有 UmbraCharacterMenu，再编译/保存并重启 PIE；不要把根菜单误改为 UmbraCharacterStatsPanel。保持原布局和切页 Graph。详见 [预览排查](BlueprintCapturePreview.md)。上轮指南遗漏了这个既有父类前置条件，本次已补正。
- 已验证：只读脚本成功（0 error/0 warning）、源码启动调用链与当前日志核对、文档差异检查。仅修改文档及 Saved 内诊断脚本/报告；没有修改源码或资产，无需新构建。GUI 未保存状态、Blueprint 完整 Graph 与实际 PIE 恢复仍待编辑器确认，不能宣称画面已恢复。

## 2026-09-29 CharacterMenu 四维通用行与槽位空图标

- 扩展现有 UmbraEquipmentSlotWidget：WBP Class Defaults 的 `EmptySlotIcons` 按原 `EUmbraEquipmentSlot` 十类型选择 `EmptyIcon`，无装备时显示，有装备时隐藏；缺配置清除旧 Brush。现有 ItemIcon、Hover/Selected/Locked/Rarity/BP_RefreshVisual 顺序保持。
- 复用 UmbraStatEntry / UmbraCharacterStatsPanel：原 Stat 枚举末尾追加四维；父容器仅读取 AttributeSet 已有 Strength/Dexterity/Intelligence/Faith，按注册行订阅 GAS，单项变化只刷新对应行。StatDisplayData 配置本地化名称与 Icon；SetStatDisplay 是纯显示接口。保留旧八项 HUD 的图标/数值事件路径。Controller 既有 PlayerState 通知覆盖嵌套 CharacterMenu 面板。
- 已用 UE 5.8.2 只读核对保存的槽位、菜单与四维资产：十槽身份已正确配置；Slot 缺 EmptyIcon；四维所属 WBP_PrimaryAttribute 及四个独立条目的原生父类均为 UserWidget，没有通用 WBP_StatEntry 资产。GUI 未保存状态、Graph 完整接线和新布局仍待编辑器确认。操作、配置覆盖顺序和完整修改清单见 [四维与空槽完整指南](CharacterMenuStatsAndIcons.md)。没有修改二进制资产、Preview 或战斗源码。
- **构建通过**：关联 UE 5.8.2，UmbraEditor / Win64 / Development；完整 UHT、C++、链接，以及最终测试修正后的增量构建均成功。验证副本 `Saved/CharacterMenuStatsValidation-20260929` 的 Source 哈希与工作区一致；日志 `Saved/Logs/CharacterMenuStatsBuild.log`。原 Editor 保持运行，原项目 DLL 未替换，需要用户保存、关闭 Editor、构建原项目并重开。工具链有非首选 MSVC 提示，完整构建有引擎 Character.h 既有弃用警告。
- **自动化通过（最终各项结果）**：`Umbra.UI.Equipment.EmptySlotIcons`、`SlotBindings`、`SlotContract` 在 `Saved/Automation/CharacterMenuStats-20260929` 为 Success，三项均0 warning/0 error。`Umbra.UI.CharacterStats.PrimaryRows` 修正临时 World 的控制器注册/重复组件初始化后，在 `Saved/Automation/CharacterMenuPrimaryRows-20260929` 单独重跑为 Success（0 error，1条 GameplayCueNotifyPaths 未配置警告）。首轮合并报告内 PrimaryRows 的失败不是最终结果。覆盖十种空图标、装备/清除/锁定/缺配置，以及四维当前值与表现数据、各项更新、单行更新隔离、GE 添加移除、ASC clear/ready、更换 ASC、旧回调隔离和销毁/重构。
- **静态检查通过**：本次代码/文档差异空白、新增文件行尾、文档本地文件链接和验证副本 Source 一致性检查。保留此前用户的未提交修改，未提交或推送。
- **未验证**：原项目加载本轮 DLL、WBP_StatEntry 创建、WBP_PrimaryAttribute Reparent/四实例替换、EmptyIcon 与纹理映射的 Editor 编译/保存、真实地图 PIE/图标字体与交互/实际 Pawn 切换、多客户端复制。NullRHI 自动化不证明实际 UI 画面；按完整指南执行最小 PIE。

## 2026-09-29 全身预览改为直接读取 Capture 蓝图配置

- 默认 `Use Blueprint Configuration=true`：复用现有头像子 BP 的 Mesh/Capture/灯光，读取组件的局部变换、FOV、Idle/AnimClass、CaptureSource/ShowFlags 和 TextureTarget。用户新建的全身 RT 是实际输出，不再默认复制；旧 WBP 覆盖/隔离 RT 模式保留为显式选项。检测其它已注册游戏 Capture 共用 RT 时拒绝开启并记录诊断。
- Equipment 的 PreviewMaterial 变为可选覆盖，未指定时读取 CharacterPreview 的 Designer Brush 材质；运行时创建 MID，释放时恢复源材质。按菜单可见性启停、玩家主体外观同步和原 HUD 独立实例保持原职责，灯光恢复遵循子 BP 原可见性。
- 新增 [Capture 蓝图编辑流程](BlueprintCapturePreview.md)，详细列出由用户新建 RT / UI 材质、在子 BP 调构图/Idle、Construction Script ShowOnly、编辑器观察 RT 和 Designer 的操作；同步替换旧指南中的相反配置步骤。本次未创建或保存任何 `.uasset/.umap`，最终视觉由用户编辑。
- **验证通过**：UE 5.8.2 的隔离副本 `Saved/EquipmentBuildValidation-20260928` 完成 UmbraEditor / Win64 / Development 构建，包含 UHT、C++ 和 DLL 链接；最终增量日志为 `Saved/Logs/EquipmentBlueprintCaptureBuild.log`。源码与验证副本哈希一致。保留编译器非首选版本/引擎既有弃用警告。
- **自动化四项通过**：`Umbra.UI.Equipment.PreviewLifecycle`、`PageVisibility`、`SlotBindings`、`SlotContract`；报告 `Saved/Automation/BlueprintCapture-20260929/index.json` 为4项Success、0失败，其中PageVisibility空测试树带一条0/10槽位诊断。新增断言验证子 BP 镜头/人物变换/FOV保留、专用 RT 直接使用、缺失/冲突拒绝和关闭停止更新。引擎启动阶段日志另有测试框架Condition failed信息，发生在本次四项执行之前；未将整份引擎启动日志标成零错误。
- **未验证**：当前 GUI Editor 未重载新 DLL；新 RT / 材质编译、用户子 BP/Widget 实际接线、编辑器实时捕获、真实地图 PIE、透明度/Idle/构图与 HUD 同屏画面仍待用户配置后验收。NullRHI 自动化不验证渲染。需保存并关闭 Editor，在 Rider 构建原项目后重开。

## 2026-09-28 EquipmentSlot 单层 UI 高亮方案更新

- 通过UE 5.8.2只读检查 `M_PP_EnemyOutline` 的Post Process Domain、Before Bloom、SceneDepth邻域函数/CustomStencil掩码/Lerp输入；默认Color为线性(1,0.054443,0,1)，作为独立UMG材质的橙红色参考。未修改或复用该Post Process资产。
- 将Blueprint指南的HoverFrame/SelectedFrame方案替换为单个HighlightFrame，BP_RefreshVisual先关、Hovered/Selected共用视觉开启、其它状态关闭；C++状态仍分开，没有源码变更。新增 [UIInteractionHighlight](UIInteractionHighlight.md)，说明透明中心/细边/柔和Glow的UI材质参数和节点、Hit Test设置、可选0.12秒淡入以及验收。
- 本轮只读材质检查成功（0错误/0警告），核对源码契约与文档链接/差异。新Material和WBP由用户亲自创建/编辑；没有生成或修改二进制资产，未执行新材质编译/视觉PIE，也未为文档修改重跑C++构建。

## 2026-09-28 装备页面 C++ 基础与全身预览

- 用户确认分工：Blueprint/Designer、材质、RT、灯光、镜头和动画资产由用户亲自编辑，Codex只负责逻辑及完整操作说明。新增 [Blueprint逐步指南](EquipmentBlueprintGuide.md)，明确现有C++对Icon/稀有度等数据驱动字段的写入、BP状态视觉事件接线、参数覆盖位置、Switcher层级、临时测试与PIE步骤。本次跟进仅修改文档，未改源码/资产；仅做文档与源码契约、相对链接和差异检查，不重跑构建或宣称新的PIE结果。

- 新增 `EUmbraEquipmentSlot` 十种槽位、显示快照、通用 `UUmbraEquipmentSlotWidget`，支持 Empty/Equipped/Hovered/Selected/Locked 和事件驱动更新；`UUmbraEquipmentMenu` 按 Designer 实例的枚举建表，重复类型不静默覆盖。未引入 Inventory、装备计算或 GAS 效果。
- 用 UE 5.8.2 只读检查原头像 BP、组件模板、RT、材质和相关 WBP。全身页复用 `BP_HerorPortraitCapture` 的独立实例及原 Mesh/Idle/Capture/两盏灯、`M_UI_HeroPortrait.PortraitTexture` 参数；新组件只扩展运行时 RT/MID 隔离、来源 Mesh/Materials 同步、独立镜头和30 Hz按需捕获。HUD 原实例/输出不变。外观刷新接口已预留。
- `UUmbraCharacterMenu` 递归监听 WidgetSwitcher/Visibility；原 Controller 打开/关闭菜单时通知它。切离装备页、关闭或隐藏时停捕获、Mesh Tick 和灯，销毁/换 Pawn 时清理。处理 UE 5.8 FieldNotify 早于 Slate active-index 更新的顺序，避免切页读到旧索引。
- **本轮构建通过**：项目关联的 UE 5.8.2，`UmbraEditor / Win64 / Development`，UHT、C++ 与 DLL 链接均成功，验证副本在 `Saved/EquipmentBuildValidation-20260928`，日志 `Saved/Logs/EquipmentBuild.log`。当前 GUI Editor 保持打开，原项目 DLL 未替换、未加载本轮新类。模块/Target 未修改。
- **本轮自动化通过**：`Umbra.UI.Equipment.PageVisibility`（包含真实缓存 Slate 的 Switcher）、`PreviewLifecycle`、`SlotBindings` 在 `Saved/Automation/Equipment-20260928` 报告为 Success；最后修正测试占位对象后，`SlotContract` 在 `Saved/Automation/EquipmentSlot-20260928` 单独重跑为 Success。PageVisibility 的空装备测试树有一条0/10槽位诊断警告；这不是已配置 WBP 的验收。NullRHI 下验证逻辑/生命周期，不代表渲染通过。源码与验证副本核对一致，Git 差异空白检查通过。
- **尚未验证/接入**：三个 WBP 的 Reparent、十槽 Designer 配置、CharacterPreview Image、装备页加入现有 Switcher、真实材质透明度/Idle/全身构图、地图 PIE、同时显示 HUD 头像、多本地玩家。没有修改任何 `.uasset/.umap`；已保存的 Equipment 只有一个槽位、CharacterMenu 尚未引用它，GUI 未保存内容未读取。完整资产证据、文件清单、参数覆盖顺序和接线/PIE步骤见 [EquipmentMenu](EquipmentMenu.md)。

## 2026-09-27 角色菜单运行时开关

- `AUmbraPlayerController` 新增可配置的 `ToggleCharacterMenuAction` 和 `CharacterMenuClass`，首次按键创建 `WBP_CharacterMenu` 实例并加入视口，后续使用 Visible/Collapsed 复用；打开与关闭态均使用 Game and UI/显示鼠标。点地移动要求可用的鼠标坐标；先前关闭态 Game Only 的永久鼠标捕获导致关闭面板后点地移动失效，已恢复项目原有 Game and UI 基线，菜单开启期间仍由 C++ 阻断 gameplay 输入。菜单宽高/布局由现有 WBP Designer 控制，C++ 不设置。不暂停、不移除默认 IMC，也不创建新 Tick。菜单开启时挡住既有点击移动/普攻、手动移动及 Character 的技能按下入口，并清掉待执行命令与技能输入缓冲；不取消已经激活的技能或改属性。
- 实际 IA/IMC 按键映射、Controller BP 的 Action/Widget 类赋值、WBP 可点击性和 PIE 多次开关均需编辑器配置/验证；步骤见 [EditorSetup](EditorSetup.md#角色菜单按键开关)。本条不把源码实现等同于蓝图资产已配置。
- 本次用本机关联 UE 5.8 构建 `UmbraEditor Win64 Development`：UHT、相关 C++ 编译与 DLL 链接均成功；有引擎 `Character.h` 的既有弃用警告。尚未在编辑器内配置新 IA/IMC/BP，也未运行真实 PIE 或菜单交互测试。
- 后续按用户反馈把初始与关闭菜单时的 `bShowMouseCursor` 改回 `true`；此前成功构建发生在这次光标修正之前。当前 Unreal Editor 正在运行，本轮仅做源码与 diff 检查，尚未对光标修正重新编译或 PIE 验证；请保存资产、关闭编辑器后构建/重启再验收。
- 随后根据 PIE 反馈恢复初始化与关闭菜单时的 Game and UI；UE 5.8 `PlayerController.cpp` 显示 Game Only 默认永久捕获鼠标且消耗首次捕获点击，Game and UI 使用按下时捕获。截图中的 Designer 与 PIE 菜单大小比较还需核对预览 Screen Size、画布缩放、运行时 DPI Scale 及 `SizeBox_Main` 的布局；未直接修改 WBP 资产。此修正仍待保存资产、关闭运行中的 Editor 后重建与 PIE 验收。
- 针对菜单可见但嵌套 WBP 按钮无法点击，Controller 以 ZOrder 100 把菜单加入视口，避免现有 HUD/调试层覆盖；具体 WBP 内部命中是否被全屏 `border`、`background`、`Image_162` 或祖先 Visibility 阻断，仍须按 [EditorSetup](EditorSetup.md#角色菜单按键开关) 在 Editor 用 Widget Reflector 检查。未直接修改 `.uasset`，不声称按钮交互已在 PIE 通过。

## 2026-09-26 属性菜单 C++ Tab 切换

- 新增 `UUmbraMenuTabButton` 和 `UUmbraAttributeMenu`：内部 UButton 点击广播实例索引，菜单验证索引后驱动三页 Switcher 并同步三个按钮的选中状态；按钮视觉由 Blueprint 事件实现，默认页为 0。委托在 `NativeConstruct` 唯一绑定、`NativeDestruct` 解绑，不使用 Tick。
- 已用项目关联的 UE 5.8 构建 `UmbraEditor / Win64 / Development`：UHT、两个新增 `.cpp` 的编译与 DLL 链接成功（引擎 `Character.h` 有既有弃用警告）。两个既有 WBP 的 Reparent、内部 UButton/子控件类型与 Is Variable、三实例 `TabIndex`、按钮选中视觉及真实 PIE 切换仍待编辑器确认和保存。操作与最小验收见 [EditorSetup](EditorSetup.md#属性菜单-tab-接线)。本条记录不代表资产已接好。
- 2026-09-26 编辑器日志显示：菜单最初 Reparent 到正确的 `UUmbraAttributeMenu` 时，三个按钮实例尚未满足 `UUmbraMenuTabButton` 类型，产生同名属性/必需 BindWidget 错误；随后菜单误 Reparent 到 `UUmbraMenuTabButton`，直接报缺少内部 `Button_0`，旧可见性事件亦失效。已将菜单四个 BindWidget 属性标为 `BlueprintReadOnly`，修复旧 Graph 读取 Switcher 的 C++ 暴露错误。修正后 UHT 与 C++ 编译成功，但当前 Editor 占用 DLL，原目录链接报 LNK1104；需保存编辑工作、关闭 Editor 后重建并按正确顺序重新编译两个 WBP。未宣称本轮 DLL 链接或 WBP 编译已通过。
- 按实际按钮 WBP 的截图，`Button_0` 已有 Normal/Hovered/Pressed/Disabled Brush，`icon_object` 由 PreConstruct 的动态材质设置图标与 `IconColor`。最终蓝图接线建议缓存原 `FButtonStyle`，复制后用现有 Pressed Brush 作选中样式的 Normal/Hovered，不新增图像、不覆盖图标材质。已将 `Button_0` C++ BindWidget 标为 `BlueprintReadOnly` 供该 WBP Graph 引用；该反射标记变更仍需关闭 Editor 后完成构建与蓝图验证。
- 当前 C++ Controller 只创建 CombatHUD，没有创建属性菜单；最近的 PIE 日志记录了 CombatHUD 创建而未记录菜单创建。WBP Designer 内点击并不是该菜单的运行时交互验收；临时 Level Blueprint 创建菜单并添加到视口的最小 PIE 步骤见 [EditorSetup](EditorSetup.md#属性菜单-tab-接线)。蓝图内部是否另有正式菜单入口尚待编辑器确认。

维护日期：2026-09-20。入口：[架构与问题证据](Architecture.md)、[编辑器配置](EditorSetup.md)、[开发规则](../AGENTS.md)。以仓库实现为准，不以文件名或规划推断已完成功能。

## 2026-09-21 四主属性基础与调试接入

- `UUmbraAttributeSet` 新增 Strength、Dexterity、Intelligence、Faith 四个非负点数属性，后备值0、无硬上限；包含 GAS 访问器、复制和 RepNotify。玩家/敌人既有一次性初始化顺序不变，DebugInitialAttributes 可整组覆盖四项。
- 属性调试状态和属性变化委托扩展为19项；原生 Add Effect 每层给四主属性各+20，Remove Effect 仍只清理当前控制器记录的句柄。正式八格 HUD及既有伤害执行未接入四主属性。
- 因新字段与历史 `Strength → AttackPower`、`Intelligence → AbilityPower` 同名，移除对应 AttributeSet、DebugInitialAttributes、AttributeDebugViewState 六条 PropertyRedirect；保留 AttackSpeedBonus 重定向。已知玩家/敌人资产未检出旧名，调试 WBP 包仍残留旧名，需在 UE 5.8 刷新节点、明确重接攻击力/法强与四主属性后 Compile/Save。
- 新增/扩展自动化断言覆盖后备值、调试初始化、非负边界、复制声明、调试 GE 叠加/移除、ViewState 与委托更新，并在伤害测试中以高主属性值确认现有伤害结果不变。`Saved/PrimaryAttributesBuildValidation-20260921` 隔离副本已通过 UE 5.8 `UmbraEditor / Win64 / Development` 构建；`Umbra.Attributes.Lifecycle` 与 `Umbra.Damage.Types` 均为 Success。原项目源码也已编译到链接阶段，但运行中的 Editor 占用 DLL，未完成原目录链接。依赖实际 Content 的 DebugOperations/DebugInputAndWidget、WBP 重接、PIE 和多人复制仍待编辑器重启后验证。

## 2026-09-20 正式角色属性面板 C++ 基础

- 新增 `UmbraStatEntry`、属性专用 `UmbraStatTooltip`、`UmbraCharacterStatsPanel`；条目正文只显示图标与数值，名称和说明交给独立 Tooltip WBP。图标固定由各条目 WBP 的 Designer Brush 配置，C++ 仅把格式化 FText 交给蓝图事件，避免编译预览时用空图标覆盖 Brush。八项槽位、初始化刷新、五项 GAS 变化委托、解绑／重复初始化／角色切换处理已实现。攻速按普通攻击配置周期与当前直接倍率计算次/秒。技能和物品 Tooltip 属后续各自的数据契约，不在本次范围。
- 攻击力与法强所要求的物理／魔法侧最终武器伤害当前没有战斗层数据源；技能急速目前没有冷却规则。这三项明确显示“—”，接入点位于 `TryReadStat`，没有借用原始 AttackPower/AbilityPower 或冷却缩减值。
- 移除 C++ 图标覆盖后的源码在 `Saved/StatEntryBuildValidation-20260920` 隔离副本通过 UE 5.8 `UmbraEditor / Win64 / Development` 构建；原项目 Editor 占用 `UnrealEditor-Umbra.dll`，直接构建在链接时被文件锁阻止，运行中的 Editor 尚未重载新模块。`WBP_StatTooltip`、各条目 WBP、`WBP_CharacterStatsPanel` 及现有 `WBP_CombatHUD` 的 Editor 接线、悬停交互、PIE 和联机验证待完成。操作清单见 [CharacterStatsPanel](CharacterStatsPanel.md)。
- 用户当前 PIE 日志在 `WBP_StatsPanel` 创建时连续七次报告 `duplicate stat 0`，且 `WBP_CombatHUD` 仅创建一次；这证实八个条目都仍使用默认 `AttackPower`，只登记最后一项。已将重复值日志改成包含两个条目名和枚举名，并提示在每个条目 WBP 的 Class Defaults 设唯一 `Stat`；该诊断源码在上述隔离副本编译通过。资产内部 TextBlock 事件接线及修改后的实际 PIE 显示仍待 Editor 确认。
- 应用户显示偏好，正式面板攻速仍计算逻辑普攻次数／秒，显示文本改为两位小数、不附加 `/s`；`Saved/StatEntryBuildValidation-20260920` 中的 UE 5.8 `UmbraEditor / Win64 / Development` 增量构建通过。运行中原项目 Editor 尚未加载这次格式改动，PIE 待确认。

## 2026-09-20 属性命名与攻速

- GAS 常驻属性恢复为 `AttackPower`（攻击力）/ `AbilityPower`（法术强度），默认值仍为 10 / 0；伤害执行、调试初值、调试增益及面板快照统一使用这两个名称。AD/AP 系数配置和 SetByCaller Tag 保持原名，数值规则不变。
- `AttackSpeedBonus` 更名为直接倍率 `AttackSpeed`：默认1.0，范围0.2～10.0，普通攻击周期直接除以该值。调试初值、增益、复制和调试面板状态使用新属性；面板按两位小数显示，1.00 为基础攻速。旧攻速加成0对应新值1.0。
- `Config/DefaultEngine.ini` 将上一版本的 Strength/Intelligence 属性引用重定向至恢复的名称，并保留攻速重定向。调试 WBP 显示“攻击力”“法术强度”“攻速”，攻速连接两位小数 `AttackSpeedDisplay`。两份角色蓝图的调试初值保持 `AttackSpeed=1.0`。
- 恢复攻击力/法术强度名称后，UE 5.8.2 `UmbraEditor / Win64 / Development` 构建成功；本次重跑 `Umbra.Attributes` 3 项及 `Umbra.Damage.Types` 1 项，全部通过。玩家、敌人与调试 WBP 三项蓝图重新加载编译，0 错误、0 警告；保存后重新读取面板文本、连线和两份初值均正确。未运行真实地图 PIE 或多人验证；`Umbra.Combat.Maintenance` 为上次修改阶段的通过记录。

## 2026-09-19 玩家普通攻击逻辑命中

- 增加一次性服务端命令 `umbra.Attack.MeasureSeconds n`：下一击起手开启 n 秒窗口，在 `IncomingDamage → Health` 结算点按普通攻击来源记录 GE 结算伤害（按结算前剩余 Health 封顶），输出起手数、结算命中数、总伤害和 DPS。其他技能、客户端预测与致死溢出伤害不计；真实 PIE 数值仍待测。
- 计时结束后，服务端向攻击者 PlayerController 发送可靠客户端消息，在其视口显示统计 10 秒；无有效 Controller 时保留日志。中断窗口也显示黄色提示。该视口路径尚未做 PIE 验证。
- 自动攻击目标高亮现由本地 Controller 持有，和鼠标悬停共同决定既有 `SetAttackHighlighted` 状态；取消、换目标、移动、失效和 Controller 清理时更新。本次源码已在 `Saved/AttackMeasureBuildValidation` 隔离副本通过 UE 5.8 `UmbraEditor / Win64 / Development` 编译，原项目运行中的编辑器未加载新模块；按用户要求未运行 PIE 或自动化测试。
- 用户现场日志先后记录 5 秒窗口 `starts=3/5`、`damagingHits=0`、`healthDamage=0`；敌人显示飘字。即时 GE 调用返回点及目标结算回调均读到 `healthLost=0`，因此此前以 Health 差值统计的版本仍输出 0。普通攻击 Spec 的来源标记已在回调中识别；当前改由同一服务端回调记录 GE 结算伤害。此前版本已通过原项目 UE 5.8 `UmbraEditor / Win64 / Development` 编译，当前修改的编译状态见下文。初始节奏偏慢与 `BaseAttackInterval=0` 使用普通 A 完整时长的配置一致，资产实际时长与 1x 周期待编辑器核对。
- 后续用户服务端日志证实每击物理 GE 的 `Incoming=14`，目标 Health 为 `2,100,000,000`，同一结算回调的 `HealthBefore/After` 相同；此量级的 float 不能表示 14 点差异。因此统计改为直接记录本次 GE 在服务端结算的正伤害，按结算前剩余 Health 封顶，并明确将日志字段改为 `settledDamage`。这使统计口径与浮点 Health 差值分开；高 Health 目标的血量本身仍需降低到合理范围，才能逐击观察 14 点扣血。本次修改在同步全部当前源码后的 `Saved/AttackMeasureBuildValidation` 隔离副本通过 UE 5.8 `UmbraEditor / Win64 / Development` 编译；运行中的原项目编辑器未加载新模块。用户要求不再运行测试，因此没有新 PIE 或自动化验证。
- 当前攻速倍率上限为 10.0（直接属性 `AttackSpeed`）；Montage 播放率上限仍独立控制视觉表现。10 倍档的实际频率、DPS 和动画对齐尚待 PIE 测量。下文 Bonus 数值属于旧阶段记录。
- 本次最新源码已在 `Saved/AttackMeasureBuildValidation` 隔离副本通过 UE 5.8 `Umbra` 与 `UmbraEditor / Win64 / Development` 编译；新增视口提示的 `UmbraEditor` 增量编译也通过；后者使用 `-NoHotReloadFromIDE`，未替换当前运行编辑器的模块。原项目编辑器处于 Live Coding 状态，常规 `UmbraEditor` 构建被 UBT 拒绝。隔离副本的未烘焙 Game 启动在加载资源时触发 `BufferReader` 断言，未进入自动化测试；用户随后要求不再测试，因此未运行 PIE 或其他测试。

- 当前代码以本击开始时间快照周期与前摇，服务端前摇 Timer 对原目标做存活、可攻击、距离、Visibility 遮挡检查，再复用现有物理伤害 GE 与受击事件。普通攻击不再订阅 Hit Window，其他能力的扫掠 Notify 类保留。跨 Ability 间隔由 ASC 保留，前摇移动取消不新增间隔，出手后取消保留已提交间隔。
- Montage/Chain Point 不门控下一击；视觉出手时间优先读取 GA 覆盖值，否则读取首个旧 Hit Window Begin。动画请求率考虑资产 Rate Scale，触及 MaxAttackMontagePlayRate 时输出姿势赶不上逻辑出手的警告。`umbra.Attack.Log 1` 可记录服务端实例、时间、倍率、结果和动画速率。
- 此节为当前实现；下方关于武器扫掠、Hit Window 决定玩家伤害和 Montage 限制玩家攻速的记录是历史验证，不代表当前行为。真实 Montage 标记、蓝图默认值、PIE 频率/DPS/双人网络与动画观感仍待编辑器确认。详细操作见 EditorSetup。
- 前一版 UE 5.8 `UmbraEditor / Win64 / Development` 编译成功；NullRHI 命令行运行 `Umbra.Combat.Maintenance` 为 Success，覆盖理论周期快照、旧 Hit Window 不扣血、服务端单击一次结算、重复/旧回调无效、前摇移动取消。本次伤害统计与 10 倍上限修改后的原项目编辑器目标和自动化测试仍待验证。该测试未推进真实攻击周期的逐帧计时，也未做双人 PIE；1.0、2.99、3.0、4.0、5.0、10.0 的频率与 DPS 目前仅有公式预测，没有实测值。运行中仍出现既有 Greystone_AnimBlueprint 的 Divide by zero 警告。

## 2026-09-18 移速驱动起步/急停表现

- `AUmbraPlayerCharacter.GetLocomotionAnimationPlayRate` 已提供由 GAS 同步后的 `MaxWalkSpeed / LocomotionAnimationReferenceSpeed` 推导的安全倍率，默认参考速度500cm/s、范围0.25～3.0；未新增属性，也不在角色 Tick 轮询 GAS。
- 已确认实际 `/Game/Blueprints/Player/Greystone_AnimBlueprint` 使用 `Locomotion / JogStart / JogStop`，对应 `Jog_Fwd_Start`、`Jog_Fwd_Stop`。C++ 接口与参数已经就绪，二进制 AnimBP 的 Event Graph 缓存、两个 Sequence Player Play Rate 及转场规则仍需按 EditorSetup 手工接线并保存。
- UE 5.8 `UmbraEditor / Win64 / DebugGame` 增量构建成功；`Umbra.Attributes.Lifecycle` 与 `Umbra.Combat.Maintenance` 为 Success，覆盖默认MoveSpeed 500、250/500/750cm/s对应0.5/1.0/1.5倍，以及0.25～3.0安全边界。自动化不渲染状态机，不能替代起步/急停观感、脚步滑动、Distance Matching 或多人 PIE 验收。

## 2026-09-19 移速驱动玩家转向速度

- ASC 的既有 MoveSpeed 委托在同步玩家 MaxWalkSpeed 时同时调用角色辅助函数更新 CharacterMovement.RotationRate.Yaw，无 Tick 轮询；动画与Yaw共享参考点，默认500cm/s→动画1.0x及Yaw 600°/s，随MoveSpeed线性增加并将Yaw限制到默认1800°/s。AttributeSet和调试后备MoveSpeed也统一为500；敌人仍由自己的默认300播种。
- 攻击朝向所有权规则未改变：攻击期间只更新数值、不重新打开 `bOrientRotationToMovement`；攻击结束后移动旋转使用最新Yaw。
- `Umbra.Combat.Maintenance` 为 Success，覆盖初始Yaw 600°/s、MoveSpeed GameplayEffect实时更新Yaw、250/500/750cm/s对应300/600/900°/s，以及1800°/s上限。完整 `Umbra` NullRHI批次共8项，其中7项成功；`Umbra.UI.WorldPresentation` 因命令行环境没有Viewport而失败，与本次移动逻辑无关，仍需在PIE单独运行。自动化不替代不同帧率、网络延迟下的实际转身观感验收。

## 2026-09-17 持续攻击、高攻速动画与伤害数字

- 一次攻击目标指令会持续攻击：Controller 负责保存目标、复用 `PrimaryAttackRange` 追近、只缓存最新移动/换目标指令；普通指令在当前击的安全衔接点执行，不会额外等待手动连击宽限。目标失效会停止；`State.Dead`、新增的 `State.Stunned`、外部取消和 Avatar 更换立即清理。
- 新增原生 `Umbra Attack Chain Point` AnimNotify。代码只接受位于最后一个 `Umbra Attack Hit Window` 之后的衔接点；未配置或配置过早时回退 Montage 正常结束。每击开始重置命中集合，同击仍只伤害一次。
- 高攻速模式默认阈值3.0最终倍率（`AttackSpeedBonus=2.0`），每击独立决定；达到阈值按 `HighSpeedAttackMontages` 循环，配置A、B即A-B-A-B，空项跳过、全空回退普通第一段，降回阈值以下从普通第一段开始并重置高速索引。`BaseAttackInterval=0` 时由普通第一段有效时长自动校准，修复基础攻速被硬编码1秒意外加速；实际 Montage PlayRate 默认最多3.0，防止更高倍率把混合、抬手和命中窗压没。要在安全上限内保持4～5倍理论周期，A、B都应配置更短的专用高攻速 Montage。
- 飘字新增蓝图可读写字号档、暴击字体倍率1.15、最终字号上限28，以及可编辑缩写单位。默认 k/M/B/T、1位小数；`999960 → 1.0M`。选档使用缩写前实际伤害，每个 Widget 只随机一次字号，既有扇形移动/停留/淡出不变。
- 玩家角色新增蓝图纯函数 `GetLocomotionAnimationPlayRate` 和可编辑参考速度/倍率上下限，供 AnimBP 的起步与急停 Sequence Player 使用；C++ 只提供由 GAS 驱动 MaxWalkSpeed 推导的表现倍率，不直接修改二进制动画资产。
- 最新调整已完成 UE 5.8 `UmbraEditor / Win64 / DebugGame` 构建，`Umbra.Combat.Maintenance` 为 Success；新增覆盖300%阈值后的A-B-A高速循环、降速恢复普通第一段并重置高速索引，同时保留自动基础周期、Montage播放率上限和每击攻速快照验证。Development目标未在本轮重新构建；真实A/B资产的通知帧、混合观感和多人预测仍需PIE验收。战斗测试仍有现有Greystone AnimBlueprint的Divide by zero警告，以及无NavMesh临时World中预期的寻路警告。
- 尚未验证实际动画衔接观感、真实 NavMesh 上移动打断/追击、不同 Montage 的混合效果、Listen Server/双客户端/专服预测一致性，以及 WBP 最终字体视觉；构建和 NullRHI 自动化成功不等同于这些 PIE 验收通过。

## 2026-09-17 属性驱动角色行为

- MoveSpeed 继续由 ASC 属性委托同步至当前 Avatar 的 MaxWalkSpeed：ActorInfo 首次绑定立即同步，GE 聚合变化实时同步，清除/更换 Avatar 时成对解绑；AttributeSet 保证不小于0，全程没有 Tick 轮询。
- 复用现有 AttackSpeedBonus，不新增属性：0为基础速度，最终倍率为 `1 + Bonus`；Bonus限制-0.8～4.0，对应最终倍率0.2～5.0。玩家普通攻击每击开始快照；实际 Montage Rate 由动画有效时长和目标攻击周期推导，Notify 命中窗口随实际 Rate 缩放。攻击中变化只影响下一击。
- 现有调试初值、Gameplay Effect、属性面板和二进制资产继续使用 AttackSpeedBonus，无迁移步骤。
- 服务端仍是伤害判定和 GameplayEffect 的唯一有效端；LocalPredicted 客户端与服务端均从复制属性读取同一倍率播放 Montage。
- 已验证 UE 5.8 `UmbraEditor / Win64 / Development` 构建成功；`Umbra.Attributes.Lifecycle`、`Umbra.Attributes.DebugOperations`、`Umbra.Attributes.DebugInputAndWidget` 与 `Umbra.Combat.Maintenance` 均 Success。战斗测试覆盖 Bonus=1 时实际 Montage PlayRate=2、0.35秒宽限缩放为0.175秒，以及攻击中把 Bonus 改为-0.5仍保留当前快照。未执行真实 PIE 动画观感、通知帧逐项检查或多人网络验收；战斗测试仍报告现有 Greystone AnimBlueprint 的 Divide by zero 警告。

## 2026-09-17 属性调试 Add Effect 叠加

- Add Effect 改为覆盖面板展示的15项属性：普通数值每层+20；AttackSpeedBonus、CriticalChance、CriticalDamageMultiplier 每层+0.2（20个百分点）。AttackSpeedBonus/CriticalChance 分别保持4.0/1.0封顶，IncomingDamage 不属于常驻展示属性，不加入增益。
- 每次点击都会新增一层，不设人为层数上限；Remove Effect 一次移除本控制器在当前目标上的全部层，并继续保留其他控制器/来源的同类效果。
- 蓝图按钮契约未改变，仍调用 `Request Operation` 的 Add Effect / Remove Effect；本轮不修改 WBP 资产。
- 已验证 UE 5.8.2 `UmbraEditor / Win64 / Development` 构建成功；`Umbra.Attributes.DebugInputAndWidget` 与 `Umbra.Attributes.DebugOperations` 均 Success（2/2、0 warning、0 error）。未执行真实 PIE 按钮、视觉与多人网络验收。

## 2026-09-17 属性调试百分比显示单位

- `FUmbraAttributeDebugViewState` 的 AttackSpeedBonus、CriticalChance、CriticalDamageMultiplier 为百分比显示值：分别把 GAS 的 `0.2/1/2` 传为 `20/100/200`。WBP 只需格式化数字并追加 `%`，不再乘100。
- GAS 内部数值与战斗计算不变；CriticalChance 仍在底层限制为0..1，Add Effect 的比例增量仍为0.2。
- 已验证 UE 5.8.2 `UmbraEditor / Win64 / DebugGame` 构建成功；DebugGame 下 `Umbra.Attributes.DebugInputAndWidget` 与 `Umbra.Attributes.DebugOperations` 均 Success（2/2、0 warning、0 error），包含 `0.25→25`、`0.4→40`、`1.75→175` 的状态断言。Development 重编因用户当前编辑器开启 Live Coding 而被 UBT 安全阻止；需在编辑器按 Ctrl+Alt+F11 或保存后关闭编辑器再编译。未执行真实 PIE 视觉与多人网络验收。

## 2026-09-17 属性调试面板 C++/蓝图边界

- 移除属性调试 Widget 的七个 `BindWidget`、C++ 文本格式化、按钮绑定和 Controller 中硬编码的锚点/位置/360×640尺寸。
- C++ 观察 GAS 并把15项属性及目标/就绪状态放入 `FUmbraAttributeDebugViewState`，通过 `Apply Attribute Debug State` 交给 WBP；三个比例/倍率字段由 C++ 转成百分比显示单位，其余单位、精度、等待/反馈文字、按钮状态、窗口尺寸和样式归蓝图。
- WBP 按钮通过唯一的 `Request Operation` 蓝图入口请求固定调试操作，服务器验证与 GE 所有权规则未改变。
- 已验证 UE 5.8.2 UmbraEditor / Win64 / Development 编译成功；`Umbra.Attributes.DebugInputAndWidget` 与 `Umbra.Attributes.DebugOperations` 均 Success（2/2、0 warning、0 error）。蓝图资产按用户分工未修改，事件实现、按钮连线和实际 PIE 视觉/输入验收待蓝图侧完成。

## 2026-09-16 血条尺寸与场景飘字修复

- 用户已确认血条只有外框的原因：旧组件120×12固定尺寸与控件上下各12 Padding冲突，Retainer Box运行高度为0。证据、原因和回归步骤已记录在 [EnemyHealthBar](EnemyHealthBar.md#2026-09-16-已确认-bug有外框但没有红色填充)。
- 血条：移除C++数值宽高，构造/注册时启用Draw at Desired Size，注册时覆盖旧资产固定尺寸模式。宽高唯一配置入口为实际Widget Class对应WBP的根SizeBox；Designer用Desired预览，运行遵循同一布局及视口DPI。
- 飘字：保存命中时世界起点，把生成时扇形位移转换为固定世界终点；逐帧用当前相机/DPI投影。角色、敌人和相机移动不再带走终点；离屏/镜头后透明但继续计时清理。实现与验收见 [DamageNumbers](DamageNumbers.md)。
- 已验证：关联UE 5.8.2的UmbraEditor / Win64 / Development最终构建成功，包含新增 `Umbra.UI.WorldPresentation` 测试；本次修改的源码/文档差异空白检查通过。未更改二进制资产，保留原工作区其他修改。
- 自动化尝试：NullRHI没有有效投影视口；默认D3D12离屏运行在测试开始前发生Renderer后台线程崩溃。DX11离屏测试执行后，除分屏位移误差断言外，其余断言通过（相机移动/旋转/距离、世界位置、离屏恢复/超时、旧尺寸模式和两组WBP根尺寸）。该误差断言后依据UE反投影整像素截断规则调整为一个像素的容差并重新编译，但最终测试尚未重跑，不能标为整项通过。日志位于 `Saved/Logs/WorldPresentation*Tests.log`。
- 用户选择自行进行现场验证；本次未完成PIE视觉、实际普攻漂字跟随镜头、帧率/DPI矩阵及多人网络验收。测试可在PIE中从Session Frontend运行，或用带实际视口的 `-game` 启动后运行；不使用NullRHI。测试只修改临时实例，结束后恢复视角。

## 已实现（静态代码确认）

- top-down 玩家移动/朝向、指针寻路与追击、可攻击目标高亮、Enhanced Input 与 GAS 标签输入。
- 玩家 PlayerState / 敌人 Character 各自持有 ASC 和属性；19 个常驻属性、一次性初始化、初始调试覆盖、边界裁剪与复制声明。
- 玩家普攻连招、socket 球扫掠；敌人简单 Tick AI 与近战距离命中；敌人受击、死亡状态和尸体处理。
- 服务器物理/魔法结算、AD/AP 混合系数、暴击、双抗、IncomingDamage 最终扣血及可开关日志。
- 敌人血条、攻击者飘字、F1/F2 属性调试面板与固定调试 GE；主要 UI 监听清理。
- 维护修复：伤害表现路由已提取为 `FUmbraDamageNotification`；MoveSpeed 已由 GAS 监听并同步到当前 Avatar 的 `MaxWalkSpeed`；玩家/敌人能力清理统一进入幂等 `EndAbility`；普通攻击 Pawn 射线忽略自身 Pawn；普攻命中检测补充剑根到剑尖的球扫掠。
- 当前玩家攻击配置实际读取到 `FX_Sword_Top`、45cm 半径、三段 Montage；玩家骨骼存在 `FX_Sword_Bottom`，已设为剑身扫掠默认根 socket。
- 不等同于当前蓝图资产已正确配置或当前地图已经跑通。

## 已验证：本次整理

- 读取现有 AGENTS.md、相关 C++、Config、已有 Docs 与目标资产文件列表，核对主要调用链及配置契约。
- 整理前工作区已有 `Content/UI/Combat/WBP_DamageNumber.uasset`、`Content/UI/Enemy/WBP_EnemyHealthBar.uasset` 修改，以及未跟踪 `Content/fantasy_gui_4/`；本次仅编辑 Markdown，保留上述内容。
- 69 处本地文件链接存在性检查通过（不包含标题锚点渲染验证）；已核对表中代码入口。`git diff --check -- AGENTS.md Docs` 通过；三份新增文档另查无行尾空白，Source/Config 无差异。
- 全仓库 diff 尝试因用户已改二进制触发 Git LFS clean filter，无法写入只读 `.git/lfs/tmp` 而失败；随后限定本次 Markdown 范围完成检查。未变更 LFS 配置或资产内容，不能把这次文档检查称为全仓库检查通过。
- 本次已运行 UE 5.8.2 `UmbraEditor / Win64 / Development` 构建；维护代码与 `UmbraCombatMaintenanceTests.cpp` 编译成功。`Umbra.Combat.Maintenance` 专项自动化成功，复现并确认了自身 Pawn 遮挡修复、实际蓝图读取、GAS 移速、攻击取消/连招清理及动画命中窗口配置。完整 `Umbra.` 测试运行中的 Lifecycle、Damage.Types、DebugInputAndWidget、DebugOperations、Maintenance 均成功；日志另有 Greystone 动画蓝图 Divide by zero 警告。
- 剑根默认值 `FX_Sword_Bottom` 及本轮持续攻击/飘字源码均已进入成功的 Development 构建；地图 PIE、网络测试和视觉验收仍未运行。

## 历史验证记录（不是本次重跑）

| 证据文档 | 文档记载结果 | 不能推断的范围 |
| --- | --- | --- |
| [AttributeSetPhase1](AttributeSetPhase1.md) 的2026-09-14记录 | UE5.8.2 UmbraEditor Win64 Development 构建成功；Umbra.Attributes.Lifecycle Success | 当前源码重跑、地图普攻、蓝图旧引用、多客户端、持续效果自然到期 |
| [AttributeDebugF1F2](AttributeDebugF1F2.md) | 构建及 DebugInputAndWidget、DebugOperations、Lifecycle Success；历史编辑器只读核对 | 当前 WBP 改动、真实鼠标射线/硬件按键、网络交互 |
| [DamageNumbers](DamageNumbers.md)、[EnemyHealthBar](EnemyHealthBar.md) | 对应阶段记录完整编译成功 | 当前 UI 视觉、实际引用、DPI/帧率、多人表现 |

这些结果是保留的历史文档记录，本次没有重新核验当时日志。MagicalDamage.md 的算例是验收步骤，不是已通过结果。

## 待验证与执行方法

先保存用户资产；若修改反射类型/模块，关闭编辑器并刷新 Rider 工程，再用关联引擎构建 **UmbraEditor / Win64 / Development**。本机引擎目录需实查；命令模板在 AGENTS.md。

Session Frontend → Automation 可运行下列现有测试：

| 准确测试名 | 源文件 | 覆盖 / 限制 |
| --- | --- | --- |
| `Umbra.Attributes.Lifecycle` | `Source/Umbra/Tests/UmbraAttributeSetTests.cpp` | ASC/GE 生命周期、初始化与属性边界；独立 World，不替代地图与网络 |
| `Umbra.Damage.Types` | `Source/Umbra/Tests/UmbraDamageTests.cpp` | 物理/魔法、混合系数、必暴、类型拒绝、IncomingDamage；直接构造 Spec，不覆盖真实普攻 GE 配置、命中和飘字 |
| `Umbra.Attributes.DebugOperations` | `Source/Umbra/Tests/UmbraAttributeDebugTests.cpp` | 权威端调试修改；依赖实际 BP_UmbraPlayerController 资产 |
| `Umbra.Attributes.DebugInputAndWidget` | `Source/Umbra/Tests/UmbraAttributeDebugInputTests.cpp` | 生效的 F1/F2 配置、实际 IMC/WBP 加载、原始属性状态刷新与部分监听清理；不验证待实现的蓝图格式/布局，也不模拟真实鼠标/按键或渲染 |
| `Umbra.Combat.Maintenance` | `Source/Umbra/Tests/UmbraCombatMaintenanceTests.cpp` | 实际玩家/敌人/GA 资产、MoveSpeed、连续攻击、同击去重、攻速阈值与快照、最新指令、目标越界/死亡、强制中断；临时 World 不含真实 NavMesh，不替代动画观感和网络 PIE |
| `Umbra.UI.DamageNumberLogic` | `Source/Umbra/Tests/UmbraDamageNumberLogicTests.cpp` | 字号档边界/非法配置、暴击倍率/上限、k/M/B/T 与舍入进位；纯逻辑，不渲染 WBP |
| `Umbra.UI.WorldPresentation` | `Source/Umbra/Tests/UmbraWorldPresentationTests.cpp` | 需已运行PIE/有渲染视口的独立游戏；验证飘字世界定位、相机移动/旋转/距离、分屏、离屏计时和血条WBP期望尺寸；不替代普攻视觉与真实多人 |

最小人工验收按修改领域选择：

1. **编辑器配置**：打开 `GA_BasicAttack` Class Defaults，核对普通Montage、命中sockets/radius、高攻速开关/阈值3.0/`HighSpeedAttackMontages`的A与B、BaseAttackInterval。逐Montage在最后一个命中窗之后添加 `Umbra Attack Chain Point`；留空时另测正常结束回退。Compile/Save后再PIE。
2. **属性**：出生满池；同 ASC 重新绑定不回血；上限增益不补血、移除裁剪；临时效果不残留。具体值见 AttributeSetPhase1。
3. **持续攻击/指令**：单击敌人观察至少三击且每击一次伤害；第二击中点击两个不同地面点，安全点后只去最后一点且不多打一击；换目标同理。把目标移出/移入范围验证追近和恢复，杀死目标验证立即停止；施加 `State.Stunned` 验证立即中断并清指令。
4. **攻速节奏**：在一击中把 AttackSpeed 从2.99改为3.0，当前击 Montage/速率不变，下一击切高速 Montage；再降回2.99，下一击从普通第一段恢复。记录相邻起手间隔，核对 `BaseAttackInterval / AttackSpeed`，并检查命中窗不漏伤害。
5. **伤害/UI**：按 MagicalDamage 设置 AD20/AP40、系数2/0.5、护甲100/魔抗300、暴击0，预期物理30/魔法15；开 `umbra.Damage.Log 1`。按 [DamageNumbers](DamageNumbers.md) 验证字号四档、暴击倍率/上限、999960进位，以及既有扇形/停留/淡出、血条与调试 UI。
6. **生命周期与网络**：Montage 中、手动宽限期、衔接点前后分别取消；Pawn/Controller 切换、目标销毁、反复进出 PIE。另跑 Listen Server、两个客户端和条件允许时专服，确认客户端 Montage 一致且每击只有服务端 GE/伤害。

记录结果时写日期、引擎版本、目标/测试名、地图与模式、结果和未测边界；生成日志放 Saved，不提交。

## 已知维护问题

按优先级：属性结算直接依赖飘字路由；GAS MoveSpeed 与实际速度双来源；能力自身清理依赖特定 Montage/Finish 回调。次要项为普通 IMC 添加/释放不对称与少量无名战斗常量。文件行号、影响和最小建议集中在 [Architecture 的维护问题表](Architecture.md#有依据的维护问题)，避免维护两套清单。

以上包含静态可见的设计负担与尚待复现的生命周期风险，不能全部称为已复现 Bug。主要 UI 订阅已有配对清理，未发现 UI 重算伤害公式。

## 下一步（尚未实施）

1. 先执行编辑器引用核对与最小战斗/UI 基线验收，将实际结果补回本文件。
2. 把前三项维护问题各自做成独立小修改；保留行为、补针对性验证，和新功能分开审查。
3. 原型功能候选仍是玩家死亡/重新挑战与近战可读性；当前 C++ 未实现玩家死亡/重生闭环。闪避、完整技能/装备/掉落/存档不属于本次交付。

## PIE 渲染默认配置（2026-10-18）

- 仅改 DefaultEngine.ini 的 RendererSettings 段：r.AntiAliasingMethod=4、r.SecondaryScreenPercentage.GameViewport=100；配置契约和优先级见 [EditorSetup](EditorSetup.md)。保留主屏幕比例、画质档位、动态分辨率、窗口/桌面设置与所有描边逻辑。
- 已核对本机 UE 5.8.2 CVar 注册、百分比使用、项目INI加载与优先级；未发现相关项目配置冲突。INI段落/唯一键及差异检查通过。未运行PIE、构建或自动化测试，未改Content；重启后的实际值、角色清晰度、描边稳定性及GPU开销待用户手动确认。

## mace 鼠标命中修复与渲染核验（2026-10-20）

- WorldItem.Mesh 允许仅 Visibility 射线查询；Controller 物品选择优先复杂查询，覆盖根球外模型部位，保留简单查询兜底。未改变拾取距离/RPC/Inventory、VisualComponent 或材质资产。详见 [LootAndPickup](LootAndPickup.md)。
- UE 5.8.2 UmbraEditor Win64 DebugGame 编译、DLL/lib 链接成功，日志 Saved/Logs/WorldItemCursorDebugBuild.log；保留非首选MSVC与引擎弃用警告。Development 因当前编辑器 Live Coding 被阻止，日志 WorldItemCursorBuild.log；当前打开的 Development 编辑器尚未加载本次修复，需要关闭后完整构建。差异空白检查通过，未运行自动化或PIE；mace实际复杂碰撞、锤头/柄与遮挡/点击一致性待手工验收。
- 现有运行日志确认 AA=4、SecondaryScreenPercentage.GameViewport=100，LastSetBy=ProjectSetting；两项已生效。主渲染比例、PIE编辑器覆盖及当前画质问题尚未定位，不盲目增加配置覆盖。用户停止了此前电脑操作，本轮未继续操作编辑器或保存Content。

## 2026-10-27 玩家生命归零死亡

- 已实现玩家 GAS Health 归零死亡状态、能力和移动取消、复制表现、动画配置入口、死亡音去重、AI 放弃尸体与攻击窗口保护；见 [PlayerDeath](PlayerDeath.md)。
- 已添加 `Umbra.Combat.PlayerDeath` 回归测试和 `Scripts/configure_player_death.py` 资产配置脚本。
- 本轮编译首次发现局部变量遮蔽错误，已修正；再次完整构建被正在运行的编辑器 Live Coding 阻止（`Saved/Logs/DeathBuild.log`）。尚无本轮成功构建、自动化或 PIE 结果。配置脚本尚未执行，动画/音效资产引用待编辑器确认；未完成有声试听和多人验证。

- 静态验证：`git diff --check -- Source Docs Scripts` 通过（仅换行提示），配置脚本 Python AST 语法通过，新增文档入口链接检查通过。全仓 diff 检查遇到沙箱 Git LFS 进程权限错误，因此不声称二进制资产检查通过。

## 2026-11-04 六档掉落 Niagara 接入

- 已将用户导入的 `/Game/LootDropVFX/Systems/Rarity` 六档预设通过 UE 5.8 编辑器保存到 `/Game/Items/Loot/BP_WorldItem` 的 LootVFXStyles。普通/魔法/稀有/史诗使用实例品质色；传说覆盖原 Mythic 红色、独特覆盖原 Legendary 金色。保持系统原顺序和形态，保留每层 HDR 亮度、Alpha、尺寸及速度，未修改供应商特效资产。
- 新增原生 LootVFX 组件及实例刷新接入；绝对旋转/缩放保持竖直，拾取、无效实例和 EndPlay 清理，专用服务器不激活。配置与来源详见 [LootVFX](LootVFX.md)。
- **本轮已验证**：UmbraEditor Win64 Development 构建成功（含最终分层亮度逻辑）；Rider 工程文件刷新成功；`Umbra.Loot.VFXAssetAudit`、`Umbra.Loot.VFXLifecycle`、`Umbra.Loot.DropPhysics` 三项通过。生命周期/物理报告为 `Saved/Automation/LootVFX/index.json`（2 成功、0 失败），原包参数见 `Saved/LootVFX/parameters.txt`。
- **真实渲染 PIE 已通过**：`Scripts/smoke_loot_vfx_pie.py` 使用六次敌人死亡走正式掉落路径，逐档核对 Niagara 系统、持续激活、世界旋转竖直、拾取成功且立即停用。报告 `Saved/LootVFX/pie.json`；人工查看 `Saved/Screenshots/WindowsEditor/HighresScreenshot00000.png`，确认白/绿/蓝/紫/红/金六种效果。测试临时排列掉落用于截图，没有保存关卡。
- 已查看 Output Log：新增逻辑没有测试失败或 Niagara/Python 执行错误；启动阶段仍有引擎 UnifiedError 自检输出、既有第三方蓝图输入轴警告及既有 Python 字段重名警告，不能将整个日志称为零警告。
- **未验证**：多人 Listen Server/客户端迟加入、打包、批量掉落 GPU 性能、专用服务器实机。单机 PIE 和自动化通过不代表这些场景已通过。
- Git LFS 属性确认 `.uasset` 使用 lfs，并执行 LFS 状态检查。没有提交、推送或改动现有无关资产。

## 2026-11-04 开局六品质随机测试器

- 已扩展 AUmbraLootTestSpawner，并通过 UE 5.8 编辑器创建 `/Game/Items/Tests/BP_SixRarityStartup`，保存到 `L_Prototype` 出生点附近（250,-375,92），标签 `Six Rarity Startup - Random Test Loot`。模板来自正式 V2 掉落表，共580个；SourceLevel=10、间距150cm，沿局部右方向排列。
- 每次关卡 BeginPlay 使用新随机种子，只在权威且非客户端执行一次；通过正式 Generate 抽样，集齐普通/魔法/稀有/史诗/传说/独特各一件。最多4096次，失败不输出部分批次；合法性、源稀有度及词条规则不变。已有固定 Items 测试模式保持默认行为。Shipping/Test 构建禁用。
- 关闭入口：地图 Actor 的 Loot Test → Enabled。等级、位置和间距均可配置；完整来源/覆盖顺序见 [SixRarityStartup](SixRarityStartup.md)。
- **已验证**：UmbraEditor Win64 Development 构建；`Umbra.Loot.SixRarityStartup` 通过（24个种子及重复种子、空池拒绝，报告 `Saved/Automation/StartupLoot/index.json`）。实际连续两次渲染 PIE 均自动生成六档各一件，模板和种子不同，12件均有激活光效且可成功拾取，报告 `Saved/SixRarityStartup/pie.json`。查看 Output Log，无本功能运行错误；引擎启动自检和既有警告不作为零警告承诺。
- **未验证**：多人客户端/迟加入实机、Shipping/Test 打包、修改模板池后的全品质可达性。生成点无地面时按既有逻辑跳过，需要移动 Actor 至有效静态地面。
- 新蓝图和地图的 Git LFS 属性已确认。未提交或推送；没有修改正式掉落概率、物品定义或 Profile。
# 2026-11-06：物品错误名称与失色石光效核查

- 已经匹配 UE 5.8.2 编辑器保存13份名称修正；独立进程检查1835份 ItemDefinition，无含 error 的显示名称，来源记录保留。导入入口增加显示名清理；catalog测试6项、清理检查5项通过。
- 失色石白光报告正在核查：六档系统与运行时颜色参数检查符合现有品质映射，未修改品质概率或 VFX规则，不能宣称现场问题已修复。回归入口、证据和边界见 [专项记录](ItemNameAndSomberVFX.md)。

## 2026-11-06：石头按等级固定品质，开局展示19种

- 用户随后明确材料品质应随石头等级固定，并确认1–2普通、3–4魔法、5–6稀有、7–8史诗、9传奇、古龙岩独特。此前随机品质诊断已被这项规则替代。生成器的材料分支读取Definition.Rarity；19份定义与Material Profile版本8已由UE保存，配方同步且保留原始来源。装备/护符随机生成不变。详见 [配置、覆盖顺序与恢复入口](StoneGradesAndStartup.md)。
- L_Prototype原六件装备及单件随机石生成器关闭，两排新生成器每次生成普通9种、失色10种各一件；原护符生成器保留。没有新增强化/消耗系统。
- **已验证**：UE 5.8.2 UmbraEditor Win64 Development构建成功；Generation自动化9/9、SmithingStacks 1/1通过。Generation有两条警告（测试World销毁缺少context、外网连通性探测超时），0失败；堆叠测试0警告。报告为 `Saved/Automation/StoneGrades`、`Saved/Automation/StoneGradesStacks`。
- 独立UE进程回读19份定义/配方通过（`Saved/StoneGrades/readback.json`）；等级映射与脚本语法检查通过。最终连续两次渲染PIE各生成准确19种，固定品质、Niagara系统/六个颜色参数、拾取后品质保留/特效清理、旧生成器停用均通过（`Saved/StoneRarityStartup/pie.json`）。已查看 `Saved/Screenshots/WindowsEditor/StoneStartup.png` 的两排等级光效。首轮PIE脚本在播放中加载Blueprint导致类查询失败，改为开始前预加载后完整重跑通过。
- 新BP/地图LFS属性已确认，diff空白检查通过。未验证多人、Cook/打包、声音或用户旧运行实例；未提交/推送。无模块/Target改动，本轮未重新生成Rider工程文件。
## 2026-11-06：移动时掉落标签抖动修正

- 标签排布移至UpdateCameraManager的Super调用之后，使用当帧相机视图；显示历史随锚点同步平移，只对避让偏移做缓动，扫掠检查也使用平移后的历史。悬停保留相对避让位置，随相机移动，不再锁住旧屏幕坐标。规则/配置边界见 [标签移动修正](WorldItemLabels.md)。没有改动Content资产或拾取/物理规则。
- **已验证**：UE 5.8.2 UmbraEditor Win64 Development构建成功；`Umbra.Loot.HoverAndLabelLayout`通过，0错误、3条测试控制器未配置输入Action的警告（报告`Saved/Automation/LootLabelMotion`）。覆盖密集静止/移动、悬停跟随、视口边缘与尺寸变化、避让插值、命中和销毁清理。
- 实际渲染PIE通过：临时六石密集组，玩家带动相机往返360帧；所有标签可见，相机最大移动83.484cm，标签避让偏移最大逐帧变化0 Slate单位。报告`Saved/LootLabelMotion/pie.json`，日志`Saved/Logs/LootLabelMotionPIE.log`；测试不保存地图。
- 未做人工连续动画观感验收、真实DPI切换、多人/分屏、Cook打包；没有将数值采样当成人工视觉验收。未提交或推送。
## 2026-11-06：Loot标签初始锚点与归位

- 标签先预留每件物品正上方的自然位置，再处理真正重叠的标签，避免前面的避让候选抢占后面物品的锚点。非悬停标签在自身位置有足够净空后收回临时偏移，解决初始聚集/落地散开后的旧偏移残留。
- NativeConstruct清除旧平移并隐藏，控制器完成测量与排布后显示；Slate视口Host几何未就绪时不显示、不缓存角落锚点。世界锚点仍为模型包围盒顶部中心加LabelOffset，没有改物品位置、资产或拾取规则。参数边界见 [标签初始锚点](WorldItemLabels.md)。
- **已验证**：UE 5.8.2 UmbraEditor Win64 Development最终构建成功；`Umbra.Loot.HoverAndLabelLayout`通过（0错误，3条测试控制器缺输入Action警告，报告`Saved/Automation/LootLabelSpawn`），含初始锚点预留、邻居不可抢占、空间释放后归位与既有移动/命中回归。
- 首轮首次可见帧检查捕获约99.79 Slate单位偏移；补充视口Host就绪检查，并排除未构建、尺寸为0的控件统计后，最终两次渲染PIE各观察180个有效可见帧至落稳，最大额外平移0，锚点距石头Actor最低41cm（`Saved/LootLabelSpawn/pie.json`）。测试只修改临时关卡实例，不保存地图。
- 最终构建下另行重跑密集移动PIE：6个标签360帧全部可见，避让偏移最大逐帧变化0，相机移动82.265cm（`Saved/LootLabelMotion/pie.json`）；上一轮移动稳定性未回退。
- 未进行人工连续视觉验收、多人/分屏、真实DPI切换或Cook打包。未提交或推送。

## 2026-11-06：Greystone 四种主动技能

- 新增 1 剑刃风暴、2 跃击震地、3 坚盾防御、4 石化复苏，保留 Q/E。四个 DA、四个 AM、四个输入 Action 和四个法环本地图标通过 UE 保存并接入原技能栏第3–6格；未重建 HUD 布局或改动 Paragon 原包资产。配置、时点、调参入口、来源与机制边界见 [Greystone 技能](GreystoneSkills.md)。
- C++ 执行器扩展风暴多脉冲、CharacterMovement 根运动跃击和真实落地伤害、临时护甲/抗性 GE、按最大生命回血 GE。所有伤害仍走 UmbraDamage/GAS；结束或中断清理定时器、根运动任务、防御增益与技能附着粒子。复苏是主动蓄力回血，不包含死亡复活或无敌。
- 视觉检查发现并修正两项素材适配问题：新建 AM 的 DefaultSlot 改为当前角色使用的 UpperBody；源 Ultimate 中段约4099.7cm的根位移用独立 `A_StoneRecovery_InPlace` 副本 Force Root Lock 消除，原动画保留。补上骨骼粒子声明的 Actor 参数，并让跃击保留动画收招。
- **已验证**：UE 5.8.2 的 UmbraEditor Win64 Development 最终构建成功，Rider 工程元数据刷新成功。`Umbra.Skills` 3/3 通过：SweepSector、CooldownView、GreystoneExpansion；包含四个定义校验、非法参数拒绝、环形/高度范围、独立冷却、正确 Slot/锁根、防御实际减伤及移除恢复、治疗上限。证据 `Saved/SkillPrototype/expansion-build-final.log`、`expansion-rider-project.log`、`expansion-automation-final.log`。
- 最终渲染 PIE **11/11 场景通过**，通过真实 Enhanced Input Action 绑定触发（不是 OS 实键）：Q/E完整释放、新四技完整释放、新四技起手/跃击中移动打断、跃击撞墙。Fixture武器通道10：横扫15、广扫10、风暴6×4.5=27、跃击20、复苏12.5；防御无伤害且属性恢复；复苏生命60→90。未受墙阻挡的跃击因目标胶囊在末端相撞实际前进约354.85cm、腾空约160cm；墙体测试前进约175.42cm，未穿墙，墙后目标伤害0。四项打断均无后续伤害/回血，费用只扣一次，六槽图标材质、键位、冷却数字/进度与四个空槽显示正确。复苏模型根骨相对角色保持在画面内，最终样本高度-90cm。报告 `Saved/SkillPrototype/expansion-pie.json`，日志 `expansion-pie-final.log`，最终截图 `Saved/Screenshots/WindowsEditor/ScreenShot00036–00042.png`。
- 已检查日志与截图；保留现有 Greystone AnimBP 初始 Divide by zero、样板旧输入、GameplayCue搜索路径、NavMesh 等警告，不承诺零警告。原包资产 Git 状态未变，新增 uasset 匹配 Git LFS。未提交/推送。
- **未验证/未覆盖**：OS 实键、联机/迟加入、死亡/眩晕逐项 PIE、切图压力、Cook/Shipping、真实DPI切换、音效试听。新四技尚未专项配置独立技能声音；本轮重点为动画/VFX、玩法与 UI。连续动作手感、粒子大小/偏移和数值平衡仍可在新 DA/AM 上人工微调。

## 2026-11-06：技能免冷却、时钟遮罩与风暴/下砸动作修正

- `DefaultEngine.ini` 设置 `umbra.Skills.IgnoreCooldowns=1`，能力跳过冷却检查和施加，技能栏同步不显示CD；保留DA原冷却、资源费用与动作锁。控制台设为0即可临时恢复，永久恢复需修改该配置。规则/配置见 [Greystone技能](GreystoneSkills.md)。
- 技能格新增 `Img_Cooldown` 与 `M_UI_SkillCooldownClock`：12点起始顺时针消退扇形遮罩，金色指针边缘；读取原GAS剩余比例，保留倒计时数字。旧进度条保留为兼容后备。未改变用户HUD框架、尺寸、间距和图标。
- 剑刃风暴使用Greystone Ability_E三段连续循环，3.8秒蒙太奇以1.2速播放约3.17秒，末段保持动作到收招。修正嵌套AnimSegments修改后缓存蒙太奇长度未刷新的问题。
- 4石化复苏改用独立 `A_StoneRecovery_Slam`，仅将源Ultimate根骨骼竖直位移等比限高到220cm，保留腾空/翻转/下降；约3.33秒触地才回血和伤害。胶囊不升空、不新增无敌。原包、旧锁根副本保留。
- **本轮已验证**：UE5.8.2 UmbraEditor Win64 Development构建成功（`Saved/SkillPrototype/refinement-build.log`）；`Umbra.Skills` 三项SweepSector/CooldownView/GreystoneExpansion全部Success，进程退出0（`refinement-automation.log`）。编辑器资产保存/回读通过（`refinement-assets.json`），新动画及材质LFS属性确认。
- **渲染PIE通过**：`Scripts/smoke_skill_refinement.py` 经Enhanced Input连续两次风暴，费用和六次伤害正确、全程CD为0、2.5秒后蒙太奇和手部姿势仍运动；复苏根骨骼先升后降、提前不结算、落地回血30与伤害12.5；临时恢复CD后检查6秒CD的三个指针进度、材质参数跟随GAS、最终隐藏，再恢复免CD。最终报告 `Saved/SkillPrototype/refinement-pie.json` 为passed，日志 `refinement-pie-final.log`，进程退出0。已人工查看渲染截图的风暴末段、复苏下降/落地和半圈遮罩。
- 首轮PIE在全部行为检查后，报告清理访问已销毁World失败；修复测试清理后完整重跑通过。Output Log启动阶段仍有既有引擎UnifiedError自检Condition failed及已有资产警告，不将整份日志描述为零错误。
- **本轮未验证**：实体键盘、多人网络、音频试听、Cook/打包；此前11场景全量PIE为历史验证。本轮将该旧回归脚本改为显式启用并恢复CD，做语法检查，未重新执行11场景。没有下载、购买或导入新声音；补库来源、许可入口与逐技能声音层见 [音效采购与分层](Audio/AudioAssetRequirements.md)。无模块/Target变更，未重新生成Rider文件；未提交或推送。
## 2026-11-06：品质环境音渐入与连续播放

- 三段现用品质环境音 `Magic_env_epic / legendary / unique` 经 UE 保存为 SeekRestart，离开范围时继续推进循环时间轴，再靠近恢复对应进度；没有替换音频内容。首次/中断重试默认渐入 0.5 秒，显式重试按固定 Pitch 与 Wave 时长计算循环偏移。
- 虚拟化切换延后一帧处理声音映射，再在音频线程设置低增益及渐入；解决编辑器单线程音频回调提前触发、指令落到旧声音实例的问题。拾取/销毁同时清理延后处理与重试定时器。配置覆盖顺序、引擎实验特性和非 SoundWave 边界见 [音频配置](Audio/AudioSetupGuide.md)。
- **已验证**：UE 5.8.2 UmbraEditor Win64 Development 构建成功，Rider 项目刷新成功；三个资产独立进程重新加载均为 SeekRestart，仍使用 Git LFS。
- 最终构建下 `Umbra.Audio` 4 项及 `Umbra.Loot.DropPhysics` 1 项共 5/5 通过，0 错误、0 测试警告，报告 `Saved/LootAmbienceContinuity/FinalAutomation/index.json`；覆盖上下文选择、循环偏移/绕回/Pitch、生成音生命周期、静音限流、落稳及拾取清理。
- 启用真实音频的 L_Prototype PIE 通过：一颗固定史诗石，靠近→远离虚拟化→靠近恢复→显式 Stop 后重试→拾取→退出。实际播放位置误差均小于 0.04 秒。音频线程实际增益的独立断言通过：首次 48 个样本、返回 32 个样本、重试 33 个样本均出现中间增益并到达 1；返回增益依次约 0.12、0.24、0.36、0.50、0.63、0.74、0.89、1。证据 `Saved/LootAmbienceContinuity/{pie,fader,readback}.json`、`Saved/LootAmbienceContinuityPassed.log`，脚本不保存关卡。
- **未验证**：人工听感、多人/分屏、Cook/Shipping、多物品并发压力、动态 Pitch/时间膨胀。当前真实音频用史诗音源验证共用路径，传奇与独特本轮做资产回查；不将数值采样当作人工试听。未提交或推送。
## 2026-11-06：暂时关闭开局锻造石

- 按用户要求关闭 `BP_StartupAllSmithingStones`、`BP_StartupAllSomberStones` 和旧 `BP_StartupSmithingStone` 的 Enabled（Blueprint 默认及 L_Prototype 实例）；通过 UE 5.8.2 保存，保留物品列表、位置和品质规则。恢复方式见 [石头开局配置](StoneGradesAndStartup.md)。
- 已验证：独立编辑器重载默认值及地图，最小 PIE 运行5秒，三个生成器均停用且没有生成掉落物；其他生成器保留1件掉落。报告 `Saved/StartupStonesDisabled/pie.json`，日志 `Saved/StartupStonesDisabledPIE.log`。
- 本轮仅改资产开关和文档，未改 C++、未重新编译或运行完整自动化；未验证联机或打包。首次保存受已打开编辑器的文件锁阻止，编辑器关闭后重试保存成功。未提交/推送。

## 2026-11-12：Greystone 六技能图标与原生动画盘点

- 通过 UE 5.8.2 保存六项战士图标（12/48/11/08/13/26），移除辉石魔砾的技能授予、输入绑定和 IMC 映射，清空非现役槽位预览。保留十格布局及辉石素材，未改技能机制、数值、地图或属性图标。当前映射、历史脚本覆盖边界及配置入口见 [Greystone 方向与素材](GreystoneDirection.md)。
- 匹配编辑器回读170个原生动画目录资产：154条序列、6个蒙太奇、10个混合/瞄准资产。查看26条关键序列共78个采样姿态，整理普攻、重斩、旋转、跃击、格挡、复苏、施法、挑衅等候选；估计合理目标为一套3–4段普攻加6–8个主动技能。属于后续设计建议，不表示本轮实现新技能。
- **本轮已验证**：现有六技能渲染PIE回归11/11通过；独立新进程验证图标材质、六项授予、后四格为空、辉石绑定和映射清除、第七槽不能触发，并查看HUD截图。证据 `Saved/GreystoneDirection/{smoke,preview,audit}.json` 及对应日志、`hud.png`、`Poses/`。图标资产来源/属性图标外部推荐见专项文档，推荐尚未导入。
- **未验证**：没有C++/模块改动，未重新构建、刷新工程或执行Session Frontend自动化；未做实体键盘、联机、音频试听、打包或全部154序列连续手感验收。日志有既有引擎启动自检、动画除零和资产警告，不称零错误。未提交或推送，保留已有无关修改。
## 2026-11-12：暂时关闭全部开局物品

- L_Prototype 全部5个 `UmbraLootTestSpawner` 地图实例的 Enabled 设为 false；本轮实际新增关闭的是 `BP_StartupAmulet_C_0`。保留生成器、物品列表、位置及 Blueprint 默认配置，恢复时在地图对应实例开启 Enabled。执行 `Scripts/disable_startup_loot.py` 可重新应用关闭配置；不影响击杀掉落。
- 已通过 UE 5.8.2 保存地图，独立进程重新加载并运行最小 PIE：游戏时间5秒后5个生成器均停用、WorldItem 数量为0。报告 `Saved/StartupLootDisabled/pie.json`，日志 `Saved/StartupLootDisabledPIE.log`，未发现 Python 错误或 Fatal error。
- 本轮仅修改地图开关及辅助脚本/文档，未改 C++、未重新编译、未运行完整自动化或打包验证。未提交或推送。

## 2026-11-12：原生六技能与数字键

- 按已确认方案改为1破阵斩、2剑刃风暴、3跃击震地、4坚盾防御、5石化复苏、6战意。新破阵斩使用Ability_Q，战意使用Cast；现役六技能均不再引用Attack_*普攻序列。旧横扫、广域横扫和辉石保留资产但不授予；数字键与输入标签、Designer预览同步，六张图标来自现有warrior无色目录。完整素材归属/备用登记、调参来源、覆盖规则见 [原生六技能](GreystoneNativeKit.md)。
- 防御改成正常速度举盾/收盾和中间延长姿态，总长2.5秒，未改供应商序列。战意0.55秒成功后通过GAS增加20个百分点暴击率6秒，受100%上限约束，重复施放刷新不叠加；成功前打断无增益，成功后移动保留，到期/死亡清理。没有实现元素附魔或修改已有战斗公式。
- **本轮已验证**：UE5.8.2 UmbraEditor Win64 Development最终构建成功（native-build-final.log）；Umbra.Skills六项自动化成功（CooldownView、GreystoneExpansion、NativePresentation、PebbleDefinition、PebbleOriginalAudio、SweepSector；Saved/GreystoneDirection/Automation）。保留旧资产的自动化并不代表它们仍在技能栏。
- 新渲染PIE **15/15通过**：六技完整释放、六技起手打断、跃击撞墙、战意重复刷新和死亡清理；核对One–Six映射、现役定义顺序、不含Attack_*动画、无色图标实际材质、后四格为空、扣费、六脉冲、护甲还原、复苏治疗以及战意增益与到期。证据Saved/GreystoneDirection/native-pie.json、native-pie.log、native-hud.png与native-skill-1至6截图；已查看HUD、防御和战意截图。测试不保存地图。
- 测试迭代：首轮启动观察窗口过窄且旧测试接口没有暴击率，补充接口/断言并重新构建；第二轮跃击测试靶沿用180cm近战位置，按既有跃击回归改到450cm落地区域后完整重跑通过。没有将失败轮次计为通过。
- **未验证**：实体键盘、联机/迟加入、Cook/打包、有声试听、所有备用动画连续手感；没有完成独立战意音效/VFX或持续增益图标。现有特效/声音配置沿用，当前免CD调试开关保留。日志有既有启动自检、AnimBP及资产警告，不称零警告。未改模块/Target、未刷新Rider工程；Git LFS状态/属性、diff空白与文档链接检查完成；未提交或推送，保留已有无关改动。

## 2026-11-12：暂不接入技能资源消耗

- 通过UE5.8.2将现役六个技能Definition.ResourceCost与Designer预览设为0，主配置脚本同步覆盖为0；不改角色资源属性、动画、伤害或冷却开关。UI费用显示0。定向脚本Scripts/disable_greystone_resource_costs.py，配置与原版视频对照见[原生六技能](GreystoneNativeKit.md)。
- 已验证：独立渲染PIE零资源施放检查6/6通过，六技均可激活、UI affordable为真、资源始终0；Saved/GreystoneDirection/no-cost-pie.json。测试每技激活后主动取消，不宣称本轮重验完整技能生命周期。未改C++，未重新编译或跑自动化；实体按键、音频、多人、打包未验证。
- 首轮复用完整流程受风暴时序检查失败影响，且空AssertionError被旧报告函数错误记录为passed；已修正异常报告为repr(error)，最终报告要求六项均明确passed。最终采用本次变更所需的零资源激活专项检查，前轮完整15场景结果为历史验证。
- 官方Greystone Overview和Epic创意总监发布页已核对来源，未声明逐帧观看/试听。未提交或推送。
## 2026-11-12：目标信息卡与整窗技能页

- 新增左上角敌人目标信息卡：名称、展示等级、头像、当前/最大生命；观察现有GAS血量，优先鼠标高亮敌人，其次自动攻击目标。当前近战敌人配置为“近战守卫”、展示等级1，头像暂用现有Fantasy通用人物图标；展示等级不参与战斗计算。
- 角色窗口顶部新增头盔/技能图标切页，沿用Fantasy按钮样式。按用户选择切换整个三栏窗口；技能页为空，共享原深蓝透明底板。进入技能页暂停装备预览，返回恢复，关闭清理。资产通过UE5.8.2编辑器保存。
- 已验证：UmbraEditor Win64 Development构建成功（Saved/UIExpansion/build-final.log）；UI自动化Equipment.EmptySlotIcons、CharacterStats.PrimaryRows、Equipment.LiveSnapshot共3/3通过（Saved/UIExpansion/Automation）。渲染PIE验证鼠标目标、生命10000→5000及数值/比例更新、无目标隐藏、整窗切页和预览启停；报告Saved/UIExpansion/pie.json。已查看enemy-target.png、character-page.png、skills-page.png最终截图。
- 验证边界：切页测试调用按钮对应函数，未做实体鼠标按钮点击；未验证多人、打包、不同分辨率/DPI矩阵，以及独立的死亡/目标替换/自动攻击回退场景。早期测试脚本的镜头跟随、Python属性访问和截图时序问题修正后重跑通过。未提交或推送，保留原有改动。
- 八项属性图标仅推荐下载，未替换；素材出处、配置入口、复验脚本及后续UI规划见[UI扩展](UIExpansion.md)。

## 2026-11-12 Greystone 音频素材接入

- 完成实际编辑器音频审计（525个声音资产，Greystone 140 Cue/337 Wave），六技能起手/生效语音、短剑风及默认库实际 SkillHit 声音配置；仍为数字1–6、资源消耗0。
- ASC 添加受技能管理的附着循环音和语音清理，取消/死亡/角色切换/注销停止；未找到合适循环、盾金属、落地重击、岩石碎裂等资源，正式配置留空并列于 [GreystoneAudio](GreystoneAudio.md)。
- 本轮编译通过；Umbra.Skills 6/6；带音频 PIE 15/15（六完整、六取消、循环正常/取消/死亡）；新进程读回确认正式配置无测试循环音。素材仅引用，未修改供应商音频。
- 人工试听、多人/打包未验证；语音适配为初选。验证脚本 `Scripts/smoke_greystone_audio.py`，完整证据与失败后重测说明见上文档。
- 独立战意语音死亡用例1/1通过（`voice-death-pie.json`），本轮音频 PIE 合计16/16。

## 2026-11-12：Nameplate 2与外置分页按钮

- 敌人卡改为复用Fantasy GUI 4的W_Nameplate_2控件树及头像/血条边框，800单位原始布局通过ScaleBox等比缩放到HUD卡片区域；名称采用支持中文的默认字体，隐藏示例Buff和重复等级。保留GAS生命监听，供应商模板未修改。
- 角色/技能按钮紧贴现有画框上方，取消页面内顶部56单位占位，不改变三栏画框。八项图标未下载/替换；已核对Game-icons官方Studio透明导出设置并记录到UIExpansion.md。
- 本轮已验证：UE5.8.2 UmbraEditor Win64 Development构建成功（Saved/UIExpansion/nameplate-build-layout.log）；通过引擎保存后在全新编辑器会话运行渲染PIE，目标卡、生命更新、无目标隐藏、整窗切页、预览暂停/恢复及关闭清理通过（nameplate-pie-clean.log、pie.json阶段end/result passed）。已查看三张最终截图；最终目标截图为技能额外造成4点伤害后的4996/10000，专项断言验证过5000时的比例。
- 迭代记录：修正Border API；DLL/资产临时占用解除后重试；首轮截图暴露模板被挤压和按钮悬空，改为等比缩放及随框定位。同会话重编译后切页测试失败，原测试把空AssertionError误记passed，已改repr及明确断言信息；该轮不计通过，最终全新会话重跑成功。
- 本轮未重跑上一轮3项UI自动化，未验证实体点击、多分辨率/DPI、多人或打包；未提交/推送。复验入口Scripts/refine_ui_nameplate.py及Scripts/smoke_ui_expansion.py。

## 2026-11-12：八项透明属性图标接入

- 使用用户已导入的/Game/Asset/Icons/8statsicons，将StatsPanel八个WBP的staticon分别映射到broadsword、magic-swirl、breastplate、magic-shield、quick-slash、stopwatch、target-arrows、wingfoot。只改Brush.ResourceObject，保留所有其他笔刷字段和Image ColorAndOpacity，不改纹理、属性数值或布局；素材来源/署名沿用UIExpansion.md。
- 已验证：通过UE5.8.2编译并保存八个WBP；对比前后8/8完整笔刷仅资源引用不同；重新加载资产检查笔刷一致，渲染PIE验证8/8实际图标引用、Brush Tint和控件颜色保持，已查看stats-icons.png透明背景实机效果。报告Saved/UIExpansion/stats-configured.json、stats-pie.json（end/passed），日志stats-configure.log、stats-pie.log；Git LFS属性已检查。
- 未改C++，未重新构建UmbraEditor或重跑其他自动化；未验证多人、打包和多分辨率。未提交或推送。配置/复验脚本为Scripts/configure_eight_stat_icons.py、Scripts/smoke_eight_stat_icons.py。

## 2026-11-12：直接引用原版W_Nameplate_2

- 按用户指定路径，将WBP_EnemyTargetPanel中的复制布局替换为NameplateTemplate子控件，其实际生成类为/Game/Asset/fantasy_gui_4/widgets/templates/nameplates/W_Nameplate_2.W_Nameplate_2_C。原供应商资产未改动；原血条材质、头像材质和装饰保留，GAS比例通过原SetProgressValue(Value)事件驱动。仅适配中文名字字体、真实生命数值叠字，并隐藏演示Buff。
- 已验证：UmbraEditor Win64 Development构建成功（Saved/UIExpansion/nameplate-direct-build-final.log），UE保存迁移成功（nameplate-direct-configure.log）；全新渲染PIE断言实际模板类、原Retainer材质Value从1变为0.5、生命文字5000/10000、无目标隐藏、两页切换及预览恢复/关闭清理通过（nameplate-direct-pie-final.log、pie.json阶段end/result passed）。已查看最终enemy-target.png原模板血条画面。供应商模板无Git改动，目标卡LFS属性正确。
- 首轮鼠标移离后重新进入目标导致无目标断言失败，改为保持光标移离并确认控制器无目标后验证隐藏，最终重跑通过。本轮未重跑其他自动化，未验证实体点击、多分辨率、多人或打包；未提交或推送。八项图标配置未改。
## 2026-11-12：关闭左侧属性调试面板

- 通过 UE 5.8.2 将 `BP_UmbraPlayerController.Enable Attribute Debug Panel` 从 true 改为 false 并保存。保留面板类和输入配置；现有 C++ 开关同时跳过面板创建与专用 F1/F2 输入绑定。恢复入口见 [属性调试面板](AttributeDebugPanel.md)。
- 已验证独立进程重载、最小 PIE 游戏时间5秒后实际控制器开关为 false，属性调试面板 Widget 数量为0。报告 `Saved/AttributeDebugDisabled/pie.json`，日志 `Saved/AttributeDebugDisabledFinalPIE.log`。首次验证脚本接口名称错误已修正后重跑通过。
- 本轮仅改 Blueprint 默认开关及脚本/文档；未改 C++、未重新编译、未执行人工视觉验收、联机或打包验证。未提交/推送。

## 2026-11-12：敌人卡思源字体与红色血条

- WBP_EnemyTargetPanel新增样式配置：名称汉字使用SourceHanSerifSC-Regular_Font（思源宋体），等级/生命数字使用SourceHanSansSC-Regular_Font（思源黑体），血条指定原供应商Mat_GUI_ProgressBar_Red。保留模板字号、布局和血条事件；原Retainer与SetProgressValue共用更新后的红色MID，继续支持生命比例及边缘发光。配置脚本Scripts/configure_nameplate_style.py，未修改供应商模板、字体或材质原资产。
- 已验证：UmbraEditor Win64 Development构建成功（Saved/UIExpansion/nameplate-style-build.log）；UE保存配置成功；渲染PIE明确断言名称、等级和生命字体路径、红色材质父资源、100%→50%血量、5000/10000文本、无目标隐藏与切页预览生命周期通过（nameplate-style-pie-final.log、pie.json阶段end/passed）。已查看最终enemy-target.png红色血条截图。
- 首轮验证脚本使用不存在的Python get_font接口，改为读取font属性后完整重跑通过。未重跑其他自动化，未验证多分辨率、多人或打包；本轮范围为当前敌人信息卡，未批量覆盖其他现有WBP字体。未提交/推送。

## 2026-11-12：复制原姓名板结构与Designer血量文本

- 按用户要求，WBP_EnemyTargetPanel改为复制W_Nameplate_2原WidgetTree，保留MainSizeBox、原布局及W_EnemyIcon_2/W_ProgressBar_Parent嵌套实例。SizeBox_2中增加HealthValueOverlay，原血条为下层，保存的HealthText为上层。外层NameplateViewport只负责等比缩放，供应商原资产未修改。
- 名称TextBlock_0、等级TextBlock_5、HealthText字体/字号/颜色/布局由Designer控制；当前配置思源宋体/思源黑体和红色血条。移除运行时创建文本、覆盖字体和替换材质逻辑；原生仅更新内容/头像参数及血量比例。旧ChineseFont/NumericFont/HealthBarMaterial默认项已移除，直接编辑控件及原血条实例ProgressBarMaterial。迁移检测HealthValueOverlay后不再重建，保留后续手改。
- 已验证：UmbraEditor Win64 Development构建成功（Saved/UIExpansion/nameplate-editable-build.log）；匹配UE保存迁移成功（nameplate-editable-configure.log）；全新渲染PIE确认HealthText存在于保存的Designer控件树、复制布局存在、字体/红材质正确，血量100%→50%与5000/10000文本、无目标隐藏、切页预览生命周期通过（nameplate-editable-pie.log、pie.json end/passed）。已查看最终enemy-target.png。
- 未重跑其他自动化，未验证实体Designer拖动操作、多分辨率、多人或打包。未提交/推送。最新手动入口见UIExpansion.md，样式重置脚本同步为修改Designer属性。

## 2026-11-13 1技能斩切波

- 按用户指定替换1技能：Attack_C_Slow（慢速第三段攻击）+新AM_SlashWave，0.38秒释放前向弧形波，0消耗；2–6技能未修改。DA路径/SkillId为兼容保留BreachSlash，显示名已改。
- 在UE中创建并保存弧形材质、蒙太奇、技能引用；第三方资产未修改。资产备份在Saved/SlashWave/Backup；调整入口见[SlashWave](SlashWave.md)。
- `Saved/SlashWaveBuild.log`：UmbraEditor Win64 Development成功；`SlashWaveAutomation.log`：Umbra.Skills 6/6成功。
- `Saved/SlashWave/pie.json`：带渲染PIE六例全部通过，正面/侧缘/偏移目标各只受15伤害，范围外/墙后/出手前取消无伤害；方向保持直线、Resource=0、投射物到期清理。独立进程加载保存配置后测试。
- 已查看`Saved/SlashWave/wave.png`确认弧形朝前可见。新动画源编辑器检查0.8秒且Notify为空；出手时间为当前初调值，未完成逐帧手感验收。启动日志仍有引擎Condition failed等既有输出；未声称零错误。
- 未验证多人客户端、打包和人工听感。现为命中第一个目标即消散，不是穿透多目标波。碰撞为弧形的宽盒近似。

## 2026-11-13 技能2：长按引导旋风

已实现按住2无限引导、松开停止，可同时WASD/地面移动；伤害围绕当前位置结算，眩晕/死亡/清空输入结束，无资源消耗。复用Ability_E持剑与Jog_Fwd腿部素材制作循环旋转，玩家动画类使用项目副本的最终DefaultSlot。当前资产与手工打磨入口见 [SwordStormChannel](SwordStormChannel.md)，该记录覆盖旧的固定时长/6脉冲说明。

本轮：UE5.8.2 UmbraEditor Development构建成功，Umbra.Skills 6项通过，带渲染PIE 9场景通过（长按8秒原地/移动各15脉冲、停走转向、短按、释放移动、眩晕、取消、其他技能取消、死亡）。查看移动截图。未验证多人网络、打包及人工混音；PIE有音频缓冲欠载日志。数据见Saved/SwordStorm/channel-pie.json，自动化日志Saved/SwordStormAutomation.log。

补充：点地移动引导PIE通过，位移636cm，停止后无持续伤害/特效；Saved/SwordStorm/channel-click-pie.json。本轮合计10场景通过。

## 2026-11-13 第8空槽：跃击选点原型

已完成8键进入、鼠标选落点、左键确认、右键/Esc取消；参数化贴花显示600cm可达圈和220cm落地判定圈，无效落点红色标识。真实落地经GAS结算，零资源消耗，现有六技能未接入。配置与人工调整见[技能目标选择](SkillTargeting.md)。

本轮验证：UE5.8.2 UmbraEditor Development构建及Rider项目刷新成功；Umbra.Skills 6/6通过；最终带渲染PIE 33项断言通过（输入消费、鼠标投影、取消无移动、距离夹紧、实际跳跃/落地、范围内外伤害、路径阻挡、死亡清理、零消耗）。新进程资产回读通过，已查看最终指示器截图。证据：Saved/TargetingBuild.log、TargetingAutomation.log、TargetingReadback.log、SkillTargeting/pie.json、SkillTargeting/targeting-preview.png。启动日志仍有Condition failed；未验证物理键鼠手感、多人、打包、复杂地形与移动平台。扇形/条形及已有技能接入不在本轮实现。

## 2026-11-13 技能2动画纠正

按用户要求恢复原Ability_E、UpperBody槽、Greystone_AnimBlueprint，不再使用合成全身旋转/迈步。保留长按引导与移动施法，人物朝向继续由原CharacterMovement随移动方向决定。详见 [SwordStormChannel](SwordStormChannel.md)。本记录替代之前全身旋转版本描述。

本轮构建成功，Umbra.Skills 6项通过，9个PIE场景通过，包含移动朝向偏差<15度断言。未重跑点地、多人与打包；历史点地验证不作为此版新结果。

## 2026-11-13 斩切波前摇打磨

- 新增投射物分段前摇接口WindupEndTime/WindupDuration与Blueprint GetReleaseDelay；原动画前0.25秒用0.65秒播放，后续PlayRate=0.65，源发射帧StrikeTime=0.38，计算实际发波0.85秒、动作约1.50秒。DA时序字段覆盖源码默认0前摇；其余技能保持默认。操作说明见[SlashWave](SlashWave.md)最后一节。
- 仅保存1技能时序，原DA备份在Saved/SlashWave/DA_BreachSlash-before-windup-*.uasset。前摇取消会清理变速与发射定时器。
- 本轮UmbraEditor Win64 Development成功（SlashWaveWindupBuild.log）；Umbra.Skills回归成功（SlashWaveWindupAutomation.log）。
- 渲染PIE 7/7成功：正面/侧缘/范围外/墙体/早取消/0.55秒前摇取消/直线。实测首次投射物出现0.86～0.87秒；约0.3秒时源动画仅播放到0.11秒，确认准备动作确实减速；零消耗与碰撞行为通过。报告Saved/SlashWave/pie.json，日志SlashWaveWindupPIE.log；已查看windup.png。
- 未验证多人预测与复制、打包、人工最终手感及音频混音。启动时仍有既有引擎测试诊断，不能声称日志零错误。

## 2026-11-13 技能文字 Tooltip

- 新增`/Game/UI/Skills/WBP_SkillTooltip`，复用物品Tooltip边框与文字样式，仅保留技能名称和机制描述，使用思源宋体。Designer可编辑SkillName/DescriptionText及布局，原物品资产未修改。
- 技能Definition增加Description（多行FText），从PlayerState当前技能槽读取；已填充配置时的六个Greystone技能和一个跃击瞄准测试技能。文本仅描述机制，不在UI复制伤害计算。配置脚本只填写空描述。
- 技能槽通过原生悬停事件展示；优先置于技能框上方居中、间距8，上方不足改在下方，按视口边距8夹紧；空/未授予槽不挂说明，换槽清理旧说明，销毁解除引用。调整技能条命中链以允许子槽悬停，不更改布局或属性图标颜色。修改入口与复验脚本见[UIExpansion](UIExpansion.md)。
- 本轮验证：UmbraEditor Win64 Development构建成功（Saved/UIExpansion/skill-tooltip-build.log）；匹配UE5.8.2编辑器编译并保存WBP及数据资产成功（skill-tooltip-configure-linked.log）。渲染PIE的数据断言已通过：10槽、7个有效技能的名称/描述与绑定TextBlock一致、空槽无Tooltip、临时换成无效槽后清理并可恢复。
- 未完成验证：实际鼠标悬停显示、移出关闭及屏幕边缘视觉检查。后台测试会话中控件绘制几何为0，鼠标命中断言失败；修正后台节流测试设置后的最后一次编辑器启动提前退出，未产生新报告。因此不能将skill-tooltip-pie.json标为整体通过。需在前台PIE运行Scripts/smoke_skill_tooltip.py或手动逐槽悬停复验。未做打包、多人及多DPI测试。
- 源码/文档范围git diff --check通过；新WBP的LFS filter已确认。全仓检查受现有LFS filter进程启动失败影响，未宣称全仓通过；保留全部既有未提交修改。

### 技能Tooltip上方定位补充

- 按用户要求改用技能框锚点：上方居中，间距8；上方不足回退下方，按视口边距8夹紧。使用视口Slate坐标且不二次除DPI；原生MouseEnter/Leave控制显示，已有刷新更新位置，销毁移除视口实例。
- 新版UmbraEditor Win64 Development编译通过：Saved/UIExpansion/skill-tooltip-above-build.log。编译中一并修复编辑器粒子检查工具的TObjectPtr取指针（RequiredModule.Get()），不改粒子行为。
- 新版PIE数据检查通过：10个槽、当前9个有效技能的描述及TextBlock一致，空槽和槽复用清理检查通过。配置变化来自同期新增瞄准测试技能，未覆盖其数据。
- 实际悬停及上方布局仍未通过自动验证：skill-tooltip-above-pie3.log / skill-tooltip-pie.json显示，临时关闭后台节流后仍得到全部槽绘制几何为0，测试在进入鼠标定位前失败。设置已恢复。不得将本次结果记为视觉/悬停通过；需要前台PIE验收。尚未验证超长描述、小分辨率、多DPI、多人和打包。

## 2026-11-13 技能2持续粒子、节拍和敌人命中火星

修复原Cascade发射器Loops=1造成约4～5.8秒后停止；项目持续副本改循环并关闭Sparks层，新增只含火星的短发射副本，GAS结算实际掉血后在各受击者位置播放。空转不播放命中火星。SyncStormToAnimation使周期/PlayRate控制伤害频率，持续粒子速度跟随PlayRate，重复Timer避免帧误差累积。原动画及朝移动方向规则不变。当前每轮一次约1.05556秒，单次伤害不变，DPS随频率降低。

最终构建通过、6项Umbra.Skills通过、10项PIE通过（原地/移动各30秒，存活粒子与命中火星位置/空转验证、伤害节拍、松键/控制/死亡清理）。多人网络/打包/群怪压力未验证。人工编辑步骤和证据见 [SwordStormChannel](SwordStormChannel.md)。

## 2026-11-13 第9/10空槽：方向型扇形与条形原型

9键进入3米/120°扇形，0键进入8米长/2米宽条形；鼠标选方向，左键确认，右键/Esc取消。确认锁定方向；斩击一次结算，波以10米/秒推进并对每目标去重。两者共用DA形状参数驱动材质和中心判定，资源消耗为0，未接入现有技能，保留第8槽跃击。具体参数与边界约定见[技能目标选择](SkillTargeting.md)。

本轮验证：UE5.8.2 UmbraEditor Development构建成功；Umbra.Skills自动化6/6通过；最终离屏渲染PIE17场景、263项断言通过，覆盖鼠标/左右键、确认方向锁定、取消、各形状边界、任意旋转远端角落、墙后遮挡、多目标穿透、推进时序、零消耗、生命周期清理。已查看扇形/条形与波推进截图。证据：Saved/DirectionalBuild.log、DirectionalAutomation.log、DirectionalTargeting/pie.json及同目录PNG。未验证多人、打包、复杂地形、大规模性能或人工物理键鼠手感；本轮未重跑跃击全套。启动阶段仍有既有Condition failed等日志，详见技能目标选择文档。

## 2026-11-13 斩切波发射对齐挥剑中段

- 在编辑器逐帧检查Attack_C_Slow：0.38秒接近收招，选取剑向前挥过的0.175秒作为发射帧。DA_BreachSlash的StrikeTime改为0.175、WindupEndTime改为0.10；WindupDuration=0.65与PlayRate=0.65保持不变。只保存1技能DA，供应商动画和其余技能未修改。
- 调整入口仍为DA中的Strike Time（源动画秒），当前实际发射延迟约0.765秒。脚本及配置源见[SlashWave](SlashWave.md)末节；默认配置脚本已同步，避免恢复旧时刻。
- 新进程渲染PIE 7/7通过（SlashWaveMidSwingPIE.log、Saved/SlashWave/pie.json），首次观察到投射物约0.775～0.783秒，碰撞/取消/零消耗正常；已查看源动画姿势和发波截图。本轮没有修改C++，未重新编译；多人、打包、人工最终手感未验证。
- 独立发射姿势检查1/1通过（Saved/SlashWave/release-pose-pie.json）：在首次看到投射物时读取蒙太奇位置，与0.175秒目标帧误差小于0.04秒。合计7个行为用例与1个姿势同步用例通过。

## 2026-11-13 工作区与版本控制整理

- `.gitignore` 补充根目录临时文件清单 `files.txt`、个人 Unreal 工作区 `Content/Developers` / `Content/Collections`、本地打包输出 `LocalBuilds` / `PackagedBuilds`，以及 Python 虚拟环境、测试和类型检查缓存。
- `files.txt` 与已被忽略但仍受跟踪的 `UpgradeLog.htm` 仅从 Git 索引移除，本地文件保留。已有 Binaries、Intermediate、Saved、DerivedDataCache、IDE 缓存和导入 Staging 规则继续生效。
- Git 仓库保存开发所需源材料，不等于最终发布包。Docs、Scripts、测试源码、UmbraEditorTools、SkillLab 和正式 EquipmentImport/Library 保留版本控制，以支持复现、回归和引用追踪。最终资源裁剪需匹配编辑器中的引用审计与打包验证；本次未认定未读取的二进制资产无用，资产内容与引用待编辑器确认。
- 本次只整理版本控制并提交现有工作区进展，不修改玩法或二进制资产。验证范围为忽略规则、暂存差异和 Git LFS；未重新构建、运行自动化、PIE 或正式打包，前文历史验证不代表本次验证。
- 依据 [辉石魔砾导入记录](GlintstonePebbleImport.md) 对失败候选的明确记录，额外忽略 Staging/GlintstonePebble 下 Source、SourceV2/V3、Retarget、RetargetV2/V3 六个目录，仅保留本地诊断文件；V4 与正式目录继续跟踪。补充 Scripts 下 .NET bin/obj 生成物规则；清理本次提交文本末尾多余空行。
