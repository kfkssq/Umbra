# 编辑器属性调试
关闭编辑器后编译 UmbraEditor Win64 Development，再重新打开。

1. 玩家：打开 /Game/Blueprints/Player/BP_UmbraPlayerState，点击 Class Defaults。
2. 敌人：打开 /Game/Blueprints/Enemies/BP_Enemy_Melee_01，点击 Class Defaults；也可选中关卡中的敌人实例。
3. 搜索 Debug Attributes，勾选 Use Debug Initial Attributes。
4. 展开 Debug Initial Attributes，直接修改各项浮点数，Compile 并 Save。
5. 重新开始 PIE。仅服务器首次初始化应用这些数值；重新绑定 ASC 不会重新应用。
6. 攻速加成、暴击率使用 0.2 = 20%；暴击倍率 2 = 两倍；恢复点/秒，移速厘米/秒，急速数值。
7. Health 和 Resource 初始化时分别填满 Max Health 和 Max Resource，因此不提供独立初始当前值输入。需要部分生命时使用现有伤害调试按钮。
8. 开关开启时，在 Initial Attributes Effect 之后通过 Instant GE Override 应用全部调试字段；关闭时沿用原初始化。Shipping 忽略该调试配置。
9. 玩家配置不生效时检查实际 GameMode 的 Player State Class 是否为上述蓝图。敌人检查关卡实例是否覆盖了蓝图默认值。
10. 此入口供开始 PIE 前调参，不支持 PIE 中直接编辑运行时 GAS 数据。运行时修改使用调试面板或 Gameplay Effect。

AttributeSet 的 FGameplayAttributeData 字段仅供蓝图读取，不作为编辑器数值入口。GAS 的 C++ 属性访问宏本身不生成蓝图 UFUNCTION；蓝图可通过 ASC 的 Get Float Attribute 等接口读取数值。PlayerState 和 Enemy 的 Get Attribute Set/Get Umbra Ability System Component 为显式蓝图纯函数。

验证：见本次交付的编译结果；尚未在 UE 编辑器确认控件展示或运行 PIE。
