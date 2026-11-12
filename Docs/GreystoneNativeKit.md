# Greystone 原生六技能（2026-11-12）

> 当前动画迭代阶段暂不接入资源消耗：六项Definition.ResourceCost及UI预览费用均设为0，零资源可释放；下表费用保留为历史设计值。定向入口为Scripts/disable_greystone_resource_costs.py；主配置脚本也统一覆盖为0，避免重跑恢复费用。UI当前显示费用0，不删除资源条。验证脚本加-NoCostSmoke在零资源下逐一释放六技。

当前配置取代 [旧图标迁移](GreystoneDirection.md) 和 Q/E 横扫技能栏。原型/辉石素材均保留，不再授予；不导入、不重定向、不改 Paragon 原包。核心规则仍由 C++ 和 GAS 执行。

| 主键区键位 | 技能 | 主动画 | 无色图标 | 当前规则 |
| --- | --- | --- | --- | --- |
| 1 | 破阵斩 | Ability_Q | warrior_12 | 0.4s结算；250cm/120°扇形；继承旧横扫武器通道伤害1.5倍；消耗15，CD4s |
| 2 | 剑刃风暴 | Ability_E 三遍 | warrior_11 | 1.2倍播放；六次0.5s间隔脉冲，300cm；消耗30，CD10s |
| 3 | 跃击震地 | Ability_R | warrior_08 | CharacterMovement跃进，真实落地300cm范围伤害；消耗25，CD8s |
| 4 | 坚盾防御 | Deflect_InPlace 分段 | warrior_13 | 2.5s临时护甲+100、物理抗性评级+50；消耗20，CD9s |
| 5 | 石化复苏 | A_StoneRecovery_Slam | warrior_26 | 约3.33s恢复30%最大生命并造成350cm范围伤害；消耗40，CD20s；没有复活或无敌 |
| 6 | 战意 | Cast | warrior_23 | 0.55s施法成功后，6s暴击率+0.2（20个百分点）；消耗20，CD12s |

图标统一来自 `/Game/Asset/Icons/flat_skills_icons_no_color/warrior`。未修改原图标。十格HUD保持原布局，后四格为空。`IA_Skill1–6` 对应 One–Six 和 `Input.Skill.One–Six`；移除这些Action的Q/E旧映射及辉石Action绑定/映射，其他操作不变。

## 战意规则

`UUmbraSkillFocusEffect` 使用既有 `CriticalChance` 属性；普通攻击和允许暴击的技能仍通过既有伤害链读取。不增加独立伤害公式，不实现元素学派或装备附魔。结果受现有0–1上限约束，角色已经100%暴击时不会继续提升概率，不修改角色原属性来掩盖这一点。

成功前移动/死亡/眩晕取消，不产生增益；成功后恢复移动不撤销已经生效的增益。再次成功施放移除同源旧效果并重新应用，刷新6秒而不叠加。到期自然还原；玩家死亡路径主动移除，即使施法能力早已结束也不会残留到复活。施法扣费一次，中断不退款。属性面板观察真实GAS数值。

FocusDuration、FocusCriticalChance由 `/Game/Skills/Greystone/DA_BattleFocus` 提供；类默认6s/.2，资产覆盖默认，没有装备/学派覆盖层。SkillId为BattleFocus、独立冷却标签Cooldown.Skill.BattleFocus。破阵斩独立定义为DA_BreachSlash，沿用旧Sweep冷却标签；旧Sweep未授予，不共享两个现役技能的冷却。

## 动画分工与“不重不漏”

- Ability_Q→破阵斩；Ability_E→风暴，E_MSA仅作叠加备用，不作为另一个技能。
- Ability_R→跃击；Attack_RMB/RMB_Targeting及MSA登记为另一套腾空/瞄准候选，不重复装备第二个跃击。
- Deflect_InPlace→防御。蒙太奇按源0–0.35s正常举盾、0.35–0.38s延长保持、0.38s至结尾正常收盾，总长2.5s；源动画不改，PlayRate=1。保持段为连续姿势片段，仍需后续人工连续手感精修。
- Ability_Ultimate→项目已有限高副本，继续完整保留当前复苏。Ultimate_Entry/Part1/Part2与FrozenPose登记为分段/姿态备选，不再把完整动作和它的分段重复串联。
- Cast→战意；CardCast及两者MSA登记为后续附魔启动变体，当前不为凑数量重复释放两个施法动作。
- Attack_PrimaryA/B/C及Attack_A–D各速度版本→普攻专用；现役六技能不引用这些动画。
- Jump_Melee及MSA→空中普攻候选；Emote_Taunt、SwordPlay等→表情候选，不冒充已实现技能。

