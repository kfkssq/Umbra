# 敌人头顶血条：UE Editor 配置与验收

## 代码与资产状态
已实现 UUmbraEnemyHealthBar、UUmbraEnemyHealthBarComponent，并在 AUmbraEnemyCharacter 创建 HealthBarComponent。
UMG 依赖沿用现有模块。血条仅监听敌人复制的 Health/MaxHealth，不写属性，不参与伤害。C++ 生成 `FUmbraEnemyHealthBarViewState`，Blueprint 通过 `Apply Enemy Health Bar State` 事件驱动任意原生、材质或复合控件；不再依赖固定子控件名或 `UProgressBar` 类型。
组件为 Screen Space，始终按 WBP 的 Desired Size 布局，相对位置默认 Z=120cm。C++ 不指定血条宽高；专用服务器跳过 InitWidget 创建。
2026-09-16 文件检查确认 `/Game/UI/Enemy/WBP_EnemyHealthBar` 已存在且有用户修改。实际父类、控件树和敌人 Widget Class 引用待编辑器确认；按以下契约核对现有资产，不重复创建。总配置入口见 [EditorSetup](EditorSetup.md)。

## 核对现有 Widget（创建步骤仅供缺失时参考）
1. 关闭编辑器，完成 UmbraEditor Win64 Development 编译后再打开；新增反射类型建议完整重启，不依赖 Live Coding。
2. Content Browser 创建目录 /Game/UI/Enemy。
3. 右键 User Interface -> Widget Blueprint，父类选择 UmbraEnemyHealthBar。
   如果对话框只列 User Widget，可先创建，再在 Class Settings -> Parent Class 改为 UmbraEnemyHealthBar。
4. 命名 WBP_EnemyHealthBar，完整路径 /Game/UI/Enemy/WBP_EnemyHealthBar。
5. Designer 删除自动 Canvas Panel（若有），从 Palette 拖 Size Box 为根；内部可以使用原生 Progress Bar，也可以嵌套 Fantasy GUI 的材质进度条。例如：

SizeBox
└─ Fantasy GUI Progress Bar（勾 Is Variable，例如 FantasyHealthBar）

6. Graph 实现 `Event Apply Enemy Health Bar State`，拆分 State，把 `Health Normalized` 传给内部控件公开的 `SetProgressValue`/`SetNormalizedValue`。该值由 C++ 校验并限制为0..1；不要传原始 Health，不要在 Blueprint 重算战斗数值。
7. 材质进度条应在 Construct 创建动态材质后应用缓存值，避免初始刷新早于材质初始化时丢失。供应商控件没有安全公开接口时，在 `/Game/UI/Umbra/Common` 创建 Adapter/副本，不修改 `fantasy_gui_4` 原件。

## 控件设置
- 尺寸唯一配置入口：WBP 根 `SizeBox` 的 Width Override / Height Override，单位为 UMG 逻辑单位。宽高由美术布局决定，C++ 没有数值默认值。Designer 预览尺寸选择 Desired（期望大小），不要把预览画布尺寸当成控件尺寸。
- 组件构造和 OnRegister 都启用 Draw at Desired Size；注册时覆盖旧蓝图/关卡实例保存的 false，使旧 Draw Size 不再挤压布局。不要通过组件 Draw Size 调整血条，修改实际 Widget Class 对应 WBP 的根 SizeBox。
- 预览和运行使用相同的期望布局；运行时仍受视口 DPI 缩放影响，不保证截图物理像素数相同。
- Border 在 SizeBox Slot：Horizontal/Vertical Alignment=Fill。
- HealthBarBackground：Padding=2；Brush Color=(0.03,0.03,0.03,0.85)，Brush Draw As=Box。
- 原生 Progress Bar 可使用 Bar Fill Type=Left to Right、Is Marquee=false、Percent预览值=1；材质控件则确认其标量参数 `Value` 使用0..1。
- Widget 的 Is Focusable=false；Visibility 设置 Not Hit-Testable (Self & All Children)。
  C++每次刷新也强制整棵Widget不可命中；归零用Collapsed。
- 不对 Percent 或 Visibility 创建 UMG Property Binding，不在 Graph 绑定ASC或每帧读取属性。C++自动设置目标、立即读取快照并绑定两个属性委托。
- Fantasy GUI 当前材质控件的 Tick 只用于边缘发光衰减；它不是血量更新来源。敌人数量增加前应改为短时 Timer/UMG Animation，或在收敛后停止 Tick。
- Compile、Save。若找不到 `Apply Enemy Health Bar State`，检查父类并在完成 C++ 编译后重启编辑器。

