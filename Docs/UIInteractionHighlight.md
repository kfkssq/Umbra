# 通用 UI 交互高亮：单个 HighlightFrame

2026-09-30：Inventory已复用此材质契约，见 [空背包完整接线](InventoryPhase1.md)。EquipmentSlot空槽的RarityFrame由C++在视觉事件返回后强制Collapsed；本页Hovered/Selected只控制HighlightFrame，不得重新显示稀有度框。

## 检查结果与修改边界

2026-09-28，使用项目关联的 UE 5.8.2 命令行编辑器只读加载材质，读取参数、材质函数和节点输入；结果保存于本地 `Saved/InteractionHighlightInspection.json`。

- `/Game/Materials/M_PP_EnemyOutline`：Post Process / Opaque，Blendable Location 为 Scene Color Before Bloom。图中使用 `BlurSampleOffsets`、多个 `SampleSceneDepth`、Abs/Power/Saturate 形成深度邻域边缘信号；`CustomStencil` 经掩码和 If 参与筛选，再作为 Lerp 掩码在 PostProcessInput0 与 Color 之间混合。
- 默认 `Color` 为 **线性 RGBA=(1, 0.054443, 0, 1)**，橙红色。这里只确认资产默认值，不声称已检查地图/运行时所有覆盖或屏幕色彩结果。
- 敌人 C++ 的 `SetAttackHighlighted` 控制 Mesh 的 RenderCustomDepth 与 Stencil，默认 Stencil 值1；UI 高亮不接触这条链。
- 另查到 `M_Highlight` 是 Surface 材质，含 VertexNormalWS / TwoSidedSign，默认颜色为偏暗红；`MI_Highlight_Inst` 继承它且无颜色/标量覆盖。它不是这次 UMG 的材质来源。

新材质由用户在 Material Editor 创建，**不修改、不复制为UI版本、不引用原 Post Process 材质或其场景纹理节点**。只参考橙红色。现有 C++ 已分别提供 Hovered/Selected；本次无需修改枚举、选择逻辑、敌人高亮或其它装备逻辑。没有创建/伪造 `.uasset`，下文是待用户实际编辑的操作。

## 1. 创建 M_UI_InteractionHighlight

1. 在你选择的通用 UI 材质目录新建 Material，命名 `M_UI_InteractionHighlight`。
2. Material Domain = **User Interface**；Blend Mode = **Translucent**。
3. 它只生成一个透明中心的矩形高亮边框，不读取物品图片或世界轮廓。UI Glow 用透明度渐变实现，不能依靠场景 Bloom；UI 材质的 Final Color 数值加大并不会自动产生后处理光晕。
4. 建立以下参数。标量宽度统一以 Image 的**高度**为单位，不是像素；下列值用于约64×64槽位的起点，最终效果由你调整。

| 参数 | 类型 | 起点 | 用途 |
| --- | --- | --- | --- |
| HighlightColor | Vector Parameter | 线性RGB=(1,0.054443,0)，A=1 | 参考敌人描边的橙红色；Alpha不用作透明度 |
| BorderWidth | Scalar Parameter | 0.012 | 亮线半宽，64高时完整核心宽约1.5像素 |
| GlowIntensity | Scalar Parameter | 0.35 | 柔和光晕的透明度强度，可设0 |
| Opacity | Scalar Parameter | 1 | 全部高亮的不透明度，范围0–1 |
| GlowWidth | Scalar Parameter | 0.04 | 亮线外的渐变范围，必须>0 |
| Softness | Scalar Parameter | 0.008 | 亮线过渡宽度，必须>0 |
| Inset | Scalar Parameter | 0.06 | 边框距Image边界的内缩，给光晕留空间 |
| SlotAspectRatio | Scalar Parameter | 1 | Image宽/高；正方形=1，长方形按实际比例填写 |

颜色应在颜色选择器中按**线性 RGB**数值输入，不把上面的数值当成sRGB的0–255通道。若界面使用Hex/sRGB输入，先确认颜色选择器的转换方式。

### 节点接线

每行都是普通 Material 节点，公式中的名称只是给中间连线取的说明名，不必创建同名参数或 Custom HLSL：

```text
UV  = TextureCoordinate（UTiling/VTiling = 1）
U   = ComponentMask(UV, R)
V   = ComponentMask(UV, G)
DX  = Min(U, OneMinus(U)) × SlotAspectRatio
DY  = Min(V, OneMinus(V))
D   = Min(DX, DY)
T   = Abs(Subtract(D, Inset))

Line = OneMinus(SmoothStep(Min=BorderWidth,
                          Max=BorderWidth + Softness,
                          Value=T))

Soft = OneMinus(SmoothStep(Min=BorderWidth,
                          Max=BorderWidth + GlowWidth,
                          Value=T))
Glow = Power(Soft, 2) × GlowIntensity

Final Color ← HighlightColor 的 RGB
Opacity     ← Saturate(Max(Line, Glow) × Opacity参数)
```

使用 ComponentMask 取 HighlightColor 的 RGB，不接它的 Alpha。`Min` 取最靠近的一条边，`Abs(D-Inset)` 形成环线；线条附近才有非零Opacity，中心没有覆盖ItemIcon的色块。

为了保持中心透明且外侧光晕不被Image边界裁掉，调整时保持：

```text
BorderWidth >= 0；Softness > 0；GlowWidth > 0；SlotAspectRatio > 0
Inset >= BorderWidth + Max(Softness, GlowWidth)
Inset + BorderWidth + Max(Softness, GlowWidth) < 0.5 × Min(SlotAspectRatio, 1)
```

