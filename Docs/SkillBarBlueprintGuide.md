> 2026-11-05 更新：用户授权先制作 UI；现有技能栏已嵌入 CombatHUD。当前布局与人工调整入口以 [连续框技能栏](SkillHUDLayout.md) 为准。下文为可选的手工自定义接线参考。

> 2026-11-06：已接入六个技能槽（Q/E/1/2/3/4），新技能通过同一 ReadViewState 与冷却 GE 自动刷新，WBP 无需新增倒计时接线。新增四种技能的 DA/AM、InputTag、图标对应见 [Greystone 技能](GreystoneSkills.md)。

# 技能栏手工接线：HUD → WBP_SkillBar → WBP_SkillIcon

## 本轮约定与当前状态

2026-11-05 用户明确：WBP 配置和接线由用户完成，实际视觉以用户自己的 HUD 为准。下面只规定数据接口、控件职责和接线；尺寸、位置、图框、字体、颜色、动效由用户决定。

现有运行链已完成：Q/E → 两个技能 spec → GAS 费用/冷却 → Greystone Montage → 单次扇形伤害。已有 `/Game/UI/Skills/WBP_SkillSlot` 和 `WBP_SkillBar` 是本轮此前生成的演示资产，不要求采用其外观。本指南建议新建 `WBP_SkillIcon`，手工接自己的控件。没有替用户创建这个新命名资产。

`configure_sweep_skills.py` 已改成只配置玩法资产、输入和授予，不再创建/挂载 UI。此调整之后未再运行资产写入。此前控制器的独立 Skill Bar Class 仍有演示值；用户需要按第1步清空它，避免出现两套技能栏。

## 1. 先把 UI 入口交回你的 HUD

1. 打开 `/Game/Blueprints/Player/BP_UmbraPlayerController` → Class Defaults。
2. 把 **Skill Bar Class = None**。这是本轮演示的独立挂载入口，清空后不会单独 AddToViewport 技能栏；Skill Bar Offset 随之不再生效。
3. **Combat HUD Class** 保持你的 HUD。当前项目读到的是 `/Game/UI/HUD/WBP_CombatHUD`；如果你使用另一份 WBP_HUD，就选你的那份。
4. Compile → Save。

控制器已经执行 CreateWidget(CombatHUDClass, OwningPlayer=this) → AddToViewport。你的 HUD 内只需要放一个技能栏子控件，不需要再在 Event BeginPlay 创建一次 HUD，也不用在技能栏里 AddToViewport。

建议层级：

```text
你的 WBP_HUD（现有父类保持不变）
└─ 现有根布局
   ├─ 现有头像、生命、资源等控件
   └─ WBP_SkillBar
      └─ HorizontalBox / 你的排列容器
         ├─ WBP_SkillIcon  SlotIndex=0
         └─ WBP_SkillIcon  SlotIndex=1
```

## 2. 新建 WBP_SkillIcon：单个技能格

Content Browser → User Interface → Widget Blueprint → All Classes → **UmbraSkillSlotWidget**，命名 `WBP_SkillIcon`。已经新建为 UserWidget 时，在 Class Settings → Parent Class 改成 UmbraSkillSlotWidget，再 Compile。

这个父类只要求 SlotIndex 和技能视图接口。当前 C++ 提供可选的自动绑定；本流程使用下面的自定义控件名，由蓝图完整控制显示，避免与父类自动表现绑定重叠。

### Designer 层级

布局容器按你的风格选择；下列 Overlay 示例足以承载图标和冷却。所有需要 Graph 引用的控件勾选 **Is Variable**。

```text
SizeBox / 你的根布局
└─ Overlay
   ├─ Image_SkillBackground   Image，可选
   ├─ Image_SkillIcon         Image
   ├─ Progress_SkillCooldown  ProgressBar
   ├─ Image_SkillFrame        Image，可选
   ├─ Text_SkillCooldown      TextBlock
   ├─ Text_SkillKey           TextBlock
   ├─ Text_SkillName          TextBlock，可选
   ├─ Text_SkillCost          TextBlock，可选
   └─ Text_SkillStatus        TextBlock，可选
```

Overlay 顺序决定覆盖顺序：背景 → 图标 → 冷却遮罩 → 框 → 文字。每项的 Overlay Slot Padding/Alignment 由你决定。名称/消耗也可以放到 Overlay 外的 VerticalBox/HorizontalBox，不影响接线。

