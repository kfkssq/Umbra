# 掉落物短时物理与固定

2026-10-21。C++ 实现，蓝图配置默认值，不需要新增 Event Graph。修订版取消正常超时瞬移；通过匹配 UE 编辑器保存6件物品的碰撞盒配置，未编辑地图或装备数值。

## 行为与边界

- `PhysicsBody` 盒体为 Actor 根，服务器运行 Chaos 重力、CCD、随机方向初速度和角速度。静态/骨骼模型不模拟；原拾取球、模型、标签跟随 Actor。专用服务器无需加载模型。VisualComponent 仍负责描边，但物理模式下不再播放曲线动画。
- 默认阻挡 WorldStatic、PhysicsBody，忽略 Pawn、WorldDynamic（含其他掉落物）。场景对象也必须阻挡 WorldDynamic。固定后关闭物理碰撞、Actor Tick 和检查 Timer；Sphere/静态模型保留 Visibility 查询，继续走原拾取 RPC、实例和库存流程。
- 每 0.05 秒检查：最短模拟时间已过、有静态支撑、线速度与角速度都小于阈值，连续成立一段时间才固定。计时排除同帧 Timer 补调用，空中低速不算落稳。
- 支撑检查使用当前旋转盒体上 1 / 下 3 cm Sweep，法线 Z ≥ 0.65，拒绝穿透并校验落点重叠。超过 Settling Start Time 后，只在有静态支撑时逐渐提高阻尼，离地恢复基础阻尼；仍须连续低速才固定原姿态，不重新放置。
- 仅低于起跳安全地面2000 cm、或连续30秒无静态支撑时触发异常回收。先检查当前位置上50/下1000 cm，再重新检查原安全地面；找不到有效地面则继续模拟，不在无效位置冻结。起跳安全点搜索上200/下1000 cm。持续无支撑的卡住/穿透也可能走这一异常兜底。
- 起点没有安全静态地面时跳过物理、保留原位置和拾取，日志显示 `Drop physics skipped ... no safe static ground`。需检查生成区域，不会启动无限下落。
- 服务器每 0.05 秒发布位置、旋转、开始/固定状态；客户端插值，最终立即应用精确姿态。迟加入/重新相关不重播。只有服务器模拟，客户端不启动 Chaos。
- 永久固定使用世界坐标，不附着移动平台；模拟期间可以撞击 PhysicsBody，但固定只接受静态支撑。只有异常回收可能有可见位置跳变。没有扩展布娃娃、布料或移动平台系统。

## 必要配置

1. 完整编译后重新打开 UE 5.8，打开 `/Game/Items/Loot/BP_WorldItem` 并 Compile，确认根为继承的 `PhysicsBody`。自动化已验证现有 BP 加载、根组件和物理启动；实际 Graph、画面和保存后的组件覆盖值仍待编辑器确认。
2. Class Defaults → **World Item → Drop Physics**：`Enabled = true`（原生默认开启）。不用给 Mesh/SkeletalMesh 勾 Simulate Physics，也不用新增 Graph。不要用 Construction Script/Tick 写 Actor 或模型变换。PhysicsBody 的碰撞、重力、材质、阻尼由 C++ 设置，直接改组件会被运行时覆盖。
3. 已为6件现有物品分别配置 Extent/Center/Rotation。今后更换模型或调整 WorldDropScale/Rotation/Offset 后，在物品定义 Details 中点击 **Fit World Drop Physics**，再 Save。按钮读取模型边界并更新碰撞盒，不修改外观和装备值，也不会自动保存。
4. 地面/墙壁要有碰撞：Object Type = WorldStatic，对 WorldDynamic = Block。需要参与碰撞的可推动物体：Object Type = PhysicsBody，对 WorldDynamic = Block。敌人掉落仍需 NavMesh；测试生成器仍需静态地面。
5. 保留 `WorldItemClass = BP_WorldItem`、`Label.WidgetClass = WBP_WorldItemLabel` 和 Stencil 2 后处理接线。Actor 与 BP 组件 Scale 保持 1，模型尺寸通过 ItemDefinition.WorldDropScale 调整。

### 每件物品的盒体

已由 UE 5.8.2 保存的配置（cm；旋转按 Pitch/Yaw/Roll，四舍五入展示）：

| 物品 | Extent | Center | Rotation |
|---|---|---|---|
| DA_Item_TestSword | (12.33,1.96,61.01) | (21.06,-30.08,-15.00) | (0,35,90) |
| DA_Item_TestMace | (14.88,15.22,52.26) | (-15.32,21.87,-10.06) | (0,35,90) |
| DA_Armor_1730_Head | (9.94,13.33,22.84) | (0.23,0.71,22.84) | (0,0,0) |
| DA_Armor_1730_Chest | (55.26,19.80,46.45) | (0,-108.05,19.80) | (0,0,-90) |
| DA_Armor_1730_Hand | (69.26,8.80,14.75) | (0,-0.16,14.75) | (0,0,0) |
| DA_Armor_1730_Legs | (19.96,15.85,53.95) | (0.05,-53.65,15.85) | (0,0,-90) |

保留了当前视觉旋转，包括用户的胸甲/腿甲躺放设置；数值、词缀、需求未修改。自动化脚本为 `Scripts/fit_drop_physics.py`；初始资产备份与拟合报告位于 `Saved/DropPhysicsFit/20261021_123055/`，最终有向盒报告位于 `Saved/DropPhysicsFit/20261021_123337/`。脚本只用 UE API 保存定义，不保存地图。

