# 跃击瞄准原型

## 使用边界

本次在空的第8技能框添加独立“跃击瞄准测试”，绑定数字8；保留已有技能配置与按键。未给已有跃击或斩切波增加瞄准。不购买素材，使用新建参数化贴花与已有 Greystone Ability_R 动画建立独立蒙太奇。原型DA始终要求 ResourceCost=0。

操作：8进入瞄准，再按8取消；鼠标选落点；左键确认；右键/Esc取消。确认点击及其释放被消费，不下发移动/普攻。瞄准时停止自动移动，手动移动被拦截。按其他技能键取消预览后保留该技能原行为；菜单、死亡、眩晕、换角色清理指示器。瞄准不激活技能，不产生冷却、动画或声音。

## 参数与编辑入口

`/Game/Skills/TargetingPrototype/DA_TargetLeap` 是独立数据源：

- bTargetedLeap=true，仅此DA启用；现有资产字段默认false。
- LeapDistance=600cm：大圈和距离校验共同使用。鼠标超距时夹到边界。
- Radius=220cm：落点小圈和实际360度范围伤害共同使用；保持既有目标中心判定与HeightTolerance，不扩大敌人碰撞体。
- TargetMaxHeightDifference=100cm：当前地面与目标地面的最大高差。
- CV_TargetLeap：归一化0..1时间，Z方向最高220cm；48段折线。预览胶囊扫掠与RootMotionMoveToForce读取同一曲线。LeapHeight不控制此路径，修改轨迹应编辑此CurveVector；LeapDuration=.75s控制实际运动时间。
- AM_TargetLeap：独立引用Ability_R的蒙太奇，StrikeTime=.15s后起跳；动画源不修改。
- M_TargetCircle：圆边与填充由UV计算，Tint/LineWidth/Fill/Invalid参数；蓝色大圈、绿色有效落点、红色叉号无效落点。实际发生落地判定时短暂显示同半径金色圈。
- ResourceCost=0，不改动已有技能的消耗。测试技能保留独立1秒冷却字段；现有 IgnoreCooldowns 调试开关仍生效。

覆盖顺序：DA保存值覆盖C++默认；原型配置脚本写入上述初值。人工调整后不要重复执行配置脚本覆盖调参。材质只表达范围，不负责游戏判定。

## 代码职责

- `UUmbraSkillTargetingComponent`：本地选择、输入消费、指示器、确认请求与反馈。`UUmbraTargetingPrompt`：显示操作说明和无效原因。
- `AUmbraPlayerController::InputKey`：在普通鼠标动作进入Enhanced Input前消费瞄准按键，并消费同一次按下的释放，防止确认后接着移动。UI命中路径不允许确认；取消逻辑独立。
- `AUmbraPlayerCharacter::AbilityInputTagPressed`：只拦截已授予且明确启用目标选择的技能；原有技能照旧。
- ASC确认：根据服务器已授予的Spec读取Definition，重新校验目标；目标以SpecHandle暂存，只供这一次同步激活消费，激活成功/失败均清除。
- `UUmbraSweepAbility`：只有bTargetedLeap分支消费确认落点。起跳前再查一次障碍；RootMotionMoveToForce经CharacterMovement碰撞移动，结束后转Falling，等待真正Landed事件再调用现有GAS伤害链。不是Teleport。

## 合法性和范围一致性

1. 地面选择射线查询WorldStatic/WorldDynamic，忽略角色自身，不把Pawn当落地表面。
2. 距离检查、地面高度/可行走坡度检查、落点完整胶囊阻挡检查、整段弧线48次胶囊扫掠。
3. 本地预览可以夹紧距离；服务器确认不接受超距目标，重新命中的地面位置偏差超过5cm拒绝，防止过期地面。
4. 移动中仍执行角色碰撞。被动态障碍阻断无法到达时取消，不在旧目标点制造伤害。实际落地与选择点水平偏差超过5cm时取消判定。5cm是运动误差容限，不是伤害半径补偿；金色圈位于实际落地处。
5. 半径边界使用敌人中心，已有高度容差和视线遮挡仍适用；小圈表示几何覆盖区域，不保证被墙隔开的目标受伤。

空中受打断会清理任务并恢复下落，避免遗留Flying。真实落地前不发生技能伤害。高差/斜坡的可落性采用保守胶囊检查，某些看似能站的边缘可能显示无效；不自动寻找远处替代落点。

## 配置与验证

`Scripts/configure_targeting_prototype.py` 必须通过匹配版本UE执行。只保存新原型资产、IA_Skill8、IMC追加映射、玩家输入绑定、PlayerState新增槽和技能栏第8槽预览。修改已有资产前备份至Saved/SkillTargeting/Backup。第8槽或Eight键已被其他内容占用时脚本停止，不覆盖。

`Scripts/smoke_targeting_prototype.py` 为不保存关卡的PIE验证，检查真实输入路径、鼠标投影、取消无移动、超距夹紧、落地位置、范围内外伤害、零消耗、路径阻挡及死亡清理。截图与报告写入Saved/SkillTargeting。

