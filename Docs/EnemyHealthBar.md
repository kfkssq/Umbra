# 敌人头顶血条：UE Editor 配置与验收

## 代码与资产状态
已实现 UUmbraEnemyHealthBar、UUmbraEnemyHealthBarComponent，并在 AUmbraEnemyCharacter 创建 HealthBarComponent。
UMG 依赖沿用现有模块。血条仅监听敌人复制的 Health/MaxHealth，不写属性，不参与伤害。
组件为 Screen Space，默认 Draw Size 120×12、相对位置 Z=120cm。专用服务器跳过 InitWidget 创建。
WBP_EnemyHealthBar 尚未创建，敌人 Widget Class 尚未配置；需要以下手动操作。

## 创建 Widget
1. 关闭编辑器，完成 UmbraEditor Win64 Development 编译后再打开；新增反射类型建议完整重启，不依赖 Live Coding。
2. Content Browser 创建目录 /Game/UI/Enemy。
3. 右键 User Interface -> Widget Blueprint，父类选择 UmbraEnemyHealthBar。
   如果对话框只列 User Widget，可先创建，再在 Class Settings -> Parent Class 改为 UmbraEnemyHealthBar。
4. 命名 WBP_EnemyHealthBar，完整路径 /Game/UI/Enemy/WBP_EnemyHealthBar。
5. Designer 删除自动 Canvas Panel（若有），从 Palette 拖 Size Box 为根，按下列层级制作：

SizeBox（名 HealthBarSize，不勾 Is Variable）
└─ Border（名 HealthBarBackground，不勾 Is Variable）
   └─ Progress Bar（名 HealthProgressBar，必须勾 Is Variable）

只有 HealthProgressBar 名字必须匹配 BindWidget，类型必须为 Progress Bar。
不需要 Canvas、锚点、按钮、文本或全屏覆盖控件。

## 控件设置
- HealthBarSize：Width Override=120、Height Override=12。
- Border 在 SizeBox Slot：Horizontal/Vertical Alignment=Fill。
- HealthBarBackground：Padding=2；Brush Color=(0.03,0.03,0.03,0.85)，Brush Draw As=Box。
- HealthProgressBar 在 Border Slot：Horizontal/Vertical Alignment=Fill。
- Progress Bar：Bar Fill Type=Left to Right，Is Marquee=false，Percent预览值=1。
- Fill Color and Opacity=(0.8,0.03,0.03,1)。
- Style -> Background Image：Tint=(0.08,0.08,0.08,1)；Fill Image Tint=白色，避免与 Fill Color 重复染色。
- Widget 的 Is Focusable=false；Visibility 设置 Not Hit-Testable (Self & All Children)。
  C++每次刷新也强制整棵Widget不可命中；归零用Collapsed。
- 不对 Percent 或 Visibility 创建 UMG Bind，不添加 Event Tick。
- 不在 Graph 绑定ASC或设置Percent。C++自动设置目标、立即读取快照并绑定两个属性委托。
- Compile、Save。缺少 HealthProgressBar 时按编译器的 required widget binding 错误检查名称、类型和父类。

## 配置敌人
1. 打开 /Game/Blueprints/Enemies/BP_Enemy_Melee_01（或其他继承 UmbraEnemyCharacter 的敌人蓝图）。
2. Components 树选中继承的 HealthBarComponent。
3. Details -> User Interface：
   Widget Class = WBP_EnemyHealthBar；
   Space = Screen；
   Draw Size X=120、Y=12；
   Draw at Desired Size=false；
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
8. 血条不出现：先检查Widget Class、HealthProgressBar绑定、组件Visible/Hidden in Game、Health是否>0；
   再用F2确认目标Health/MaxHealth，检查World空间位置Z与实际使用的敌人蓝图。
9. MaxHealth<=0或非有限数值时比例安全返回0，正常GAS边界下MaxHealth最低1。

## 验证状态
UmbraEditor Win64 Development完整编译与链接成功。
尚未创建WBP资产；未运行实际血条PIE、多人/专用服务器或输入交互验收。
