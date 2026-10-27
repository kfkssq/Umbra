# Umbra 音频第一阶段配置与验收

## 当前交付边界

源码框架和可靠触发点已接入；**本轮未新增、覆盖或保存任何 Content 资产，也没有配置可听声音**。没有资源时正常静默。新增 WAV 后只需编辑音频 DataAsset，无需逐个改 C++。挥动、发力、脚步需要在编辑器手动添加 Notify；格挡/闪避、完整技能及环境声仅留入口。完整清单见 [AudioAssetRequirements](AudioAssetRequirements.md)，项目实测依据见 [AudioAudit](AudioAudit.md)。

## 目录与配置入口

建议在 Content Browser 内新建 `/Game/Audio/`，包含 `Libraries`、`Waves/UI`、`Waves/Combat`、`Waves/Character`、`Waves/Equipment`、`Waves/Loot`、`Waves/Ambience`、`Cues`、`Classes`、`Attenuation`、`Concurrency`。已有第三方资源保留原目录，通过引用使用。

1. 重启编译后的 UE 5.8，确认玩家、敌人和 WorldItem 蓝图 Components 面板出现原生 **AudioEvents**。Controller 也有此组件；不再手动添加第二个。
2. Content Browser → Miscellaneous → Data Asset → **UmbraAudioLibrary**，创建 `DA_Audio_Default`。先保持 Events 为空验证静默运行。
3. Project Settings → Game → **Umbra Audio** → **Default Library** 选择该资产，保存项目设置。配置保存在 DefaultGame.ini 的 `/Script/Umbra.UmbraAudioSettings`。该软引用在每个游戏 World 初始化时加载一次；改换默认库后重新 PIE。
4. 可创建 `DA_Audio_Player`、`DA_Audio_EnemyMelee` 等覆盖库，在角色 BP 的 AudioEvents → **Library Override** 设置；**Character Type** 填一致的 FName，如 `Player.Greystone`、`Enemy.Melee`。Controller 的覆盖库只控制本地 UI；WorldItem 的覆盖库只控制掉落。不要为不同敌人使用同一 Greystone 声音占位。
5. 打包时将 `/Game/Audio/Libraries` 加入 Packaging → Additional Asset Directories to Cook，或用项目既有 Asset Manager 规则显式包含这些库。库内 Sounds/Attenuation/Concurrency 是硬引用，会随库收集；本轮未验证打包，不依赖只存在于 ini 的软路径自动被 cook。

配置覆盖顺序：组件覆盖库中最具体匹配行 → 项目默认库中最具体匹配行 → 静默。覆盖库存在匹配但 Sounds 为空表示**明确静音**，不会退回默认库。Match 每个非通配维度算一级具体度，相同具体度以数组中较早行优先；把需要同时匹配武器+材质的行放入库，不能期望把两条独立行自动合并。

框架的概率、变体与 Pitch 选择使用每个 World 独立的 FRandomStream，种子来自平台时钟，不消耗玩法的全局随机序列。SoundCue/MetaSound 内部随机节点仍由 UE 自身管理。

## 配置每个事件

向 Events 添加一行，选择 [需求表](AudioAssetRequirements.md) 中同名 **Event**；**Category** 按表设置（Environment 在枚举里为 Ambience）。把导入的 WAV 所生成的 SoundWave 拖入 **Sounds**，也可以用 SoundCue 或 MetaSoundSource（均继承 SoundBase）。多个有效项等概率随机；空槽忽略。使用自带随机节点的 Cue 时可只填一个 Cue。

| 字段 | 单位与含义 | 建议 |
| --- | --- | --- |
| Volume | 线性倍率，0–4 | UI 0.3–0.6；世界音先以 1 调混音 |
| Pitch Min / Max | 倍率，0.25–4；顺序反向会自动纠正 | 0.95–1.05；不要把攻速写入 Pitch |
| Probability | 0–1 | 普通交互/命中 1；AttackVoice 0.2–0.35 |
| Cooldown | 每来源 Actor/Controller 每事件的真实秒数，跨变体共享 | UIHover 0.08；Hit 0.05–0.10；Hurt 0.6；AttackVoice 1.5–3 |
| Global Cooldown | 同一 World 所有来源每事件间隔，真实秒数 | Hit 0.02–0.04；AttackVoice 0.15；Death 0（不希望同帧其他角色被抑制时） |
| Spatial | 3D 开关 | UI 与装备/拾取结果关闭；世界音打开。PlayUI 强制 2D |
| Attenuation / Concurrency | 原生资产覆盖 | 留空有运行时安全默认，正式资源建议显式配置 |
| Match.Weapon | 现有 EUmbraWeaponType；Unknown 为通配 | 使用实际主手 ItemDefinition.WeaponType；未装备/敌人可用组件 DefaultWeapon |
| Match.Surface | Physical Surface；Default 为通配 | Hit 来源为实际 HitResult.PhysMaterial；无 HitResult 时用受击组件 HitSurface |
| Match.bHasRarity / Rarity | 显式稀有度过滤 | 为 Common/Magic/Rare/Epic/Legendary/Unique 分别建行，Common 留空即可静音 |
| Match.CharacterType | None 为通配 | 语音按发声者组件角色类型；命中按受击者角色类型 |

