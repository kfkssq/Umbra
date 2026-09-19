# F1/F2 修复与准确操作

本文为历史修复与验证记录。2026-09-17 起，C++ 只向 `Apply Attribute Debug State` 传递原始状态，窗口布局、文字和按钮事件由 WBP 实现；下方旧视觉结果需在蓝图迁移后重验。当前状态见 [Progress](Progress.md)，配置入口见 [EditorSetup](EditorSetup.md)。

## 已定位并修复的问题

1. 引擎 BaseInput.ini 自带 F1 → viewmode wireframe、F2 → viewmode unlit。项目运行日志确实记录了 Wireframe/Unlit 切换。之前只绑定 Enhanced Input，未移除引擎的另一组调试绑定，这是实现遗漏。现已在项目 DefaultInput.ini 精确移除这两条绑定，保留其他调试键。
2. BP_Enemy_Melee_01 的 Capsule 和 Mesh 均忽略 Visibility。之前 F2 只用 Visibility 命中，因而选不中这类敌人。现已改为复用原攻击悬停的 Pawn 查询与 Attackable 筛选，并用 Visibility 检查是否有更近的障碍物。保留用户现有碰撞资产，不穿墙选敌。
3. F1/F2 现在在面板下方提供选择反馈，Output Log 同时记录 Attribute debug: F1/F2，便于区分“没收到输入”与“收到了但没选中”。

## 是否实现了查看敌人属性

已经实现。F2 的处理函数最终调用 UUmbraAttributeDebugPanel::ViewEnemy；面板从该敌人自身的 ASC 读取完整属性，并通过属性变化委托更新。
查看玩家时，面板通过现有角色接口取得 PlayerState 上的 ASC。

本次已经通过匹配版本 Unreal Editor 读取确认：

- BP_UmbraPlayerController 已开启 Enable Attribute Debug Panel。
- Widget Class 正确引用 WBP_AttributeDebugPanel。
- 专用 IMC 和两个 Input Action 引用已设置。
- 两个 Input Action 均为 Digital (bool)，没有额外 Trigger/Modifier。
- 你创建的 WBP 当前蓝图状态为 Up To Date。

**F1/F2 是切换左侧已有面板的查看对象，不会弹出第二个窗口，不会改变视角，也不会改变材质/颜色。**

## 先恢复正常画面

1. 结束 PIE，关闭 Unreal Editor，重新打开项目，让新 C++ DLL 和 DefaultInput.ini 生效。
2. Play 下拉菜单选择 New Editor Window (PIE)，便于区分运行窗口和编辑器场景视口。
3. 如果运行画面仍保留旧的线框/无光照状态，在游戏窗口按 F3；或按波浪号打开控制台，输入 viewmode lit，回车，再关闭控制台。
4. 若异常发生在未运行的编辑器场景视口，使用视口左上角的视图模式菜单切回 Lit（光照）。

## F1：查看玩家

1. 确保正在 Play，而不是只在编辑地图或 Simulate。
2. 点击 PIE 窗口中的左侧面板背景，让该游戏窗口获得焦点；不要在 Console、文本输入框或编辑器资产窗口里按键。
3. 单独按一下 F1，不按 Shift/Ctrl/Alt。
4. 左侧面板顶部应显示本地玩家角色名称，属性区域显示玩家生命、资源和其他属性；底部提示“正在查看本地玩家”。
5. 默认已经查看玩家，因此第一次按 F1 可能只有底部提示变化，这是正常的。
6. 四个测试按钮此时作用于玩家；F1 本身不回血、不重置属性。

## F2：锁定查看敌人

1. 保持游戏窗口有焦点，把鼠标移到活着的 BP_Enemy_Melee_01 身体/胶囊碰撞区域上，不要放在左侧面板、地面或敌人名字标签上。
2. 不需要点击敌人。单独按一次 F2。
3. 面板顶部应变为该敌人的名称，底部提示“已锁定敌人，移开鼠标仍保持查看”，属性区域显示敌人的数值。
4. 把鼠标移到空地或面板上，查看对象仍然是刚才的敌人。此时点测试按钮，作用的是这名敌人。
5. 在 UI 外用原来的方式攻击该敌人，生命数值应随属性委托变化。
6. 悬停另一名敌人再按 F2 可换目标。对地面、UI、被遮挡或不可攻击的对象按 F2，底部提示“未命中可查看的敌人，保持当前目标”。
7. 已锁定的敌人死亡后，尸体存在时可以显示 0 生命；Actor 销毁后自动切回玩家。
8. 任意时刻按 F1 切回玩家。鼠标移开、F1/F2 切换都不会自动移除目标上的测试效果。

若想让敌人站着不反击，使用 BP_Enemy_Melee_01 或关卡实例的 AI → Enable AI Behavior，取消勾选后重新 PIE。AI 开关不会让活敌人失去被查看/被攻击资格。

## 没有出现预期结果时

- **画面又变色：** 确认已重启编辑器、正在新的 PIE 会话；查看 Output Log 是否仍出现 Set new viewmode。新的默认配置不会再由 F1/F2触发上述模式。
- **没有左侧面板：** 检查实际使用的 GameMode/PlayerController，不能只检查未被关卡使用的某个蓝图。
- **底部没有按键反馈：** 检查游戏窗口焦点，确认 IMC_AttributeDebug 默认映射中 IA_Debug_ViewPlayer=F1、IA_Debug_LockHovered=F2；不要在蓝图再绑定另一组同名事件。
- **F2提示未命中：** 在敌人身体中间重新悬停，避开墙体等遮挡，确认敌人未死亡。Output Log 会显示 Pawn 命中、Visibility 命中和 Occluded 状态。
- **名称已变，数值看起来没变：** 玩家和敌人可能使用相同初始值；以顶部名称为准，再对当前目标施加10点伤害观察。
- **按键后新开窗口或切相机：** 这不是本调试功能的行为，检查是否额外编写了蓝图 F1/F2 事件。

更完整的控件与输入配置见同目录 AttributeDebugPanel.md。

## 本次验证结果

- UmbraEditor / Win64 / Development 编译成功。
- Umbra.Attributes.DebugInputAndWidget：历史版本曾验证 WBP 文字；当前测试改为验证有效 F1/F2 配置、实际 WBP 可实例化、原始状态中的敌人/Health 更新、销毁回退及监听清理，不验证蓝图格式和布局。
- Umbra.Attributes.DebugOperations、Lifecycle：均 Success；自动化进程退出码0。
- 测试日志：Saved/Logs/AttributeDebugFKeysTests.log（生成文件，不提交）。
- 未手动执行真实 PIE 鼠标/按键/画面验收；新增测试调用了 F2 最终使用的 ViewEnemy 接口，但没有模拟真实鼠标射线、键盘硬件输入或联网客户端。
- 本次只读检查蓝图资产，没有改写 .uasset/.umap；保留用户已经保存的资产改动。
