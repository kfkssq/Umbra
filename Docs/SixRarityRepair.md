# 六品质、展示与掉落音效修复

2026-10-31。用户确认的顺序为 Common白、Magic绿、Rare蓝、Epic深紫、Legendary玫红、Unique暗金。枚举数值0–5保持不变。

| 品质 | 颜色 | 本轮sRGB取值 |
|---|---|---|
| Common | 白 | FFFFFF |
| Magic | 绿 | 4BCB63 |
| Rare | 蓝 | 448AFF |
| Epic | 深紫 | 6B30B7 |
| Legendary | 玫红 | E83E8C |
| Unique | 暗金 | A88732 |

用户指定色系；以上具体色值为本轮选取的可调参数，入口 [rarity_palette.py](../Scripts/rarity_palette.py)，转换为LinearColor后写入UE资产。固定物品读取 Definition.RarityColor 与四个背景/边框引用；随机实例读取 GenerationProfile 中实际 Rarity 对应行，不再继承模板另一品质的背景图。TintRarityBackgrounds（固定物品）/TintBackgrounds（生成行）使中性灰纹理按同一品质色着色；自带颜色的自定义纹理可关闭该开关。物品Icon本体不染色。Tooltip标题改为品质色，覆盖之前“标题固定白色”的展示要求。

## 生成与配置

- 生成器原校验最大只允许Rare，现允许至Unique，重复/非法枚举/权重/词缀筛选规则保持。
- RandomMVP生成配置扩展为六行，规则版本升至2；原三行权重/数量保留，新三行为测试等权重1、词缀数3/4/5。新建测试配置为六品质等权、词缀数0–5。这是便于覆盖的测试分布，不是正式掉落平衡承诺。
- RandomM4伤害快照夹具仍固定Rare，以保证A/X回归稳定；其颜色改为蓝色。单品质测试夹具不代表生成器只能生成该品质。
- [configure_six_rarities.py](../Scripts/configure_six_rarities.py) 是显式UE迁移脚本，会保存Items目录固定定义的展示字段和生成配置的品质外观，不能把它当只读审计。它不修改地图、音效库、词缀本身、装备需求、武器数值或固定物品的品质枚举。生成定义自身的M5托管字段不动。
- 本次保存清单见 Saved/SixRarityMigration.json，包含6个固定定义和2个生成配置；音效库迁移前后SHA256相同。已有Common但使用紫背景的固定资产按其Common身份恢复白色，并未擅自改成Epic。

## 声音触发

音效仍由 AudioLibrary 的 LootSpawn + Match.Rarity 选择。已实际读取六条配置，分别指向 pop_common、magic_pop_magic、magic_pop_rare、magic_pop_epic、magic_pop_legendary、magic_pop_unique；未替换用户音源。

旧逻辑在GameState/服务器时间可用前就消耗bSpawnAudioSent；客户端时间暂时落后时不会再尝试。现以0.05秒间隔最多重试20次，不在缺失/未来时钟时立即消耗。新掉落即使已落稳仍允许首次生成提示，避免落稳状态抢先到达造成静音；年龄超过1秒的历史物品仍不补播。结束生命周期清理重试计时器。

LootSpawn全局冷却改为按品质分开，六种品质同时掉落不会互相压掉。相同品质仍遵循其GlobalCooldown，默认0.015秒；需要每件都响可由音效配置将该值设0，当前未修改库。单物品仅消费一次提示，落地音与循环环境音保持独立。

Verbose级LogUmbra新增AudioDispatch记录，含事件、品质、音源、发声Actor。记录证明引擎收到播放请求，不等同用户实际听感、音量和设备输出验收。先前怀疑BeginPlay未完成的判断已通过引擎源码排除，未据此修改引擎生命周期。

## Tooltip客户端

客户端装备组件没有服务端需求查询器，原先FromInventory/FromEquipment因ContextUnavailable隐藏整张Tooltip。现保持有效实例显示：详细需求信息明确为未知，已装备是否满足需求仍取服务器快照。服务端原校验、GAS计算及穿戴权限不变，不在UI复制公式。

## 复验入口

- 构建 UmbraEditor Win64 Development。
- Automation：Umbra.Audio、Umbra.Items、Umbra.UI.Items、Umbra.Loot.DropPhysics。新增 SixRarities、LootSpawnLifecycle 覆盖六品质、实例背景投影、同帧六音/同品质限流、时钟恢复、一次播放、旧掉落抑制。标题测试按新需求检查颜色切换与清空。
- [smoke_six_rarity_death.py](../Scripts/smoke_six_rarity_death.py)：L_Prototype中临时克隆6个敌人，使用仅内存的强制品质配置，沿真实Health归零→死亡→掉落路径生成六品质；验证拾取和Tooltip投影，不保存地图或临时定义。运行无-nosound，启用LogUmbra Verbose后可将报告里的Actor对应到六条具体音源。测试驱动不参与Shipping。
- [smoke_random_m4_network.py](../Scripts/smoke_random_m4_network.py) 加-UmbraTooltipRegression（可同时-UmbraM4Dedicated）：真实远端背包/装备Tooltip数据验证；详细需求不得伪装已知，已装备聚合状态应与服务器一致。
- 人工：单机或客户端启动，悬停六品质背包/装备物品，看标题/品质文字/背景/边框一致；击杀敌人听生成音与落地音，确认旧掉落离开再返回不重播。视觉对比、耳听验收和打包独立客户端不能用headless数据测试替代。