音量乘法：`Entry.Volume × 选中库.CategoryVolumes[Category] × 本地 World 的 SetCategoryVolume × 声音自身/SoundClass/SoundMix`。CategoryVolumes 缺键视为 1；非法浮点数与范围会被运行时夹取。运行时类别音量通过 Blueprint `Get World Subsystem (UmbraAudioSubsystem)` → `Set Category Volume` 调整，仅影响后续播放；持续声音实时推拉请用原生 SoundClass/SoundMix。概率判定失败不改变游戏结果。冷却不会改 Ability 激活、攻速或命中数。

推荐首批只配 UIHover、UIClick、MenuOpen/Close、EquipSuccess/EquipFailed、PickupSuccess/PickupFailed、Hit、Hurt、Death，随后配置 Notify 事件。Critical 必须只含附加层，普通 Hit 已独立播放一次。InventoryTransfer 默认留空，除非确实需要与 Equip/Unequip 两层叠加。TooltipShown 默认不启用；仅为有意义详情将 WBP_ItemTooltip 的 **Play Show Audio** 开启，并设置至少 0.3 秒冷却。

## 原生音频资产

- 创建 `SC_Master`，子类 `SC_UI`、`SC_Combat`、`SC_Character`、`SC_Equipment`、`SC_Loot`、`SC_Ambience`；在 Wave/Cue/MetaSoundSource 的 Sound Class 字段指定。本阶段 Category 是配置分组，不会自动修改被引用声音资产的 SoundClass。需要音量设置菜单时，用原生 SoundMix Class Override 调整；本轮没有新建设置 UI。
- `ATT_Combat` 起步 Inner Radius 200 cm、Falloff 2000 cm，开启 Spatialization；俯视角镜头较高，必须 PIE 确认监听点与角色距离。脚步可用更短半径，稀有度提示可适度增大。未配置时系统用球形 200 cm + 2000 cm 衰减，避免 3D 事件变成全图等响。
- `CON_UI` Max Count 4–6；`CON_Impact` 8–12；`CON_Voice` 2–4（角色语音可 Limit To Owner）；脚步 6–8。一般选 Stop Oldest 或 Prevent New，按试听决定。未配置的所有事件共享本世界 MaxCount=24、StopOldest 的默认并发组。配置独立 Concurrency 后使用该组，不再受默认组 24 限制；全局事件冷却仍有效。
- 批量高攻速要用短声音、冷却与并发共同控制。不要为了每次攻击都出声缩短原有攻击周期。攻击发力、受伤、死亡分别配置事件，避免一个长语音 Cue 又包含同一个命中音。
- Ambience 当前入口适合一次性环境声；不要给即放即忘接口配置无限循环音。长期环境 Loop 用关卡原生 AmbientSound/AudioComponent 管理开始、停止、淡出，不由这个框架新建全局音乐管理器。

## 动画 Notify：需要手动保存的步骤