默认值满足上述条件。若矩形特别狭长，应减小Inset/宽度。材质不会自动探测UMG尺寸，Inventory等新槽位使用对应比例的材质实例；不要为此增加Tick轮询。

Apply、Save。可以右键创建 `MI_UI_InteractionHighlight_Equipment`，以后Inventory使用另一个MI调整宽度/颜色，所有MI共用这一个UI父材质。

## 2. 把双框换成 HighlightFrame

在 **WBP_EquipmentSlot** 中由你完成：

1. 若已经创建HoverFrame/SelectedFrame，先在Graph查找引用，把状态视觉接线改为下文方案，再合并为一个Image `HighlightFrame`；不要改动ItemIcon/RarityFrame/LockedOverlay。若还没创建双框，直接添加HighlightFrame。
2. HighlightFrame勾选Is Variable，放在ItemIcon/RarityFrame上方、LockedOverlay下方。现有 `slot_background` 可以继续保留原名，不必为了示意结构改名。
3. Brush → Image选择新UI材质或其MI；Draw As = Image。布局由你调，通常与槽位区域重合。材质Inset已经预留光晕，不需要再加很大的Padding。
4. Color and Opacity设白色、Alpha=1；Render Opacity=1。初始Visibility = **Collapsed**。
5. 显示时使用 **Not Hit-Testable (Self & All Children)**（Blueprint枚举为HitTestInvisible），不要设为Visible拦截鼠标。Collapsed时不渲染，也不参与Hit Test。

```text
Overlay
├─ Background / slot_background
├─ ItemIcon
├─ RarityFrame
├─ HighlightFrame
└─ LockedOverlay
```

HighlightFrame是你自己的Blueprint Image；不需要增加C++ BindWidget成员。固定颜色时直接使用MI即可，无需Create Dynamic Material Instance。未来要逐槽动态改参数时，再从该Image调用Get Dynamic Material，并只修改返回的每控件MID。

## 3. BP_RefreshVisual 接线

```text
Event BP Refresh Visual(State, ItemDisplay, bHasItem)
    → HighlightFrame.SetVisibility(Collapsed)
    → Switch on EUmbraEquipmentSlotState(State)
        Hovered  → HighlightFrame.SetVisibility(HitTestInvisible)
        Selected → HighlightFrame.SetVisibility(HitTestInvisible)
        Empty    → 不开启Highlight
        Equipped → 不开启Highlight
        Locked   → 不开启Highlight（继续使用原LockedOverlay）
```

Hovered和Selected共用同一个Image、同一个MI和同一套参数，但逻辑状态不合并。C++仍保持 Locked > Selected > Hovered > Equipped/Empty 的优先级：选中后鼠标移走仍亮，锁定后由锁定外观接管。稀有度框仍只表示稀有度，不被改成交互框。

每次事件先Collapsed再分支开启，均在同一个同步刷新中完成，避免旧状态残留。**事件内不调用RefreshVisual、SetItem、ClearItem、SetHovered、SetSelected、SetLocked或SetSlotType**，它们都会再次触发刷新。只操作Image的显示/颜色/动画。

## 4. 可选0.12秒淡入

先完成静态高亮验收，再按需增加淡入；不需要新增C++。

1. 在UMG Animations创建 `Anim_HighlightIn`，只给HighlightFrame的 **Render Opacity**加轨：0秒=0、0.12秒=1，播放一次。不要动画整个槽位，否则图标和稀有度框也会淡入。
2. 添加普通Blueprint Bool `bHighlightVisualActive`，默认false；它只记录视觉过渡，不替代Hovered/Selected状态。
3. 刷新时先保存该Bool到临时值 `bWasOn`，然后照常Collapsed，再按State计算 `bShouldHighlight`。
4. 若false：Stop Animation(Anim_HighlightIn) → RenderOpacity=0 → bHighlightVisualActive=false；保持Collapsed。
5. 若true：设HitTestInvisible；仅当bWasOn=false时，将RenderOpacity=0并从头Play Animation；bWasOn=true时保留当前透明度/播放进度，不重启动画。最后设bHighlightVisualActive=true。
6. Play Animation的Restore State设false，结束时保持Opacity=1。Hovered→Selected连续切换不会重新闪一下；中途移出/锁定会立即停动画并关闭。

不添加动画完成后自动显示Image的回调，否则可能把已经退出Hovered/Selected的高亮重新打开。页面重新构造/复用时，可以在自己的Construct中复位这个视觉Bool并调用一次RefreshVisual；不能在BP_RefreshVisual内部这么做。

## 5. 验收与当前状态

- Material预览：中心完全透明；边缘有细亮线和柔和渐变；Opacity=0全部透明，GlowIntensity=0仅保留细边。正方形和一个长方形MI各检查一次比例。
- WBP运行：空槽/已装备且未悬停不亮，悬停亮、移开灭；选择后移开仍亮，改选其它槽旧框灭；锁定时高亮灭，原锁定层正常。
- 检查高亮层显示前后点击目标不变；RarityFrame/Icon不被更改；若有动画，快速移入移出、Hovered→Selected、锁定均不残留或反复闪烁。
- 世界敌人描边仍正常；UI材质没有引用M_PP_EnemyOutline、SceneTexture、CustomStencil或SceneDepth。

本次已验证：原材质只读加载成功（0错误/0警告）、参数及输入节点读取、现有C++状态契约、文档链接/差异。未创建新材质资产、未修改WBP、未运行新材质编译或视觉PIE；以上节点仍须你在编辑器搭建和验收。不因文档变更重跑C++构建。
