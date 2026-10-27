# 音效素材需求与来源登记

2026-10-24。扫描依据见 [审计](AudioAudit.md) 与 [486 项实际资产清单](AudioAssetInventory.md)。已有指资产确实存在，不代表已试听、已接线或许可证已核实。缺失指尚无可确认用途的素材；不以 Greystone 发力语音冒充武器破风、撞击或敌人声音。下表 Environment 对应代码分类 Ambience。

P0 为原型主循环，P1 为体验补齐，P2 为当前仅接口。时长单位为秒，变体数量为每种武器/表面/角色所建议准备的数量。

| EventName | Category | TriggerLocation | Description | SearchKeywords | VariationCount | Priority | AssetStatus | RecommendedDuration | PlaybackMode |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| UIHover | UI | MenuTabButton.HandleButtonHovered（背包/装备槽位已禁用 Hover 音效） | 轻微悬停 | fantasy ui hover soft tick | 2–3 | P0 | 已有：fantasy_gui_4 btn_over / hovered Cue，待试听许可 | 0.03–0.12 | 2D |
| UIClick | UI | MenuTabButton.HandleButtonClicked；其他按钮手动 PlayUI | 确认点击 | medieval ui button click | 2–3 | P0 | 已有：btn_down/up/pressed Cue，待试听许可 | 0.05–0.20 | 2D |
| PageChanged | UI | AttributeMenu.HandleTabClicked 实际换页 | 纸页/轻扫 | fantasy menu page turn | 2 | P1 | 缺失 | 0.15–0.40 | 2D |
| MenuOpen | UI | PlayerController.ToggleCharacterMenu | 打开菜单 | inventory open leather book | 2 | P0 | 缺失 | 0.20–0.50 | 2D |
| MenuClose | UI | 同上关闭分支 | 关闭菜单 | inventory close leather book | 2 | P0 | 缺失 | 0.15–0.40 | 2D |
| EquipmentSelected | UI | EquipmentMenu.HandleSelection | 选中装备槽 | equipment slot select | 2 | P1 | 待确认：已有 UI click 是否适合，不能默认复用 | 0.05–0.20 | 2D |
| InventorySelected | UI | InventoryMenu.HandleSelection | 选中物品 | inventory item select | 2 | P1 | 待确认：已有 UI click | 0.05–0.20 | 2D |
| TooltipShown | UI | ItemTooltip.PresentAt，默认关闭 | 有意义详情出现 | subtle ui detail reveal | 1–2 | P2 | 缺失 | 0.03–0.10 | 2D |
| OperationFailed | UI | 通用 PlayUI 手动入口 | 通用失败提示 | ui denied soft error | 2 | P1 | 待确认：Greystone_Ability_No/LowMana 是语音，不宜默认作为通用 UI 错误 | 0.10–0.35 | 2D |
| EquipSuccess | Equipment | InventoryMenu.FinishEquip Success | 穿戴/武器入手 | equip sword armor leather buckle | 3–5 | P0 | 缺失 | 0.20–0.70 | 2D |
| UnequipSuccess | Equipment | EquipmentMenu.RequestUnequipSlot Success | 卸下/收纳 | unequip armor inventory stow | 3–5 | P0 | 缺失 | 0.20–0.60 | 2D |
| EquipFailed | Equipment | 上述最终失败/AppliedInvalid | 装备操作未完成或属性异常 | equipment denied ui error | 2 | P0 | 缺失 | 0.10–0.30 | 2D |
| InventoryTransfer | Equipment | 已成功装备↔背包转移 | 物品移动底层轻响；默认可不配，避免与 Equip 叠加过重 | inventory item move rustle | 3 | P1 | 缺失 | 0.10–0.35 | 2D |
| LootSpawn | Loot | WorldItem.PlaySpawnAudio 新鲜实例首次呈现，实际 Definition.Rarity | 六种稀有度分别配置生成短提示；高品质更突出，不冒充碰撞 | loot spawn rarity magical chime legendary drop | 2–3/稀有度 | P1 | 待确认：用户新增音频目录，尚未试听或验证映射 | 0.10–1.50 | 3D |
| LootLand | Loot | WorldItem.HandleAudioGroundHit 首次真实接地 | 金属/木/皮甲落地 | sword drop stone armor drop wood | 4–6 | P0 | 缺失 | 0.10–0.60 | 3D |
| LootRarity | Environment | WorldItem.RefreshLootAmbience，落稳/复制状态/旧掉落首次呈现 | 按稀有度持续的空间环境音；拾取或销毁停止，Common 建议静音 | magical item aura seamless ambient loop rare loot hum | 1–3/稀有度 | P1 | 待确认：需配置真正循环的 Wave/Cue/MetaSound | 4–12 秒无缝循环 | 3D |
| PickupSuccess | Loot | PlayerController.ClientPickupResult Success | 实际拾取确认 | loot pickup item collect | 3–5 | P0 | 缺失 | 0.10–0.40 | 2D |
| PickupFailed | Loot | PlayerController.ReportPickupResult 非成功且非取消 | 满包/不可达/物品已取走 | inventory full pickup denied | 2 | P0 | 缺失 | 0.10–0.30 | 2D |
| AttackSwing | Combat | UmbraAnimNotify_Audio 手动放到普攻 Montage 挥动帧 | 剑/锤/其他武器破风 | sword swing whoosh mace swing | 4–8/武器 | P0 | 缺失；Effort_Swing 是语音候选 | 0.10–0.45 | 3D |
| Hit | Combat | AttributeSet → FUmbraDamageAudio 正数伤害结算 | 肉/甲/骨/木/石命中 | sword flesh impact metal armor hit bone | 4–8/武器×表面 | P0 | 缺失 | 0.08–0.40 | 3D |
| Critical | Combat | 同上 Damage_ResultCritical=1 的附加层 | 不含第二次普通撞击的暴击反馈 | critical hit sweetener fantasy accent | 2–4 | P1 | 待确认：Greystone_Ability_ScoredCrit 是语音候选，不是通用撞击层 | 0.10–0.50 | 3D |
| Hurt | Character | FUmbraDamageAudio 非致死结算 | 受伤呻吟 | male warrior pain grunt monster hurt | 5–10/角色 | P0 | 已有玩家候选 Greystone_Effort_Pain/PainHeavy；敌人缺失 | 0.20–1.00 | 3D |
| Kill | Combat | FUmbraDamageAudio 致死结算的攻击者 | 可选击杀反馈 | subtle kill confirm fantasy | 2–4 | P2 | 待确认：Greystone 语音需试听；无通用候选确认 | 0.15–0.60 | 3D |
| Death | Character | Enemy.Die；玩家伤害致零；组件一次性保护 | 死亡声音 | warrior death grunt creature death | 3–6/角色 | P0 | 已有玩家候选 Greystone_Effort_Death/Death；敌人缺失 | 0.50–2.00 | 3D |
| SkillCast | Combat | Audio Notify / AudioComponent 接口；专门技能尚无接入对象 | 技能施放；每技能需独立角色类型或未来扩展上下文 | magic spell cast fire lightning | 3–5/技能 | P2 | 待确认：Greystone Ability Q/RMB 是角色语音候选 | 0.20–1.00 | 3D |
| SkillHit | Combat | 统一伤害入口带 Damage_Source_Skill 的结算 | 技能实际造成伤害 | magic spell impact fire lightning | 3–5/技能 | P2 | 缺失 | 0.20–0.80 | 3D |
| Block | Combat | 预留，无当前可靠玩法 | 格挡碰撞 | shield block metal parry | 4–6 | P2 | 缺失；Effort_Block 不能证明已有格挡系统 | 0.10–0.50 | 3D |
| Dodge | Combat | 预留，无当前可靠玩法 | 闪避衣物/移动 | dodge roll cloth swoosh | 3–5 | P2 | 缺失 | 0.15–0.50 | 3D |
| AttackVoice | Character | Audio Notify，挥击发力帧 | 普攻呼气、发力，低概率+长冷却 | warrior exertion attack grunt | 6–12/角色 | P1 | 已有 Greystone_Effort_Attack/Swing/Ability_Primary；其他角色缺失 | 0.15–0.70 | 3D |
| Footstep | Character | Audio Notify 左/右脚接触帧，物理表面射线 | 石/土/木/金属脚步 | armored footsteps stone dirt wood metal | 6–10/表面 | P0 | 缺失 | 0.05–0.25 | 3D |
| Clothing | Character | 预留 Audio Notify | 衣物/盔甲摩擦 | armor chainmail leather movement cloth | 5–8 | P2 | 缺失 | 0.10–0.40 | 3D |
| Ambience | Environment | 预留 AudioComponent.PlayLocal/Authority；关卡无新接线 | 风/地牢远声等一次性环境声；持续环绕另用原生 AmbientSound 管生命周期 | dark dungeon wind ambience one shot | 3–6 | P2 | 缺失 | 1–8 | 3D |