1. 打开 `/Game/ParagonGreystone/Characters/Heroes/Greystone/Animations/Attack_PrimaryA_Montage`、`Attack_PrimaryB_Montage`、`Attack_PrimaryC_Montage`；高速另查 `Attack_A_Fast_Montage`、`Attack_B_Fast_Montage`。实际 GA_BasicAttack 普通依赖已通过 AssetRegistry 确认，高速数组与使用状态待编辑器确认。
2. 在 Notifies 新增独立 `Audio` 轨道，找到**剑开始快速划过空气的帧**（通常在接触前），右键 Add Notify → **Umbra Audio**，Event=`AttackSwing`。只新增声音 Notify，不挪动 Attack Hit Window / Chain Point，不改长度、Rate Scale、section、VisualStrikeTimeOverrides 或任何攻击参数。Socket Name 可填实际存在的武器 socket；留空使用角色位置。骨骼/socket 名称以 Skeleton 为准，本轮未推测。
3. 同一 Montage 的发力帧可以另加 Event=`AttackVoice`，库里设置概率与较长冷却。不要用 Effort_Swing 代替 AttackSwing 的武器破风层。
4. 敌人 `/Game/Blueprints/Enemies/Animation/AM_Enemy_BasicAttack` 同样添加挥动/发力 Notify。Notify 在每个可见客户端本地执行，服务器不再广播挥动，Dedicated Server 直接跳过。
5. 玩家 `/Game/Blueprints/Player/Greystone_AnimBlueprint` 和敌人 `/Game/Blueprints/Enemies/ABP_Enemy_01` 中查实际 Locomotion 引用，逐个打开真正使用的走/跑 Sequence，在左脚、右脚接地帧各加 Event=`Footstep`，Socket Name 指向相应脚骨。动画资产内部 Locomotion 引用和接触帧**待编辑器确认**，不提供假秒数。
6. Footstep 只在 Character 主 Mesh、地面移动且速度非零时播放；Notify 时做一次地面射线获取 Physical Material Surface。Project Settings → Physics → Physical Surface 配置 Flesh/Metal/Stone/Wood/Dirt 等，再给实际物理材质指定对应类型；DataAsset 的 Match.Surface 对应同一类型。
7. **命中、受伤、暴击和死亡不要添加声音 Notify**：它们已由结果链触发。此 Notify 类只允许 AttackSwing、AttackVoice、Footstep、Clothing、SkillCast；即使误选 Hit 也不会播放。已有死亡/受击 Montage 不需音频接线。
8. 检查 Montage 和底层 Sequence 不会重复触发同一音；检查继承的 PlaySound Notify 和 AnimBP Graph 已有声音。只保留一个所有者，再编译/保存本次实际修改的资产。未在本轮批量执行这些操作。

播放速率由 UE 的原生 Notify 时间轴自然适配 Montage；音频本身不会改变 Montage 速率，也不会触发实际伤害。非常高速、动画混合跳帧的听感需要实际 PIE 验证。

## UI 与结果接线

2026-10-26：背包和装备槽位无论是否有物品，均不播放 Hover 音效。高亮、选择和 Tooltip 交互保留；普通按钮的 Hover 音效保留。此规则覆盖早期槽位 Hover 接入说明。

现有原生 MenuTabButton 的 Button_0 自动订阅 Hover/Unhover/Click，运行时清空该按钮样式中的 Hovered/Pressed SlateSound，防止原生样式双播。槽位进入/离开仅保留高亮和 Tooltip 交互，不发 Hover 音效；选择声音在 Menu 接受选择变化处播放，不在快照刷新或 SetSelected 中播放。

已有接入的按钮/槽位、AttributeMenu、CharacterMenu、装备/拾取结果不再在父控件或 BP_Result 事件里调用 Play Sound。检查原有 Graph 是否自带播放节点；本轮没有改这些 Graph。其他通用按钮可以复用 `UUmbraMenuTabButton`（内部按钮须为 Button_0），或仅在最内层按钮 OnHovered/OnClicked 调用 `UmbraAudioFunctionLibrary.PlayUI(GetOwningPlayer, UIHover/UIClick, Context)`；两种选一种，禁用该按钮旧 SlateSound，父控件不重复处理。不要重构布局。

装备仅现有 UI 的最终 Result=Success 播成功；AppliedInvalid 播 EquipFailed，表示物品已经转移但属性异常，不能重新发放物品。现有装备客户端调用仍可能返回 AuthorityRequired，本轮不新增装备 RPC。底层 Inventory Add/Remove、初始化快照、回滚不播放转移音。

拾取上下文在服务器 TryPickup 前从实际 ItemInstance.Definition 捕获，成功只在 Reliable ClientPickupResult 收到后播；ReportPickupResult 处理失败，不对 Cancelled 播失败。重复点击同一已取物品只返回 Unavailable，不会再播成功；延迟导致晚到的成功仍代表真实已完成操作。

