# 敌人信息卡与角色/技能页面（2026-11-12）

## 当前设计与数据边界

- `WBP_CombatHUD` 左上角增加 `WBP_EnemyTargetPanel`，位置24/24、尺寸360×100（Slate布局单位，随现有DPI缩放），显示头像、名称、等级、生命条及当前/最大生命。无有效目标时隐藏，整个卡片不拦截鼠标。
- 高亮目标优先级：有效鼠标悬停目标 → 有效持续普攻高亮目标 → 无目标。保留原有高亮与攻击逻辑；Controller发送变化通知，卡片复用EnemyHealthBar的GAS属性监听与销毁清理，不重新计算生命或伤害。
- 名称、等级、头像来源：敌人蓝图类默认值 `UI|Identity` 下的 `EnemyDisplayName / EnemyDisplayLevel / EnemyPortrait`。基础类默认Enemy/1/空；当前BP_Enemy_Melee_01配置“近战守卫”、1级。显示等级是显式配置，尚无等级驱动敌人战力的新增系统，也不挪用LootItemLevel。头像为空时使用Fantasy GUI 4的profile通用头像，本轮没有制作专属敌人肖像。
- 头像覆盖顺序为EnemyPortrait → WBP_EnemyTargetPanel.FallbackPortrait。当前生命来自敌人ASC的Health/MaxHealth；名称、等级、头像是类默认静态元数据，不是运行时复制的可变属性。

## 全窗口切页

按用户明确选择：技能页面占满整个三栏角色窗口，切页时一起隐藏左侧属性、中央装备与角色预览、右侧背包。

- `WBP_CharacterMenu` 保留现有根布局、三栏及背景。原HorizontalBox_0整体移入新增MainPageSwitcher第0页；第1页SkillsPage为空Canvas，不创建虚假技能树节点或待实现按钮。
- 顶部中央新增48×48的角色/技能按钮，间距6，标题通过Tooltip显示；角色为Fantasy GUI 4头盔，技能为同包firesword交叉武器图标。按钮样式来自现有W_ButtonBrownSquare_Icon，与左侧分页按钮同系列。
- 两页共用原Image_162深蓝透明底板；页内容保持完整原尺寸；48×48分页按钮位于画框顶部外侧，不占用内部空间。技能页不复制背包或装备面板。
- 原生 `UUmbraCharacterMenu` 负责两按钮点击与激活状态；WidgetSwitcher和原页面可见性监听负责暂停隐藏的装备/背包页面。返回角色页恢复原页面实例，避免重建背包状态；窗口再次打开保留当前主页面。
- 资产迁移入口 `Scripts/configure_ui_expansion.py`；编辑器辅助函数 `CreateTargetAndMenuPages` 按命名控件识别迁移；显式迁移`AdoptEnemyNameplateTemplate`把旧实例包装替换为原结构副本，并在SizeBox_2中添加HealthValueOverlay（原血条+HealthText）。检测到HealthValueOverlay后不再重建，保留后续手动调整，菜单页内容保留。后续不要对已手工调整的资产无意重跑。脚本会重设当前测试敌人的显示名/等级以及通用备用头像；人工修改这些配置后不要无意重跑。
- 素材不改供应商原包；实际引用为 `/Game/Asset/fantasy_gui_4/textures/icons/items/fg4_icons_helmet`、`fg4_icons_firesword`、`/Game/Asset/fantasy_gui_4/textures/icons/gold/fg4_iconsGold_profile`，目标卡复制`/Game/Asset/fantasy_gui_4/widgets/templates/nameplates/W_Nameplate_2`的原WidgetTree到WBP_EnemyTargetPanel，保留MainSizeBox及其下原布局和嵌套控件，保留原框体、头像材质和W_ProgressBar_Parent血条材质；GAS比例调用原模板SetProgressValue(Value)事件，保留液体/边缘发光逻辑。名称使用思源宋体SourceHanSerifSC-Regular_Font，等级和生命数值使用思源黑体SourceHanSansSC-Regular_Font；血条使用供应商Mat_GUI_ProgressBar_Red，保留原SetProgressValue事件和发光逻辑，血量文本叠加于原血条，隐藏没有真实状态数据的示例Buff。来源规则沿用[EditorSetup](EditorSetup.md)。

## Combat HUD 八项图标（已导入并配置）

推荐统一使用Game-icons.net的Lorc系列白色透明PNG，在UMG中着色；保留SVG母版。建议下载256或512尺寸，UI显示约24–32布局单位。设置UI纹理组、透明Alpha与统一内边距；八项图标必须保留真实Alpha透明背景，不添加黑色、白色或彩色方形底板；网页预览底色不应烘焙进PNG。