- Class Defaults：Is Focusable=false。
- 根布局 Visibility：**Not Hit-Testable (Self & All Children)**，只显示，不截获场景点击。
- Progress_SkillCooldown：Percent=0；Is Marquee=false；Bar Fill Type=Bottom to Top；Bar Fill Style=Scale；Border Padding=(0,0)。Style.BackgroundImage.DrawAs=None；Style.FillImage.DrawAs=Image、Tint=白色；Fill Color and Opacity 设你希望的半透明遮罩色。建议起点黑色 Alpha=0.7。
- Text_SkillCooldown 和 Progress_SkillCooldown 初始 Visibility=Hidden；冷却事件控制其显示。
- 数字放在遮罩上方。根 SizeBox 的 Width/Height、图标大小和边框均以你的 HUD 为准。

### 两种绑定方式不要混用

本指南使用 `Image_SkillIcon`、`Progress_SkillCooldown`、`Text_SkillCooldown` 等自定义名称。

父类保留的自动绑定名称是 `Img_Icon`、`Pb_Cooldown`、`Txt_Name`、`Txt_Key`、`Txt_Cost`、`Txt_Cooldown`、`Txt_Status`。使用这些名字时父类会自动写入外观，不需要再写相同蓝图逻辑。若要完全自己接线，请使用上述自定义名。名字仅大小写不同仍可能视为相同，不能用大小写规避绑定。

## 3. WBP_SkillIcon Event Graph：完整数据接线

### 3.1 入口

1. 在 Graph 的 Overrides/可重写事件中找到 **On Skill View Updated**；也可右键搜索该事件。
2. 从事件的 **New View** 引脚拉出 **Break Umbra Skill Slot View**。
3. 添加 Sequence，按下述顺序接各更新段；也可建立函数 `ApplySkillView`，输入类型为 `UmbraSkillSlotView`，事件调用该函数。
4. 不需要 Event Tick、Delay 或自建倒计时。父类构造后每0.05秒读取一次当前玩家 ASC 并触发该事件，析构时清理。

可用字段：Name、Key Label、Icon、Resource Cost、Cooldown Remaining、Cooldown Duration、Cooldown Fraction、Granted、Affordable、Blocked、Active。

### 3.2 图标、名字、按键、消耗

| Target | 节点 | Value 接到哪里 |
|---|---|---|
| Image_SkillIcon | Set Brush from Texture | Texture ← Icon；Match Size=false |
| Text_SkillName | Set Text | In Text ← Name |
| Text_SkillKey | Set Text | In Text ← Key Label |
| Text_SkillCost | Set Text | Resource Cost → To Text(Float) → In Text |

消耗的 ToText 可设 Minimum Fractional Digits=0、Maximum Fractional Digits=0；这是显示格式，不能再向 GAS 扣一次数值。资源消耗暂时没有小数，未来有小数时再调整显示精度。

不要在 Image 的 Brush 或 Text 的 Text 属性上再创建 UMG Binding，避免与事件 Set 节点抢写。

### 3.3 冷却遮罩

```text
Cooldown Fraction → Progress_SkillCooldown.SetPercent(InPercent)
Cooldown Remaining > 0 → Cooling（可用局部布尔/纯比较输出）
Cooling → Select(ESlateVisibility)
    true  = Not Hit-Testable (Self & All Children)
    false = Hidden
→ Progress_SkillCooldown.SetVisibility
```

Cooldown Fraction 已是 **剩余时间 / 实际 GE 总时间**，不再除以4或6，也不做 `1-Fraction`。当前推荐遮罩从满到空；如果你改为“充能条从空到满”，只在显示层使用 `1-Fraction`。

### 3.4 冷却数字

```text
Cooldown Remaining
→ ToText(Float)
   Minimum Integral Digits=1
   Minimum Fractional Digits=1
   Maximum Fractional Digits=1
→ Text_SkillCooldown.SetText

Cooling → Select(ESlateVisibility)
    true  = Not Hit-Testable (Self & All Children)
    false = Hidden
→ Text_SkillCooldown.SetVisibility
```

冷却结束判断用原始 Remaining>0，不能用格式化文字等于“0.0”判断。最后不到0.05秒可能四舍五入显示0.0，但状态仍按真实时间判断。希望始终向上取整时，用 `Ceil(Remaining*10)/10.0` 仅处理显示数字。

### 3.5 可用状态、压暗和提示

从 Break 输出组合：

```text
Ready = Granted AND Affordable AND NOT Blocked AND NOT Cooling
```

Ready → Select Float(true=1.0,false=你希望的不透明度，例如0.4) → Image_SkillIcon.SetRenderOpacity。只改图标透明度，冷却文字仍保持清晰。边框颜色、闪烁、材质等也从这里由你配置。

可选状态文字，按以下顺序接 Branch，命中后 SetText：