LootSpawn 在本地首次收到年龄 0–1 秒且尚未落稳的新实例时尝试一次，携带实际 Definition.Rarity。超过窗口、没有客户端 GameState 或时钟显示未来的实例保守静默，不补播历史生成提示。极高网络延迟可能听不到。真实落地 LootLand 使用服务器 PhysicsBody.OnComponentHit，第一次与 WorldStatic 且法线 Z≥0.5 的接触；滚动/重复碰撞/FreezeDrop/异常回收不会响。动态物体接触目前不作为落地提示。

### 稀有度生成提示与持续环境音（2026-10-26）

- 在现有 Audio Library 中为 `LootSpawn` 添加 Common、Magic、Rare、Epic、Legendary、Unique 六行：每行启用 `Match.bHasRarity`、指定 `Match.Rarity`、Category=Loot、Spatial=true，并分配不同的短音素材。建议普通为轻响、魔法带微弱魔力音、稀有更清晰、史诗更厚、传奇/独特使用各自辨识度高的提示。没有指定品质行时使用通配行；指定品质的空行表示静音。源码已传入实际品质，不会自动加工素材或替你填入声音。
- 为 `LootRarity` 单独添加六种品质行，Category=Ambience，普通品质可留空。它现在仅由掉落物管理，不再通过通用 PlayLocal/PlayUI/PlayAuthority 播放。物理掉落在复制的落稳状态到达后启动；关闭物理的旧动画模式在动画窗口结束后启动。旧物品首次进入网络相关范围也会启动环境音；没有安全地面而未启动物理的物品不启动。
- Sounds 必须引用**已设置循环的音源**：SoundWave 打开 Looping，或 SoundCue 的 Wave Player 打开 Looping；MetaSound 使用循环 Wave Player 并正确设置无限时长。代码以 SoundBase.IsLooping 检查，不满足的条目静默跳过，不改共享音源。建议 Virtualization Mode=Restart，远离后再次靠近可恢复。不要直接使用旧的一次性稀有度提示音。尚未修改任何用户资产。
- 空间播放强制开启，使用该行 Attenuation；推荐独立的短距离衰减并在俯视角镜头下试听。并发与短音分组隔离：默认最多4个循环，优先淘汰更远的声音、等距不抢占；可在该行设置独立 SoundConcurrency。被距离/并发停止的组件每秒尝试恢复一次，不重复创建组件或重抽概率，没有 Tick 音频判断。
- Probability 在每件物品本地首次准备环境音时抽取一次，建议1；Cooldown/GlobalCooldown 不用于持续音，防止同帧落地永久丢失环境音。Volume/Pitch/CategoryVolumes 在创建时应用；实时调节已经播放的循环音请使用 SC_Ambience 的 SoundMix Class Override。拾取成功、Actor 销毁、客户端失去网络相关性或退出关卡时停止并销毁组件、清理重试定时器；拾取失败仍保留声音。
- PIE 检查六种品质的生成提示、悬空无环境音、落稳持续循环、远近衰减、旧掉落重新可见、多件并发与靠近恢复、拾取失败继续/成功停止、退出关卡无残留。无资源时应静默；未循环的旧音源应静默。

本次编译尝试被正在运行的 Live Coding 拒绝（`Saved/Logs/AudioLootLoopBuild.log`），未得到本轮编译或自动化通过结果，也未进行 PIE 试听。新增稀有度映射、一次性入口保护和晚到复制/拾取清理断言待重跑。涉及反射头文件变更，建议保存工作并关闭编辑器后完整构建，再启动编辑器验收。

伤害 Hit/SkillHit、Critical、Hurt、Kill 经权威轻量组件 Unreliable Multicast，Listen Server 仅在 multicast 实现播放一次，Dedicated Server 不播放；掉线/丢包允许漏掉瞬时声音，不重试成噪声。Death 每组件终身只发送一次，不从 OnRep 尸体状态补播；若未来增加同 Actor 复活/池化，应新增明确重置生命周期接口，当前项目没有该机制。玩家死亡流程现由 Health 委托触发：直接通过 GAS 改为零也发出 Death，组件去重避免与 IncomingDamage 重复。配置与验证状态见 [PlayerDeath](../PlayerDeath.md)。

## 验证记录与 PIE 清单

本轮验证结果在本节末尾更新；历史文档里的测试结果不代表当前验证。自动化命令使用 -nullrhi -nosound，因此即使通过也不等于听感通过。

