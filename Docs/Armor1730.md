# 1730 防具世界掉落

本次仅接入定义和世界显示，不处理角色穿戴外观。源码复用现有槽位、实例工厂、拾取 RPC、PlayerState 转移、Inventory 快照与 GAS。Feet 保留，本套腿甲只占 Legs。

## 数据资产

在 Content Browser → Miscellaneous → Data Asset 选择 `UmbraItemDefinition`，保存到 `/Game/Items/Armor`；也可启用编辑器 Python Editor Script Plugin 后，通过 Tools → Execute Python Script 执行 `Scripts/create_armor_1730.py`。必须先完整编译并重启编辑器。脚本先加载并检查全部模型为 SkeletalMesh，只创建缺失资产，已有同名资产保留不覆盖。

| 资产名 | DisplayName | AllowedSlots（仅一项） | Armor | MagicResistance | Weight | WorldDropSkeletalMesh |
|---|---|---|---:|---:|---:|---|
| DA_Armor_1730_Head | 1730 Helmet | Head | 8 | 4 | 3 | /Game/Asset/Equipments/Head/head_01 |
| DA_Armor_1730_Chest | 1730 Chest Armor | Chest | 20 | 8 | 8 | /Game/Asset/Equipments/Body/body |
| DA_Armor_1730_Hands | 1730 Gauntlets | Hands | 6 | 3 | 2 | /Game/Asset/Equipments/Arm/arm |
| DA_Armor_1730_Legs | 1730 Leg Armor | Legs | 12 | 5 | 5 | /Game/Asset/Equipments/Legs/legs |

统一 ItemCategory=Armor、RequiredLevel=1、Requirements/PrimaryBonuses 全零；AttributeBonuses、DamageBonuses、AffixRollConfigs 为空；Weapon=None。数值使用现有原生单位，仅供测试；不要再把 Armor/MagicResistance 重复填进词缀。

以上路径来自当前工作区文件检查，创建脚本已通过 UE 加载验证四件均为 SkeletalMesh，材质依次为 M_HD_tree、M_BD_tree、M_AM_tree、M_LG_tree。来源为用户导入的《艾尔登法环》资源；本次不改变来源/许可记录，不迁移模型或材质。文件名本身不证明骨骼、材质与 Blueprint 内容，未加载内容仍待编辑器确认。

## 世界模型与 Blueprint

- 复用 `/Game/Items/Loot/BP_WorldItem`，父类应为 UmbraWorldItem，Label.WidgetClass 应为 WBP_WorldItemLabel；已通过 UE 加载确认父类及 Widget 引用；Graph 和视觉效果仍待编辑器确认。不需增加生成、AddItem 或拾取 Graph。
- WorldDropSkeletalMesh 有值时优先；为空时沿用 WorldDropMesh 静态模型。加载失败保留原拾取球和标签，不会悄悄换成另一个模型。
- 两种模型共享 WorldDropScale/Rotation/Offset。配置来源为 ItemDefinition，覆盖组件默认变换；Scale 无单位，Rotation 为度，Offset 为 cm。初始 Scale=(1,1,1)、Rotation=(0,0,0)、Offset=(0,0,0)，骨骼参考姿势、原材质、不绑定角色动画。
- 骨骼模型将变换后资产包围盒底部对齐掉落 Actor 原点，再加 Offset.Z；原点离地高度由原掉落生成流程负责。静态武器原有变换规则不变。斜坡、模型包围盒与动画旋转中的地面穿插需 PIE 检查；必要时调整该定义的旋转/缩放/正 Z 偏移。
- SkeletalMesh 无碰撞、无模拟、无导航影响。原根球仍负责鼠标命中，骨骼表面超出球范围的部分不保证可点击（不依赖未配置的 PhysicsAsset）。测试时将模型调整为根球附近的紧凑显示。静态武器保留原 Visibility 表面查询。
- VisualComponent 保留原落下动画及 CustomStencilValue=2，敌人继续使用1；Config 已有 `r.CustomDepth=3`。实际后处理材质是否支持2、是否挂到当前相机/Volume，待编辑器确认。

