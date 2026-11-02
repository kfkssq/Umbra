# 匕首直接法线验证与普通模板自动绑定

2026-11-01：WP_A_0100 原始 `_n` 经既有 DDS→PNG 保真转换后，直接使用 UE Normalmap、sRGB=false、Normal 参数，技术验证通过。B/A 不作为其他材质输入，`_m` 不自动解释。

## 默认规则与使用

[dcx_rules.json](../Scripts/EquipmentImport/dcx_rules.json) 的 metal、leather、fabric、belt 普通模板显式使用 `er_rg_direct`，沿用金属/皮革/布料参数预设。这是普通 ER parts RG 法线约定的模板级处理；匕首进行了渲染检查，不表示所有装备均已逐件视觉验收。特殊模板无自动兜底。

[normal_rules.py](../Scripts/EquipmentImport/normal_rules.py) 要求 FLVER 有 NormalMap 语义，非空显式引用与精确模板 stem 一致，缩放为单位值；BaseColor/Normal 可以不同分辨率，但长宽比须一致。DDS→PNG 通道误差不得超过8位量化容差。冲突或detail法线报告异常，不猜选。

原 PNG 直接导入，Normalmap 压缩、sRGB=false、WorldNormalMap 组、flip_green=false。B/A 和 `_m` 不接线，默认不提取 `_n.B` 粗糙度。1140 原先的派生法线/粗糙度仍是单独 opt-in 配方。normal_rules.py 已纳入转换缓存指纹，旧未绑定法线的缓存不会冒充新结果。手工manifest中的unknown法线继续保持不连接。

在项目根目录执行，Apply前关闭UE：

```powershell
.\Scripts\EquipmentImport\ImportItems.ps1 -Selection .\Scripts\EquipmentImport\item_normal_smoke.json
.\Scripts\EquipmentImport\ImportItems.ps1 -Selection .\Scripts\EquipmentImport\item_normal_smoke.json -Apply
```

无需额外指定Rules。新样品在 `/Game/EquipmentImport/Staging/ItemNormalAuto01`，Drafts含匕首和旅行手套。原ItemBridgeSmoke01/Test01和正式物品不被重配。规则变化需要新batch，既有配方仍受保护。

## 证据

- 独立匕首资产：`/Game/EquipmentImport/Staging/DaggerNormalReview`。
- 编辑器真实帧更新下，使用同相机/光照比较DefaultNormal和直接_n，输出光照图和法线缓冲。可观察刀身槽线、护手、握柄表面响应变化。
- 诊断图仅在临时动态材质中使用Metallic=0、Roughness=0.65来隔离金属反射，没有写回资产。原金属参数的独立对比为material_*文件。曝光校准阶段的过暗/过曝图不作最终外观依据。
- 输出在 `Saved/EquipmentImport/DaggerNormal`；`diagnostic_angled_flat_lit.png` / `diagnostic_angled_direct_lit.png` 为同条件诊断图；material_*为真实预设。脚本 [review_dagger_normal.py](../Scripts/EquipmentImport/review_dagger_normal.py) 以完整编辑器 `-ExecutePythonScript` 运行，空白测试地图不保存。同步commandlet不会正常推进场景更新，早期捕获已废弃。
- 默认一站式成功报告：`Saved/EquipmentImport/Items/ItemNormalAuto01_8cdc9aaa/report.json`。匕首、附属模型、手套三处材质均自动接线，2份物品草稿与icon正常创建，共19包，真实RHI材质编译无错误。
- 重跑 `ItemNormalAuto01_74c04868`：模型、icon、草稿均零保存。
- 本次19包重跑后哈希一致。上一轮任务留下的85包历史快照中，ItemBridgeSmoke01的手套MI_0和匕首主Mesh有2处差异，保留现状、不回滚，不将该旧快照当成本次开始基线；其余83包一致。本次写入路径均为独立新批次。
- 新UE进程回读通过：同目录normal_acceptance.json。检查MI参数、Normalmap/sRGB/green/纹理组，并比对源PNG SHA256，确认确实使用原始 `_n.png`。脚本 [verify_auto_normals.py](../Scripts/EquipmentImport/verify_auto_normals.py)。
- 33项Python测试通过，包含单位缩放tuple/list兼容、引用冲突、detail缩放拒绝、同长宽比不同分辨率支持、转换误差拒绝、direct编码连接。

首次默认回归因插件tuple与JSON list比较方式错误而拒绝，已修复并加入测试；失败报告保留，首次失败未创建UE草稿。

## 边界

确认的是直接绑定及表面细节生效，默认不翻转绿色通道。没有原游戏同机位光照基准，不能声称逐像素还原、所有凹凸方向符合原作或新装备无需视觉检查。原始金属/粗糙度仍采用统一近似预设，特殊编码、透明、detail等继续报告。未运行PIE、角色穿戴或全库导入，无C++修改。

原理参考：[Epic Normalmap/BC5说明](https://dev.epicgames.com/documentation/unreal-engine/texture-compression-settings?application_version=4.27)、[SceneCapture API](https://dev.epicgames.com/documentation/en-us/unreal-engine/python-api/class/SceneCaptureComponent2D?application_version=5.6)。实际执行为项目关联的UE5.8.2。
