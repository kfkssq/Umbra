# 技能2：原动画与可移动引导（2026-11-13修正）

按住2持续释放，松开停止，支持WASD和点地移动。人物维持原有朝移动方向转向规则，不额外旋转角色或根骨。资源消耗仍为0；眩晕、死亡、取消和清空输入结束引导。

## 当前动画配置

- 动画使用原生 `/Game/ParagonGreystone/Characters/Heroes/Greystone/Animations/Ability_E`，不使用额外制作的旋转/跑步合成动画。
- `/Game/Skills/Greystone/AM_SwordStorm_Channel` 保留Loop段自连接，动画槽恢复UpperBody；现有动画蓝图负责移动时的腿部融合。
- 玩家Mesh恢复 `/Game/Blueprints/Player/Greystone_AnimBlueprint`。新增的ABP_GreystoneMobileStorm及A_SwordStorm_Spin保留为未使用资源，不删除用户文件。
- CharacterMovement的朝移动方向转向、转向速率均不改动。技能仅解除攻击期间的移动锁，不接管朝向。

## 规则和手工调整

`DA_SwordStorm.ChannelWhileHeld=true` 时忽略PulseCount，按住持续结算。起手延迟为StrikeTime/PlayRate，当前0.48/1.2=0.4秒；之后按动画同步周期结算当前角色周围300cm伤害（当前约1.05556秒，详见最新特效修正章节）。提前松开无伤害，松键停止计时器，动画淡出0.15秒。冷却维持启动时计算的原机制。

配置来源：C++默认→DA_SwordStorm保存值；PlayRate影响动画速度，同步开启时由动画周期、PlayRate与StormPulsesPerCycle决定伤害频率，PulseInterval仅作关闭同步的备用值。调整动画请打开Ability_E的项目副本/蒙太奇，不修改第三方源资源；不得移除蒙太奇Loop自连接，否则伤害持续但动画结束。

`Scripts/configure_mobile_sword_storm.py` 现在用于恢复原动画、UpperBody槽及原动画类，保留引导；已有AM_SwordStorm_Channel的Loop自连接为前置条件。通过匹配版本UE编辑器保存，备份位于Saved/SwordStorm/Backup，其他技能参数不修改。

## 验证

`Scripts/smoke_channel_sword_storm.py` 检查持续输入、释放、移动、控制、死亡；移动时采样Actor朝向与速度方向，稳定移动误差需小于15度，不再要求根骨整圈旋转。`-ClickStorm` 检查点地移动。历史旋转版本的10场景结果不作为本修正版验证结果。

本修正版：UE5.8.2 UmbraEditor Win64 Development构建成功（SwordStormRestoreBuild.log）；Umbra.Skills 6项通过（SwordStormRestoreAutomation.log）；9个PIE场景通过（SwordStormNativeChannelPIE.log、SwordStorm/channel-pie.json），含移动方向与角色朝向误差小于15度、移动腿部变化、8秒引导与松键/控制/死亡停止。未重跑点地场景，未测试多人网络与打包；旧点地结果属于旋转版。

## 2026-11-13 持续特效与命中火星修正

原 P_Greystone_SwordSpinning_01 的 Whirlwind/Spiral_Lines 等发射器设置EmitterLoops=1，Duration约4～5.8秒。组件存在不代表仍有粒子，因此旧的is_active检查漏掉了消失。原持续特效还含Sparks与SparksWideToNarrow，不能把整套原效果当成纯旋风长期挂在角色中心。

当前项目副本：

- `/Game/Skills/Greystone/FX/P_SwordStorm_Channel`：持续旋风，禁用所有名字含Sparks的发射器；其余发射器无限循环，基准周期与Ability_E的1.266667秒一致。
- `/Game/Skills/Greystone/FX/P_SwordStorm_HitSparks`：从原效果分离火星层，其余层禁用；每次短促发射0.08秒，粒子按自身Lifetime自然消退。技能配置缩放0.2，偏移0，出生在每个受击者位置。
- 伤害由服务器GAS原链结算，只有目标Health实际降低才广播命中火星；空转、伤害被拒绝或未扣血不播放。不会在角色中心播放原ClearAPath_Impact，挥舞音效仍每伤害节拍播放一次，不随敌人数倍增。
- ActiveEffect仍附着角色根组件，原偏移Z=-90cm、缩放1保持；CastSocket为空，人物继续由原移动规则转向。

