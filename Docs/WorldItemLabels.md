# 掉落物悬停、标签避让与 Tooltip 标题

2026-10-21。本轮只调整展示与鼠标目标选择，沿用原有拾取 RPC、距离检查和库存转移。

## 行为

- Tooltip 顶部 ItemName 固定白色；RarityText 仍按稀有度着色，地面名称、背包/装备框及其他数值颜色不变。
- 骨骼防具模型使用 QueryOnly / Visibility Block / Per Poly Collision，覆盖根部24cm拾取球外的真实模型表面。其余通道 Ignore，不启用模型物理或导航。物理运动继续使用独立 PhysicsBody。
- 四个源 SkeletalMesh（head_01、body、arm、legs）已通过匹配 UE 编辑器启用并保存逐三角形碰撞数据。没有修改骨骼、材质、网格形状、物品数值或地图。修改前备份位于 `Saved/ArmorCursorCollision/`。脚本为 `Scripts/enable_armor_cursor_collision.py`。
- 鼠标在可见 WorldItemLabel 的排布矩形内时，优先选择该标签对应装备，模型和名称共用原 SetHighlighted；点击标签走原拾取/自动接近流程，不会攻击标签背后的敌人。移出、切换或销毁清理高亮。角色菜单与属性面板输入拦截保留。
- 每个本地 PlayerController 将可投影到屏幕内的名称按稳定实例ID顺序排布。测量实际 Widget DesiredSize，使用 DPI 换算后的 Slate 单位，优先保留上一帧有效布局，必要时上下/左右避让；只改标签 RenderTranslation，不移动装备或拾取球。
- 屏幕边缘尽量向内收拢。若当前候选位置都被占用或名称大于视口，暂时隐藏该标签并取消其矩形命中；下一帧重新尝试。模型本身仍可悬停/拾取。该行为防止极端密度下强行叠字，并非无限容量保证。
- 标签仍为 HitTestInvisible；控制器按显示位置做命中检测，不需要在 WBP 中加 Mouse Enter/Leave 或 Click Graph。优先权和点击目标与高亮一致。

## 编辑器配置

1. 完整编译后重新打开项目。现有四件防具已配置碰撞数据，无需重复修改。不要给骨骼模型启用 Simulate Physics。
2. `WBP_WorldItemLabel` 保留原父类 UmbraWorldItemLabel 与 ItemName 绑定，根控件使用正常自动尺寸；避免固定巨大 Canvas、根 RenderScale/Angle 或 Graph 写 RenderTranslation/Opacity，这些会干扰测量或被本地布局覆盖。
3. `BP_PlayerController`（项目所用控制器蓝图）Class Defaults → **Loot → Labels → World Item Label Gap** 可调整间距，默认4 Slate单位，允许0..32。原生默认 → Blueprint Class Defaults → 运行时范围校验；不复制到服务器。保持默认即可使用。
4. 今后新增骨骼掉落模型，在 Skeletal Mesh 编辑器启用 **Enable Per Poly Collision** 并保存，以便打包包含三角形碰撞数据。只在组件上设置是不够的。静态武器沿用自身复杂查询碰撞。
5. 同时保证 Stencil 2 的物品描边后处理已经接线。查询命中与渲染描边是两件事。

## 验证

自动化：`Umbra.Loot.HoverAndLabelLayout` 检查20件同锚点标签不相交、边缘约束、空间不足拒绝叠字、排布矩形解析到对应物品、销毁清理，以及四个真实防具模型上的 Visibility 射线和拾取球外命中。`Umbra.UI.Items.TooltipTitleColor` 检查标题白色、稀有度文字保色、复用/清空。实际执行结果见 [Progress](Progress.md)。

人工 PIE：

