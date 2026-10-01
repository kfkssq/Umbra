# Umbra 进度与验证状态

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
