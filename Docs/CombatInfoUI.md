# 第二页战斗属性 UI：现有 WBP_CombatInfo 接线

## 当前交付与边界

2026-10-01：用户确认蓝图手动修改。本轮只交付两个纯 UI C++ 类及原生测试；**没有保存任何 Content 资产，下面布局尚待手动接线与 PIE 验收**。磁盘尚无 WBP_CombatStatEntry，需按下文在编辑器创建。交付入口是本指南，不运行 Saved 下的临时编辑脚本。

- [UmbraCombatInfo](../Source/Umbra/UI/Combat/UmbraCombatInfo.h)：作为现有 WBP_CombatInfo 的父类，只绑定两个分类按钮，控制 Rows 的 Visible/Collapsed 和箭头。默认两组展开，同一实例重新 Construct 保留折叠状态，Destruct 成对解绑；没有生成行、Tick 或属性订阅。
- [UmbraCombatStatEntry](../Source/Umbra/UI/Combat/UmbraCombatStatEntry.h)：继承现有 UUmbraStatEntry 的纯显示能力，PreConstruct 将 `TestValue` 原样交给 `SetDisplayValue`。Designer 与运行时均显示固定测试文本，无世界/PlayerState/ASC 依赖。
- 现有 UUmbraStatEntry、UUmbraAttributeMenu、UUmbraCharacterStatsPanel 和 Gameplay 类均不修改。没有新增 GAS 属性枚举、AttributeSet 字段或计算。

## 1. 已确认的菜单关系

UE 5.8.2 只读加载的已保存资产，与用户截图一致：

```text
/Game/UI/CharacterMenu/AttributeMenu/WBP_AttributeMenu
└─ WidgetSwitcher_0
   ├─ [0] HeroInfo（ScrollBox）
   │      └─ … → WBP_HeroInfo + WBP_PrimaryAttribute
   ├─ [1] WBP_CombatInfo
   └─ [2] WBP_TalentInfo

WBP_CombatInfo（当前）
└─ ScrollBox_1
   └─ Overlay_151
      ├─ Image_188（现有 Paper_01）
      └─ VerticalBox_1
         └─ WBP_PrimaryAttribute（仅占位实例）
```

第一页面是 HeroInfo 滚动容器，其中包含真正的主要属性面板；不要把它误改成只有一个 PrimaryAttribute 的新页面。旧 `/Game/UI/CharacterMenu/WBP_AttributeMenu` 是重定向器，不创建同名替代资产，不清理或移动它。

## 2. 编译后创建可复用属性行

新增反射类需完整编译后重新打开编辑器，不靠 Live Coding。模块/Target 未改变，无需重新生成工程；Rider 未显示新文件时刷新工程。构建目标为 UmbraEditor / Win64 / Development，使用 Umbra.uproject 关联的 UE 5.8。本轮构建和自动化结果见 [Progress](Progress.md)。

1. Content Browser 找到 `/Game/UI/CharacterMenu/StatEntry/WBP_StatEntry`，**在编辑器中 Duplicate** 到 `/Game/UI/CharacterMenu/AttributeMenu/WBP_CombatStatEntry`。只改副本，原 StatEntry 仍服务于第一页。
2. 打开副本 → File → Reparent Blueprint → **UmbraCombatStatEntry**。它仍继承 UmbraStatEntry，无需给原 StatEntry 换父类。
3. Class Defaults：`Tooltip Class=None`；`Stat Name=属性名称`；`Test Value=0.0%`。继承的 `Stat`、`Description` 不用于本阶段；无需为战斗行增加 Stat 枚举，也不要把行注册到 CharacterStatsPanel。
4. 保留已有 `StatNameText`、`StatValueText`，勾选 Is Variable。名称必须准确，因为父类通过 BindWidgetOptional 更新它们。Text 不添加 Binding。
5. 副本里的 StatIcon 设 **Collapsed**，不占空间；不用给战斗行配置图标。原图标资源不删除。
6. 复用原 SizeBox/HorizontalBox，调整为下面的布局。行根 Height Override 建议82，关闭 Width Override，宽度跟随父容器。以下数字均为 UMG 布局单位，不是最终屏幕像素；现有 ScaleBox 和 DPI 仍会缩放。

