# L4D2 Players

Self-contained player utilities and Survivor session management for Left 4 Dead 2.

`L4D2 Players` 为求生之路 2 服务器提供自动加入、每局 baseline Survivor population、统一 human↔Bot Survivor identity、原生 Idle、Free Spectator、重新加入、5+ 中途出生/装备、自杀、个人自动连跳、个人夜视仪、角色切换和 AFK 管理。插件内置 `NextBotCreatePlayerBot<SurvivorBot>` 与 `CTerrorPlayer::RoundRespawn` 引擎调用，可在官方 `survivor_limit 4` 保持不变时按需创建 5–16 名 Survivor。

## 功能

| 命令 | 功能 |
|---|---|
| `!player` / `!players` / `!p` | 打开玩家菜单 |
| `!afk` | 将当前 Survivor 交给 Bot，进入原生 Idle |
| `!spec` / `!spectate` | 解除 Idle Bot 归属并进入真正的 Free Spectator |
| `!join` | 接回自己的 Bot、接管空闲 Bot，或创建新的 Survivor Bot |
| `!zs` | 当前 Survivor 立即自杀 |
| `!bhop` | 切换个人自动连跳；偏好由 ClientPrefs 保存 |
| `!csm` | 在 8 名 Survivor 角色间切换，不持久化 |
| `!ysy` | 打开个人夜视仪菜单 |

Root 管理员可使用 `sm_l4dp_addbot [count]` 诊断 5+ 创建能力。该命令与 `!join` 共用同一内部创建函数、默认创建 1 个 Bot，并受 `sm_l4dp_survivor_limit` 限制；它不会出现在普通玩家菜单中。

新真人进入后，Population Manager 会先在回合初始化窗口内一次性补足 `sm_l4dp_min_survivors`（默认 4），Auto Join 再调用现有 Join 流程接管其中一个 Bot。因此标准 Coop 的第一名真人应得到“1 真人 + 3 Bot”，而不是只创建一个 body。Population 不是每秒维持器，战斗中 Bot 死亡和 Human Team Wipe 都不会触发补员。

Auto Join 每次连接只执行一次；玩家主动 `!spec` 后，本次连接及后续换图都不会再次被自动拉回，仍可随时手动 `!join`。只有 baseline 已建立、没有可接管 Bot、并且仍低于 `sm_l4dp_survivor_limit` 时，Join 才创建真正的 5+ Survivor。

只有 Players 为本次 Join 新创建的 5+ Survivor，在 takeover 最终验证成功后才会执行中途出生策略：安全放置在随机存活队友附近，并获得随机 T1 单喷/微冲、地图可用近战（失败时 pistol）及 pills/adrenaline。接回 Idle Bot、接管现有 Bot、换角色、普通复活和过图均不会重置位置或装备。

独立 Identity Lifecycle 同时监听 `player_bot_replace` 和 `bot_player_replace`：human 变 Bot 时把 character/model 传给 replacement Bot，Bot 被 takeover 时再把身份传给 human并于下一帧验证。这一路径统一覆盖 `!afk` ↔ `!join`、existing Bot、newly-created Bot 和 Auto Join；`!csm` 也复用 `character.inc` 中唯一的角色/模型定义。

Idle 是事务性转换：记录 client serial、reason、原 identity、replacement Bot 与开始时间；只有 spectator、Bot 归属和 identity 全部收敛才提交 Engine Idle。验证超时会通过现有 takeover 状态机回滚到原 Survivor，不会把普通 Free Spectator 当成 Idle 成功。

个人夜视仪由独立 `nightvision.inc` 管理，使用 parent 到玩家的纯白 `light_dynamic`，并通过 `SDKHook_SetTransmit` 仅向拥有者发送。菜单提供开关和 1–8 档（125–1000 distance）；亮度固定，档位不会被误用为 Source brightness。双击 F 可打开/关闭该菜单，不拦截原版 flashlight impulse 100。ClientPrefs 只保存档位；重连后默认关闭。Idle/旁观/死亡会删除光源，但本次连接的开启意愿可在 `!join`/复活后恢复。

对应功能开关/容量为 `sm_l4dp_min_survivors`、`sm_l4dp_auto_join`、`sm_l4dp_midjoin_spawn_near_player`、`sm_l4dp_midjoin_loadout`、`sm_l4dp_nightvision_enabled` 和 `sm_l4dp_nightvision_default_level`，v1.0.0 均使用文档中的默认值。

AFK Manager 默认在 120 秒无操作后自动 Idle，Auto Idle 倒计时使用独立 CenterText，不与 L4D2 原生 spectator/takeover HintText 竞争；Idle 600 秒后踢出。Free Spectator 可以无限旁观，不会因旁观时间被本插件踢出。Human Team Wipe 会在所有本轮参与真人真正死亡后杀死剩余普通 Survivor Bot，让游戏自然判定团灭。所有提示提供英文和简体中文翻译。

## 依赖

- Left 4 Dead 2 dedicated server
- MetaMod:Source
- SourceMod 1.12
- SDKTools、SDKHooks、ClientPrefs（随 SourceMod 提供）
- 5+ 真人服务器需要由 L4DToolZ 等基础设施提供足够客户端槽位，但本插件不调用其 API

不要求安装任何其他功能型 SourceMod 插件。

v1.0.0 已替代 MultiSlots 的加入功能、`l4d_CreateSurvivorBot` / CreateSurvivorBot 的扩容功能以及旧 AFK dead-bot fix。请勿同时启用这些重叠实现。Left4DHooks 可能仍被服务器上的其他插件使用，但 Players 本身不 include 或调用其 API；L4DToolZ 只负责提供足够 client slots。

## 构建

目标编译器锁定为 `SourcePawn Compiler 1.12.0.7246`：

```powershell
Copy-Item scripts/config.example.ps1 scripts/config.local.ps1
# 编辑 config.local.ps1
./scripts/build.ps1
./scripts/validate.ps1
./scripts/package.ps1
```

构建产物位于 `dist/l4d2_players.smx`，Release 包为 `dist/l4d2-plugin-players-v1.0.0.zip`。

安装、服务器配置和迁移说明见 [INSTALL.zh-CN.md](INSTALL.zh-CN.md)。Build、静态依赖和本机 Windows gamedata 签名验证已通过；Linux dedicated server 已完成核心功能与生命周期实机验证，5/8/12/16 总 Survivor 数量已以 1 真人 + Bots 验证。5+ 真人并发场景仍未覆盖。
当前支持的插件 CVar 可参考 [l4d2_players.cfg.example](l4d2_players.cfg.example)。

## 许可证

[MIT](LICENSE)