## 配置敌人
1. 打开 /Game/Blueprints/Enemies/BP_Enemy_Melee_01（或其他继承 UmbraEnemyCharacter 的敌人蓝图）。
2. Components 树选中继承的 HealthBarComponent。
3. Details -> User Interface：
   Widget Class = WBP_EnemyHealthBar；
   Space = Screen；
   Draw at Desired Size=true（C++ 注册时强制启用）；Draw Size 不再作为血条尺寸来源；
   Pivot=(0.5,0.5)，Window Focusable=false。
4. Details -> Transform -> Location=(0,0,120)。该位置相对胶囊中心，按模型高度调整 Z。
5. Collision 保持 NoCollision；Visible=true，Hidden in Game=false。
6. 不再手动添加另一个 Widget Component，不在 BeginPlay 中 Create Widget 或 Add to Viewport。
7. Compile、Save。若组件未出现，确认蓝图父类，关闭编辑器重新编译后再打开。
8. 修改关卡实例时注意其覆写值优先于蓝图默认值。
蓝图 OnDeathStarted 不要 Destroy Component 或隐藏整个 Actor；这会阻止后续恢复正生命时重新显示。

## 按顺序 PIE 验收
1. 放置两个 BP_Enemy_Melee_01，关闭 Enable AI Behavior，各自启用 Debug Initial Attributes，Max Health 分别100和200。
2. PIE应各自显示满条。F2锁定其中一个，用固定10点伤害按钮：
   最大生命100的应变90%，最大生命200的应变95%，其他敌人不变。
3. 治疗10，恢复满条；正常攻击时同步减少。鼠标穿过血条仍能选择/高亮/攻击敌人。
4. 测试最大生命：100/100时添加测试效果（最大生命+100），应变100/200=50%，不回血；
   治疗到110/200=55%，移除效果后裁剪到100/100=100%。
5. 致死到0时隐藏。测试恢复显示前把 Corpse Lifetime 设0，保留敌人Actor；
   使用已有服务器Instant治疗GE把Health恢复正数，应重新显示。
   现有死亡逻辑不复活AI、碰撞或动画；血条重新出现不等于角色复活。
   若调试面板拒绝死亡目标治疗，需用测试能力给该ASC应用治疗GE，不能直接写AttributeSet。
6. 销毁敌人，应无悬空回调；多个敌人分别受伤互不影响。
7. 两客户端+专用服务器PIE，分别观察同一敌人伤害/治疗后比例一致；
   专用服务器无需Widget。此项尚未验证。
8. 血条不出现：先检查Widget Class、`Apply Enemy Health Bar State` 是否实现、材质动态实例与 `Value` 参数、组件Visible/Hidden in Game、Health是否>0；
   再用F2确认目标Health/MaxHealth，检查World空间位置Z与实际使用的敌人蓝图。
9. MaxHealth<=0或非有限数值时比例安全返回0，正常GAS边界下MaxHealth最低1。

## 验证状态
2026-09-16 使用项目关联的 UE 5.8 完成 `UmbraEditor / Win64 / Development` 构建；UHT、`UmbraEnemyHealthBar.cpp` 编译与模块链接成功。
当前 WBP 已存在但本次未直接编辑二进制资产；`Apply Enemy Health Bar State` Graph、材质参数、实际血条 PIE、多人/专用服务器及输入交互仍待编辑器确认。见 [Progress](Progress.md)。

## 2026-09-16 已确认 Bug：有外框但没有红色填充

- 现象：Designer 显示正常，PIE 只有背景/边框，满血和受伤均无红条。
- 证据：`Saved/Logs/Umbra.log` 20:04:30 的 `WBP_EnemyHealthBar_01_C_0` 打印1.0，同帧 `SRetainerWidget` 报 `W:98 H:0`；此前受伤打印0.86，说明 GAS → Blueprint 状态传递正常。用户修改尺寸/内边距后已确认该诊断。
- 原因：旧组件固定分配120×12，与嵌套进度条的上下各12 Padding 冲突，导致内部 Retainer Box 实际绘制高度为0。设计器较大的预览画布没有暴露此问题。
- 修复：组件始终采用 WBP Desired Size；宽高在根 SizeBox 配置。仍需保证根高度大于各层上下内边距之和；自动尺寸不能修复 WBP 自身互相冲突的尺寸约束。
- 独立隐患：截图中的 Construct 把 Set Progress Value 写死为1，应接 CachedHealthPercent；材质创建完成后应用缓存，并以有效动态材质为就绪条件。旧日志的 DynamicMaterial 为空与本次零高度问题要分别排查。此蓝图接线本次未修改。
- 回归步骤：使用实际敌人的 Widget Class，在其 WBP 根 SizeBox 改两组宽高，Desired 预览和 PIE 都应采用相同布局；正常伤害后按比例减少，日志无 `H:0`。本次构建与视觉验证结果见 Progress。