```text
SizeBox（高度82，宽度不覆盖）
└─ HorizontalBox
   ├─ StatIcon（Collapsed，可保留）
   ├─ 名称容器 SizeBox（不覆盖宽度）
   │  └─ StatNameText
   ├─ DividerSize : SizeBox（Width Override=2）
   │  └─ ColumnDivider : Image
   └─ 数值容器 SizeBox（关闭原来的400宽覆盖）
      └─ StatValueText
```

7. 名称容器 HorizontalBox Slot：Size=Fill 0.70，水平Fill、垂直Fill，Padding=(20,0,18,0)。StatNameText 自身左对齐、垂直居中。
8. DividerSize Slot：Size=Auto，垂直Fill，Padding=(0,12,0,12)。ColumnDivider 使用单色 Brush，Draw As=Image，Tint 可先用线性 RGBA=(0.62,0.50,0.32,0.65)，Visibility=Not Hit-Testable (Self & All Children)。分隔线每行位置统一。
9. 数值容器 Slot：Size=Fill 0.30，Padding=(18,0,0,0)。StatValueText 在其 SizeBox Slot 中水平Right、垂直Center，右边距20，文字 Justification=Right。
10. 名称白色；数值淡黄色，线性 RGBA 可先用(1,0.86,0.57,1)。沿用原 `SourceHanSerifSC-Regular_Font`，字号40；可加黑色1×1阴影帮助纸张背景上的可读性。长名称“光耀/神圣伤害”必须完整显示。
11. 无需给行写 Event Tick、Construct 或 PreConstruct 图表。原副本若有额外写 Text 的 Graph/Binding，检查并移除副本中的重复写入；C++ PreConstruct 已写入名称和 TestValue。Compile/Save。

配置顺序：C++ 空文本后备 → WBP_CombatStatEntry Class Defaults → WBP_CombatInfo 中每个行实例的 Stat Name / Test Value；PreConstruct 使用最终实例参数。空测试值显示“—”。百分号、小数位均是文本的一部分，`122.0%` 不转换成倍率，不读取真实攻速。

## 3. 仅修改原 WBP_CombatInfo 内部

1. 打开现有 `/Game/UI/CharacterMenu/AttributeMenu/WBP_CombatInfo`。在 Hierarchy 中选它内部的 **WBP_PrimaryAttribute 实例**并移除；不要删除 Content Browser 中的 WBP_PrimaryAttribute 资产，也不要动 AttributeMenu 第一页里的实例。
2. 保留 ScrollBox_1、Overlay_151、Image_188、VerticalBox_1 和纸张 Brush。只向原 VerticalBox_1 添加下面两组。可以先做完控件再 Reparent，避免 BindWidget 缺失导致临时编译错误。
3. 在 VerticalBox_1 内创建两个 VerticalBox：AttackSection、DefenseSection。每组先放 Button，再放 VerticalBox。Button 里放 HorizontalBox，包含两个 TextBlock（箭头和标题）。

```text
WBP_CombatInfo [Parent: UmbraCombatInfo]
└─ ScrollBox_1（原有）
   └─ Overlay_151（原有）
      ├─ Image_188（原有纸张）
      └─ VerticalBox_1（原有）
         ├─ AttackSection : VerticalBox
         │  ├─ AttackToggle : Button
         │  │  └─ HorizontalBox
         │  │     ├─ AttackArrow : TextBlock（▼）
         │  │     └─ AttackTitle : TextBlock（攻击）
         │  └─ AttackRows : VerticalBox
         │     └─ 16个 WBP_CombatStatEntry 实例
         └─ DefenseSection : VerticalBox
            ├─ DefenseToggle : Button
            │  └─ HorizontalBox
            │     ├─ DefenseArrow : TextBlock（▼）
            │     └─ DefenseTitle : TextBlock（防御与抗性）
            └─ DefenseRows : VerticalBox
               └─ 10个 WBP_CombatStatEntry 实例
```