全部技能相关动作组均有用途或备用状态登记；“不漏”不表示强迫每个MSA、分段、冻结姿势独占技能槽。未实现备用动作的玩法。

## 配置与验证入口

唯一当前迁移入口 `Scripts/configure_greystone_native_kit.py`：先检查数字键冲突，UE创建/保存新DA、AM并更新现役槽位。它会覆盖表中定义与图标/输入，手工调参后不要无意重跑。旧configure_sweep_skills、configure_greystone_expansion、configure_greystone_warrior_ui及辉石引导脚本不是当前配置入口，重跑会恢复旧阶段内容。

`Scripts/smoke_greystone_native_kit.py`：六技完整释放、六技起手取消、跃击撞墙、战意刷新、战意成功后死亡，共15个渲染PIE场景；另核对One–Six映射、图标材质、空槽、资源消耗、风暴六脉冲、护甲恢复、治疗和战意到期。输入注入走Enhanced Input绑定，不代表实体键盘测试。

当前 `umbra.Skills.IgnoreCooldowns=1` 仍用于迭代；资产CD数值保留。`Umbra.Skills`自动化继续验证几何、实际冷却视图和既有四技能定义。没有模块/Target变化，不要求刷新Rider工程。

音效及既有五技特效沿用已有配置；本轮未承诺完成逐技能声音制作与试听。战意当前以原生施法动作和属性变化反馈，不借用复活/爆炸表现冒充元素附魔。持续增益图标/独立战意特效属于后续表现打磨。

本轮实际验证结果见 [Progress](Progress.md)，本地证据位于 `Saved/GreystoneDirection/native-*`。不提交Saved。

## 原版视频对照

- [Paragon官方频道：Greystone Overview（2016-07-12）](https://www.youtube.com/watch?v=_WdrwhcOZg8)：官方搜索结果确认标题、频道及发布日期，适合对照技能运行。
- [PlayStation发布的原版Overview](https://www.youtube.com/watch?v=kh8rRk8T1w8)：同一时期宣传入口。
- [Epic创意总监发布的Greystone介绍，含原版预告视频](https://blog.playstation.com/2016/07/07/paragon-new-hero-greystone-joins-the-fight-july-12/)：原版Assault the Gates为接敌、Make Way为周身范围伤害、Stoic为被动抗性、Reforged为死亡复生。

建议对照风暴的身体动作与环绕剑刃是否需要同时持续、跃击起跳/落地节奏、复苏石像与灵体的分离、声音层次及特效尺度。Umbra破阵斩、主动坚盾与战意均不能按原版独立技能逐槽对应。此处提供可核验视频来源，不声明本轮已逐帧观看或听音，也不提供未经核实的技能时间戳。

## 2026-11-13 1技能改为斩切波

旧破阵斩已被[斩切波](SlashWave.md)替代：用户明确指定慢速第三段攻击动画Attack_C_Slow，作为早期“不使用普攻动画”要求的本技能例外；当前1技能按新版文档，以上六技能首版记录为历史。内部DA_BreachSlash/SkillId仅为兼容引用保留。

## 2026-11-13 技能2改为可移动旋风引导

当前行为与配置以 [SwordStormChannel](SwordStormChannel.md) 为准：按住2持续释放，松开结束；可移动，整身旋转带迈步动画，持续脉冲跟随角色位置。上方固定六次的技能2记录为历史。

技能2后续纠正：恢复原Ability_E和原动画蓝图，人物仍朝移动方向转向；只保留长按持续释放、松开停止及移动施法。当前说明见 [SwordStormChannel](SwordStormChannel.md)。
