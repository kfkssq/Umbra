# 伤害漂字：编辑器操作与验收
## 已实现
UUmbraDamageNumber 用 NativeTick/DeltaTime 和累计时间驱动出现、停留、淡出；跨阶段大DeltaTime保留超出时间。
每次只随机一次角度和距离；以生成时屏幕正上方为中心，在Spread Angle Degrees总张角内均匀采样。按生成时相机和DPI，把屏幕位移转换成经过命中位置、平行于相机画面的世界平面上的固定终点。移动与缩放各自用SmoothStep，移动结束后的停留/淡出时间表保持不变。
每个命中独立Spec携带Damage.Type与Damage.ResultCritical；IncomingDamage清零扣血入口发送计算伤害，过量伤害不按生命差裁剪。
路由已提取至 `Source/Umbra/AbilitySystem/Damage/UmbraDamageNotification.h/.cpp`：Capture 在扣血前保存位置与接收者，Dispatch 在扣血后发送。AttributeSet 只保留结算与通知调用，不再访问具体 Enemy/Mesh/PlayerController。
仅攻击者PC收到Unreliable客户端RPC；单机/Listen走同一个创建入口。丢包可丢失表现，不影响伤害。
没有自定义EffectContext，无需配置AbilitySystemGlobals。固定10点调试按钮没有执行结果和攻击者归属，不产生漂字，扣血行为不变。

## 核对现有资产
2026-09-16 文件检查确认下列 WBP 已存在且有用户修改；父类、控件树、Class Defaults 和 PC 引用待编辑器确认。以下创建步骤仅供缺失时参考，不重复创建。总入口见 [EditorSetup](EditorSetup.md)。
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
显示格式由 Class Defaults 的缩写参数控制；默认小于1000显示整数，达到1000后固定一位小数并使用 k/M/B/T；暴击后缀仍为 `!`。格式化只影响文字，不修改实际伤害或字号选档。

## 动画参数
WBP_DamageNumber -> Class Defaults -> Damage Number：
Initial Scale=0.4
Normal Scale=1
Critical Scale=1.3
Appear Duration=0.25秒（原字段保留，控制移动时长）
Pop Duration=0.08秒（仅缩放；最小0.001秒）
Spread Angle Degrees=120（总张角，0~360；0正上，120左右各60度，360全方向）
Min Distance=40、Max Distance=90（生成时的UMG逻辑单位；仅生成时转换为世界位移，之后不随相机重新计算终点）
Hold Duration=0.3秒
Fade Duration=0.3秒
Physical Color=(1,1,1,1)
Magical Color=(0.45,0.75,1,1)
所有参数可在此调整。0时长阶段直接跳过；无对象池。
Compile并Save。

## 字号分档与数字缩写

`WBP_DamageNumber -> Class Defaults -> Damage Number / Style`：

- `Font Size Tiers` 每项配置 `Damage Lower Bound`、`Min Font Size`、`Max Font Size`；下一项下界自然构成上一项上界。
- 整个数组及每项字段均为 `BlueprintReadWrite`，可在 Widget Blueprint 的 Class Defaults 中直接编辑，也可在蓝图逻辑中读写；若升级原生类后面板未出现，关闭 Editor 完成一次非 Live Coding 编译，再重新打开 WBP。
- 原生默认依次为 `[0,1000):16–18`、`[1000,1000000):18–20`、`[1000000,1000000000):20–22`、`[1000000000,+∞):22–24`。
- `Critical Font Size Multiplier=1.15`，`Max Final Font Size=28`。先在档内随机一次，再乘暴击倍率并限制上限；现有 `Critical Scale=1.3` 仍是弹出动画缩放，两者职责不同。
- 运行时会排序档位、让同下界的最后一个有效配置生效、交换写反的最小/最大字号并忽略负下界、非有限数或非正字号；空配置回退 `DamageText` 当前字号。

`Damage Number / Formatting`：

- `bEnableDamageAbbreviation=true`、`AbbreviationStartValue=1000`、`AbbreviationDecimalPlaces=1`。
- `AbbreviationUnits` 默认 `1000:k`、`1000000:M`、`1000000000:B`、`1000000000000:T`。运行时排序、去重并忽略非法阈值/空后缀。
- 舍入先检查是否应提升到下一单位，所以 `999960` 显示 `1.0M`，不会显示 `1000.0k`。字号档仍按999960的实际值选择，不按格式化结果重新选档。