4. 以下**六个名称必须准确且 Is Variable=勾选**：AttackToggle、DefenseToggle、AttackArrow、DefenseArrow、AttackRows、DefenseRows。前两个必须是原生 Button，不是包装按钮的 UserWidget；中间两个 TextBlock，最后两个 VerticalBox。
5. File → Reparent Blueprint → **UmbraCombatInfo**。Compile；如报缺少绑定，按上一步检查名称/类型，而不是新增第二个 Combat 页面。
6. 两个标题按钮可使用半透明深棕底，Normal/Hovered/Pressed 配置不同深浅，保留原菜单的棕色与纸张风格。Content Padding=(18,14,18,14)，Is Focusable=false。箭头右边距18，标题沿用中文字体40，文字白色。DefenseSection 顶边距24。
7. 不要额外连接 OnClicked、Set Visibility 或箭头文字 Binding。C++ 已唯一绑定按钮；点击只折叠对应 Rows，不隐藏标题，不清除行，不切页。初始全部展开，同一菜单实例折叠状态在关闭重开后保留；新实例恢复展开。
8. Rows、Section、VerticalBox_1 用 Not Hit-Testable (Self Only)，Button 保持Visible、Enabled。文字及纸张 Image_188 用 Not Hit-Testable (Self & All Children)，防止装饰挡住按钮/滚轮。

## 4. 26行测试值

在对应 Rows 里拖入 WBP_CombatStatEntry，逐个选实例填写 Details → Character Stats → **Stat Name** 和 Combat UI → Test Data → **Test Value**。不要填原 TextBlock 的 Designer 文本来替代实例参数，PreConstruct 会覆盖它。

所有实例的 VerticalBox Slot：Size=Auto、水平Fill，不能纵向Fill均分剩余高度。攻击第7、第10、第16行可加Top Padding=14，将物理伤害、元素伤害和荆棘分开；不增加其他属性或子分类标题。

| 分类 | 顺序 | Stat Name | Test Value |
| --- | --- | --- | --- |
| 攻击 | 1 | 攻击力 | 186 |
| 攻击 | 2 | 攻速 | 122.0% |
| 攻击 | 3 | 暴击几率 | 10.1% |
| 攻击 | 4 | 暴击伤害 | 50.0% |
| 攻击 | 5 | 易伤伤害 | 20.0% |
| 攻击 | 6 | 所有伤害 | 12.0% |
| 攻击 | 7 | 挥砍伤害 | 15.0% |
| 攻击 | 8 | 钝击伤害 | 15.0% |
| 攻击 | 9 | 穿刺伤害 | 15.0% |
| 攻击 | 10 | 火焰伤害 | 10.0% |
| 攻击 | 11 | 闪电伤害 | 10.0% |
| 攻击 | 12 | 冰霜伤害 | 10.0% |
| 攻击 | 13 | 光耀/神圣伤害 | 10.0% |
| 攻击 | 14 | 毒素伤害 | 10.0% |
| 攻击 | 15 | 暗影伤害 | 10.0% |
| 攻击 | 16 | 荆棘 | 24 |
| 防御与抗性 | 1 | 护甲值 | 2896 |
| 防御与抗性 | 2 | 挥砍抗性 | 19.2% |
| 防御与抗性 | 3 | 钝击抗性 | 19.2% |
| 防御与抗性 | 4 | 穿刺抗性 | 19.2% |
| 防御与抗性 | 5 | 火焰抗性 | 19.2% |
| 防御与抗性 | 6 | 闪电抗性 | 19.2% |
| 防御与抗性 | 7 | 冰霜抗性 | 19.2% |
| 防御与抗性 | 8 | 光耀/神圣抗性 | 19.2% |
| 防御与抗性 | 9 | 毒素抗性 | 19.2% |
| 防御与抗性 | 10 | 暗影抗性 | 19.2% |

