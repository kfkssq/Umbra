# Umbra 音频审计（2026-10-24，源码实现前基线）

## 证据与范围

工作区开始时 `git status --short` 为空。实际读取 Source/Umbra、Config、Umbra.uproject 与现有维护文档；使用 LauncherInstalled.dat 定位 UE 5.8.2，通过只读 Python commandlet 查询 AssetRegistry。没有修改或保存 Content 资产。完整资产清单见 [AudioAssetInventory.md](AudioAssetInventory.md)。脚本为 `Scripts/audit_audio.py`，原始输出为本地 `Saved/AudioAssetAudit.json`。

扫描实际完成并输出 AUDIO_AUDIT_COMPLETE；commandlet 因本机 DDC 不可写记录 Error 而返回 1，使用 -DDC-ForceMemoryCache 后完成扫描。此结果不是编译或 PIE 通过。AssetRegistry 的父类和依赖是实测数据，但不等于读取完整 Blueprint Graph、按钮样式或 Notify 时间轴；这些内容标记为 **待编辑器确认**。

## 现有系统与触发点

| 系统 | 实际源码与行为 | 音效策略 / 尚缺内容 |
| --- | --- | --- |
| 玩家 | Characters/UmbraPlayerCharacter；Player/UmbraPlayerState 持有 ASC、Equipment、Inventory | 角色配置组件；玩家没有完整原生 Die 流程，伤害致零可作一次性死亡表现入口，不补建死亡玩法 |
| 敌人 | Characters/UmbraEnemyCharacter::HandleHealthChanged → Die → SpawnDeathLoot / ApplyDeathState；bIsDead 与 bDeathStateApplied 去重 | 权威 Die 一次性通知死亡，避免 OnRep 再响及晚相关尸体补响 |
| 玩家普攻 | AbilitySystem/Abilities/UmbraBasicAttackAbility::PlayCurrentAttackMontage / ResolveLogicalStrike | 逻辑计时器决定伤害；动画 Notify 只负责挥动/发力声，不改 AttackSpeed、前摇、周期或蒙太奇速率 |
| 敌人普攻 | UmbraEnemyBasicAttackAbility::HandleHitWindow 消费 HitWindowTick，距离检查后 Apply，bDamageApplied 限一次 | 该标记含被拒绝效果，不能据此播放命中成功声 |
| 命中/暴击 | UmbraDamage::Apply 统一提交；UmbraAttributeSet::PostGameplayEffectExecute 处理 IncomingDamage；Damage_ResultCritical 已由结算写入 | 在正数伤害结算点捕获音频表现，读取已有暴击结果；不复制公式，不在挥动 Notify 播命中 |
| 现有伤害通知 | FUmbraDamageNotification 仅向打中敌人的攻击者回传伤害数字 | 不把此路径冒充所有角色受伤广播；音频独立使用受击 Actor 的轻量组件 |
| HitReact | UmbraHitReactAbility 响应 HitReceived、播放敌人 HitReact Montage，受 SuperArmor/Dead 等标签限制 | 受伤呻吟以伤害结算为准，避免动画受阻后漏播或双播 |
| 移动 | UmbraPlayerCharacter Move / MoveTowardWorldLocation 与 CharacterMovement；无脚步音频代码 | 新音频 Notify 在左右脚接触帧手动添加；不增加脚步 Tick |
| UI Tab | UUmbraMenuTabButton 绑定 Button_0 OnClicked；AttributeMenu::SetActiveTab 校验索引后切页 | 内部按钮独占 Hover/Click；切页只在用户点击造成实际变化时响，初始化不响 |
| 菜单 | UmbraPlayerController::ToggleCharacterMenu 创建/显示成功后设置打开状态 | 打开/关闭各一次，EndPlay 清理不播放关闭声 |
| 槽位 | InventoryMenu::HandleSelection；EquipmentMenu::HandleSelection；两个 Slot 已有 Hover 进出事件 | 选择改变后播；不从快照刷新/SetSelected 播，避免初始化与重复双击声音 |
| Tooltip | InventoryTooltip / EquipmentTooltip 校验快照并缓存 ItemTooltip，可能多次刷新 | 显示入口需核对实际呈现状态；默认空配置，不以每次数据刷新当作显示 |
| 装备转移 | PlayerState.EquipFromInventory / UnequipToInventory 负责原子转移与回滚；InventoryMenu::FinishEquip、EquipmentMenu::RequestUnequipSlot 获得最终结果 | 基于最终结果播放；AppliedInvalid 表示转移完成但属性无效，使用失败/警告反馈。现有 UI 不具备装备 RPC，不为音频重构网络玩法 |
| 背包 | InventoryComponent 发布 OwnerOnly 快照；Add/Remove 也参与失败回滚 | 不订阅每次快照当转移音；仅已完成的装备↔背包操作通知 |
| 拾取 | Controller ServerPickupWorldItem → WorldItem.TryPickup → State.AcceptLootInstance → ClientPickupResult → ReportPickupResult | 已有可靠结果回传；补携带物品音频上下文，成功在结果收到后播放，重复请求的 Unavailable 不能响成功 |
| 掉落 | WorldItem.InitializeInstance 与 BeginPlay；不可变实例 Definition 带稀有度 | 服务器生成事件与稀有度提示分开，客户端不从每次 RefreshPresentation 推断生成 |
| 实际落地 | WorldItemPhysics 当前仅模拟/支撑查询/收敛，未绑定 OnComponentHit；客户端只插值服务器姿态 | 新增服务器实际碰撞回调，只首个合格静态地面接触，非 FreezeDrop/超时恢复假落地 |
| 格挡/闪避/技能 | 未找到完整可用格挡、闪避玩法或专门技能施放/命中 Ability | 只预留音频事件；资源名中的 Block/Q/Ultimate 不是已经存在的玩法 |

