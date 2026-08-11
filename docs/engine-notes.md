# Engine Notes

最后验证的本机环境：L4D2 Steam build `23990068`，Windows 32-bit `left4dead2/bin/server.dll`，2026-07-01 文件版本。

| Gamedata 条目 | 内部用途 | SDKCall 约定 |
|---|---|---|
| `CDirector` | 调用 Director 创建 Survivor Bot | gamedata address；Windows 从 `CDirectorMusicBanks::OnRoundStart + 12` 读取 |
| `CTerrorPlayer::GoAwayFromKeyboard` | 进入原生 Take a Break | Player call，无参数，bool 返回 |
| `SurvivorBot::SetHumanSpectator` | 将真人绑定到指定 Survivor Bot | Player call，CBasePlayer 指针参数，bool 返回 |
| `CTerrorPlayer::TakeOverBot` | 真人接管已绑定 Bot | Player call，bool 参数固定 `true`，bool 返回 |
| `CDirector::AddSurvivorBot` | 创建新的 Survivor Bot | Raw call，Director this 指针和 0–3 character 参数 |

Windows 五个签名均通过 `scripts/validate.py` 在上述二进制中验证为唯一匹配。Linux 使用对应 C++ 导出符号，尚需在实际 Linux 服务器验证。

Idle 与 takeover 的 SDKCall 同步返回值只用于诊断。Linux 实机的最终状态可能在后续帧才收敛，因此插件最多验证 64 帧/1 秒，并分别以 Engine Idle 和 Active Survivor 状态作为成功条件。

函数名称、调用语义和签名研究参考公开的 L4D2 引擎资料；项目业务实现独立编写，不复制第三方 SourcePawn 实现。游戏更新后必须重新验证签名唯一性和实际调用行为。
