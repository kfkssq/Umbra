# DCX → Blender → UE 装备导入（2026-11-01）

最新：[普通模板直接法线](DaggerNormalValidation.md) 已加入默认规则并完成小批验证，_n直接绑定；B/A与_m仍不推断。下文未接法线的结果属于此前历史测试。

后续：[AM_M_1140材质审计](Material1140Audit.md) 新增哈希限定的RG法线/B光滑度候选转换与线性粗糙度贴图接口；默认规则不变，需显式选择review规则，视觉与绿色方向尚待验收。

用户目标是直接输入 `C:\Users\kfkssq\Desktop\EldenRingAsset\parts`。现在已接通这个入口：复制原包到隔离缓存 → WitchyBND递归解包 → 本机FLVER插件导入 → Blender转换DDS和导出FBX → 自动生成清单 → 调用M1材质/模型导入器。原始DCX只读，不再要求逐件手工操作Blender或填写槽映射；**当前是基础材质规则支持的链路，不是所有法环材质完整还原**。Icon、params、ItemDefinition、穿戴适配均未接入。

## 一次配置，本机已完成

工具配置在 `Saved/EquipmentImport/dcx_tools.json`（本机路径，不提交）。新机器从 [配置示例](../Scripts/EquipmentImport/dcx_tools.example.json) 复制并修改四个路径：

- Blender：已发现 Steam 安装，实测 **5.2.3 LTS**。
- FLVER Importer：已安装的 `io_import_flver` **0.2.0**，Felix Benter；代码来源标识 https://github.com/shintheweapon/Blender-flver-importer ，本次仅运行已安装插件，不修改或复制其代码。
- WitchyBND：本机 **3.0.1.0**，位于 Desktop/EldenRingExtractionTools；TPF也用它解包。调用 `-s -p -u -c`，只解包、不打包、不弹出提示。PromptPlus拒绝重定向，因此为Witchy创建隐藏原生控制台，记录退出码并验证实际输出；Blender/UE完整输出分别写日志。
- UE根目录：与Umbra.uproject匹配的5.8，实际验证5.8.2。

工具按上述官方/项目自身协议使用；外部资源的来源、许可和使用范围仍保留原记录。管线不会下载游戏数据或安装/修改第三方插件。

## 最简操作

在Umbra项目根目录执行。默认源目录就是用户提供的parts路径；多个Names必须用引号包住逗号分隔字符串。

```powershell
# 只列出选定包和计划；不解包、不运行Blender、不写UE。
.\Scripts\EquipmentImport\ImportDcx.ps1 -Batch MyEquipment -Names 'wp_a_0100,am_m_1130'

# 保存并关闭UE编辑器后，执行选定包的完整链路。
.\Scripts\EquipmentImport\ImportDcx.ps1 -Batch MyEquipment -Names 'wp_a_0100,am_m_1130' -Apply

# 完整目录：省略Names。仍会跳过低清/不支持命名的包，错误逐包列出。
.\Scripts\EquipmentImport\ImportDcx.ps1 -Batch MyEquipment -Apply

# 仅解包和转换，先检查conversion.json，不启动UE。
.\Scripts\EquipmentImport\ImportDcx.ps1 -Batch MyEquipment -Names 'am_m_1140' -Apply -ConvertOnly
```

其他位置使用 `-SourceRoot 'D:\OtherParts'`，自定义工具文件用 `-Tools`，自定义规则用 `-Rules`。执行结束输出 `REPORT .../report.json` 和 OK/INCOMPLETE，失败进程返回非零。不要把全部目录首次导入当成无异常保证；当前3046个包中按命名筛出1373个全清支持类别候选（约4.05GiB原包），其余1673个低清或不支持类别在skipped中说明，**尚未全部转换或导入**。

输出资产：`/Game/EquipmentImport/Staging/<Batch>/WorldDrop/<包名_模型名>/`。包里多个FLVER分别导出、分别生成资产，不把附属模型合并到主模型。武器目前按Static Mesh世界掉落用途输出；防具按Skeletal Mesh输出新Skeleton，不指定角色骨架。没有宣称这些防具已能穿到角色上。

## 规则与保留信息

[blender_dcx.py](../Scripts/EquipmentImport/blender_dcx.py) 读取插件解析出的材质索引，临时包装create_mesh，为每个材质索引建立稳定 `Slot_000` 等槽；结束即恢复函数，不改插件文件。所有原材质名、模板路径、纹理引用都记录在conversion.json。现有插件本身不创建材质槽，此适配补齐该步骤。

