# Fantasy GUI 4 模板技能栏与本地图标

2026-11-06 技能扩展：当前前六格依次为 Q/E 横扫、1 剑刃风暴、2 跃击震地、3 坚盾防御、4 石化复苏，后四格为空。新四格使用法环本地图标08303/08400/08361/08421；原框架、布局和经验条保持不变。详细配置和验证见 [Greystone 技能](GreystoneSkills.md)。下文“两种技能/八个空槽”为此前阶段记录。

## 2026-11-06 法环技能图标接入

当前技能Icon已从warrior_12/warrior_48切换为本地法环图标；下文“未导入”的旧盘点记录保留为历史。

| 技能 | 本地编号 | 项目纹理 | 选择依据 |
|---|---|---|---|
| Q 横扫 | 08314 | `/Game/UI/Icons/EldenRing/T_Skill_Sweep_ER08314` | 近身持剑、短横向剑光，作为普通武器斩击标识 |
| E 广域横扫 | 08309 | `/Game/UI/Icons/EldenRing/T_Skill_WideSweep_ER08309` | 更宽的武器挥斩轨迹，用于区分范围变体 |

以画面动作意象选图，不把图中的武器类型或轨迹解释成当前技能的新机制；两招仍使用原250/350cm、120/170度扇形、原动画与单次命中配置。未宣称对应法环的正式战技名称。

运行时配置入口：`DA_Sweep.Icon` / `DA_WideSweep.Icon`；原生ViewState将图标投影到现有`W_Item_Slot.item_icon`材质的`IconTexture`参数。Designer预览同步更新WBP_SkillSlot默认PreviewIcon及WBP_SkillBar前两格的PreviewIcon。未改动技能栏尺寸、边框、键位、冷却连接或经验条。

导入保持原1024×1024图像内容与透明通道，使用UI纹理组、EditorIcon压缩、sRGB、NoMipmaps和NeverStream。原PNG没有改写。导入脚本：`Scripts/import_skill_icons_er.py`；导入、旧引用及数值快照：`Saved/SkillHUD/er-icon-import.json`。

来源为用户已有目录 `C:/baidunetdiskdownload/艾尔登法环图标/艾尔登法环图标/其它/`，分别是 `menu_knowledge_08314out.obj.png` 和 `menu_knowledge_08309out.obj.png`。源提取工具、游戏资源版本及分发/商业使用授权未核实，沿用项目原有资源来源限制。

- 08314 SHA256：`210e2891d2b298608966719a57c41fa1912fd1404b9873c991c8090ec9fffe1e`
- 08309 SHA256：`43fabbfdcb808f53cda928f6e245d80301707005e8a7fd866d6b0ca3c0427fee`

## 2026-11-06 底部经验条（仅显示）

已采用用户最新的1332×256总框、800×201.33中列及技能栏Padding=(0,3,0,0)。本节覆盖下文旧的500宽记录。

- 在 `WBP_PlayerHUDGroup → Size_ExistingVitals → VerticalBox_126` 中，将 `WBP_PlayerExperienceBar` 放在资源条后面。Horizontal Alignment=Fill，Padding=(0,-4,0,0)；`Size_ExistingVitals` 高度80→84，其他现有控件的尺寸和Slot参数保持。
- `/Game/UI/HUD/WBP_PlayerExperienceBar` 是普通UserWidget，只复制资源条WidgetTree，不复制属性条父类和GAS事件图。沿用SizeBox/Overlay/Border及原`W_ProgressBar_Red`模板子控件；该名称是原模板名称，不代表最终材质颜色。
- 根尺寸800×14；ValueText折叠；进度条外部与内部上下Padding均改为1，以适应细条。横向Padding、边框画刷和流动效果沿用资源条。
- `/Game/UI/HUD/Materials/MI_ExperienceBar_Preview` 继承素材包 `Mat_GUI_ProgressBar_Violet`，仅覆盖 `Value=0.5`。预览比例在此材质实例中修改；尚未接入经验、等级、GAS属性或游戏数据。
- 手工修改高度：经验条根SizeBox_0；位置/间距：HUD中该实例的VerticalBox Slot；颜色和预览比例：上述材质实例。增加高度时需给`Size_ExistingVitals`及中列预留足够空间。
- 工具：`Scripts/audit_experience_bar.py`、`Scripts/create_experience_bar.py`、`Scripts/smoke_experience_bar.py`。原HUD只读副本备份位于`Saved/SkillHUD/BeforeExperience`。所有uasset写入均由UE5.8.2完成，素材包原资产不修改。

## 当前布局：直接复用 WBP 模板

2026-11-05：按用户要求，替换此前“图片重拼、宽64/图框72重叠”的实现。当前直接复制已保存的 `W_Window_Actionbar` 中央栏控件树，以及 `W_Actionbar_Item_Slot` 完整控件树。原素材包资产不写入；项目 WBP 名称与 HUD 引用保持。