## 资产核查

AssetRegistry 在 /Game 下找到 **341 SoundWave、145 SoundCue**；本次所列音频类型中没有 MetaSoundSource、MetaSoundPatch、SoundClass、SoundMix、SoundAttenuation、SoundConcurrency、SoundSubmix 资产。引擎/插件内置资源不计入 /Game。

- `/Game/Asset/fantasy_gui_4/sounds/`：4 Wave、5 Cue，按钮 Hover/Click 候选。
- `/Game/ParagonGreystone/Audio/`：Greystone Effort_Attack、Effort_Swing、Effort_Pain、Effort_Death 等语音候选。已有资产不代表适合其他敌人，也不代表许可已经完成复核；需试听与来源记录。
- 武器挥动、物理撞击、脚步、装备、拾取、稀有度与环境声不能仅凭文件名确认满足需求，逐项见 [素材需求](AudioAssetRequirements.md)。不导入或伪造占位 SoundWave。
- AssetRegistry 确认 GA_BasicAttack 父类为 UmbraBasicAttackAbility，引用 Attack_PrimaryA/B/C_Montage；另存在 Attack_A_Fast_Montage、Attack_B_Fast_Montage，是否在当前 BP 默认值使用仍待编辑器确认。
- GA_EnemyBasicAttack 引用 `/Game/Blueprints/Enemies/Animation/AM_Enemy_BasicAttack`。BP_Enemy_Melee_01 父类 UmbraEnemyCharacter，引用 ABP_Enemy_01、AM_Enemy_Death、AM_Enemy_HitReact_Front；BP_WorldItem 父类 UmbraWorldItem。
- UI 导入包可能在 ButtonStyle 自带 HoveredSound/PressedSound；完整样式、Graph 与动画内嵌声音 **待编辑器确认**，接入时必须选择单一播放所有者。

## 推荐架构与预计变更

保持单一 Umbra 模块，新增 Audio 目录，不建立新的战斗系统。

- `UmbraAudioTypes`：稳定事件枚举、分类、上下文（武器、稀有度、物理表面、角色类型）。
- `UmbraAudioLibrary`：DataAsset 映射，随机 SoundBase、音量/Pitch/概率/冷却、2D/3D、Attenuation/Concurrency；最具体上下文匹配优先，空资源静默。
- `UmbraAudioSettings`：项目默认库入口。
- `UmbraAudioSubsystem`：每个游戏世界的本地配置/冷却/并发策略，无 Tick；不持有业务状态。
- `UmbraAudioComponent`：玩家、敌人、掉落 Actor 的声音库覆盖、角色类型与服务器表现通知；不接收客户端伤害指令。
- `UmbraAudioFunctionLibrary`：本地 UI 与蓝图调用入口；`UmbraAnimNotify_Audio`：挥动、发力、脚步等纯表现通知。
- 预计修改 PlayerController（UI/拾取结果）、PlayerCharacter/EnemyCharacter（组件）、AttributeSet（结算后表现）、WorldItem（生成/碰撞）、MenuTabButton/AttributeMenu/InventoryMenu/EquipmentMenu（UI 结果）；追加测试与文档。现有 Attack Ability 计时/公式不改。

## 手动资产操作与风险

需手动创建默认库、角色/敌人覆盖库、各分类 SoundClass、3D 衰减和高频并发资产；为玩家普通/高速攻击及敌人攻击 Montage 添加挥动 Notify；移动序列接触帧添加脚步 Notify。不修改原有 HitWindow/ChainPoint；不可同时在 Montage 与其底层 Sequence 同一时刻加同一声音。

网络：UI/拾取仅 owning client；伤害/死亡/落地权威通知且 dedicated server 不播放；动画 Notify 各可见客户端本地播放，不再 multicast 同一挥动。晚相关 Actor 不重放历史瞬时声音。并发不能替代业务结果去重；随机/概率只影响表现。冷却按来源+事件，额外全局高频限流避免多目标叠加。音频上下文必须在物品销毁/回滚前捕获，不依赖已经销毁的 Actor。数据资产宜启动加载，播放时不反复同步加载。

本报告是接入前基线，最终已实现范围与验证结果见 [配置与验收](AudioSetupGuide.md)。

## 实现后只读复核补充

相同 UE 5.8.2 在具备引擎缓存访问权限后再次运行审计 commandlet，退出码 0、0 Error（2 条既有 Tooltip 字段 Python 重名警告）。使用 AnimationLibrary 的只读 Notify API 成功核查全部 9 个 Montage：普通 A/B/C 各有 AttackHitWindow 与命名 Notify SaveAttack/ResetCombo；高速 A/B 各有 AttackHitWindow、AttackChainPoint；敌人攻击有 AttackHitWindow；敌人死亡和受击 Montage 无 Notify；LevelStart 有 PlayParticleEffect。上述 Montage 本层没有音频 Notify；底层 Sequence、AnimBP Graph 或 Cue 内部仍需手工核查，不能据此断言所有动画完全没有声音。

实现中的掉落生成/稀有度采用复制实例首次本地呈现，限 0–1 秒新鲜窗口并消费一次；这是对上文“服务器生成事件”方案的修正，避免 Actor 初始复制通道尚未建立而丢失 multicast。真实碰撞仍由服务器发瞬时表现通知。