这些都是 UI 测试字符串，无数值含义；攻击力未来的主属性/武器缩放不在本阶段实现。没有武器速度、生命、治疗、格挡、躲闪、减伤、屏障、恢复、资源或异常抗性。

## 5. 滚动与宽度

1. ScrollBox_1：Orientation=Vertical，Allow Overscroll=false，Consume Mouse Wheel=When Scrolling Possible，Clipping=Clip to Bounds。滚动条厚度可用10×10；Visibility 保持Visible（由滚动框决定是否需要显示）。
2. 原 VerticalBox_1 的 **Overlay Slot** 当前是Left/Top，必须改成 **Horizontal Alignment=Fill、Vertical Alignment=Top**；Padding建议(28,24,28,32)。这是避免第二页沿用占位内容的小宽度的关键。
3. Overlay_151 的 **ScrollBox Slot** 建议 Size=Auto、Horizontal=Fill、Vertical=Top。内容按实际行高变长，ScrollBox 从已有 WidgetSwitcher 获得有限视口高度。不在 ScrollBox 外新增按全部内容撑高的 SizeBox，不重构 AttributeMenu。
4. Paper_01 仍在 Overlay 最底层，Image 的 Overlay Slot 双向Fill；两分类共用这张纸。折叠后背景跟随内容高度变化属于当前结构的表现；若希望始终铺满视口，可在编辑器试用原来的 ScrollBox Slot Fill 1，并验收内容超高时仍可滚动。不要为填背景把内部 Rows/行设成Fill。
5. 标题/行及名称容器取消固定宽度，保持70%名称、30%数值和中间2单位线。实际屏幕上太小时，统一调整副本字号和行高，不更改第一页字体或菜单全局缩放。

## 6. 菜单接入与验收

WBP_AttributeMenu 不需要任何修改：Switcher索引1仍引用同一个 WBP_CombatInfo 资产，索引0和2、按钮TabIndex、HeroInfo显示逻辑全部保留。不在父菜单替换页面、不写新的切页Graph、不接 GAS。

完成接线后 Compile/Save 行副本与 CombatInfo。打开 L_Prototype 的 Output Log，再运行最小 PIE：

1. 按既有菜单键打开角色菜单。第一页仍显示原 HeroInfo 和主要属性。
2. 点击第二页按钮：显示“▼ 攻击”，16行正确；向下滚动能看到“▼ 防御与抗性”及最后“暗影抗性”。名称白色、数值淡黄色、所有竖线和右边缘对齐，长名称不截断。
3. 点击攻击标题：攻击16行全部折叠，箭头变▶，防御标题向上移动；再点恢复。防御标题单独折叠/展开同样正确。点击标题不得切到其他页。
4. 连续执行1→2→3→2→1至少3次，第三页内容不变；第一页仍按原机制显示真实主要属性，第二页始终显示本表测试值。
5. 关闭重开菜单5次，行数始终16+10；同一实例保留折叠状态；重新启动PIE，两组初始展开。滚轮和滚动条均可到达最后一行。
6. Output Log没有BindWidget缺失、Accessed None、蓝图Compile错误。原生测试不替代这些资产与真实输入验证。

Session Frontend → Automation：运行 `Umbra.UI.CombatInfo.SectionLifecycle`、`Umbra.UI.CombatInfo.TestTextWithoutGameplay`，回归 `Umbra.UI.CharacterStats.PrimaryRows`。前两项只验证C++行为，最后一项是现有主要属性观察逻辑；当前尚无已接线战斗页或三页PIE验收结果。

字体及 Paper_01 均复用项目现有资源，不新增第三方下载或修改许可证。来源规则仍见 [EditorSetup](EditorSetup.md)。新建 uasset 延用仓库现有 Git LFS 规则。