```text
WBP_PlayerHUDGroup → SizeBox_33 → VB_SkillsAndVitals
├─ WBP_SkillBar                         500 × 70.8333，下Padding 8
│  └─ Size_TemplateViewport
│     └─ Scale_Template                 ScaleToFit，整体等比缩放
│        └─ Size_TemplateDesign         1200 × 170，样品坐标
│           └─ Overlay_2
│              ├─ Border_1 → SizeBox_1 → Overlay_0
│              │  ├─ TextureFill       样品石纹，Image/Both
│              │  └─ TextureBorder     样品边缘，Box
│              └─ HorizontalBox_0      下Padding 40，十格Auto，子格Padding 0
│                 └─ SkillSlot_0…9      WBP_SkillSlot，SlotIndex 0…9
└─ Size_ExistingVitals                 原生命/资源条区域，高80
```

HUD中列高158.8333，保持原生命/资源区域的底部位置。只提取样品中央技能栏，因此移除项目副本中的示例血球和Buff行；背景原来的左右200、技能行的左右450为血球预留，改为0。其他背景画刷、内部结构和技能行下间距沿用样品。不是包含两侧血球的整个样品窗口的逐像素复制。

十格中前两格接现有Q/E横扫与广域横扫，其余八格留空，不显示假图标、假数量或未绑定按键。未添加新的技能或输入映射。当前仍使用warrior_12/warrior_48。

## 技能格与子控件

```text
WBP_SkillSlot（UmbraSkillSlotWidget）
└─ SizeBox_0                           120 × 120
   └─ CanvasPanel_1
      ├─ SizeBox_1 → ContentBorder     fg4_slotBrown，Box/Margin .2
      │  └─ CanvasPanel_2              原Padding 12
      │     ├─ W_Item_Slot             直接使用原素材包子WBP
      │     │  ├─ ScaleBox_0 → item_icon
      │     │  ├─ Button_0
      │     │  └─ item_quantity
      │     ├─ Pb_Cooldown             新增，按图标内部8单位边距
      │     └─ Txt_Cooldown            新增，居中
      └─ level → border → keyboard_shortcut_label
```

键位控件保留原Canvas锚点、50×50尺寸、Y RenderTranslation=-20、ZOrder=10、fg4_slotTooltip1、24号字体、描边、颜色与居中方式。`W_Item_Slot` 保留原模板结构、Button画刷和内部8单位Padding；运行时仅更新实际图标、费用及可用状态。样品默认数量不代表技能费用，实际`item_quantity`改为显示GAS定义投影的15/25。

## 人工修正入口与绑定

- 整体宽高：`WBP_SkillBar.Size_TemplateViewport`。保持宽高比1200:170；`Size_TemplateDesign`保留原样品坐标，避免逐个子控件缩放导致间距漂移。
- 整条位置：`WBP_PlayerHUDGroup` 中 `WBP_SkillBar` 的VerticalBox Slot。调整技能栏高度时同步 `SizeBox_33.HeightOverride = 技能栏高度 + 上下Padding + 80`。
- 连续背景：`Border_1`、`TextureFill`、`TextureBorder`，各自使用样品实际画刷，而非单张重拼纹理。
- 格间排列：`HorizontalBox_0`；每格Auto，Padding全部0，原始宽120。当前缩放后每格50，未使用负间距或框体重叠。
- 单格外框与键位：`WBP_SkillSlot.ContentBorder` 和 `level → border → keyboard_shortcut_label`。修改项目WBP即可。
- 原生子WBP样式：`W_Item_Slot` 直接引用素材包，修改其原资产会影响其他使用者；需要单独改变内部风格时先在编辑器复制成项目子WBP，再替换引用。
- `WBP_SkillSlot` Class Defaults → Skill / Presentation：`IconWidgetPath=W_Item_Slot.item_icon`、`KeyWidgetPath=keyboard_shortcut_label`、`CostWidgetPath=W_Item_Slot.item_quantity`。这是穿过嵌套UserWidget的控件名路径；如重命名需同步路径。
- `Pb_Cooldown`、`Txt_Cooldown`继续使用自动绑定名称；无需新增Graph接线。费用与冷却仍来自PlayerState/GAS，UI不计算战斗规则，不独立倒计时。
- Skill / Preview 的Icon、KeyLabel、ResourceCost只用于Designer预览；运行时由现有技能配置覆盖。每个栏位可独立预览，空格PreviewIcon为空。
- 技能数据与实际图标仍在 `DA_Sweep`、`DA_WideSweep`；控制器独立`SkillBarClass`保持为空，避免重复挂载。