[dcx_rules.json](../Scripts/EquipmentImport/dcx_rules.json) 是显式基础近似规则：精确模板后缀metal/leather/fabric/belt分别选金属、皮革、布料、皮革预设。从模板 `P[纹理组名]_类型` 得出纹理组，要求精确存在 `<纹理组>_a`；FLVER有非空Albedo引用时还必须与规则一致，不能模糊择一。原游戏着色器数据不在包中时，不声称这些PBR参数等同原版。

- DDS由Blender以Non-Color、CHANNEL_PACKED读取，保存PNG后重新加载，验证每个RGBA通道差异不超过8bit量化误差；HDR或超范围数据拒绝转换。这里只输出基本图像层，不保留DDS压缩编码和完整mipmap链。
- `_n` 转为PNG并保留通道，但缺少已确认 `normal_encoding=standard_dx` 规则时**不接入UE法线**；`_m` 转换但不连接。
- BaseColor alpha不是全1而无明确alpha_policy时拒绝该模型；不把毛发、树枝、透明贴片默认当Opaque或Masked。
- Hair、crystal等未知模板失败。可在规则文件 `materials` 里以完整模板路径指定 `texture_stem`、surface、preset、alpha_policy和normal_encoding；逐模板覆盖优先于templates规则。应检查纹理用途后再写规则，不通过扩大猜测范围掩盖异常。
- UV、骨架和权重使用原插件导入行为；防具导出前拒绝无骨架或零权重顶点。该检查不是动画变形正确性的完整证明。FBX不开骨骼自动连接、不烘焙动画/修改器，输出单位米并由FBX单位转换到UE厘米。

## 重复运行与异常

[dcx_run.py](../Scripts/EquipmentImport/dcx_run.py) 的缓存键包含原包SHA256、转换脚本、规则、Blender/Witchy和插件代码指纹。成功缓存中的每个文件重算哈希，完整才复用；失败重试创建新attempt目录，不覆盖失败现场。变更规则/工具会产生新转换缓存，UE旧配方会拒绝覆盖，应使用新Batch复验；当前不是自动重导入系统。

每个包独立失败；其他成功包继续进UE。保守地不导入一个存在转换失败的包中的部分模型，避免丢失部件。UE阶段逐模型记录失败，已经保存的成功包通过M1配方保护复用。中途终止没有跨包事务；保存完整性仍受M1文档描述的边界约束。

报告包含源哈希、缓存复用、转换报告位置、失败ID、UE报告和引擎警告；详细conversion.json有模板/纹理映射、通道范围、PNG回读误差、几何数量和异常。缓存和日志位于Saved，不提交。源文件与同批人工编辑UE资产都不覆盖；工作中未保存地图、原装备定义或旧手工导入模型。

## 实际验证与限制

- **原始新武器 `wp_a_0100.partsbnd.dcx`**：Witchy解出两个FLVER和六张DDS；Blender输出两个FBX及六张PNG；UE自动创建基础材质绑定成功，7个新包。完整入口证据：`Saved/EquipmentImport/DCX/Runs/20261101T114502Z_aded63ae/ue.json`。同一运行中am_m_1010失败，因此总报告是INCOMPLETE；武器行成功。
- **原始新防具 `am_m_1130.partsbnd.dcx`**：端到端通过，1个骨骼模型、新Skeleton、BaseColor、MI、Master共5包；`Runs/20261101T114744Z_040c1fa4/ue.json`。最终Converter已包含PNG逐通道回读检查。
- **重复运行**：`Runs/20261101T114856Z_92493ad1/report.json` 显示缓存复用true，UE `saved=[]`；独立编辑器进程读取已有模型、Skeleton和材质引用成功。
- **am_m_1140**：转换预检成功，仅验证Blender，未导入UE。
- **异常实际触发**：am_m_1010纹理组名不一致；am_m_1100含Hair模板；am_m_1120有未声明用途的Alpha；am_m_1160含crystal模板。均保留原文件，报告失败，没有擅自连接。
- **19项Python测试通过**：原M1十三项＋DCX扫描选择、低清排除、未知名称拒绝、源只读及原子报告写入六项。第一次在沙箱临时目录遇到Windows原子重命名拒绝，正常权限环境复跑全部通过，不是忽略失败。
- **完整目录只读计划通过**：`Runs/20261101T115159Z_1b2de660/report.json`，1373候选、1673跳过；只记录计划和哈希，未对整个目录解包/导入。Python语法、PowerShell语法、文档相对链接与本次范围diff检查通过，新uasset命中LFS规则。

未验证：GPU着色编译/视感、PIE、角色穿戴/动画变形、完整3046包或百件规模、特殊法线/遮罩语义、原法环材质完整还原。当前只支持所述基础规则；**不能将“可输入整个文件夹”解读为“整个文件夹无需规则扩展即可全部成功”**。Blender原插件对法线/坐标转换的行为仍需视觉核对。没有改C++，未重新编译、提交或推送。