- 全空 DefaultLibrary / 空 Events / 空 Sounds：进入原型、打开关闭菜单、移动、攻击、拾取，Output Log 无音频持续错误。
- 启用一项 UIHover：普通按钮进入一次只响一次，原地不重复；背包/装备槽位（空槽与有物品）均不响 Hover。点击不会同时触发父控件同音；切页初始化不响，点击当前页不响 PageChanged。
- EquipSuccess/EquipFailed 用明显不同临时**合法已有**声音：成功响成功，等级不足/槽位错误/背包满/AuthorityRequired/AppliedInvalid 不响成功；回滚不响 InventoryTransfer。
- 服务器与一个客户端 PIE，模拟延迟：拾取请求发出时不响成功，成功回传响一次；重复取同物品不响成功。装备网络限制按现有系统验证，不假定已支持远程穿戴。
- 生成物品空中只有 Spawn/Rarity；第一次真实接地才有 Land；连续滚动只一次；支撑查询/超时恢复不冒充接地；旧物品晚相关不补播 Spawn/Rarity。
- 普通与高速 Montage、攻击取消、目标出范围/遮挡/挥空：只有实际 Notify 挥动声音；没有正数伤害结算就没有 Hit/Critical/Hurt。暴击只增加 Critical 层，普通 Hit 不重复。
- 每种武器、敌人角色库、目标 HitSurface、真实 HitResult PhysMaterial、物品六种稀有度逐项检查命中匹配与通配回退；空覆盖行必须静音。
- 玩家/敌人受伤、连续伤害致死、重复死亡回调：死亡最多一次，HitReact 动画不重复呻吟；高攻速、多目标情况下查看并发，调 Cooldown/GlobalCooldown。
- 走跑左右脚、原地、空中、菜单预览：只实际主 Mesh 地面脚步，预览 Actor 不应添加 AudioEvents；无脚步 Tick。
- 正式 3D 音量、听觉距离、声像、随机音量一致性、多人丢包/延迟、Cook/打包另行验收；未完成前不得称完整可听音效系统验收通过。

本地日志：`Saved/Logs/AudioBuildFinal.log`、`AudioProjectFiles.log`、`AudioAutomation.log`、`AudioAutomationFinal.log`、`AudioPIE.log`。自动化报告在 `Saved/Automation/AudioPhase1` 与 `AudioFinal`；都属于生成数据，不提交。

### 2026-10-24 实际执行结果

- UE 5.8.2 **UmbraEditor Win64 Development 构建成功**，最终日志 AudioBuildFinal.log；MSVC 14.51 为引擎非首选版本，并有引擎 GetMovementBase 弃用警告，无编译错误。
- Rider 项目文件刷新 **Succeeded**；安装版引擎不支持部分 Program Target 的 include-data 生成警告已记录在 AudioProjectFiles.log。
- 首轮 **7/7 通过**：`Umbra.Audio.ContextSelection`、`Umbra.Audio.SilentAndRatePolicy`、`Umbra.Attributes.Lifecycle`、`Umbra.Damage.Types`、`Umbra.Inventory.Transfer`、`Umbra.UI.Inventory.EquipInteraction`、`Umbra.Loot.DropPhysics`。
- 最终掉落生成策略调整后，重新执行音频两项与 DropPhysics，**3/3 通过**。DropPhysics 新增断言确认实际 Chaos 地面碰撞已消费落地事件，空中支撑查询不会消费落地事件。音频测试覆盖上下文具体度/回退、空资源、概率零、来源冷却和多来源全局限流；测试用瞬态空 Cue 检查分发策略，不代表听到了声音。
- `Scripts/smoke_audio_pie.py` 在 `/Game/Maps/L_Prototype` 进入 PIE，找到 `BP_UmbraPlayerCharacter_C` 的 AudioEvents，执行空库 UIHover/MenuOpen/PickupFailed 与 AttackSwing 调用，运行五秒后正常 EndPIE，结果 **passed**（`Saved/AudioPIESmoke.json`）。这是 `-nullrhi -nosound` 的无画面/无声冒烟，不包含真实鼠标 UI、攻击命中、拾取操作或声音设备验证。早期脚本因 Python API 命名和跨帧自动退出设置失败，已修正后重跑。
- 已检查 Output Log，仍见引擎启动 `LogAutomationTest: Condition failed`、现有 Greystone_AnimBlueprint 除零、供应商 GreystonePlayerCharacter 旧输入映射缺失、RecastNavMesh 缺失及 GameplayCue 路径未配置等诊断；本轮未修改这些系统，不能声称“项目日志零警告/零错误”。音频空库调用未产生音频错误或崩溃。
- `git diff --check` 通过；Content 与 Config 无修改，无音频资产下载/导入；未提交或推送。没有运行带声音的并发听感、网络丢包/延迟、各种动画接触帧、实际多客户端重复播放或打包验证。

