# 正式角色属性面板

实现位置：`UUmbraStatEntry` / `UUmbraStatTooltip` / `UUmbraCharacterStatsPanel`。此文是 UE 5.8 Editor 接线清单；WBP 资产须在 Editor 中创建并保存，不直接改 `.uasset`。当前 C++ 已编译，以下 WBP 操作和 PIE 结果仍待 Editor 确认。

## 数据契约

| 位置（行／列） | Stat 枚举 | 显示 | 来源 |
| --- | --- | --- | --- |
| 1／左 | AttackPower | — | 缺最终物理侧武器伤害接口 |
| 1／右 | AbilityPower | — | 缺最终魔法侧武器伤害接口 |
| 2／左 | Armor | 整数 | GAS Armor |
| 2／右 | MagicResistance | 整数 | GAS MagicResistance |
| 3／左 | AttackSpeed | 每秒次数，仅显示两位小数 | GAS AttackSpeed ÷ `GA_BasicAttack` 的 1x 基础攻击周期 |
| 3／右 | AbilityHaste | — | 现有 GAS AbilityHaste 仅是存储值，尚未接入冷却规则 |
| 4／左 | CriticalChance | 整数百分比 | GAS CriticalChance × 100 |
| 4／右 | MoveSpeed | 整数 | GAS MoveSpeed，cm/s |

“—”表示无可信数值，也用于 ASC 尚未就绪和攻速周期未配置。当前伤害执行使用 `AttackPower × AD 系数 + AbilityPower × AP 系数`，没有独立武器基础伤害、物理／魔法两路最终值；`DamageType` 只选择目标抗性。不能把 `AttackPower`、`AbilityPower` 原值或某次攻击的混合伤害改名当作两项面板值。未来战斗层提供明确的最终派生值接口后，在 `UUmbraCharacterStatsPanel::TryReadStat` 的两个分支读取，并订阅该值依赖项的变化委托。技能急速需要冷却系统确定生效规则后同样接入；不要用冷却缩减百分比代替。

数值配置来源依次是 `BP_UmbraPlayerState.InitialAttributesEffect`、可选的 `DebugInitialAttributes` 覆盖、运行中 GAS Effect 聚合结果；面板只读取最终 GAS 当前值。攻速基础周期来自 `BP_UmbraPlayerState.InitialAbilities` 中的 `GA_BasicAttack` Class Defaults，正 `BaseAttackInterval` 优先于普通 A Montage 自动周期。条目正文只显示图标和数值；图标由各属性条目 WBP 的 Image → Brush 在 Details 中配置，C++ 不覆盖它。名称与说明在条目实例上配置并传给独立属性 Tooltip。字体、颜色、间距和布局由 WBP 控制。

Tooltip 计划分为属性、技能、物品三类。本次仅实现属性专用 `UUmbraStatTooltip` 与 `WBP_StatTooltip`；技能和物品 Tooltip 待各自的数据契约明确后另做。

攻击周期与战斗共用 `UUmbraBasicAttackAbility::GetConfiguredBaseAttackInterval()`：正 `BaseAttackInterval` 优先；否则取普通 `AttackMontages[0]` 的完整 `GetPlayLength()/RateScale`。`AttackSpeed` 按现有限幅 0.2～10 倍。面板显示的是配置周期下的理论逻辑起手次数／秒；服务器低帧率、目标不在范围、攻击取消或其他限制会降低实际观察到的次数，不改变此属性值。普通攻击每击开始快照倍率，面板则在属性变化时立即显示下一击将使用的当前倍率。

## Editor 创建和接入

