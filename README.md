# L4D2 Players

Self-contained player utilities and Survivor session management for Left 4 Dead 2.

`L4D2 Players` 为求生之路 2 服务器提供统一的玩家菜单、原生 Idle、重新加入、自杀、个人自动连跳、角色切换和 AFK 管理。插件自行封装 Survivor Bot 创建、human/bot 绑定与 takeover，不依赖 Left4DHooks、MultiSlots、CreateSurvivorBot 或 Stats。

## 功能

| 命令 | 功能 |
|---|---|
| `!player` / `!players` / `!p` | 打开玩家菜单 |
| `!afk` | 将当前 Survivor 交给 Bot，进入原生 Idle |
| `!join` | 接回自己的 Bot、接管空闲 Bot，或创建新的 Survivor Bot |
| `!zs` | 当前 Survivor 立即自杀 |
| `!bhop` | 切换个人自动连跳；偏好由 ClientPrefs 保存 |
| `!csm` | 在 8 名 Survivor 角色间切换，不持久化 |

AFK Manager 默认在 120 秒无操作后自动 Idle，Idle 600 秒后踢出；Free Spectator 也有独立宽限期与超时。Human Team Wipe 会在所有本轮参与真人真正死亡后杀死剩余普通 Survivor Bot，让游戏自然判定团灭。所有提示提供英文和简体中文翻译。

## 依赖

- Left 4 Dead 2 dedicated server
- MetaMod:Source
- SourceMod 1.12
- SDKTools、SDKHooks、ClientPrefs（随 SourceMod 提供）
- 5+ 真人服务器需要由 L4DToolZ 等基础设施提供足够客户端槽位，但本插件不调用其 API

不要求安装任何其他功能型 SourceMod 插件。

## 构建

目标编译器锁定为 `SourcePawn Compiler 1.12.0.7246`：

```powershell
Copy-Item scripts/config.example.ps1 scripts/config.local.ps1
# 编辑 config.local.ps1
./scripts/build.ps1
./scripts/validate.ps1
./scripts/package.ps1
```

构建产物位于 `dist/l4d2_players.smx`，Release 包为 `dist/l4d2-plugin-players-v0.3.2.zip`。

安装、服务器配置和迁移说明见 [INSTALL.zh-CN.md](INSTALL.zh-CN.md)。当前实现已完成编译、静态依赖和本机 Windows gamedata 签名验证；多人运行行为必须按 `docs/` 下的清单在测试服验证。

## 许可证

[MIT](LICENSE)