### 2026-10-25 交付复核

框架随机选择已隔离为音频专用随机流，避免监听服务器上的表现代码消耗全局玩法随机数。`AudioBuildReview.log` 记录 UmbraEditor Win64 Development 构建成功；`AudioAutomationReview.log` / `Saved/Automation/AudioReview` 记录 ContextSelection、SilentAndRatePolicy **2/2 通过**。此次仅重跑相关音频自动化；五秒空库 PIE 仍为上节 2026-10-24 的实测结果，随机流修改后未重复 PIE。Content/Config 仍无改动。

## 玩家受伤与暴击语音（2026-10-27）

- `Hurt` 已由 `UmbraAttributeSet::PostGameplayEffectExecute → FUmbraDamageAudio::Dispatch` 在正数 IncomingDamage 结算后触发，播放位置为受击角色位置；致死伤害改播 Death。不要在受击 Montage 再加 Hurt Notify。
- 本次排查发现默认库已有 `Hurt / Match.CharacterType=Player.Greystone / Greystone_Effort_Pain`，但 `BP_UmbraPlayerCharacter.AudioEvents.CharacterType=None`，导致匹配失败。通过 `Scripts/configure_character_audio.py` 在匹配版本编辑器内配置玩家类型；执行前备份到 `Saved/CharacterAudioBackup`。实际执行/验证状态见 Progress。
- 新增 `CriticalVoice`：读取本次 GAS Spec 的 `Damage.ResultCritical`，在正数有效伤害结算后从攻击者位置播放，使用攻击者的角色类型及 Library Override。暴击击杀同样触发；自伤不触发攻击者语音。现有 `Critical` 仍为受击位置的附加音效，互不替代。
- 默认库增加 `CriticalVoice / Character / Player.Greystone → Greystone_Ability_ScoredCrit`，Spatial=true、Probability=1、Pitch=1、每角色冷却2秒。与其它事件一样，组件覆盖库优先、未匹配才查默认库；匹配到空行表示静音。每次暴击发出事件，冷却期内不重复台词；可在库中改冷却。
- PIE 验证：玩家受非致死伤害应从玩家位置播 Pain；敌人受伤不能使用玩家 Pain。将玩家暴击率设为1，攻击敌人应听到目标位置的 critical_hit 和玩家位置的 ScoredCrit；连续暴击遵守2秒冷却。暴击率设为0不播 ScoredCrit；致死伤害只播 Death、不播 Hurt。多人另查远端播放位置和去重。

## 敌人攻击 Swing 已配置（2026-10-27）

- 编辑器读取确认 `BP_Enemy_Melee_01 → GA_EnemyBasicAttack → /Game/Blueprints/Enemies/Animation/AM_Enemy_BasicAttack`；底层 `ParagonMinions/.../Melee/Attack_A` 没有 Notify。通过 UE 5.8.2 在蒙太奇 Audio 轨添加唯一 `Umbra Audio / AttackSwing`，时间0.33909455秒（比原命中窗口提前0.06秒）。原命中时间0.39909455秒、持续0.07873476秒保持不变。
- 使用默认库现有通配 AttackSwing：`/Game/Asset/Audio/Asset/Attack/swordswing_fast_01`，Spatial=true，Volume=0.85。未改库、蓝图或底层动画；可在蒙太奇 Audio 轨调整同步时刻。执行脚本 `Scripts/configure_enemy_swing.py`；原始资产备份 `Saved/EnemySwingBackup/20261027_104453/`。
- 独立 UE 进程重载确认仅一个 Swing、命中窗口不变、底层动画无重复 Notify；`Scripts/smoke_enemy_swing_pie.py` 验证原型关卡敌人 AudioEvents 存在、1秒攻击蒙太奇成功启动、5秒 PIE 后正常退出。日志 `Saved/Logs/EnemySwingPIE.log`，报告 `Saved/EnemySwingPIE.json`。
- 此次使用 -nullrhi -nosound，未验证真实听感、可见挥动帧同步、多人音频或自然 AI 攻击全过程。需实际 PIE 靠近敌人，检查每次挥动一声、空挥仍播、打断前未到 Notify 时不播。Output Log 仍有既有 Greystone 动画除零、输入轴及导航警告。
