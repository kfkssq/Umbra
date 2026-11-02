# AM_M_1140 材质审计与候选转换（2026-11-01）

目标：从用户已导入的 `Test01` 开始，确认贴图布局，生成可复验候选，不覆盖手工材质、不把单一样品结论扩展到全库。当前默认 `dcx_rules.json` 未变；候选需显式选择 [dcx_rules_1140_review.json](../Scripts/EquipmentImport/dcx_rules_1140_review.json)。

## 已确认的本机事实

- FLVER仅一个使用中的材质：`Fabric_Leather`，槽`Slot_000`，模板 `N:\GR\data\Material\mtd\Parts\matxml\P[AM_M_1140]_fabric.matxml`。纹理参数名可读取，但引用路径为空，不能据此宣称完整原版材质接线已知。
- TPF只有 `AM_M_1140_a.dds` 和 `AM_M_1140_n.dds`，均1024×1024，没有 `_m`。DDS DX10头分别为DXGI 72（BC1_UNORM_SRGB）、98（BC7_UNORM）。审计直接读取DDS头和Blender解码像素。
- `_a` Alpha全1。`_n` B范围0–0.5568628，不应直接作为标准RGB法线Z；`_n` Alpha约15.208%像素非1，其用途未确认，不自动作为透明度/AO/金属度。
- 用户已导入的 `/Game/EquipmentImport/Staging/Test01` 实际MI只连接BaseColor；Normal仍为Engine DefaultNormal，Metallic=0、Roughness≈0.85。原模型、骨架、材质和纹理共5包均已记录SHA256。

证据：`Saved/EquipmentImport/Material1140/Channels/audit.json` 与 `ue_original_audit.json`。通道图 `Channels/AM_M_1140_n_RGBA.png` 从左到右为R/G/B/A；没有经过自动对比度拉伸。

## 布局依据与推断边界

1. 实际制作/转换记录：[Texture Maps in Elden Ring vs. Baldur's Gate 3](https://www.nexusmods.com/eldenring/articles/427) 描述法环normal的RG为法线XY、B为反转粗糙度、Alpha为mask。这里用作作者实践证据，不视为每一种法环着色器的完整规范。
2. 本机现有查看器 `Desktop/EldenRingExtractionTools/Assets/Shaders/FlverShader/FlverShader.frag`：第225行使用`1-normalColor.z`作为粗糙度，第229–231行用RG重建并归一化法线。它是查看器实现，不是游戏原始shader。
3. 本机通道图与上述布局相符；因此可建立**该纹理的候选转换**。绿色方向取决于最终UV/切线约定，不能仅靠通道直方图证明UE表面凹凸方向正确。原版材质可能还有参数乘法、细节混合和mask用途，本次未重建。

## 新增的可复用能力

[material_channels.py](../Scripts/EquipmentImport/material_channels.py) 提供显式 `decode_rg_gloss`：

```text
x = 2R - 1
y = 2G - 1                 # 仅显式 flip_green 时取反
z = sqrt(max(0, 1-x²-y²))
N = normalize(x,y,z)
输出RGB = N*0.5+0.5         # 新PNG Alpha固定1，原Alpha保留在原始转换PNG中
Roughness = 1-B
```

全图约1.61%的像素XY位于单位圆外（包括纹理空白/填充区），转换按查看器的饱和重建再归一化并记录该比例；不静默产生NaN。这里不能据此判断UV实际覆盖区是否有瑕疵，需检查模型。生成粗糙度范围约0.4431–1.0。

- 候选规则是完整模板路径覆盖，并要求 `_n.dds` SHA256恰为 `65b1a0841b4596bc55823befd7e3ac7337d189548e556e5116e63cfa6d4bb98b`；其他版本或材质不会误用。
- `normal_encoding=er_rg_gloss_candidate` 加显式`flip_green=false`生成新法线/粗糙度PNG，再接入UE。原DDS和原始转换PNG不改，派生PNG保存后回读验证量化误差。
- M1 manifest新增可选`roughness_texture`。旧材质路径/配方不变；有粗糙度贴图时创建独立的`M_Equipment_<Surface>_RoughnessMap`，采样R通道乘以`Roughness`标量。该标量默认1，显式值可覆盖；没有贴图时仍用原预设常量。
- BaseColor保持sRGB=true；派生Normal使用sRGB=false/TC_NORMALMAP；Roughness为sRGB=false/TC_DEFAULT、LinearColor采样。粗糙度图是三通道相同的线性灰度，未当作颜色做gamma处理。
- RoughnessMap Master的默认纹理引用该批首个对应实例的线性粗糙度纹理，所有MI都会显式覆盖；保留该默认纹理依赖，不要独立删除它。
- DCX缓存指纹加入通道转换模块；汇总manifest同步重写粗糙度纹理路径。未知通道仍不连接，默认模板规则没有全局升级。

## 候选查看与复跑

候选资产位于 `/Game/EquipmentImport/Staging/Material1140Review`，原资产仍为`Test01`。打开两个Skeletal Mesh分别查看：皮革边缘、布料褶皱在侧向光照下应有一致的凹凸方向；粗糙度不应导致异常镜面反射；检查纹理接缝、背面和轮廓。不要仅凭法线贴图颜色判断通过。

```powershell
.\Scripts\EquipmentImport\ImportDcx.ps1 -Batch Material1140Review -Names 'am_m_1140' -Rules .\Scripts\EquipmentImport\dcx_rules_1140_review.json -Apply
```

需先关闭UE。相同输入复用缓存；规则改变需新Batch，避免覆盖手工配置。审计目录同时提供翻转绿色的`NormalGreenFlippedComparison.png`供对照，但没有自动选择或替换为翻转版本。

## 验证记录

- Blender 5.2.3：DDS逐通道统计、通道预览、候选法线/粗糙度导出成功；7项转换测试通过（中心法线、绿色取反、单位圆外归一化、Alpha独立、B仅影响粗糙度、错误维度、NaN/越界拒绝）。
- UE 5.8.2：实际候选导入成功，7包；`Saved/EquipmentImport/DCX/Runs/20261101T125448Z_9fde5df2/ue.json`。首次新增粗糙度路径未在汇总清单重写，预检拒绝且零保存；修正后通过。
- 21项纯Python回归通过，包括新增roughness_texture默认倍率和缺失文件拒绝。独立UE回读正确纹理引用、sRGB、Normalmap压缩及Roughness=1、Metallic=0；重复导入零保存；Test01原包哈希不变。
- `verify_1140_editor.py` 必须用 `-AllowCommandletRendering` 才能验证实际编译统计；仅去掉`-nullrhi`仍可能无渲染。首次无RHI统计为0，不能算GPU通过。最终RHI验证结果见 `Saved/EquipmentImport/Material1140/acceptance.json` 和 `rhi_verify.log`。
- **最终D3D12/SM6编译通过**：0编译错误，材质统计206像素指令、153顶点指令、4个采样器；命令行进程退出0。原Test01与候选包哈希均未变。这证明候选shader可编译，不替代模型视觉验收。

**未验收：** UE表面视觉、绿色/切线方向、原版shader还原、Alpha实际用途、角色穿戴和其他材质/装备。规则仍为单样品opt-in候选；不将“数值转换正确/成功导入”当成“最终视觉正确”。未修改C++或用户Test01资产。