1. NOT Granted → “未授予”。
2. Active → “施放中”。
3. Cooling → “冷却中”。
4. NOT Affordable → “资源不足”。
5. Blocked → “不可施放”。
6. 其他 → “就绪”。

Active 只代表该格技能正在执行；Blocked 含死亡、眩晕或正在攻击等共用状态。另一格可以显示“不可施放”但没有冷却数字，这是正常互斥。

可选冷却结束闪光：创建布尔 `WasCooling`，每次事件先判断 `WasCooling AND NOT Cooling`，成立才 Play Animation(你创建的闪光动画)，事件末尾再 Set WasCooling=Cooling。不能每次刷新都 Play Animation。

### 3.6 Designer 预览

运行时数据事件仅在实际 Widget 构造后开始。Designer 直接给控件设置预览图和预览文字即可；想更丰富时用 Event PreConstruct → Branch(IsDesignTime=true)，只更新预览值。不要在 PreConstruct 中授予技能、扣费或创建冷却 GE。

## 4. WBP_SkillBar：放置和选择技能格

1. 新建或编辑 `WBP_SkillBar`。父类可用已有 **UmbraSkillBar**；它是布局空基类。普通 UserWidget 也能承载这些子控件。
2. 根使用你的布局容器，例如 HorizontalBox。
3. 从 Palette → User Created 拖入两个 `WBP_SkillIcon`。
4. 第一个实例命名 `SkillIcon_Q`，Details → Skill → **Slot Index=0**。
5. 第二个实例命名 `SkillIcon_E`，**Slot Index=1**。
6. Spacing、Padding、对齐、Slot Size 和是否包 SizeBox 由你决定。
7. Compile → Save。

SlotIndex 对应 PlayerState.InitialSkillSlots 的数组下标，不能两格都默认0。当前是 Designer 实例配置，不是蓝图运行时 Set 属性。先完成这两个固定槽；动态换技能和热键重绑定不在本轮范围内。

技能栏 Graph 可以保持空。子控件自己通过 Owning Player 查询 PlayerState，无需从 HUD 逐格传 ASC、冷却秒数或技能伤害。

## 5. 把技能栏接进你的 WBP_HUD

1. 打开你的 HUD。当前工程是 `WBP_CombatHUD` → `WBP_PlayerHUDGroup`，包含现有头像、生命条、资源条、属性面板。
2. 在你希望管理技能栏位置的那层 Designer 内，拖入 **一个** WBP_SkillBar。
3. 如果放在根 Canvas：Anchor、Alignment、Position、Size/Auto Size 按你的 HUD 设计；底部居中可用 Anchor=(0.5,1)、Alignment=(0.5,1)，具体 Offset 由你决定。
4. 如果放在 WBP_PlayerHUDGroup 内，按现有 Horizontal/VerticalBox 层级排列，不需要 Canvas 坐标。
5. Compile → Save。

设计器嵌套方式自动传递 Owning Player，是本轮推荐方式。若你选择动态创建，则在 HUD 的 Construct：

```text
Get Owning Player
→ Create Widget(WBP_SkillBar, Owning Player=上面的玩家控制器)
→ 保存返回值（防重复创建）
→ Add Child to 你的 HUD 容器
```

动态创建的子栏也不要 AddToViewport；只有最外层 HUD 走原控制器的 AddToViewport。不要对 HUD 整体设 Not Hit-Testable(Self & All Children)，以免影响已有可点击控件；只对技能显示区域这样设置。

## 6. 技能图标与玩法数据入口

| 配置 | 横扫/Q | 广域横扫/E |
|---|---|---|
| Definition | `/Game/Skills/Prototype/DA_Sweep` | `/Game/Skills/Prototype/DA_WideSweep` |
| SlotIndex | 0 | 1 |
| InputTag | Input.Skill.One | Input.Skill.Two |
| KeyLabel | Q | E |
| ResourceCost | 15 | 25 |
| CooldownSeconds | 4秒 | 6秒 |
| CooldownTag | Cooldown.Skill.Sweep | Cooldown.Skill.WideSweep |
| Radius / Arc | 250cm / 120° | 350cm / 170° |
| WeaponMultiplier | 1.5 | 1.0 |
| Icon | warrior_12 | warrior_48 |

图标目录为 `/Game/Asset/Icons/flat_skills_icons_no_color/warrior/`。更换技能图标应改 **DA.Icon**，运行时 `NewView.Icon` 会传给 WBP。图框/背景/颜色由 WBP 决定，与 DA.Icon 分离。

原有灰框可选 `/Game/Asset/classic-rpg-ui/Assets/ClassicRPGUI/ClassicUI/SpecialFrames/Frame_grey` 和 `Background_grey`，但无需采用演示布局。

