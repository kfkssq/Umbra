# Greystone 技能扩展

> 当前现役配置由 [原生六技能](GreystoneNativeKit.md) 覆盖：数字键1–6、破阵斩/风暴/跃击/防御/复苏/战意、无色图标。下文Q/E及旧图标为历史记录。

> 2026-11-12：技能栏已统一使用 `/Game/Asset/Icons/flat_skills_icons_color/warrior` 的六个图标，辉石魔砾从授予、输入和预览中移除，原资产保留。下文法环图标列为历史来源，本阶段当前映射、脚本覆盖边界、原生动画盘点与本轮验证见 [Greystone 方向与素材](GreystoneDirection.md)。

## 配置与操作

当前暂时关闭技能冷却：`Config/DefaultEngine.ini` 的 `[ConsoleVariables] umbra.Skills.IgnoreCooldowns=1`。控制台 `umbra.Skills.IgnoreCooldowns 0` 临时恢复，改 ini 为0可持久恢复。此开关跳过技能冷却检查和施加并隐藏冷却 UI，不改下面的设计数值，不取消资源消耗、动作锁或眩晕/死亡限制。开启前已存在的冷却 GE 不被删除，恢复后仍可读取其剩余时间；新 PIE 中默认无冷却。当前是原型调试开关，联机两端须使用一致配置。

保留 Q 横扫、E 广域横扫；第 3–6 个技能槽配置以下四种技能。键位为键盘主键区的 1–4。HUD 沿用现有 Fantasy GUI 4 模板、用户调整的宽度、头像、属性框及经验条，仅补充槽位图标、按键和消耗预览。

| 按键 | 技能 | 当前原型效果 | 消耗 | 设计冷却（当前旁路） |
| --- | --- | --- | --- | --- |
| 1 | 剑刃风暴 | 起手后每 0.5 秒命中一次，共 6 次；周围 300 cm，每次 0.45 倍武器通道伤害 | 30 | 10 s |
| 2 | 跃击震地 | 朝当前面向跃进 450 cm，轨迹高度 160 cm；实际落地后半径 300 cm、2 倍武器通道伤害 | 25 | 8 s |
| 3 | 坚盾防御 | 原地持续 2.5 秒，临时 +100 护甲、+50 斩击/钝击/穿刺抗性评级 | 20 | 9 s |
| 4 | 石化复苏 | 第二段腾空下砸，约3.33秒落地时恢复30%最大生命，半径350cm、1.25倍武器通道伤害 | 40 | 20 s |

防御数值是评级而非固定减伤百分比，继续走现有类型伤害公式与抗性上限。石化复苏是存活时主动技能，不是死亡自动复活，也不提供无敌。当前按角色已有面向释放；不新增鼠标选点或目标锁定。

正常模式下技能提交时扣费并启动冷却；当前调试模式只扣费。移动命令、眩晕、死亡及取消会中断，不退款。中断后不再产生后续伤害/回血，已造成的伤害保留。风暴原地连续播放3段 Ability_E（AM长3.8秒、PlayRate=1.2，实际约3.17秒），覆盖6次脉冲并保留收招，不再用不足1秒的 Ability_Q 起手后站立等待。防御的临时 GE 在正常结束及中断时移除。跃击使用 CharacterMovement 根运动力与碰撞扫掠；落地事件才允许震击，中断或超时不在预期终点补伤害。

## 资产与参数来源

新增资源在 `/Game/Skills/Greystone`：每项有 `DA_<SkillId>` 和 `AM_<SkillId>`。Definition 是玩法与表现参数的配置源。未给运行时装备/技能树添加新的覆盖层；伤害结算仍读取当前 GAS/武器通道和目标抗性。AttackSpeed、AbilityHaste 暂不缩放技能动画/冷却。

| ID | Greystone 原动画 | 主要原特效 | 法环本地图标 |
| --- | --- | --- | --- |
| SwordStorm | Ability_E，连续3段 | ClearAPath / P_Greystone_SwordSpinning_01 | 08303，环形挥砍 |
| LeapSlam | Ability_R | LeapAOE / P_Greystone_LeapAOE_GroundSmash | 08400，跳跃震地 |
| Guard | Deflect_InPlace | Deflect / P_Greystone_Deflect_Looping | 08361，发光盾牌 |
| StoneRecovery | Ability_Ultimate 的独立限高下砸副本 | Ultimate / P_Greystone_Ultimate_Stone、P_Greystone_HToKill_Resurrect | 08421，蓄力姿态 |

动画和粒子来源为项目现有 ParagonGreystone；未编辑原动画、骨架、原始粒子或样板 WBP。Ability_E 与其他动画并不自动代表新增完整玩法，本次只接入表中四组。蒙太奇由 UE 的 AnimMontageFactory 从源动画建立，Slot 改为当前 AnimBP 与已有横扫使用的 UpperBody；技能时点由 Definition 配置，不向源动画添加伤害 Notify。

