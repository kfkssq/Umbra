# Greystone 技能 UI 与原生动画盘点（2026-11-12）

> 后续已实施 [原生六技能](GreystoneNativeKit.md)，取代本文的Q/E技能栏与彩色图标配置。本文动画盘点和外部属性图标建议仍作为参考。

本轮方向：先打磨现有 Greystone 动画与技能组，再制作元素学派附魔，最后扩展其他 Paragon 角色。不新增或重定向动画。本轮实现仅涉及六个技能图标、辉石魔砾停用；下述技能扩展与属性图标均为建议，未实现或导入。

## 已保存的技能栏

图标统一引用 `/Game/Asset/Icons/flat_skills_icons_color/warrior/` 现有 Texture2D；没有修改供应商图标本身。

| 键位 | 技能定义 | 图标 | 视觉含义 |
| --- | --- | --- | --- |
| Q | `/Game/Skills/Prototype/DA_Sweep` | `warrior_12` | 单剑挥斩 |
| E | `/Game/Skills/Prototype/DA_WideSweep` | `warrior_48` | 握剑大幅挥斩 |
| 1 | `/Game/Skills/Greystone/DA_SwordStorm` | `warrior_11` | 多道剑刃轨迹 |
| 2 | `/Game/Skills/Greystone/DA_LeapSlam` | `warrior_08` | 冲击裂地 |
| 3 | `/Game/Skills/Greystone/DA_Guard` | `warrior_13` | 格挡盾牌 |
| 4 | `/Game/Skills/Greystone/DA_StoneRecovery` | `warrior_26` | 人形恢复/升起 |

`BP_UmbraPlayerState.InitialSkillSlots` 仅保留以上六项；`BP_UmbraPlayerCharacter.AbilityInputActions` 移除 `IA_GlintstonePebble`；`IMC_Default` 移除该 Action 的全部映射。`WBP_SkillBar` 同步六项预览，清空其余槽位的图标、键位、费用和冷却预览。保留十格布局，后四格为空。未删除辉石魔砾资产、重定向素材或投射物 C++，未改数值、冷却开关、属性面板及地图。

配置优先级仍是技能 Definition.Icon → 运行时技能槽材质；Designer PreviewIcon 只负责编辑器预览，不覆盖运行时。显式迁移入口为 `Scripts/configure_greystone_warrior_ui.py`，先核对前六槽定义再写入，重复执行不会追加技能。历史 `configure_sweep_skills.py`、`configure_greystone_expansion.py`、`import_skill_icons_er.py` 可能恢复旧图标，`SkillImport/configure_pebble_skill.py` 会重新加入辉石；它们是旧阶段引导脚本，不应作为本阶段常规刷新入口。若刻意重跑旧引导脚本，应再执行本轮迁移并重新验证。

图标来源是项目现存 Icons 资产包；没有新增下载或更改第三方授权记录，资产作者/具体授权仍沿用 [EditorSetup](EditorSetup.md) 的来源确认规则。

## 截图中八项属性的图标建议

截图对应 `/Game/UI/HUD/StatsPanel/WBP_StatsPanel`，不是角色菜单的长属性列表。为与战士技能图标区分，建议属性使用统一单色符号、透明底、固定属性色，技能保留彩色背景。以下是外部候选，尚未下载、导入或替换。

