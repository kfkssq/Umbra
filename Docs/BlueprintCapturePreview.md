# 全身预览：直接编辑现有 Capture 蓝图（2026-09-29）

本页取代之前“在Equipment里填写相机/人物变换，并复制transient RT”的默认流程。C++已改为默认 `Use Blueprint Configuration=true`。资产仍由用户亲自创建、编辑、保存。

## 最终连接

### PIE 里人物一直静止：先检查菜单父类

2026-09-29 17:32 用关联 UE 5.8.2 只读检查保存资产，发现 WBP_CharacterMenu 仍以 UserWidget 为父类，而 WBP_Equipment 已正确继承 UmbraEquipmentMenu。Controller 的 ToggleCharacterMenu 只对 UmbraCharacterMenu 调用 SetMenuOpen；根类不匹配会跳过装备页激活，界面可能保留 RT 的旧静态画面。检查同时确认 BP_CharacterFullBodyPreview 的 PortraitMesh 使用 Idle、Playing/Looping=true、播放率1、Pause Anims=false；没有发现动画配置被关闭。

停止 PIE → 打开 WBP_CharacterMenu → File / Reparent Blueprint → UmbraCharacterMenu → Compile / Save → 重新 PIE 打开装备页。保留布局、Switcher 和按钮 Graph，不要重建 Capture/RT 或用 Event Tick 强行刷新。PIE 中可以在 WBP_Equipment 实例观察 IsPageActive 和 GetPreviewComponent → IsPreviewActive，打开装备页时两者应为 true；若仍静止，再据运行时状态检查捕获与动画。此次只读结果在 Saved/PreviewStoppedInspection.json；未代改资产、未执行实际 PIE 恢复验收。

```text
BP_CharacterFullBodyPreview（已有头像BP的子类）
  PortraitMesh：人物相对变换、Idle/专用AnimBP
  PortraitCapture：相机相对变换、FOV、TextureTarget
  KeyLight / FillLight：灯光
          ↓
RT_CharacterFullBody（新建专用资产）
          ↓
M_UI_CharacterFullBody（新建UI材质）
          ↓
WBP_Equipment.CharacterPreview 的 Designer Brush
```

复用的是已有Capture组件和蓝图结构，不是让HUD和装备页争用同一个运行时Capture实例。原头像BP、原RT、原材质保持不变。运行时Equipment仍创建子BP实例，并按菜单状态启停；不移动/旋转真实玩家。

## 1. 创建独立RT

### 人物会动但呈蓝灰色剪影

**后续回归调查取代下列早期推断**：已实际完成原流程、跳过材质复制（A）、再跳过 C++ Capture 覆盖（B）的真实 RHI 渲染，三次 RT 均仍蓝灰。尚未确认根因或正式修复；此前优先建议关闭雾的排查顺序已撤回，不应凭该推断继续修改配置。完整源码差异、历史实例删除记录、运行日志与验证边界见 [Preview 回归报告](PreviewRegression.md)。下方保留早期检查记录，不作为当前修复步骤；也不要重建本页后续创建流程中的 RT/材质。

**UE 5.8 实际入口（根据本机引擎 Details 定制源码核对）**：在 BP_CharacterFullBodyPreview 的 Components 面板选中 PortraitCapture，清空 Details 搜索，在 Details 的视图选项启用 **Show All Advanced Details**。展开 **Scene Capture → General Show Flags**，取消 Fog 与 Atmosphere；再展开 **Scene Capture → Lighting Features Show Flags**，取消 Volumetric Fog，保留 Lighting。这些组被引擎标为高级显示，原始 ShowFlagSettings 数组被定制面板隐藏，所以不一定能搜到一个同名数组或统一的“Show Flags”项目。中文界面可搜索“雾”；不要去材质编辑器或 Blueprint 视口的 Show 菜单找。此入口已核对源码，未通过当前 GUI 截图确认用户面板状态。

2026-09-29 用户在接回 UmbraCharacterMenu 后确认动画已恢复，但人物为纯蓝灰色。17:39 的只读报告 Saved/PreviewColorInspection.json 确认：CharacterPreview 的 Brush 使用 M_UI_CharacterFullBody，Image/Brush tint 均为白色；材质颜色输出来自 Multiply（输入为 PortraitTexture 与 brightness=1），Opacity 来自 OneMinus；CaptureSource=SceneColorHDR，灯光可见且强度非零；PreviewActorTransform.Z=-100000cm，PortraitCapture.ShowFlagSettings 为空。引擎 SceneCapture 默认使用游戏 ShowFlags，没有默认关闭 Fog。

