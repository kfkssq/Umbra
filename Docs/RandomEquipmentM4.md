# M4 公共伤害词缀与拥有者装备请求

实施日期：2026-10-30。承接 [M1–M3](RandomEquipmentMVP.md)。最终运行结果、尚未验收项目见 [Progress](Progress.md)。

## 伤害快照

公共池现在支持 Damage：A 仍是加法比例，X 仍是乘法因子；复用原 FUmbraDamageBonus 条件、面板汇总及 TypedExecution。不同 Damage 档必须具有相同桶、伤害类型集合、攻击来源、暴击/易伤条件及 Source/TargetRequirements；语义重复判断忽略候选值和类型集合的排列顺序。与固定同条件奖励重叠仍拒绝。

装备时 ApplyRolledAffixes 将固定 DamageBonuses 和实例 RolledAffixes 的伤害条目写入 `FUmbraEquipmentEffectContext`。固定曲线在装备 GE 等级求值，随机 Magnitude 直接采用已存 Value。之后 GetBonuses 读取快照，既不遍历池，也不根据当前 AffixRollConfigs 重建随机条件。旧内嵌随机 A/X 与新公共池共用这条路径；仍一件装备一个 GE handle。

Context 只为装备 Spec 显式构造，无全局 GAS Context 类型切换。Duplicate 复制条目并深拷贝 HitResult。NetSerialize 先调用父类处理来源/命中信息，再发送有界条目数据（最多64条，编码上限64KiB）。条目使用名称字符串形式序列化 Tags/TagQuery，Magnitude 已转为无曲线的标量；客户端不加载池或重新掷值。超过64条拒绝应用GE，超过编码大小上限拒绝序列化。快照为活动 GE 生命周期内不可变；卸装后再次装备会重新读取固定奖励，但随机条件和值仍来自实例。

## 请求与一致性

Controller 提供 RequestItemTransfer/SubmitItemTransfer，服务端执行器 ProcessItemTransfer 仅操作该 Controller 拥有的 PlayerState。请求只有递增 RequestId、Operation、InstanceId、装备槽/背包目标格及预期背包/装备 Revision；没有玩家目标、定义指针、等级、品质或数值字段。

- 服务端拒绝非权威、旧/重复序号、非拥有者状态、过期 Revision 和非法 GUID；之后调用原 PlayerState 事务。格子占用、满包、门槛、GE 拒绝及回滚仍由原接口处理。
- 背包增加 Revision，仅服务端修改时递增；客户端 OnRep 不递增。装备沿用现有 Revision。
- 每个本地 Controller 一次只挂起一个请求；请求/结果用 Reliable RPC。结果关联 RequestId，只有当前挂起请求可完成回调与播放结果音频。旧结果忽略，UI 不提前清空格子或假定成功。
- 结果包含服务端确认的实例用于品质音效和反馈。它不直接替换背包/装备快照；状态依然以组件复制为准。RPC结果可能先于快照到达，过早下一次操作会得到 StaleRequest，界面提示刷新重试。
- 菜单回调用弱引用保护已销毁控件；远端穿戴/卸装/拖拽移动走同一请求入口。单机仍保留原同步接口与反馈。

这不是断线装备存档或跨服务器迁移。Controller 连接生命周期结束后，挂起回调随对象销毁；新连接重新建立请求序号。没有新增客户端可执行属性或预测装备 GE。

## 验证入口与资产

`Umbra.Items.Generation.DamageSnapshot` 覆盖公共池生成、单句柄、NetBitWriter/Reader 位流往返（含TagQuery）、Context深拷贝、模板变更隔离、实际基础攻击180/技能100扣血、卸装恢复100及再次装备保持180。

`Umbra.Inventory.RequestProtocol` 覆盖服务端拥有者请求、重复序号、过期快照、未知/外部 GUID、正常卸装/移动、非法格子和所有权丢失。M0伤害测试继续覆盖 A/X 生效及失败回滚。

独立6个测试资产位于 `/Game/Items/Tests/RandomM4`：SlashA/SlashX、池、固定Rare且两条词缀的Profile、DamageSword模板及两件掉落生成器。范围 A=0.1–0.3，X=1.1–1.5，限 BasicAttack/Slashing。创建脚本 [create_random_m4.py](../Scripts/create_random_m4.py) 只创建缺失目标。

[smoke_random_m4_network.py](../Scripts/smoke_random_m4_network.py) 在 L_Prototype 临时放置新生成器；默认Listen+远端，传 `-UmbraM4Dedicated` 改为编辑器内专用服务器+客户端。请求100ms延迟、10%丢包；验证真实拥有者RPC、背包/装备复制、实际客户端活动GE Context及卸装移除。读取测试库只检查真实GE，不重建词缀。脚本恢复播放设置且不保存地图。是否实际通过以 Saved JSON及日志为准，不以脚本存在作为验证证据。

网络PIE使用 `-EnablePlugins=EditorScriptingUtilities -nullrhi -nosound`，不能代替人工视听、打包独立服务器、多进程断线重连和晚相关测试。

2026-10-30最终结果：Listen（2世界）和Dedicated（3世界）均通过，JSON为 `Saved/RandomM4NetworkListen.json` / `Saved/RandomM4NetworkDedicated.json`。Python通过Tests专用桥接暂时关闭其编辑器强制本地调用保护，使RPC实际经网络传输；这不改变生产服务端权限检查。