输入核对：

- IMC_Default：IA_Skill1→Q，IA_Skill2→E；动作 Value Type=Digital(bool)。
- BP_UmbraPlayerCharacter.AbilityInputActions：各 IA 对应上表 InputTag。
- BP_UmbraPlayerState.InitialSkillSlots：两项 Definition/InputTag/KeyLabel 对应上表。
- KeyLabel 是展示文本，重绑按键时需同步修改。
- 不在 WBP OnKeyDown 中释放技能，也不在图标更新事件中调用 Ability。键盘输入由角色已有 Enhanced Input → ASC 处理。

参数来源：原生默认值 → DA 保存值 → ability 执行；Slot 只选择定义和输入。伤害在命中时通过现有 GAS 武器通道、技能增伤、防御/抗性等结算。UI 只格式化结果，不计算伤害/冷却规则。

两份技能共享 `/Game/Skills/Prototype/AM_Sweep`，来自 Greystone Attack_PrimaryB_Montage 的独立副本，去除原普攻 Notify。StrikeTime=0.4秒，PlayRate=1，完整动作约1.4秒；使用 ClearAPath 的 Cascade Swipe 特效与已有 swordswing_fast_01 声音引用。当前固定朝角色施放瞬间朝向，单次扇形伤害；不自动朝鼠标。移动/死亡/眩晕可取消，已付费用与冷却不退。冷却/播放速率暂不受急速/攻速影响。

## 7. 接完后的最终验收

1. Play：HUD 中只显示你接的一个技能栏；两格名称/图标/Q/E/消耗正确，无冷却遮罩和数字。
2. 面向近处敌人，按 Q 一次，不移动：资源扣15、出刀、一次伤害；Q冷却约4秒递减，E没有被同时置入冷却。
3. 冷却中再按 Q：不再扣费、不再命中。等Q数字和遮罩消失，状态回到就绪。
4. 按 E：扣25、一次伤害；E冷却约6秒，图标正确；完整动作结束恢复可操作。
5. 资源不足时图标压暗/提示符合你的设计；按键不扣负数、不施放。
6. 关闭/重新打开 HUD 或重新 PIE，没有重复栏/重复定时器，Owner仍正确。

排错顺序：

- 两个都显示Q：两个实例 SlotIndex 都是0。
- 有背景却没数据：检查父类、OnSkillViewUpdated事件、SlotIndex、OwningPlayer、InitialSkillSlots。
- 名字正确但冷却不动：检查事件是否连到 SetPercent/SetText/SetVisibility，是否意外做了第二套属性 Binding。
- 数据刷新覆盖了你的外观：检查用了父类的保留自动绑定名，改用本指南自定义名字。
- 两套栏/位置不听HUD：控制器 SkillBarClass 没清空，或子栏也 AddToViewport。
- 按Q没有技能：先看角色和PlayerState输入Tag是否一致，不去WBP补OnKeyDown。
- 只能看到数字，看不到遮罩：检查ProgressBar的Fill brush/透明度与层级，确认不是被图标盖住。

已有验证只证明底层和此前演示WBP；你尚未完成的新HUD/WBP接线不能标为已验收。最新已测范围见 [Progress](Progress.md)。

## 8. 免费图标包建议

1. **[CraftPix 48 Free Swordsman Skills Icon Pack](https://free-game-assets.itch.io/48-free-swordsman-skills-icon-pack)**：48张256×256 PNG，优先选择。单弧剑光适合横扫，更宽剑弧适合广域；不要用火焰或多段斩图表达当前纯武器单次伤害。
2. **[CraftPix Free Skill Icons for Knight](https://free-game-assets.itch.io/free-skill-icons-for-knight)**：20张PNG，可补充骑士、守护、武器系。
3. **[Game-icons.net](https://game-icons.net/)**：适合统一单色轮廓。示例 [Lorc / Sword spin](https://game-icons.net/1x1/lorc/sword-spin.html) 提供SVG/PNG，CC BY3.0须署名；此图更适合未来360°旋风，当前前方横扫应选单弧斩击图。

[CraftPix Freebie License](https://craftpix.net/file-licenses/)允许个人/商业游戏使用与修改，无需署名，不能把图标源文件作为素材包转售/再分发。两款itch页面有免费/name-your-own-price入口。本轮只核对与推荐，未下载/导入新素材，也未替现有图标补造来源许可。

导入后建议 TextureGroup=UI、Compression=UserInterface2D(RGBA)，核对透明通道/sRGB和最终小尺寸可读性，保留包来源与许可。把纹理填入DA.Icon；WBP只处理边框和最终显示风格。
