# 玩家生命归零与死亡表现

## 规则与配置

`AUmbraPlayerCharacter` 绑定 PlayerState ASC 的 Health 变化。权威端 Health <= 0 时进入不可逆的 `bIsDead` 状态，复制给客户端；不在 UI 计算伤害。关闭移动和碰撞、清空连击目标与技能输入，添加 `State.Dead` 并取消能力。敌人 AI 放弃死亡玩家，攻击命中窗口也检查死亡标签。旧的 FinishSpawnAnimation 回调不会恢复移动。保留玩家 Pawn 和相机，不实现复活、重生或死亡菜单；治疗不能解除死亡状态。

- `BP_UmbraPlayerCharacter → Animation|Death → Death Animation`：配置完整身体的 AnimSequence，来源为蓝图类默认值，C++ 默认空。不循环播放并停留末帧，不依赖 AnimBP Slot；死亡后切换单节点播放，因此不再运行原来的移动动画图。玩家尸体持续保留。
- 配置脚本 `Scripts/configure_player_death.py` 仅在匹配 UE 5.8 编辑器中运行：验证原生父类和骨架相同，再引用现有 Greystone `Animations/Death`；保存前备份蓝图和音频库到 `Saved/PlayerDeathBackup`。不修改第三方动画或音源。
- `AudioEvents.CharacterType=Player.Greystone`；Death 事件使用 `DA_Audio_Default` 的对应角色行，声音为 `Greystone_Effort_Death`，空间播放、概率1、Pitch=1、冷却0秒。组件 LibraryOverride 的匹配行优先，之后项目默认库，最后静默；覆盖库空匹配行仍可明确静音。
- 死亡音经现有 AudioEvents 权威入口发送一次，和 IncomingDamage 致死音频共享组件去重；直接通过 GAS 把 Health 改为0也触发。OnRep 只恢复死亡表现，不补播旧尸体音效。网络瞬时音效仍允许丢包。
- `OnDeathStarted` 是一次性的 Blueprint 表现扩展事件，不承担死亡判定。不要再次播放同一死亡音或恢复移动。

## 验证

`Umbra.Combat.PlayerDeath`：真实玩家蓝图实例 + PlayerState ASC，GAS 致死伤害、死亡标签、移动/碰撞禁用、出生回调保护、攻击拒绝、重复生命过零去重和销毁时解绑。

PIE 最小步骤：在 L_Prototype 让敌人将玩家生命打至0；检查倒地、死亡声一次、无法移动/攻击、敌人停止攻击；等动画结束确认不回站立。另一轮通过 GAS 直接把 Health 改为0，确认同样触发。检查 Output Log；多人复制和有声试听须单独执行，自动化或无声 PIE 不等于听感验证。

当前执行结果见 [Progress](Progress.md)。资产配置脚本尚未成功运行前，蓝图引用与最终表现均标为“待编辑器确认”。