2026-11-13验证：UE5.8.2 UmbraEditor Win64 Development构建成功；Rider项目文件刷新成功；Umbra.Skills自动化6/6通过。最终带渲染PIE报告`Saved/SkillTargeting/pie.json`的33项断言通过，包含实际鼠标投影、左右键按下/释放消费、取消无位移、600cm夹紧、实际空中轨迹和落地、220cm边界内外伤害、障碍拒绝、死亡清理及Resource=0。输入由测试脚本注入引擎路径，未做物理键鼠人工手感验收。

已查看最终`Saved/SkillTargeting/targeting-preview.png`确认两圈和技能栏上方提示可见。指示器由独立本地可见Actor承载，避免PlayerController默认隐藏导致Decal不显示。新进程资产回读通过，原有六技能均未启用目标选择。日志仍有启动阶段`Condition failed`输出，不声称全日志无错误。

多人、打包、大规模性能、各种复杂台阶与移动平台尚未验证。首轮仅实现跃击圆形选点；本次新增扇形/条形见下节，原有技能接入仍留待后续。

## 2026-11-13 扇形与条形独立原型

新增第9槽（9键）和第10槽（0键），第8槽跃击保留。两个新槽均按键进入瞄准、鼠标选方向、左键确认、右键/Esc取消；再次按对应技能键取消。原有技能定义未开启bTargetedDirection。

| 资产 | 形状与默认参数 | 执行 |
| --- | --- | --- |
| `/Game/Skills/TargetingPrototype/DA_TargetSector` | Radius=300cm，ArcDegrees=120° | 确认后0.25秒出手，扇形内目标各结算一次 |
| `/Game/Skills/TargetingPrototype/DA_TargetWave` | TargetLineLength=800cm，TargetLineHalfWidth=100cm（全宽200cm） | 确认后0.25秒发波，TargetWaveSpeed=1000cm/s，从近到远推进，每目标最多一次 |

两个定义ResourceCost=0，独立CooldownTag，CooldownSeconds=1；现有IgnoreCooldowns开关仍有效。复用现有技能动画的独立蒙太奇副本，不修改原技能资产或原蒙太奇。当前表现仅用于判定测试，未新增音效。参数配置优先级仍为DA保存值覆盖C++默认；`configure_directional_targeting.py`是初始配置脚本，人工调参后不要重复运行覆盖。

`M_TargetDirection`直接用世界XY、ShapeOrigin与ShapeForward计算扇形/矩形，避免贴花UV朝向与角色方向不一致。预览与实际效果从同一个DA读取Radius/ArcDegrees/TargetLineLength/TargetLineHalfWidth；材质不决定伤害。绿色区域表示几何覆盖，鼠标距角色不足10cm时不能确定方向，显示无效；波覆盖角色前方0..Length，左右HalfWidth，不增加敌人胶囊半径。

服务端再次验证选择并锁定施法起点/方向。前摇中若被水平推移超过5cm取消；确认后移动鼠标不会改变方向。`AUmbraDirectionalSkillEffect`由确认后的释放事件创建，扇形一次结算，波使用30cm前沿带随时间推进、帧间扫过区间补齐，保持目标中心判定并去重，经既有GAS伤害链结算。波发出后独立完成，不随施法者后续移动转弯。效果结束销毁贴花Actor。

判定约定与既有近战一致：HeightTolerance限制高度，WorldStatic/WorldDynamic世界几何射线阻挡的目标不受伤；其他敌人不截断穿透波。地面图形表示无障碍时的几何覆盖，不绘制墙后阴影；不会对墙后目标制造穿墙伤害。此原型是可穿透多个敌人的直线波，不采用旧投射物碰撞盒/自动追踪，避免敌人胶囊扩大预览范围。

`Scripts/smoke_directional_targeting.py`在新进程带渲染PIE检查保存配置、鼠标投影、按键消费、旋转方向、角度/半径/宽度/长度内外边界、背后目标、取消、推进时序、单目标去重、墙后遮挡、零消耗与效果清理。报告和截图写入Saved/DirectionalTargeting。建议以-RenderOffscreen运行，避免编辑器弹窗和桌面鼠标干扰；脚本在瞄准阶段逐帧定位测试光标，仍经过真实InputKey按下/释放入口。

本轮最终验证：UE5.8.2 UmbraEditor Win64 Development构建成功（Saved/DirectionalBuild.log）；Umbra.Skills 6/6成功（DirectionalAutomation.log）；独立新进程离屏渲染PIE 17场景、263项断言全部通过（DirectionalTargeting/pie.json）。包括鼠标方向与世界方向一致、确认后移动鼠标不改变出手方向、取消无伤害/位移、59°/61°角边界、299/301cm半径边界、99/101cm半宽边界、799/801cm长度边界、7°旋转远端角落、背后目标排除、墙后遮挡、穿透两目标、每目标至多一次、零资源及Actor清理。6米处首次命中发生在确认后约0.84秒（0.25秒前摇+0.6秒传播，帧时序容差）。

已查看最终扇形/条形预览及波推进截图；新进程检查前8槽引用保留、现有六技能未开启方向瞄准。日志仍有既有启动Condition failed、原版蓝图输入轴/动画和Niagara元数据警告，不声称全日志无警告。本轮未验证多人网络、打包、复杂地形、物理键鼠手感及大规模性能；未重复执行跃击全套PIE。