| 属性 | 图标 | 建议色 |
| --- | --- | --- |
| 物攻 | [Broadsword](https://game-icons.net/1x1/lorc/broadsword.html) | 橙 |
| 法强 | [Magic swirl](https://game-icons.net/1x1/lorc/magic-swirl.html) | 紫 |
| 物抗/护甲 | [Breastplate](https://game-icons.net/1x1/lorc/breastplate.html) | 金 |
| 法抗 | [Magic shield](https://game-icons.net/1x1/lorc/magic-shield.html) | 青蓝 |
| 攻速 | [Quick slash](https://game-icons.net/1x1/lorc/quick-slash.html) | 浅黄 |
| 技能急速 | [Stopwatch](https://game-icons.net/1x1/lorc/stopwatch.html) | 浅蓝 |
| 暴击率 | [Target arrows](https://game-icons.net/1x1/lorc/target-arrows.html) | 红 |
| 移速 | [Wingfoot](https://game-icons.net/1x1/lorc/wingfoot.html) | 白 |

官方[FAQ](https://game-icons.net/faq.html)说明署名要求；上述Lorc图标按各页CC BY 3.0登记作者、来源和修改说明。只借鉴英雄联盟的紧凑属性符号设计，不需要提取其客户端图标。用户已导入到`/Game/Asset/Icons/8statsicons`，现已将StatsPanel目录八个WBP的staticon资源逐项替换为表中图标；只修改Brush.ResourceObject，保留原Brush Tint、Image ColorAndOpacity、尺寸和布局。配置入口`Scripts/configure_eight_stat_icons.py`。技能急速“仅存储”提示保持真实业务状态。

下载设置：左侧Studio → Background → 颜色圆点 → Alpha/A设为0，Foreground保持白色不透明，Frame宽度0，再使用右侧PNG下载（256/512）。默认黑色方形是可编辑背景，不代表无法透明导出。官方说明：https://game-icons.net/studio.html 。

## 后续UI优先级

1. 技能悬停说明：名称、按键、资源、冷却、范围、机制说明；伤害预览读取现有GAS结果。
2. 玩家及目标状态条：增益/减益、层数、剩余时间；将学派附魔效果纳入同一表现入口。
3. 技能配置页：先做技能列表、详情、六个装备槽；之后再考虑树形节点，避免当前就承诺完整天赋树规则。
4. 操作失败反馈：资源不足、冷却、目标无效、背包满等，统一位置和短时提示，避免满屏弹窗。
5. 目标施法条及精英/Boss身份：已有相应玩法数据后接入；普通目标卡与Boss条避免重复展示。
6. 设置页：UI缩放、音量、按键提示；小地图、任务追踪等随地图/任务系统成熟再安排。

## 复验入口

`Scripts/smoke_ui_expansion.py` 使用临时PIE场景检查鼠标目标、GAS血量更新、无目标隐藏、整窗切页、角色预览暂停/恢复及关闭清理。截图与报告在 `Saved/UIExpansion`，测试不保存地图。构建、自动化与实际完成边界见[Progress](Progress.md)。

### 敌人卡Designer修改入口

直接打开`/Game/UI/HUD/WBP_EnemyTargetPanel`：

- `TextBlock_0`：名称；思源宋体，字体/字号/颜色直接在Font和Appearance调整。
- `TextBlock_5`：等级；思源黑体。
- `SizeBox_2 → HealthValueOverlay → HealthText`：已保存的血量TextBlock，思源黑体；可调整字号、颜色、Overlay对齐和Padding。
- `SizeBox_2 → HealthValueOverlay → W_ProgressBar_Parent`：原嵌套血条；实例属性ProgressBarMaterial配置Mat_GUI_ProgressBar_Red。
- `NameplateViewport → MainSizeBox`：外层等比缩放与原模板尺寸；屏幕位置和卡片大小仍在WBP_CombatHUD的EnemyTargetPanel Canvas Slot。

C++只绑定数据、更新名称/等级/生命文字、设置头像参数并调用原血条SetProgressValue；不创建TextBlock，不覆盖字体/字号/颜色/布局/血条材质。不改供应商模板。不要重命名数据绑定控件，若需改名应同步原生查找名/BindWidget。配置脚本configure_nameplate_style.py仅供明确重置当前字体及红色材质时使用，日常手动调整无需执行。

## 技能文字Tooltip（2026-11-13）

- `/Game/UI/Skills/WBP_SkillTooltip`独立Designer资产，原生父类UUmbraSkillTooltip；只有SkillName、DescriptionText两段文字。DescriptionFrame复用物品WBP_ItemTooltip的Border_1笔刷，文字分别来自ItemName/FlavorTextBlock样式，使用思源宋体；不复制物品图标、品质、词条、重量或装备要求。
- TooltipSize初始宽360，自动高度；SkillName字号24，DescriptionText沿用正文18并去除斜体，WrapTextAt330。Designer可直接调整，不由运行时覆盖。初次生成后不重建已存在Tooltip布局。
- 数据来源：PlayerState.InitialSkillSlots → Definition.DisplayName / Description。Description是DataAsset中可编辑、多行的本地化FText；本轮填充当前已配置技能的定性机制说明（配置时为Greystone六技及一个跃击瞄准测试槽），不在UI计算伤害。配置脚本只填空描述，保留人工编辑文本与原技能调参。
- WBP_SkillSlot类默认及WBP_SkillBar中现有槽实例的SkillTooltipClass均指定该WBP。现有RefreshSkill复用状态采样，每槽缓存一个Tooltip；未授予/空槽清理引用，冷却或不可施放时仍可查看说明，Destruct解除Tooltip。原生鼠标进入/移出控制视口展示；优先在技能框上方居中，间距8，上方空间不足回退到下方，最终按视口边距8夹紧。已有20Hz刷新负责跟踪槽位置及文字布局，不跟随鼠标。
- 允许SkillBar的外层及嵌入分支SelfHitTestInvisible，技能槽自身Visible，Tooltip全部HitTestInvisible。仅开放悬停检测，不新增点击施放行为。备用独立SkillBar视口路径同步允许子控件命中。
- 配置：Scripts/configure_skill_tooltip.py。复验：Scripts/smoke_skill_tooltip.py。描述应随机制变化人工维护，现阶段不展示动态伤害、冷却或消耗数值。
