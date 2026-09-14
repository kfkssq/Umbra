# 伤害漂字：编辑器操作与验收
## 已实现
UUmbraDamageNumber 用 NativeTick/DeltaTime 和累计时间驱动出现、停留、淡出；跨阶段大DeltaTime保留超出时间。
每次只随机一次角度和距离；以屏幕正上方为中心，在Spread Angle Degrees总张角内均匀采样。移动与缩放各自用SmoothStep，移动结束后的停留/淡出时间表保持不变。
每个命中独立Spec携带Damage.Type与Damage.ResultCritical；IncomingDamage清零扣血入口发送计算伤害，过量伤害不按生命差裁剪。
仅攻击者PC收到Unreliable客户端RPC；单机/Listen走同一个创建入口。丢包可丢失表现，不影响伤害。
没有自定义EffectContext，无需配置AbilitySystemGlobals。固定10点调试按钮没有执行结果和攻击者归属，不产生漂字，扣血行为不变。

## 需要手动创建的唯一资产
路径 /Game/UI/Combat/WBP_DamageNumber
类型 Widget Blueprint，父类 UmbraDamageNumber。
关闭编辑器完成UmbraEditor编译后重新打开；右键User Interface -> Widget Blueprint选择该父类。
如果先选了UserWidget，在Class Settings -> Parent Class改为UmbraDamageNumber。

Designer完整层级：
WBP_DamageNumber
└─ Text Block：DamageText（根控件，必须勾Is Variable）

删除自动Canvas Panel（若有），直接以Text Block为根，不设置全屏画布或固定SizeBox。
DamageText名字和类型必须匹配BindWidget。
字体：选择支持数字与!的字体，例如项目默认Roboto Bold，字号28，Justification=Center。
Outline Size=1~2、黑色；可选黑色Shadow Offset=(1,1)，透明度0.5。
关闭Auto Wrap Text，不设置宽度限制。文字示例123!仅用于预览。
Visibility=Not Hit-Testable (Self & All Children)，Is Focusable=false。
Widget Class Defaults -> Tick Frequency保持Auto，不设Never。
不要创建UMG动画、Timeline、Percent/Text/Opacity绑定或蓝图Tick。
C++自动绑定DamageText，设置数字、颜色、透视投影后的位置、中心对齐、缩放、透明度并销毁。
颜色由Class Defaults中的Physical Color（白）/Magical Color（淡蓝）配置，Designer颜色会被运行时覆盖。
原始正伤害<1显示一位小数，其余显示整数（显示舍入，不改伤害）；暴击后缀!。

## 动画参数
WBP_DamageNumber -> Class Defaults -> Damage Number：
Initial Scale=0.4
Normal Scale=1
Critical Scale=1.3
Appear Duration=0.25秒（原字段保留，控制移动时长）
Pop Duration=0.08秒（仅缩放；最小0.001秒）
Spread Angle Degrees=120（总张角，0~360；0正上，120左右各60度，360全方向）
Min Distance=40、Max Distance=90（UMG逻辑单位）
Hold Duration=0.3秒
Fade Duration=0.3秒
Physical Color=(1,1,1,1)
Magical Color=(0.45,0.75,1,1)
所有参数可在此调整。0时长阶段直接跳过；无对象池。
Compile并Save。

## PlayerController
打开 /Game/Blueprints/Player/BP_UmbraPlayerController -> Class Defaults。
搜索Damage Number Class，位于UI / Damage Numbers。
选择WBP_DamageNumber，Compile并Save。
不在蓝图中Create Widget或Add to Viewport，不另接伤害事件；避免重复显示。
无需新增GE、输入资产或敌人WidgetComponent。
投影使用Mesh Bounds中心（无Mesh回退Actor），不使用头顶组件；投影到本地玩家子视口，像素除DPI一次，SetPositionInViewport关闭再次除DPI。
Widget使用AddToPlayerScreen，中心对齐；出现后位置是固定屏幕坐标，不再引用敌人。
中心在视口外或镜头后不创建。

## PIE验收
1. 设置上述资产引用；被测敌人关闭AI。打开umbra.Damage.Log 1。
2. 普通物理攻击：白数字从Mesh中心出现，0.08秒内从0.4缩放到1，同时在向上120度扇形内随机移动；移动0.25秒后停留0.3秒，再0.3秒淡出。
3. Magical类型：淡蓝；暴击率1/倍率2：数字后带!且最大缩放1.3。与日志Final按显示精度比较。
4. 将敌人生命降到10，单次计算伤害30：漂字30，不是10。
5. 测试0伤害不生成，正伤害0.5显示0.5。连招每次命中各生成一个，不能同一命中重复。
6. 致死后立即销毁敌人，已生成漂字仍独立完成；离开关卡PC EndPlay清理剩余Widget。
7. 30/60/120 FPS和低帧大DeltaTime下，三阶段总时长相同；不同分辨率/DPI及窗口尺寸下从中心出现。
8. 屏外或镜头后敌人不生成；多个客户端仅攻击者看到，Listen主机命中只显示一次。
9. 检查敌人血条与调试面板仍刷新，漂字区域不拦截攻击/移动。
10. 不显示时检查实际GameMode使用的PlayerController类及DamageNumberClass；WBP编译检查DamageText绑定；伤害是否由玩家且通过Execution结算。

## 验证范围
WBP资产和PC引用尚需用户手动配置。实际视觉、帧率/DPI、过量伤害漂字、Listen/多人表现与鼠标交互尚未PIE验证。

本次扇形/弹出调整已通过UmbraEditor Win64 Development完整编译；尚未PIE视觉验证。重新打开WBP_DamageNumber -> Class Defaults -> Damage Number，设置Spread Angle Degrees及Pop Duration，Compile/Save。Pop Duration不改变总生命周期，若配置得超过生命周期，Widget仍按原移动+停留+淡出时间移除。

