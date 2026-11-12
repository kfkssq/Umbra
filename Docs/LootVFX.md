# 六档掉落光效（2026-11-04）

## 配置与边界

`AUmbraWorldItem` 的 `LootVFX` 是随 Actor 销毁的 Niagara 组件，跟随根节点位置、使用绝对旋转/缩放，物理翻滚不倾斜光柱。实例初始化、BeginPlay、实例复制和视觉刷新时更新；专用服务器、无效实例、缺少配置和拾取后停用并清空资产。重复刷新不主动重启仍在运行的系统。原有物理、拾取、生成概率和 UI 配色不变。

配置来源：C++ 空配置 → `/Game/Items/Loot/BP_WorldItem` 的 `World Item|VFX` 默认值。`LootVFXStyles` 按实际 `ItemInstance.GetRarity()` 查找：Common/Magic/Rare/Epic/Legendary/Unique 对应原包 Common/Uncommon/Rare/Epic/Legendary/Mythic 系统。没有配置时不播放，无隐式默认资产。

颜色优先级：该档 `Override Color` → `ItemInstance.GetRarityColor()`（生成实例读取 GenerationProfile 品质行；固定物品读取 Definition.RarityColor）。前四档沿用现有品质色；后两档仅在光效层交换原包的金色与红色，不交换系统形态，也不修改品质/标签/背包颜色。每层原始 RGB 最大值作为 HDR 亮度乘以新色，Alpha 乘以原始 Alpha，保留烟雾/火焰/射线的亮度层次。

`LootVFXColorParameters` 已核验为 `User.C_Circle`、`User.C_Fade_Magic`、`User.C_M_Flame`、`User.C_Magic_Floor`、`User.C_Rays`、`User.C_Smoke`。高档覆盖色为线性 RGB：Legendary=(1,0.045,0.12)，Unique=(1,0.37,0.025)，分别取自原 Mythic/Legendary 的归一化颜色。原包 `User.Flame Size`、`User.Speed_Anim` 及其它粒子参数不改。通过 `Scripts/configure_loot_vfx.py` 在匹配编辑器中保存生产 BP 配置。

## 来源

用户已导入 `/Game/LootDropVFX`。作者 Targi VFX，来源 [Loot Drops VFX – Niagara Rarity Effects](https://www.fab.com/listings/9036618c-f389-4551-9494-93b4bad2d1a3)。网页标注免费、UE 5.8；领取时接受的具体许可版本以用户 Fab 记录为准，未从本地确认许可凭据。保留供应商目录，不直接修改原特效资产。

## 验证入口

- `Umbra.Loot.VFXAssetAudit`：通过 UE 加载六个原始 Niagara 系统并导出真实参数到 `Saved/LootVFX/parameters.txt`。
- `Umbra.Loot.VFXLifecycle`：生产 BP 六档映射、实例颜色/覆盖色、参数存在、物理转向与拾取清理。
- `Scripts/smoke_loot_vfx_pie.py`：实际 PIE 中六次敌人死亡生成掉落，验证对应特效五秒后仍激活、保持竖直、拾取后停用。只使用临时夹具，不保存关卡。
- 人工视觉验收：正常俯视距离观察六档形态，传说红/独特金；检查几十个掉落物叠加后的辨识度、遮挡与 GPU 时间；Listen Server/客户端和迟加入另测。

本轮实际执行结果见 [Progress](Progress.md)。
