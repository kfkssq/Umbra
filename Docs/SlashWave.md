# 斩切波（1技能）

2026-11-13 用户指定将1技能改成斩切波，并允许该技能使用慢速第三段普攻动画。它取代旧破阵斩，2–6技能不变。

## 资产与参数

- 配置仍为 `/Game/Skills/Greystone/DA_BreachSlash`，内部 SkillId 保留 BreachSlash，避免破坏已有技能栏/输入引用；显示名为斩切波。
- 新蒙太奇 `/Game/Skills/Greystone/AM_SlashWave` 引用 `Attack_C_Slow`，编辑器确认0.8秒、Notify为空；使用1.0播放速率，不修改供应商动画。
- 出手时刻 `StrikeTime=0.38s`（实际时间除以PlayRate）；从近身扇形伤害改为投射物，伤害沿现有GAS链结算。沿角色施放朝向直线飞行，转向率0同时禁用初始自动瞄准。
- 速度1000cm/s，寿命0.9s，从发射点飞行约900cm。ProjectileRadius=45cm、ProjectileHalfWidth=135cm：前向盒体半尺寸(45,135,45)，逐子步扫掠；这是对可见弧形的宽体近似，并非逐像素碰撞。
- 单次命中敌人或世界阻挡后消散，不穿透；出手前取消不发射，发射后取消收招不撤回已飞出的冲击波。角色至发射点也检查同样宽度的障碍。
- 原有伤害配置与冷却保留，资源消耗0，按键1不变。旧近身挥击特效清空，避免出现两次攻击反馈。
- 弧形外观使用 Engine Plane + 项目新建 `/Game/Skills/Greystone/M_SlashWave`：双面、Unlit、Additive，程序化弧形遮罩；WaveColor调整颜色，ProjectileMeshScale调整尺寸。没有引入外部素材。
- 发射声继续使用 swordswing_slow，起手沿用已配置的发力语音；真实命中仍走SkillHit。落点使用现有Greystone冲击粒子，不重复播放额外ImpactSound。

## 调整与维护

C++只提供可复用投射物表现/宽体扫掠，具体配置在DA，材质外观在M_SlashWave。球形投射物保持HalfWidth=0。配置脚本 `Scripts/configure_slash_wave.py` 在UE保存并备份修改前资源到Saved/SlashWave/Backup；脚本会覆盖1技能上述字段，人工打磨后不要随意重跑。旧native-kit整组迁移增加保护，遇到斩切波时拒绝恢复旧版1技能。

验证记录见 Progress；检查脚本 `Scripts/smoke_slash_wave.py`。需要人工最终确认挥剑到发射的手感、亮度与音效混音。

本轮验证：Development编译成功；Umbra.Skills 6/6；新进程PIE正面、侧缘、范围外、墙体、取消、直线6/6。已查看渲染截图确认外观。未验证多人、打包、人工听感与逐帧动作手感。

## 2026-11-13 前摇与挥剑速度接口（当前配置）

打开 `/Game/Skills/Greystone/DA_BreachSlash`（显示名斩切波），停止PIE后在Details修改并保存，重新PIE即可，不需要重新编译C++。

| 编辑器字段 | 当前值 | 调整效果 |
| --- | --- | --- |
| Timing → Windup → Windup Duration | 0.65秒 | 抬剑蓄力实际持续时间，增大则前摇更长 |
| Timing → Windup → Windup End Time | 0.25秒 | 原动画0～0.25秒作为准备动作；通常先保持不变 |
| Play Rate | 0.65 | 前摇结束后挥剑/收招的倍率；越小越慢，不再影响前摇时长 |
| Strike Time | 0.38秒 | 原动画中释放冲击波的帧位置；不是按键后实际秒数 |

当前：原动画0～0.25秒在0.65秒内播放（约0.385倍速）；后续动作0.65倍速。实际发波时刻=`WindupDuration + (StrikeTime - WindupEndTime) / PlayRate`，即约0.85秒，完整0.8秒原动画约1.50秒。发波音效随实际发射事件，不会仍在旧0.38秒播放。

建议先只改Windup Duration（0.5～0.8秒）和Play Rate（0.55～0.85）。需要重新选择蓄力动作截止帧时再改Windup End Time；必须满足 `0 < WindupEndTime < StrikeTime < 蒙太奇长度`。Windup Duration上限3秒；关闭分段前摇时将两个Windup字段都设为0，恢复StrikeTime/PlayRate旧规则。非法配置会拒绝激活。

Blueprint可调用Get Release Delay读取计算后的实际发波延迟。前摇通过降低动画前段速度实现，不是先站着等待；前摇计时器在取消/死亡时清理。该功能仅允许投射物技能启用，其余技能默认两个字段为0。

实现：UUmbraSweepDefinition保存参数并计算时序；UUmbraSweepAbility以准备段速度启动蒙太奇，前摇结束后通过ASC切换播放速度。参数来自DA（覆盖源码零前摇默认），无额外ini或蓝图覆盖。仅修改1技能的四个时序值，保存脚本Scripts/configure_slash_wave_windup.py。重新运行配置脚本会覆盖手动打磨参数。

本次前摇验证：Development构建成功、技能自动化回归通过；渲染PIE 7/7，首次发波0.86～0.87秒，前摇源动画进度约0.11秒，前摇内两种时机取消均无投射物。已查看前摇截图；多人/打包/主观手感未验证。

## 当前发波帧：挥剑中段（2026-11-13校正）

逐帧预览Attack_C_Slow的0～0.25秒后发现，旧StrikeTime=0.38已经接近收招；前摇区间截止0.25也包含了主要挥剑动作。以剑向前挥过的0.175秒姿势为当前发波点，而不是用完整0.8秒片段的时间中点。

当前DA_BreachSlash：StrikeTime=0.175，WindupEndTime=0.10，WindupDuration=0.65，PlayRate=0.65。覆盖上文首版时序数值。真实发波延迟约0.765秒；完整动作约1.73秒。前摇真实时长和后段速度保留，仅重新划定源动画准备段与发射帧。

继续人工微调发波点时调整Strike Time（源动画秒）；Windup End Time保持0.10，且必须小于Strike Time。发射声和波同一事件触发。保存脚本Scripts/align_slash_wave_release.py仅改两个帧位置，旧DA备份Saved/SlashWave/DA_BreachSlash-before-mid-swing-*.uasset；两个整套配置脚本同步此默认值，避免重跑恢复旧帧。

源姿势证据：Saved/SlashWave/motion-review.json与pose-0～8.png，0.175秒对应pose-5.png。预览脚本Scripts/preview_slash_wave_release.py，不保存关卡或供应商动画。

本次验证：7项行为PIE与独立发射姿势同步用例均通过；首次出现投射物时蒙太奇位置与0.175秒目标误差小于0.04秒。未改C++，未重复构建；多人和打包未验证。
