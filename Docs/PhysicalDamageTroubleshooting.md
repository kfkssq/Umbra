# Umbra 物理伤害未生效：原因与修复

本文保留历史故障与迁移步骤；2026-09-16 未重验资产内部，当前两个 GE 是否已符合契约待编辑器确认。当前公式参数见 [MagicalDamage](MagicalDamage.md)，整体入口见 [EditorSetup](EditorSetup.md)。

## 已确认的直接原因

Output Log 已记录：

Physical damage GE GE_Damage_PlayerBasic_C must be Instant, have no Modifiers, and exactly one UmbraPhysicalDamageExecution. Remove legacy Health/IncomingDamage modifiers.

这说明普攻已经命中并进入了新物理伤害入口，但 DamageEffectClass 指向的旧资产仍包含旧的 Health/IncomingDamage Modifier。入口为了防止双重扣血而主动拒绝该 GE，所以最终不会写入 IncomingDamage。

## 必须手动修复的两个资产

打开：

- /Game/Blueprints/Abilities/Effects/GE_Damage_PlayerBasic
- /Game/Blueprints/Abilities/Effects/GE_Damage_EnemyBasic

逐个执行：

1. Duration Policy = Instant。
2. 删除 Modifiers 数组中的所有旧项，尤其是 Health、IncomingDamage 或负 Health。
3. 在 Executions 数组添加一个 UmbraPhysicalDamageExecution。
4. 确认 Executions 中只有一个执行器，并且没有其他 Execution。
5. Compile、Save。
6. 打开 GA_BasicAttack，确认 Damage Effect Class 指向 GE_Damage_PlayerBasic。
7. 打开 GA_EnemyBasicAttack，确认 Damage Effect Class 指向 GE_Damage_EnemyBasic。

不要在这两个 GE 里手动添加 Damage Modifier。执行器会把最终值写入 IncomingDamage，AttributeSet 再清零并扣除 Health。

## 玩家攻击链路的修复

玩家基本攻击是 LocalPredicted。瞬时的 PrimaryAttackTarget 指针原来只存在客户端，服务器可能在执行攻击蒙太奇时没有目标。
现在客户端通过玩家拥有的 ASC 调用 ServerReceivePrimaryAttackIntent，服务器验证目标、距离和 IUmbraAttackable，再设置服务器目标并启动/继续权威攻击。敌人攻击本来就是 ServerOnly。

## 伤害日志

控制台执行：

umbra.Damage.Log 1

攻击命中时应看到 [Damage]，包括 Type、AD、ADCoefficient、AP、APCoefficient、Chance、CriticalMultiplier、Crit、ResistanceType、Resistance、Raw、Final。生命结算时应看到 [DamageHealth]。当前没有 Base/CanCrit 字段。

若只看到 Physical damage GE ... must be Instant，说明资产不符合当前契约。若看到 missing source/target Umbra attributes，检查双方 AttributeSet 和 ActorInfo。若完全没有 [Damage]，检查攻击能力命中窗口、Damage Effect Class 和日志开关。

## PIE 验收顺序

1. 关闭 Unreal Editor 后重新编译并重新打开项目。
2. 迁移两个 GE 并保存。
3. 确认玩家/敌人初始 GE 设置 AttackPower、CriticalChance、CriticalDamageMultiplier、Armor。
4. 执行 umbra.Damage.Log 1。
5. 玩家攻击敌人；预期顺序是命中窗口、[Damage]、[DamageHealth]，生命只减少一次。
6. 设 CriticalChance=1、CriticalDamageMultiplier=2，确认最终伤害翻倍。
7. 提高 Armor，确认伤害按 100/(100+Armor) 降低。
8. 调试面板“受到10点伤害”仍固定扣10，不经过护甲和暴击。
9. 敌人反击时重复检查敌人 GE 和 GA_EnemyBasicAttack。
10. 最后执行 umbra.Damage.Log 0。

历史修复记录：当时完成代码检查和服务器目标兜底，最终链接和真实地图 PIE 未重新验证；当前状态统一见 [Progress](Progress.md)。

当前 C++ 后备值为 AttackPower=10、AttackPowerCoefficient=1（基础伤害字段已移除），因此调试面板显示的攻击力与默认普攻 10 点一致。若初始 GE 或攻击能力蓝图显式覆盖这些值，则以蓝图配置为准。