## 测试生成与验收

1. 将已创建的 `/Game/Items/Loot/BP_LootTest_Armor1730` 拖入独立测试地图的开阔可行走区域（需要静态地面碰撞和 NavMesh）。也可放置原生 `UmbraLootTestSpawner` 手工配置。
2. 测试 Blueprint 已预填四件 DA 和 BP_WorldItem；手工配置时在 Details → Loot Test：Items 依次加入四件 DA，WorldItemClass 选现有 BP_WorldItem。可加第五件现有 DA_Item_TestSword 或 DA_Item_TestMace 作回归。空 WorldItemClass 使用原生类，没有配置好的 WBP 标签。
3. 生成器沿自身右向每100cm放一件，向上下500cm找静态地面，离地2cm；地面坡度过大/找不到地面/定义无效则日志提示并跳过。只在非 Shipping/Test 构建、服务端 BeginPlay 一次生成。每次重新 PIE 是新测试批次；不要在正式地图放置。
4. PIE 查看 Output Log，确认四件各一个、各自模型和原材质、最终位置不陷地。悬停根球附近验证标签和 Stencil2 描边，离开恢复；同时检查敌人 Stencil1。
5. 记录每件 GetItemInstance.InstanceId，逐件点击拾取；地面 Actor 消失、背包增加对应名称，GUID 不变、词缀为空。重复点击不能再次入包。
6. 背包满时物品保留，空出格子后可拾取；近距离、远距离自动接近都检查。开双人 Listen Server 测试同时抢一件，只能一人成功。
7. 第五件静态武器检查原模型、材质、表面 Hover、掉落动画、拾取和装备功能。四件装备分别装入对应槽位；满足需求时固有防御合计 Armor46、MagicResistance20、Weight18，实际总属性需扣除角色/其他装备基值。
8. Session Frontend → Automation：运行 `Umbra.Loot`、`Umbra.Inventory.Transfer` 和 `Umbra.Equipment.RequirementsAndLifecycle`。新增测试验证真实定义/模型可加载、唯一 GUID、拒绝二次初始化、实际 TryPickup→PlayerState→Inventory 转移/销毁/拒绝再次拾取及两种组件描边切换；不等于完整鼠标拾取、网络或渲染验收。

敌人已有 `UmbraLootTable`。如需手工试敌人掉落，在测试敌人专用表 Entries 加四件定义，设置各自 Weight/DropChance/MinCount/MaxCount，并指定 WorldItemClass。现有规则每次选一个条目，因此单次死亡不保证四件全掉。本次不修改正式敌人/掉落表概率。

## 本次验证

具体编译、资产创建和测试执行结果记录于 [Progress](Progress.md)。未执行的 PIE、视觉、Blueprint 和多人步骤不得视为通过。

## 源码交付清单

- 修改 `Source/Umbra/Items/UmbraItemDefinition.h`：新增可选骨骼世界模型软引用。
- 修改 `Source/Umbra/Loot/UmbraWorldItem.h/.cpp`：骨骼组件、异步加载分流、底部高度补偿及标签包围盒。
- 修改 `Source/Umbra/Loot/UmbraWorldItemVisualComponent.h/.cpp`：静态/骨骼共用原动画和 CustomDepth 生命周期。
- 新增 `Source/Umbra/Loot/UmbraLootTestSpawner.h/.cpp`：非正式地图一次性权威测试生成。
- 新增 `Source/Umbra/Tests/UmbraArmorLootTests.cpp`、`UmbraLootTestTypes.h`：真实资产及实际拾取/身份/销毁、视觉组件兼容回归。
- 新增 `Scripts/create_armor_1730.py`：通过编辑器创建并保存四件定义和测试 Blueprint，检查资源类型/材质/既有 BP 引用；重复运行保留已有资产。
