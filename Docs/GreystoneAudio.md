# Greystone 技能音频接入

## 素材审计

2026-11-12 使用 UE 5.8.2 AssetRegistry 加载 /Game 下全部 SoundWave、SoundCue、MetaSoundSource，读取类型、时长、依赖和 Wave 循环标记。525 个声音资产：379 SoundWave、146 SoundCue；其中 Greystone 337 Wave、140 Cue。Cue 与其 Wave 是引用关系，不能算作 477 段独立录音。详见 `Saved/GreystoneAudio/audit.json`；重跑脚本 `Scripts/audit_greystone_audio.py`。

已确认剑风素材：swordswing_slow 0.70s、fast_01 0.99s、fast_02 0.31s；剑命中 sword_hit_01 0.66s、02 1.30s。Greystone 有攻击/挥动/跳跃/格挡/受伤/死亡/技能发力、复苏、增益等 Cue。字幕字段为空，未完成语音内容转写或主观试听；以下语义适配为按 Cue 分类及其实际 Wave 引用作出的初选，不声称恢复原版声音设计。

## 六技能配置与缺口

所有原始声音仅引用，不修改第三方 Wave/Cue，不接入资源消耗。

| 按键/技能 | 已配置 | 缺少或待制作 |
| --- | --- | --- |
| 1 破阵斩 | 起手 Effort_Swing；攻击时 swordswing_slow；真实伤害触发 SkillHit | 更厚重的专属剑风可选升级 |
| 2 剑刃风暴 | 起手 Effort_Ability_Q；每次攻击脉冲 fast_02 短剑风；真实命中 | 无缝旋转剑风 Loop、启动/收尾声；现有短剑风不是循环素材 |
| 3 跃击震地 | 起手 Effort_JumpHeavy；实际落地伤害走 SkillHit | 起跳/腾空破风、落地重击、碎石；未用剑命中冒充地面爆破 |
| 4 坚盾防御 | 起手 Effort_Block 发力语音 | 举盾/收盾金属动作、维持声；真实格挡撞击还需要格挡判定，当前仅防御增益 |
| 5 石化复苏 | 治疗/震击生效时 Effort_Ability_Ultimate_Rebirth；伤害走 SkillHit | 岩石凝聚、蓄力、下砸碎裂、非语音恢复提示；没有使用死亡语音模拟主动治疗 |
| 6 战意 | 增益生效时 Status_Buffed | 专属非语音增益提示/消退声；未使用掉落品质声代替 |

语音路径前缀 `/Game/ParagonGreystone/Audio/Cues/Greystone_`；剑风与命中前缀 `/Game/Asset/Audio/Asset/Attack/`。现有掉落循环、UI 声音、辉石技能声音未移用到正式 Greystone 配置。缺口表示在本次审计资产中未找到明确合适的专用素材，仍可通过后续试听重新评估。

## 配置源与触发边界

- `Scripts/configure_greystone_audio.py` 通过 UE 保存六个 DA、独立衰减/并发资产及默认音频库。执行前备份修改的已有资产到 `Saved/GreystoneAudio/Backup/<timestamp>`；可重复运行。重新执行 native kit 全量配置后应再执行此音频配置，避免破阵斩被旧原型声音覆盖。
- 六个 `/Game/Skills/Greystone/DA_*` 的 CastVoice 起手播，ResolveVoice 随实际技能表现结算播。风暴禁止 ResolveVoice，避免每个伤害脉冲重复长语音。此轮未修改动画或新增 Notify，配置报告核对蒙太奇与源动画已有 Notify。
- StrikeSound 是空挥/区域发动声音，不是命中判定。命中走现有 `FUmbraDamageAudio::Dispatch → SkillHit`，本次默认库补齐通用 SkillHit 行，两段 sword_hit 随机，音量0.65、每来源0.1s/全局0.035s冷却。它是当前近战原型的通用回退；未来新增元素或远程角色时应扩展匹配条件。
- SkillSoundVolume / SkillVoiceVolume 在六个 DA 配置为0.65；源码默认分别1和0.65，旧技能保留原攻击声增益。它们是技能专用声音参数，不受 UmbraAudioLibrary 类别音量直接控制；声音自身 SoundClass 仍有效。SkillHit 继续走既有音频库混音。
- `ATT_GreystoneSkills`：球形内半径250cm，衰减距离2200cm；`CON_GreystoneSkills`：8个并发，停止最旧实例。技能语音单独替换上一段技能语音，不受剑风/命中并发组挤占。
- ActiveSound 支持有生命周期的附着声音；当前六技能全空，因为缺少合适循环素材。每次开始先清理同技能旧循环；结束/取消淡出0.12s。取消语音淡出0.08s，正常结束允许语音自然尾音；死亡、换角色、清理 ASC 时停止。声音跟随角色，角色销毁即停。
- 配置优先级：DA 保存值覆盖 C++ 默认；技能命中音仍按组件音频库覆盖→默认库匹配→静默。原有受伤、死亡、暴击语音行保持不变。

## 验证

验证结果完成后记录。人工试听重点：空挥只有剑风；风暴命中不会叠六段长语音；跃击落地时机；取消无尾随循环；战意重复刷新不叠长语音；俯视镜头下衰减合理。未试听不能用资产计数或组件播放状态代替听感结论。

### 本轮实测记录

- `Saved/GreystoneAudioBuild.log`：UmbraEditor Win64 Development 构建成功；首次编译暴露 UE 5.8 PlaySoundAtLocation 重载差异，修正后通过。
- `Saved/GreystoneAudioAutomation.log`：Umbra.Skills 6/6 成功（CooldownView、GreystoneExpansion、NativePresentation、PebbleDefinition、PebbleOriginalAudio、SweepSector）。引擎启动阶段存在 UnifiedErrorTest/Condition failed 输出，不作为这六项失败，也不声称日志零错误。
- `Saved/GreystoneAudio/configuration.json`：六个蒙太奇及六个实际源动画 Notify 轨均为空，没有发现来自这些轨道的重复音频触发。
- `Saved/GreystoneAudio/pie.json`：带实际音频设备的 PIE 15/15 成功：六技能完整播放、六技能取消、临时循环声正常结束/取消/死亡清理。读取真实 AudioComponent 的 sound 和 is_playing；未使用 -nosound。Loop 测试临时借用已循环的掉落音，不保存 DA 或关卡。
- 首次将“循环声死亡”与“战意语音死亡”连续测试时，死亡后的角色未重生，后续输入无法激活技能，最后一例失败。保留 `pie-first-attempt.json`；拆为独立 PIE 后重测，不把未激活误当作声音功能失败。
- `Saved/GreystoneAudioReadback.log` / `audit.json`：新编辑器进程从磁盘读取六技能资源消耗均为0、ActiveSound 均为空、声音引用与 SkillHit 库行存在。第三方声音未修改；新增二进制资产匹配 Git LFS filter。
- 未验证：人工听感、逐句语义转写、多人客户端去重/网络丢包、打包、物理键盘操作；PIE 用真实 Enhanced Input 注入。当前结果证明配置及组件播放生命周期，不代表完成声音美术验收。
- 独立 `-VoiceDeathOnly` PIE 1/1 成功，`Saved/GreystoneAudio/voice-death-pie.json`：战意语音确实播放，施法动作结束后死亡也能停止。合计16个音频用例通过。

## 2026-11-13 斩切波

1技能替代旧破阵斩，音效仍沿用挥剑发力、慢剑风和实际命中音；发动时机改随投射物释放。新行为及调整入口见[斩切波](SlashWave.md)。