图标导入 `/Game/UI/Icons/EldenRing/T_Skill_<SkillId>_ER<编号>`，设置 UI 纹理组、sRGB、无 mip、NeverStream。原文件位于用户本地 `C:/baidunetdiskdownload/艾尔登法环图标/艾尔登法环图标/其它`；文件 SHA256 与引用记录在 `Saved/SkillPrototype/expansion-configuration.json`。图标按画面语义选取，没有断言法环原技能名称；源版本与商用/再分发授权仍未确认，沿用项目第三方来源限制。

源 Ability_Ultimate 中段根骨高度约41米。旧 A_StoneRecovery_InPlace 全锁根版本会丢失第二段下降，现由 `A_StoneRecovery_Slam` 替代：用原始根轨迹按比例压缩到220cm，保留原始翻身和下砸时序，Force Root Lock=false、Enable Root Motion=false。升降仅为模型表现，碰撞胶囊不升空，也不提供无敌；回血/范围伤害在3.333333秒由服务端结算，原序列不变。粒子运行时自动填充其声明的 Actor 参数（例如 BoneSocketActor），再绑定施法角色并启动，避免骨骼粒子缺失来源。

关键可调字段：`Execution` 选择机制；`StrikeTime / PlayRate` 为起手时点（Guard 不使用该时点）；`PulseCount/PulseInterval` 控制风暴；`LeapDistance/LeapHeight/LeapDuration` 控制跃击；`GuardDuration/GuardArmor/GuardPhysicalResistance` 控制防御；`HealMaxHealthFraction` 控制复苏。`StrikeEffect` 是一次性效果，`ActiveEffect` 是技能拥有生命周期的附着效果，两者有独立偏移/缩放。循环效果使用可靠开始/停止多播，命中表现沿用原有一次性多播；命中和回血不依赖特效播放完成。

`BP_UmbraPlayerState.InitialSkillSlots` 管理槽位、Definition、InputTag 与显示键位。`BP_UmbraPlayerCharacter.AbilityInputActions` 与 `IMC_Default` 绑定 `IA_Skill3–6`，标签 `Input.Skill.Three–Six`。`WBP_SkillBar` 的实例 `SlotIndex=2–5` 更新 Designer 预览；运行时仍由 `ReadViewState` 获取 Definition 图标和真实冷却 GE，不在 WBP 维护第二套计时器。

`WBP_SkillSlot.Img_Cooldown` 使用 `/Game/UI/Skills/M_UI_SkillCooldownClock`，`Remaining` 接真实剩余/总时长：12点起顺时针清除深色扇区，金色边缘为时钟指针；数字保留，旧 Pb_Cooldown 隐藏。沿用旧遮罩的 Canvas 参数，不改框、间距、血条或 HUD 大小。Designer 的 PreviewCooldownFraction 可设0.75/0.5/0.25预览，运行时不会读取预览值。

## 实现与维护

`UUmbraSweepDefinition` / `UUmbraSweepAbility` 保留历史类型名以兼容已保存 Q/E 资产；默认 Execution=Sector。扩展为五种受控执行方式，没有建立技能树、投射物、自动复活或装备词缀框架。每次范围命中独立按 Actor 去重，使用圆/扇形、高度与 Visibility 遮挡检测；同一目标可在不同风暴脉冲重复受伤。

显式配置脚本：`Scripts/configure_greystone_expansion.py`。它会覆盖本次四个 DA 的表中调参，并同步输入及 UI 预览；人工调参后不要无意重跑。脚本在写入前检查 1–4 键冲突，不修改 Q/E 定义，也不重建 HUD 控件树。资产始终通过匹配版本 UE 创建/保存。

本次定向修复入口为 `Scripts/refine_greystone_skills.py`；只更新风暴/复苏动作及时点和时钟遮罩，不重置其他技能参数。原生 SetSkillMontageSequence 同时刷新蒙太奇的缓存总时长，仅改嵌套 AnimSegments 会保留旧时长而提前停止。验证脚本 `Scripts/smoke_skill_refinement.py` 覆盖默认免冷却连放、风暴末段实际骨骼运动、复苏先升后降/落地结算、临时恢复冷却后的三个指针方位。

验证入口：`Umbra.Skills.SweepSector`、`Umbra.Skills.CooldownView`、`Umbra.Skills.GreystoneExpansion`；`Scripts/smoke_greystone_expansion.py` 为仅限 PIE 的临时场景测试，注入真实 Enhanced Input Action，不等同操作系统实体按键测试。当前轮次的结果与限制见 [Progress](Progress.md)。

人工复验：进入 L_Prototype，站在敌人附近分别按 1–4；观察每项技能动画、特效与费用，当前默认无CD，验收冷却数字/指针前先执行 `umbra.Skills.IgnoreCooldowns 0`，完成后设回1。跃击朝空地/墙体各释放一次；风暴期间移动应停止后续脉冲；防御中移动应立即还原属性面板；受伤后按 4 并等待蓄力完成应回血，提前移动则不回血。修改视觉时优先调整新 DA 的效果偏移/缩放及新 AM，不改原包素材。