## 来源与许可

本轮没有下载或导入第三方音频。已有资源的来源包仅能从实际路径确认；不能从包名推断已拥有商业授权。

| 来源 | 本轮确认范围 | 许可证及署名 | 下一步 |
| --- | --- | --- | --- |
| 现有 fantasy_gui_4 | /Game/Asset/fantasy_gui_4/sounds 的 9 个资源 | 待补原购买页面/订单、作者、许可版本、署名要求 | 使用者保留购买凭证与包内原许可，试听 4 Wave/5 Cue |
| 现有 ParagonGreystone | /Game/ParagonGreystone/Audio 的 477 个资源 | 待核对获取该包时的许可、UE 使用限制及商标/角色使用条款 | 不将“已经在项目内”视为授权证据；不挪用为其他敌人占位 |
| [Freesound 许可说明](https://freesound.org/help/faq/#licenses) | 仅检索官方许可说明，未选择或下载单个音频 | 优先逐条 CC0；CC BY 可商用但需署名及许可链接；不选 CC BY-NC | 从单条声音页登记作者、声音 ID、版本、许可 URL、修改说明 |
| [Sonniss GameAudioGDC](https://sonniss.com/gameaudiogdc/) / [Bundle License](https://sonniss.com/gdc-bundle-license/) | 仅核查官方 Bundle 许可入口 | 该 Bundle 许可允许商业项目使用和修改、不要求署名；须遵守完整条款，不单独转售原音效 | 按实际选用年份/包版本保存适用许可副本，不推及 Sonniss 全站其他资源 |

选定每个文件后补登记：`EventName / 原始文件名 / UE 资产路径 / 来源单条 URL / 作者 / 下载或购买日期 / 许可证版本与 URL / 署名原文 / 本地许可凭证位置 / 剪辑修改说明 / 审核状态`。未确认前保持库的 Sounds 数组为空。上述在线来源核查日期为 2026-10-24。
