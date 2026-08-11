# L4D2 Players

Self-contained player utilities and Survivor session management for Left 4 Dead 2.

`L4D2 Players` 为求生之路 2 服务器提供自动加入、统一玩家菜单、原生 Idle、Free Spectator、重新加入、5+ 中途出生/装备、自杀、个人自动连跳、角色切换和 AFK 管理。插件内置 `NextBotCreatePlayerBot<SurvivorBot>` 与 `CTerrorPlayer::RoundRespawn` 引擎调用，可在官方 `survivor_limit 4` 保持不变时按需创建 5–16 名 Survivor。

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

Root 管理员可使用 `sm_l4dp_addbot [count]` 诊断 5+ 创建能力。该命令与 `!join` 共用同一内部创建函数、默认创建 1 个 Bot，并受 `sm_l4dp_survivor_limit` 限制；它不会出现在普通玩家菜单中。

新真人进入游戏约 2.5 秒后会自动调用现有 Join 流程：优先接管空闲 Bot，没有空闲 Bot时按 Players 容量创建新 Bot。每次连接只执行一次；玩家主动 `!spec` 后，本次连接及后续换图都不会再次被自动拉回，仍可随时手动 `!join`。

只有 Players 为本次 Join 新创建的 5+ Survivor，在 takeover 最终验证成功后才会执行中途出生策略：安全放置在随机存活队友附近，并获得随机 T1 单喷/微冲、地图可用近战（失败时 pistol）及 pills/adrenaline。接回 Idle Bot、接管现有 Bot、换角色、普通复活和过图均不会重置位置或装备。

对应功能开关为 `sm_l4dp_auto_join`、`sm_l4dp_midjoin_spawn_near_player` 和 `sm_l4dp_midjoin_loadout`，v1.0.0 均默认开启。

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

安装、服务器配置和迁移说明见 [INSTALL.zh-CN.md](INSTALL.zh-CN.md)。当前实现已完成编译、静态依赖和本机 Windows gamedata 签名验证；多人运行行为必须按 `docs/` 下的清单在测试服验证。
当前支持的插件 CVar 可参考 [l4d2_players.cfg.example](l4d2_players.cfg.example)。

## 许可证

[MIT](LICENSE)