盒体沿模型自身坐标轴旋转，避免斜放的剑被巨大轴对齐盒包围。Extent 来自应用 Scale 后的本地模型边界，Rotation 来自 WorldDropRotation，Center 包含 Scale/Rotation/Offset 和骨骼防具原有的底部补偿。自动拟合是单个有向包围盒，不是贴面凸包；锤头/柄、多片手甲之间的空隙仍可能影响接触姿态。

```text
WorldDropPhysicsExtent = 缩放后本地模型边界的半尺寸
WorldDropPhysicsRotation = WorldDropRotation
WorldDropPhysicsCenter = 最终展示边界中心（含原有防具底部补偿）
```

例：沿 X 放平、长100/宽12/厚6 cm、底部 Z=0、XY 居中的剑，Extent=(50,6,3)，Center=(0,0,3)。这是计算示例，不是现有资源的实测值。PIE 用 `show collision` 检查盒体；偏大使外观悬空，偏小使模型穿地。改变视觉比例/旋转/偏移后需重新检查。

运行时 Actor 原点移到盒体中心，朝向变为盒体方向，模型局部变换乘盒体变换的逆，因此初始外观仍遵循旧配置，之后随 Actor 运动。碰撞尺寸已写入定义，专服不需要加载模型。

| ItemDefinition 参数 | 默认 | 运行时边界 |
|---|---|---|
| WorldDropPhysicsExtent | (20,12,8) cm | 每轴1..200；非有限值回退12 |
| WorldDropPhysicsCenter | (0,0,8) cm | 每轴-500..500；非有限向量回退默认 |
| WorldDropPhysicsRotation | (0,0,0) ° | 非有限旋转回退零；自动拟合取模型展示旋转 |

### 通用参数

来源/覆盖顺序：C++ 默认 → BP_WorldItem Class Defaults → 服务器有限值/范围校验。几何参数仅来自物品定义。不读取装备 Weight 或 GAS 数值。

| Drop Physics 参数 | 默认 | 单位、范围及用途 |
|---|---:|---|
| Enabled | true | false 恢复原曲线动画 |
| Upward Speed | 260 | cm/s，0..2000 |
| Horizontal Speed | 80 | cm/s，0..1000，随机水平方向 |
| Angular Speed Degrees | 180 | °/s，0..1440，随机旋转轴 |
| Linear Damping | 0.5 | 0..20 |
| Angular Damping | 3 | 0..20 |
| Friction | 0.7 | 0..1，与接触面按 Max 合并 |
| Restitution | 0.15 | 0..1，与接触面按 Min 合并；地面为0时可不弹跳 |
| Minimum Simulation Time | 0.4 | s，0..10 |
| Stable Time | 0.3 | s，0.05..2 |
| Settling Start Time | 3 | s，上限15，至少 Minimum + Stable；旧序列化名 MaximumSimulationTime 保留，但不再是硬截止 |
| Settling Blend Time | 0.8 | s，0.1..5，仅接地累计 |
| Settling Linear Damping | 2 | 基础线阻尼..20 |
| Settling Angular Damping | 8 | 基础角阻尼..20 |
| Unsupported Recovery Time | 30 | s，10..120，连续无静态支撑才异常回收 |
| Recovery Fall Distance | 2000 | cm，500..10000，低于安全地面的距离 |
| Linear Sleep Threshold | 5 | cm/s，0..100 |
| Angular Sleep Threshold Degrees | 10 | °/s，0..360 |
| Collide With Physics Bodies | true | 是否阻挡 PhysicsBody；不改变 Pawn/其他掉落物忽略规则 |

增大 Upward Speed 会弹得更高；增大 Angular Damping/Friction 可减小滚动；Angular Speed Degrees=0 不施加初始旋转。需要更早收敛可减小 Settling Start Time；需要柔和过渡可增大 Settling Blend Time，不会在空中加强阻尼。旧 VisualComponent 的 Rise/Fall/Bounce/Scale/Rotation 只在 Enabled=false 生效；Highlight 两种模式均有效。

## 验证与人工 PIE

Session Frontend → Automation：运行 `Umbra.Loot.DropPhysics`。相关回归：`Umbra.Loot.Armor1730Definitions`、`Umbra.Loot.VisualMeshCompatibility`、`Umbra.Inventory.Transfer`、`Umbra.Equipment.RequirementsAndLifecycle`。本次结果见 [Progress](Progress.md)。

1. 独立测试地图放 `BP_LootTest_Armor1730`，另加静态武器。检查弹出、翻转、墙地碰撞、稳定后固定并保持10秒不动。刻意让物品在3秒时仍在空中/运动，确认没有突然复位；不再要求3秒内固定。
2. 平地、斜坡、台阶、墙角、悬崖和高速弹出检查穿透/回收；无静态地面有明确警告，超时不在半空冻结。
3. 模拟中/固定后检查标签始终位于模型上方、正向显示，静态武器剑尖/锤头 Hover、骨骼/静态描边、远近拾取、背包满时保留物品。
4. 弹出中立即拾取或销毁，确认没有 Timer 错误、重复入包或 GUID 变化。角色经过和其他掉落不推动已固定物品。
5. Listen Server 双人抢同件物品，检查唯一拾取、落点一致；延迟/丢包后最终收敛；迟加入/重新相关不重播；单独检查专用服务器。
6. Enabled=false 回归旧曲线；无模型、加载延迟/失败仍可拾取，加载完成后贴合当前姿态，不从出生位置重新播放。

自动化物理 World 测试不能替代实际 PIE、网络和渲染验收。