| 属性 | 推荐资产 | 建议颜色 |
| --- | --- | --- |
| 攻击力 | [Broadsword](https://game-icons.net/1x1/lorc/broadsword.html) | 橙色 |
| 法术强度 | [Magic swirl](https://game-icons.net/1x1/lorc/magic-swirl.html) | 紫色 |
| 护甲 | [Breastplate](https://game-icons.net/1x1/lorc/breastplate.html) | 金色 |
| 魔法抗性 | [Magic shield](https://game-icons.net/1x1/lorc/magic-shield.html) | 青蓝色 |
| 攻击速度 | [Quick slash](https://game-icons.net/1x1/lorc/quick-slash.html) | 浅黄色 |
| 技能急速 | [Stopwatch](https://game-icons.net/1x1/lorc/stopwatch.html) | 浅蓝色 |
| 暴击率 | [Target arrows](https://game-icons.net/1x1/lorc/target-arrows.html) | 红色 |
| 移动速度 | [Wingfoot](https://game-icons.net/1x1/lorc/wingfoot.html) | 白色 |

以上为 Game-icons.net 的 Lorc 系列；已核对官方页面/检索，按其 CC BY 3.0 标注保留作者、来源、许可及修改说明，参见 [官方 FAQ](https://game-icons.net/faq.html)。使用白色透明底 PNG 在 UMG 着色即可形成一致风格。借鉴英雄联盟的属性语义和紧凑排布，不需要导入其游戏素材。技能急速当前显示“仅存储”的业务状态不因换图标改变。

## 原生动画实际盘点

匹配 UE 5.8.2 回读目录 `/Game/ParagonGreystone/Characters/Heroes/Greystone/Animations`，递归共 **170 个资产：154 条 AnimSequence、6 个 AnimMontage、8 个 BlendSpace、1 个 BlendSpace1D、1 个 AimOffsetBlendSpace**。同骨架与加载成功不代表所有动画可直接作为完整技能。

154 条序列按互斥类别统计：

| 类别 | 数量 | 内容与用途 |
| --- | ---: | --- |
| 攻击 | 36 | `Attack_PrimaryA/B/C`；`Attack_A/B/C/D` 的 Fast、Med、Slow、Slow_Recovery 及对应 MSA；`Attack_RMB`。其中16条是MSA叠加版本，不能重复算成16个独立攻击。 |
| 技能/终极技 | 9 | `Ability_Q/E/R/Ultimate`、`Ability_E_MSA`、`Ability_Ultimate_FrozenPose`、`Ultimate_Entry/Part1/Part2`。分段/冻结姿势不各算一个技能。 |
| 移动与跳跃 | 68 | Jog、TravelMode、启停、转身、坡度、Spin、Idle，以及跳起/空中/落地/Jump_Melee等。 |
| 瞄准偏移 | 15 | `AimOffsets/Idle_AO_*` 单帧叠加姿势，供朝向混合使用。 |
| 表情 | 8 | 两个 Taunt、SwordPlay、OooLaLa、EpicRiff及分段，适合截取部分动作或角色表现。 |
| 其他支持动作 | 18 | `Deflect_InPlace`、Cast/CardCast及MSA、HitReact四方向、Stun两段、Death、Respawn、Recall、LevelStart、RMB_Targeting及MSA、Bound。 |

关键素材与开发候选：

| 素材 | 读回时长 | 可支持的技能/动作 | 边界 |
| --- | --- | --- | --- |
| `Attack_PrimaryA/B/C` | 1.67 / 1.40 / 1.87 s | 三段基础普攻 | 需要连击衔接和输入缓存，不按三项主动技计数。 |
| `Attack_A/B/C/D_Fast` | 各0.60 s | 四方向挥砍、快攻连段 | 快/中/慢版本多为同一攻击家族。 |
| `Attack_A/B/C/D_Slow` + Recovery | 攻击各0.80 s；收招0.50–0.83 s | 蓄力重斩、破甲斩、终结斩 | 蓄力等待、释放和霸体/破甲规则需新实现；采样显示明显不同挥砍方向。 |
| `Ability_Q` | 0.93 s | 短促剑盾动作、强化斩/短范围技候选 | 最终用途需依据完整动作节奏定时，不能由字母推断原玩法。 |
| `Ability_E` | 1.27 s | 旋转斩、剑刃风暴 | 已用于当前循环风暴。 |
| `Ability_R` | 1.87 s | 跃击震地 | 已有游戏移动与落地伤害；动画自身根轨迹不是完整位移规则。 |
| `Attack_RMB` | 3.03 s | 另一种腾空攻击候选 | 采样姿态有明显升高；根轨迹为零也不代表模型不升高，不应直接当普通蓄力站姿。 |
| `Deflect_InPlace` | 1.13 s | 坚盾防御、精准格挡后反击 | 反击可接攻击序列，格挡窗口/触发规则尚未实现。 |
| `Ability_Ultimate` | 4.37 s | 石化复苏、重型腾空下砸 | 21点原始根轨迹采样最高约4106cm，必须继续用项目适配副本；不能直接播放整段原始轨迹。 |
| `Ultimate_Entry/Part1/Part2` | 0.97 / 1.67 / 2.27 s | 终极技阶段组合 | 采样有倒置、翻转、落地姿态，不能当独立普通技能直接拼接。 |
| `Jump_Melee` | 1.07 s | 空中挥砍 | 需空中施法限制/落地处理，容易与跃击用途重叠。 |
| `Cast / CardCast` | 1.13 / 0.60 s | 战意、护体、附魔启动动作 | 原生剑盾手势可复用；不是已验证的法杖/投射物专用动作。 |
| 两个 `Emote_Taunt_*` | 6.27 / 3.80 s | 嘲讽、挑衅 | 应截取短片段并重新定时，不能原长直接锁定角色；AI嘲讽规则未实现。 |
| `Emote_Master_SwordPlay_T3` | 8.33 s | 剑术展示、连段片段候选 | 优先作为表情，不能把表演动作直接认定为可用战斗连招。 |

**开发容量评估：一套3–4段普攻 + 6–8个主动技能较合理。** 当前六项全部保留；新候选优先“蓄力重斩”和“挑衅/战吼”中的机制差异。精准格挡反击宜先做坚盾防御的升级/变体；空中挥砍、另一个腾空技和现有跃击重复，应择优使用。若刻意复用动作可设计8–10项，但这不是8–10套独立动画，不建议以数量为目标。元素附魔作为既有技能变体，不另外计算基础动作数。

Q横扫、E广域横扫本轮不合并，后续打磨时应明确单体/范围/起手/资源效率差异。原生目录未发现专门翻滚序列；本轮不承诺无需动画制作即可获得完整翻滚。Death、Stun、HitReact、Recall等作为战斗状态和支持动作维护。

## 本轮验证与限制

- UE 5.8.2 编辑器 API 保存六个 Definition、两个玩家 Blueprint、输入上下文、技能栏，独立新进程回读成功。
- `Scripts/smoke_greystone_expansion.py` 渲染 PIE **11/11 通过**，覆盖六技能释放、新四技打断、跃击撞墙、扣费、冷却、图标材质和空槽；进入 Enhanced Input 绑定，不是实体键盘。证据 `Saved/GreystoneDirection/smoke.json`、`smoke.log`。
- `Scripts/preview_greystone_direction.py` 独立确认六个正确图标实际进入材质、其他槽位无授予/图标/键位、辉石Action不在玩家绑定及IMC中、槽6不能触发。报告 `preview.json` 为 passed，截图 `hud.png` 已查看。
- 全部170个动画目录资产完成元数据回读；26条关键序列各20%/50%/80%取样，共78张姿态截图，查看四张联系表及RMB完整帧。视觉预览仅在临时PIE补偿原始根平移，便于查看姿态；不代表新技能的移动/碰撞验证。完整连续播放手感、未抽样的其余序列观感仍待后续打磨。
- 审计与预览入口：`Scripts/audit_greystone_ui_direction.py`、`Scripts/preview_greystone_direction.py`；元数据证据 `Saved/GreystoneDirection/audit.json`，78帧位于 `Poses/`。Saved为本地证据，不提交。
- 本轮无C++/模块变更，未重新构建、未刷新Rider工程、未运行Session Frontend自动化；未验证多人、实体按键、音频、Cook/打包。日志仍有既有启动UnifiedError自检、Greystone AnimBP除零/旧输入、GameplayCue/Niagara警告，未将整个日志称为零错误。
- 未提交或推送；保留任务开始前的所有其他改动。全部uasset仍按仓库规则使用Git LFS。