### 同步规则

DA_SwordStorm开启SyncStormToAnimation，StormPulsesPerCycle=1。伤害间隔从蒙太奇首段循环长度 / PlayRate / StormPulsesPerCycle推导，PulseInterval成为关闭同步时的备用值。当前PlayRate=1.2，故一轮及伤害间隔约1.05556秒，约0.947次/秒；首次伤害仍在0.48/1.2=0.4秒。此前独立0.5秒伤害间隔不再生效，单次伤害未补偿，总DPS会随新频率变化。

循环伤害使用重复Timer，避免逐次重设计时器积累每帧误差。持续粒子CustomTimeDilation跟随PlayRate，原动画和粒子时间同时加速；命中粒子由伤害事件触发。这里同步的是循环和触发节拍，不代表材质内部所有自转曲线已经自动匹配剑刃角度，最终挥剑接触相位仍要人工微调StrikeTime。

### 手工调整观感（建议顺序）

1. 打开DA_SwordStorm，在PIE中站到单个敌人旁，按住2至少30秒，分别观察原地、移动、空转。每次改一个参数，结束PIE后保存，再重新测试。
2. 先调PlayRate（例如1.0～1.2）决定整套快慢；保持SyncStormToAnimation开启。StormPulsesPerCycle决定每动画周期几次伤害；改此值会影响DPS。StrikeTime是动画时间，控制首次接触相位。不要再用PulseInterval与动画各调各的。
3. 双击P_SwordStorm_Channel，使用Cascade编辑器逐列Solo发射器。Whirlwind/Spiral_Lines/Swipe影响旋风轮廓；RefracCylinder影响折射扭曲；Light影响亮光；EndSmoke/BlurryRocksB影响烟尘碎屑。过花先降低Spawn数量或关闭烟尘/折射层，再改Color/Alpha、Initial Size；不要用全局缩放同时修正亮度和密度。
4. 保持Required → Emitter Loops=0；循环Duration维持动画基准长度1.266667秒。Lifetime是单个粒子寿命，不是技能持续时间。不要只开启Cascade预览窗口的Loop按钮，那仅影响预览重播。
5. 火星打开P_SwordStorm_HitSparks，Solo Sparks系列。数量调Spawn，范围调Initial Location，飞散调Initial Velocity，拖尾长短调Lifetime，亮度调Color/Alpha或材质。DA的EffectScale只缩放命中火星，EffectOffset调受击点高度；ActiveEffectScale/Offset只调人物周围旋风，不要混用。
6. 旋风半径与判定Radius不是自动同一值。先用300cm判定边界作参照，再调ActiveEffectScale和粒子Initial Location；如果烟雾遮住腿部，优先减密度/透明度。

只编辑/Game/Skills/Greystone/FX下副本，不改Paragon源资源。重跑configure_storm_fx.py会覆盖循环/火星分层及这些基础配置，人工打磨后不要随意重跑。

参考：[Epic Required/Spawn模块](https://dev.epicgames.com/documentation/en-us/unreal-engine/default-required-and-spawn-modules?application_version=4.27)、[Cascade编辑器](https://dev.epicgames.com/documentation/unreal-engine/cascade-particle-editor-reference?application_version=4.27)。这里使用Paragon自带Cascade资源，因此对应Cascade文档而非Niagara文档。


### 本次最终验证

UE5.8.2 UmbraEditor Development构建通过；Umbra.Skills 6项回归通过。最终资源PIE 10场景通过：原地与移动分别长按30秒，各28次伤害，实测平均间隔约1.06秒；29～30秒仍有存活粒子。验证每秒时间窗持续有粒子，停止后无持续伤害/旋风；原地敌人位置有命中火星，角色位置无命中火星，空转0伤害且无命中火星。保存后核查持续副本Sparks层全部禁用、命中副本仅Sparks层启用，已查看PIE截图。

记录：Saved/SwordStorm/channel-pie.json、fx-configuration.json；Saved/SwordStormFXBuild.log、SwordStormFXAutomation.log、SwordStormFXPIE.log。未进行多人网络、打包、多人群怪压力和人工混音验收；PIE出现音频Buffer Underrun日志。粒子层持续存在/出生位置验证不等于最终观感验收。