优先怀疑远低于关卡的展示位置受到高度雾覆盖；此为配置与截图支持的诊断推断，未做 GPU 开关对照，尚不能标记为已证实根因。先停止 PIE，打开 BP_CharacterFullBodyPreview，选 PortraitCapture，在 Details 的 Show Flags / Show Flag Settings 关闭 Fog、Atmosphere、Volumetric Fog，保留 Lighting 与材质相关开关。Compile/Save 后重新 PIE。仅调整该 Capture，不关闭整个关卡的雾，不用提高 Brightness 补偿。当前 UseBlueprintConfiguration=true，C++ 会保留这些组件设置。

如果关闭后仍为纯色，在 PIE 菜单打开时查看 RT_CharacterFullBody：RT 本身纯色则继续检查捕获/场景；RT 有正常人物颜色而 UI 纯色，则检查运行时材质参数及 UI 覆盖。Content Browser 的 RT/材质缩略图可能不是同一时刻的内容，不能仅由缩略图判断实时输出。

17:41 进一步只读核对材质输出端口（Saved/PreviewMaterialPins.json）：PortraitTexture 的 RGB 接 Multiply.A，brightness 接 Multiply.B，纹理 A 接 OneMinus；结合前次输出节点检查，保存材质的 RGB/Alpha 接线正确，无须重接。关闭 Capture 雾效的实际对照仍未执行。本轮未改资产或源码，也未宣称修复画面已验收。

1. Content Browser创建Texture Render Target 2D，命名 `RT_CharacterFullBody`。
2. Size X=512，Size Y=1024，Format=RGBA16f。
3. Clear Color设为线性(0,0,0,1)。捕获源SceneColor HDR的Alpha为反向不透明度，透明背景对应Alpha=1。
4. 保存。这个资产是实际输出，不再只是会被复制的模板。

## 2. 编辑已经创建的预览子BP

继续使用 `BP_CharacterFullBodyPreview`，不要重复创建或复制整套组件。

1. 选中继承的 `PortraitMesh`，确认Skeletal Mesh不是None，使用原头像Greystone或与你实际玩家兼容的角色资源。
2. 为便于编辑器Viewport观察，组件默认Visible=true、Render in Main Pass=true、Visible in Scene Capture Only=false、Scale非零。运行时C++只在展示实例上开启SceneCaptureOnly，不改资产默认值。
3. 在此直接调Mesh的Location/Rotation/Scale。可先从原yaw=-90°开始，再按你自己的构图调整。
4. Animation Mode=Use Animation Asset，Anim To Play=原Greystone Idle，Looping/Playing勾选。若你使用专用展示AnimBP，则在此选择Animation Blueprint模式和Anim Class，C++会读取该选择。玩家Mesh必须与它骨架兼容。
5. 选中 `PortraitCapture`，Texture Target设置为新建的 `RT_CharacterFullBody`，不能保留 `RT_HeroPortrait`。
6. Capture Source选择SceneColor (HDR) in RGB, Inv Opacity in A；Primitive Render Mode=Use ShowOnly List。
7. 直接在该组件上调Location、Rotation、FOV。起点可用位置(450,0,100)cm、Pitch=0/Yaw=180/Roll=0、FOV=30°；最终根据实际Mesh朝向调整。
8. 灯光在继承的KeyLight/FillLight上调整。默认隐藏的灯运行时也保持隐藏；菜单关闭时暂时关闭已启用的灯，重开后恢复。
9. 如需透明背景，在Capture的Show Flags中关闭Fog、Volumetric Fog、Atmosphere；不再由C++强行覆盖这些美术配置。

这些相机/FOV/人物Transform/动画配置现在以**子BP组件值**为准。WBP_Equipment的旧CameraTransform等字段不会覆盖它们。

### 编辑状态也能捕获人物

原头像的ShowOnly可能只在BeginPlay初始化，编辑状态不会执行BeginPlay。你可以在**子BP的Construction Script**中添加：

```text
Construction Script
  → Call Parent Construction Script（保留原有父逻辑）
  → PortraitCapture.Clear Show Only Components
  → PortraitCapture.Show Only Component(Component=PortraitMesh)
```

编辑预览期间可在子BP Capture上勾选Capture Every Frame、Capture on Movement，并打开预览视口Realtime。需要单帧刷新时，可以在上述列表设置完成后调用Capture Scene。不要清理父HUD实例的组件列表；所有Target都用本子BP的组件引用。

**蓝图Viewport显示的是模型与相机组件，不等于Capture输出。** 以打开RT资产/材质后的画面为准。若蓝图编辑器的预览世界没有持续更新Capture，把该子BP临时放进一个专用预览关卡，编辑它的组件/蓝图并查看RT；动画的编辑状态播放能力也以对应预览世界为准，稳定Idle最终在PIE确认。