## PlayerController
打开 /Game/Blueprints/Player/BP_UmbraPlayerController -> Class Defaults。
搜索Damage Number Class，位于UI / Damage Numbers。
选择WBP_DamageNumber，Compile并Save。
不在蓝图中Create Widget或Add to Viewport，不另接伤害事件；避免重复显示。
无需新增GE、输入资产或敌人WidgetComponent。
起点使用命中时Mesh Bounds中心（无Mesh回退Actor），不使用头顶组件，也不继续引用敌人。Start保存世界起点和世界位移，NativeTick按当前相机投影当前位置到本地玩家子视口，像素除当前DPI一次，SetPositionInViewport关闭再次除DPI。
Widget使用AddToPlayerScreen，中心对齐；移动结束后停留在固定世界终点，角色移动、镜头平移/旋转/缩放时仍贴合该场景位置；文字字号沿用UI缩放参数，不做场景遮挡。
生成时中心在视口外或镜头后不创建。生成后离屏/转到镜头后将透明度置0，保留最后可见屏幕位置并继续计时，避免Hidden/Collapsed或移出画布导致Tick停止；回到视野恢复当前阶段透明度，超时正常移除。

## PIE验收
1. 设置上述资产引用；被测敌人关闭AI。打开umbra.Damage.Log 1。
2. 普通物理攻击：白数字从Mesh中心出现，0.08秒内从0.4缩放到1，同时在向上120度扇形内随机移动；移动0.25秒后停留0.3秒，再0.3秒淡出。
3. Magical类型：淡蓝；暴击率1/倍率2：数字后带!，字体基准乘1.15且不超过28，弹出缩放仍到1.3。与日志Final按显示精度比较。
4. 将敌人生命降到10，单次计算伤害30：漂字30，不是10。
5. 测试0伤害不生成；999/1000/999960/1000000分别显示`999`/`1.0k`/`1.0M`/`1.0M`。连招每次命中各生成一个，不能同一击重复。
6. 把伤害固定在999、1000、999999、1000000、999999999、1000000000附近，多次触发确认只在对应档内随机；同一飘字运动期间字号不跳变。再测暴击倍率及28上限。
7. 致死后立即销毁敌人，已生成漂字仍独立完成；离开关卡PC EndPlay清理剩余Widget。
8. 30/60/120 FPS和低帧大DeltaTime下，三阶段总时长相同；不同分辨率/DPI及窗口尺寸下从中心出现。
9. 屏外或镜头后敌人不生成；多个客户端仅攻击者看到，Listen主机命中只显示一次。
10. 检查敌人血条与调试面板仍刷新，漂字区域不拦截攻击/移动。
11. 不显示时检查实际GameMode使用的PlayerController类及DamageNumberClass；WBP编译检查DamageText绑定；伤害是否由玩家且通过Execution结算。
12. 命中后立刻移动玩家/相机：已有漂字应留在命中附近的场景位置，不粘在屏幕上，也不追随移动的敌人；移动结束后旋转/缩放镜头，终点仍固定。
13. 镜头移开或转向使漂字离屏/在镜头后，再在生命周期内移回：不应闪到屏幕边缘或冻结；生命周期结束后ActiveDamageNumbers应清理。

## 2026-09-16 定位修复

旧入口仅在RPC到达时投影一次，把 `Pixels / DPI` 作为二维 Origin 传给Start；后续Tick只累加屏幕Travel，因此镜头跟随角色移动时漂字仍粘在屏幕。现改为保存世界坐标并逐帧投影，伤害路由、RPC载荷、随机角度/距离参数与动画时间表保持原有契约。本次验证见 Progress。

## 验证范围
WBP 资产已存在；PC 引用及 WBP 内部待编辑器确认。2026-09-17 `Umbra.UI.DamageNumberLogic` 自动化已通过，覆盖默认缩写、999960单位进位、乱序单位、字号档边界/乱序/反向范围、空配置回退、暴击倍率与最终上限。仍未执行实际视觉、随机分布观感、帧率/DPI、过量伤害漂字或 Listen/多人表现 PIE 验证。

历史记录：扇形/弹出调整当时通过 UmbraEditor Win64 Development 完整编译，未 PIE 视觉验证；本次文档整理未重跑。重新打开 WBP_DamageNumber -> Class Defaults -> Damage Number，核对 Spread Angle Degrees 及 Pop Duration。Pop Duration 不改变总生命周期，若配置得超过生命周期，Widget 仍按原移动+停留+淡出时间移除。当前验证汇总见 [Progress](Progress.md)。