1. 让四件防具和剑/锤近距离掉落；名称应散开。缩放/移动相机、窗口尺寸与DPI变化时检查标签仍不相交。
2. 分别悬停头盔上缘、胸甲边缘、手甲两端、腿甲末端，确认模型和名称高亮；再移动到移开的名称上，目标应保持对应，点击仍能接近/拾取。
3. 两件装备相邻时来回移动鼠标，确认只有一个装备高亮；拾取/销毁后原位置不能继续选择旧物品。
4. 打开背包/装备 Tooltip，确认顶部名称白色，稀有度文字与其他颜色不变；打开菜单和属性调试面板检查遮挡输入不穿透。
5. 密集掉落、长名称、视口边缘、物理运动中、无模型、迟加入/客户端分别检查。当前布局面向单本地玩家视口；分屏多人共享 WidgetComponent 的独立布局不在本次验证范围。

无界面自动化不等于真实鼠标、渲染及网络验收。

## 标签稳定性修复（2026-10-21）

排查结果：原 Controller 每帧清空命中矩形，没有布局历史，重复从锚点寻找最近候选。候选用浮点距离严格排序，接近等距时投影像素取整、DPI换算及 DesiredSize 微变可改变选择。原有实例 GUID 排序已稳定，不是 Actor 遍历顺序问题。原代码根据独立世界投影和 DesiredSize 计算位置，没有把 RenderTranslation 累加到下一帧测量；原生高亮只改颜色，不改变字号。蓝图自定义尺寸变化仍需 PIE 检查。逐帧归零 RenderTranslation/Opacity 已移除，避免不必要的显示状态重置。

现在复用 UmbraWorldLabelLayout 和控制器缓存：

- 每个弱引用物品记录 Anchor、TargetPosition、RenderPosition 和有效可见状态；先预留所有有效历史位置，再安置需要修复或新出现的标签。GUID 顺序保持不变。
- 普通标签的目标随锚点差值移动，独立于插值后的显示位置；物理稳定时也持续处理相机投影，不依赖物理固定事件。有效历史位置不会仅因另一个候选稍近就被替换。
- 旧位置无效必须修复；修复候选在距离差不超过 SwitchThreshold 时优先靠近历史目标。候选距离按0.01 Slate单位分桶，平局保持原候选顺序；间距比较容差最多0.01且小于间距，不允许实际矩形重叠。
- 显示使用 1-exp(-Speed*DeltaTime) 插值，接近目标0.01单位时收敛为精确目标。用显示位置创建命中矩形。每帧检查所有显示路径的保守扫掠矩形；可能交叉或越界则整批瞬时切到互不重叠的目标，不让文字穿过彼此。高密度群组因此可能直接调整，不保证所有调整都有动画。
- 鼠标命中上一帧可见标签时，优先预留其实际显示位置。只有视口/尺寸约束已不允许保留时才修复。悬停不改变稳定身份排序。
- 拾取沿用原 Destroy 流程，OnDestroyed 立即删除布局和命中缓存、清除悬停；隐藏/失效物品在下一次布局清理。Controller EndPlay 解绑并释放缓存。

无需修改 WBP 或资产。BP_PlayerController → Class Defaults → Loot → Labels：Gap默认4（0..32）、SwitchThreshold默认12 Slate单位（0..64）、MoveSpeed默认16每秒（1..60）。来源优先级为原生默认 → 蓝图类默认覆盖 → 控制器运行时有限值/范围校验。保留默认即可；不要在 WBP 写入根 RenderTranslation/Opacity，也不需要新增 Tick/Mouse Graph。

自动化追加：20个同锚点连续300帧身份位置不变；静态不漂移；轻微相机左右扰动保持槽位；两个标签接近后不交换；悬停保留显示位置；模拟DPI/视口和尺寸变化；销毁立即清缓存/命中/Hover；每个被测移动帧可见矩形不重叠；独立标签30/60Hz插值等价。DPI测试通过Slate坐标输入模拟，不代表真实Windows DPI切换已验收。

手动PIE专项验收：同点掉落20件并静止观察至少5秒；悬停标签保持鼠标不动并观察名称和描边；缓慢移动/缩放相机、拖动窗口边缘及改变DPI；在平滑移动中的名称上点击，确认选择对应物品；让两个物品靠近、密集掉落至隐藏再拾取释放空间；拾取/销毁悬停物品后确认旧位置不再命中。检查物品弹出期间标签跟随、落稳后不抖动，尤其是屏幕边缘与长名称。以上真实画面/鼠标行为仍待人工验收。