1. 退出正在运行的 Unreal Editor，使用与 `Umbra.uproject` 关联的 UE 5.8 构建 `UmbraEditor / Win64 / Development`，再重开项目。带新 `UCLASS` 和枚举的模块不要只靠 Live Coding 加载。检查 Output Log 中没有类加载错误。
2. 在 `/Game/UI/HUD` 新建 Widget Blueprint `WBP_StatTooltip`，Parent Class 选 `UmbraStatTooltip`。Designer 放属性名称 Text 和说明 Text，按需要加背景、边距。Graph 实现 `Apply Stat Tooltip Content`：把 `InName`、`InDescription` 分别赋给对应 Text。这个 WBP 只负责属性 Tooltip 的样式和内容。Compile、Save。
3. 在同目录新建 Widget Blueprint `WBP_StatEntry`，Parent Class 选 `UmbraStatEntry`。Designer **只放图标 Image 和数值 Text**，根控件保持可命中以接收悬停，Image/Text 自身可设为不可命中。Text 勾选 `Is Variable`；Graph 实现 `Apply Stat Value`，把事件的 `Value` 直接接到该 Text 的 `SetText`。C++ 读取并格式化 FText，WBP 仅更新视觉控件。Image 不需勾 `Is Variable`，直接在它的 Details → Appearance → Brush → Image 设置纹理；C++ 不触碰 Brush，所以编译和运行都会使用该配置。Class Defaults 的 `Tooltip Class` 设为 `WBP_StatTooltip`。C++ 构建条目时创建 Tooltip，传入名称、说明并挂到条目的 Tool Tip。不要使用 UMG Text 每帧绑定。Compile、Save。
4. 为八项建立各自继承 `WBP_StatEntry` 的条目 WBP（例如 `WBP_AttackPower`），分别在子 WBP 的 Image Brush 设置图标。若编辑器不允许在子 WBP Designer 覆盖继承控件的 Brush，则直接分别创建八个以 `UmbraStatEntry` 为父类的 WBP，复制同样的 Image + Text 布局和 `Apply Stat Value` 接线。这样每项有独立的 Designer 图标，无需运行时图标变量。再新建 `WBP_CharacterStatsPanel`，Parent Class 选 `UmbraCharacterStatsPanel`，用 Uniform Grid Panel 或两列 Grid Panel 放八个对应条目 WBP，按上表四行两列。每个实例在 Details 设置 `Stat`、`Stat Name`、`Description`；各实例的 Stat 枚举必须唯一。图标选择有授权记录的现有资源或新导入资源，先核实纹理、许可和引用；不要仅凭文件名判断资产内部内容。
5. 面板构建时会扫描自身 WidgetTree 并注册八个 `UmbraStatEntry` 子控件，不依赖子控件或 TextBlock 名称；只有运行时动态添加条目才需调用 `Register Entry`。不要在 WBP 中自行读取 GAS、计算伤害／攻速、使用 Tick 或每帧属性绑定。Compile、Save。
6. 打开 `/Game/UI/HUD/WBP_CombatHUD`，在所需位置嵌入一个 `WBP_CharacterStatsPanel` 实例，调整尺寸、锚点、层级和可见性，Compile、Save。打开 `/Game/Blueprints/Player/BP_UmbraPlayerController`，确认 Class Defaults 的 `Combat HUD Class` 指向这个 `WBP_CombatHUD`；不要在 Level Blueprint 或 HUD Graph 中再 `Create Widget` 一份。现有 `InitializeCombatHUD` 只创建一次根 HUD，面板随根 HUD 构建；PlayerState 切换时 Controller 会通知嵌入的面板。
7. 检查八项 Tooltip 名称顺序：`攻击力｜法强`、`护甲｜魔抗`、`攻速｜技能急速`、`暴击率｜移速`。说明建议明确单位：攻速“次/秒”、移速“cm/s”；对三个未接入值解释“暂未接入”。首次 PIE 中，前三个缺失项应是“—”，其他五项从当前角色 GAS 初始化值显示。

若八项分别用独立 WBP，必须在**每个 WBP 的 Class Defaults** 指定其 `Stat`：`WBP_AttackPower → AttackPower`、`WBP_AbilityPower → AbilityPower`、`WBP_Armor → Armor`、`WBP_MagicResistance → MagicResistance`、`WBP_AttackSpeed → AttackSpeed`、`WBP_AbilityCooldown → AbilityHaste`、`WBP_CriticalHitChance → CriticalChance`、`WBP_MoveSpeed → MoveSpeed`。默认枚举值全是 AttackPower；若不改，Output Log 会出现 `duplicate stat`，面板只登记一个属性。每个条目的 `Apply Stat Value` 事件还需把传入的 `Value` 直接接到自己的 TextBlock `SetText`，否则数值位置会保持空白。

若已按旧步骤在条目 Graph 做过 `Apply Stat Icon → Set Brush from Texture`，该事件暂时保留以便旧 WBP 加载，但 C++ 不再调用。重新打开资产后可删除这个旧事件及连线；在 Image 自身 Details 中设置 Brush，Compile/Save。运行中的 Editor 仍可能加载旧模块，需要先关闭 Editor、构建原项目并重开，才能看到 C++ 改动。

## PIE 验证

1. 开启单人 PIE，确认只出现一份大 HUD 和一份属性面板；每格正文只有图标与数值，图标应与该条目 WBP 的 Image Brush 配置一致。逐格悬停，确认出现独立属性 Tooltip，名称和说明对应，移开后关闭；检查初始 Armor、MagicResistance、AttackSpeed、CriticalChance、MoveSpeed 不需要点击或打开面板就显示。若 AttackSpeed 为“—”，核对 `BP_UmbraPlayerState.InitialAbilities` 是否包含 `GA_BasicAttack`，以及其 `BaseAttackInterval` 或普通 `AttackMontages[0]` 是否有效。
2. 用已有属性调试面板向玩家施加 Add Effect（或用 GE 改变这五项），确认数值在效果生效时立即变化、Remove Effect 后回退。攻速核对 `AttackSpeed / BaseAttackInterval`；例如基础周期 1 秒、倍率 2 时应显示 `2.00`。暴击率 0.2 显示 `20%`。护甲、魔抗、移速按整数显示。调试面板既有 F1/F2 路径见 [AttributeDebugPanel](AttributeDebugPanel.md)。
3. 在双人 PIE 的客户端重复，确认复制后的属性变化能更新本地面板；拥有者 PlayerState/ASC 未就绪时显示“—”，就绪后自动刷新。切换 Pawn、重新 Possess、重复 Show/Hide HUD 和再次初始化，确认没有重复 HUD、重复更新或旧角色回调；结束 PIE 后 Output Log 不应有失效 Widget 回调。根 HUD 如在 WBP 中被移除再添加，条目会重新发现并重新绑定。
4. 记录 Editor 中 WBP 的实际名称、图标来源、PIE 数值与 Output Log。仓库中的二进制资产内容在上述操作完成之前标记为“待编辑器确认”。