原来直接控件名 `Img_Icon` / `Txt_Key` / `Txt_Cost` 的可选绑定继续支持；设置了对应Presentation路径时优先该路径。完全自定义事件接法仍见 [接线指南](SkillBarBlueprintGuide.md)。

## 本地图标盘点

根目录：`C:/baidunetdiskdownload/艾尔登法环图标/艾尔登法环图标`。

- “技能”子目录：173张PNG，6000–6999编号段；实际图像主要为魔法/祷告。
- “其它”子目录：8300–8499编号段有91张动作/战技图像候选。8000段总共137张，但其中包含砥石/属性武器等，不能把137张全部算成技能。
- 本次重点候选均为1024×1024PNG，已查看图像。

| 编号/文件 | 本地证据 | 对 Umbra 的适用方向 |
|---|---|---|
| `其它/menu_knowledge_08309out.obj.png` | 人物横向挥砍、浅色弧线；正式技能名未映射 | 当前物理横扫的优先候选 |
| `其它/menu_knowledge_08303out.obj.png` | 人物与明显环形剑弧；正式技能名未映射 | 更适合未来旋转斩；当前广域仍是170°，不能据图宣称360° |
| `其它/menu_knowledge_08314out.obj.png` | 人物横向持剑与剑光；正式技能名未映射 | 单手剑动作/剑技候选，需结合最终动画选 |
| `其它/menu_knowledge_08360out.obj.png` | 持盾防御动作；正式技能名未映射 | 格挡、防御技能 |
| `技能/menu_knowledge_06000out.obj.png` | Goods ID4000 → iconId6000 → GoodsName“辉石魔砾” | 魔法弹 |
| `技能/menu_knowledge_06014out.obj.png` | Goods ID4070 → iconId6014 → “辉石弯弧” | 蓝色弧形魔法波，当前物理横扫不宜用 |
| `技能/menu_knowledge_06050out.obj.png` | Goods ID4430 → iconId6050 → “卡利亚大剑” | 魔法巨剑斩击 |
| `技能/menu_knowledge_06052out.obj.png` | Goods ID4450 → iconId6052 → “卡利亚贯刺” | 魔法剑突刺 |
| `技能/menu_knowledge_06200out.obj.png` | Goods ID6000 → iconId6200 → “燃火” | 近身火焰 |

名称核对来源为本地 `EldenRingAsset/EquipParamGoods.csv` 和 `msg/itemtext_chinese.json`，不是按图片文件名猜测。本地虽有 GemName/ArtsName 文本，但没有 EquipParamGem.csv / SwordArtsParam.csv，不能把它们的行号直接当图标编号，因此战技候选仅标明图像特征。源数据版本、素材许可继续按项目原有来源记录，不因可读取PNG而认定免费商用。

`Scripts/catalog_local_skill_icons.py` 只读源目录，输出 `Saved/SkillHUD/local-icons.json`：264条候选路径、尺寸、SHA256和可验证的Goods映射。重点候选见 `icon-candidates.json`；联系表 `er-skills-0/1/2.png` 与 `er-ashes-0/1/2.png` 供人工选图。

## 修改与验证工具

- `Scripts/audit_skill_hud.py`：只读编辑器布局/画刷、导出已有纹理供确认。
- `Scripts/embed_skill_hud.py`：本轮明确的UI迁移；匹配已检查的HUD层级后改动，经UE保存。
- `UmbraEditorTools.EmbedSkillBarInHUD`：保留原控件、把中列包成技能栏+生命资源的组合；重复执行不重置手工布局。
- `Scripts/smoke_embedded_skill_hud.py`：带渲染PIE检查内嵌唯一实例、Owner、一次横扫和真实冷却清除，并保存UI截图。离屏CachedGeometry返回零值，尺寸/对齐以保存的Designer配置及截图人工观察为证，未通过运行时坐标断言。技能由测试调用真实ASC激活，不冒充本轮键盘注入。

本轮具体通过/未验证项以 [Progress](Progress.md) 最新记录为准。

## 顺时针技能冷却（2026-11-06）

`WBP_SkillSlot` 内新增 `Img_Cooldown`，沿用原 `Pb_Cooldown` 的Canvas布局及层级，材质为 `/Game/UI/Skills/M_UI_SkillCooldownClock`。运行时 `Remaining` 参数来自GAS冷却比例，12点开始顺时针消退并显示金色指针；数字继续复用原文本。原进度条有新图像时隐藏。Designer可调整 `PreviewCooldownFraction` 检查外观，不改变运行时规则。当前 `umbra.Skills.IgnoreCooldowns=1`，所以正常游戏不显示冷却；控制台设为0可验收旋转效果。外框、键位框、图标和用户布局未重建。详见 [技能指南](GreystoneSkills.md)。