正式游戏关卡不要额外常驻一个使用同RT的全身Capture，否则运行时自动创建的实例会检测到RT冲突。预览关卡用完切回游戏地图；不要让编辑预览与PIE同时向同一RT持续写入。

## 3. 创建自己的UI材质

1. 新建 `M_UI_CharacterFullBody`，Material Domain=User Interface，Blend Mode=Translucent。
2. 添加Texture Sample Parameter 2D，Parameter Name=`PortraitTexture`，默认Texture指定 `RT_CharacterFullBody`；采样类型使用适合线性RenderTarget的Linear Color。
3. 添加Scalar Parameter `Brightness`，默认1，可按你喜欢的画面调节。
4. 节点连接：

```text
PortraitTexture.RGB × Brightness → Final Color
OneMinus(PortraitTexture.A)       → Opacity
```

5. Apply、Save。可以创建专用MI调Brightness。该材质只读取全身RT，不修改 `M_UI_HeroPortrait`。

## 4. 配置WBP_Equipment

1. 父类保持 `UmbraEquipmentMenu`。在Designer选中Image `CharacterPreview`，Brush.Image设置为新材质/MI，Color and Opacity=白色/Alpha1、RenderOpacity=1，保持1:2的输出比例。
2. Class Defaults只需选 `Preview Actor Class=BP_CharacterFullBodyPreview`；`Preview Material`可以留空，C++默认读取Designer Brush里的材质。若该属性仍填着旧头像材质，清空它或改成新材质，因为显式PreviewMaterial优先于Designer Brush。
3. `Texture Parameter Name=PortraitTexture`。运行时生成MID时仍用这个参数，指向同一个专用RT；重建/换Pawn时会恢复源材质，避免丢失Designer配置。
4. Preview Settings保持 **Use Blueprint Configuration=true**，Capture Rate默认30Hz。旧镜头/动画/RT模板字段会隐藏且不参与配置。
5. Preview Actor Transform仍是世界中的展示位置，默认(0,0,-100000)cm；它只决定运行时实例放在哪里，不覆盖子BP组件的局部构图。
6. Compile、Save，重开PIE验证。新增反射属性需要保存工作、关闭Editor并构建原项目，再重开；隔离验证构建不会更新当前Editor DLL。

Designer Image显示的是RT最近一帧；如果还没有任何Capture写入新RT，它会是空白。先按第2步捕获一次再看Designer，不需要启动装备Gameplay系统。

## 5. 运行时仍由逻辑管理的部分

- 打开且切到装备页：读取子BP配置、同步玩家主体Mesh/Materials、恢复循环Idle或专用AnimBP，启动Capture Timer。
- 切离装备页/关闭：停止Capture与主体Mesh更新，暂时关灯；保留RT最近画面，不清空输出。
- 运行时接管实例的Capture Every Frame/OnMovement会关闭，由30Hz Timer驱动，避免菜单关闭后仍捕获。
- 运行时重建ShowOnly只包含本展示Actor的组件；禁用展示碰撞/阴影、开启仅捕获可见。原玩家和HUD捕获实例不参与。
- 专用RT只允许一个游戏Capture使用；误填HUD RT或另一实例共用RT会输出警告并拒绝开启，不强抢输出。多本地预览需要不同RT；也可显式切回下面的旧隔离模式。

编辑器布局、美术效果和新资产由用户维护；没有新增装备玩法系统。

## 兼容模式

只有显式取消Use Blueprint Configuration时，才重新启用旧的WBP PreviewSettings镜头/人物/动画覆盖，以及运行时新建或复制RT。这个模式保留给需要每实例隔离输出的情况，不是本次推荐的编辑流程。

## 验收

1. 改子BP的Capture位置/FOV后，编辑RT预览和运行时构图均跟随，不再被WBP旧值覆盖。
2. 运行时Capture.TextureTarget与新建RT资产相同，UI材质使用同一RT；原HUD RT不变。
3. 关闭/切页后全身IsPreviewActive=false、画面停止更新；重新打开恢复。
4. 误填头像RT或重复Capture时应给出dedicated full-body RT警告；改回专用RT后重开页面。
5. 每次保存默认参数修改后重新PIE，确认原玩家、HUD头像、十槽和单层交互高亮保持正常。

构建与逻辑自动化记录见 [Progress](Progress.md)。未替用户创建/修改任何材质、RT或蓝图资产，编辑器实时预览和真实PIE画面仍需实际接线后确认。
