# Engine Notes

最后验证的本机环境：L4D2 Steam build `23990068`，Windows 32-bit `left4dead2/bin/server.dll`，2026-07-01 文件版本。

| Gamedata 条目 | 内部用途 | SDKCall 约定 |
|---|---|---|
| `CTerrorPlayer::GoAwayFromKeyboard` | 进入原生 Take a Break | Player call，无参数，bool 返回 |
| `SurvivorBot::SetHumanSpectator` | 将真人绑定到指定 Survivor Bot | Player call，CBasePlayer 指针参数，bool 返回 |
| `CTerrorPlayer::TakeOverBot` | 真人接管已绑定 Bot | Player call，bool 参数固定 `true`，bool 返回 |
| `NextBotCreatePlayerBot<SurvivorBot>` | 直接创建不受官方 4 Survivor 数量限制的 Bot client | Static call，字符串指针参数，CBasePlayer 返回 |
| `CTerrorPlayer::RoundRespawn` | 让新建但未存活的 Survivor Bot 复活 | Player call，无参数、无返回值 |

Windows 五个签名均通过 `scripts/validate.py` 在上述二进制中验证为唯一匹配。NextBot 的 Windows 签名定位 `CDirector::AddSurvivorBot` 内唯一的 `E8` 调用点，engine layer 读取 rel32 并解析到真实函数入口；校验脚本也确认解析目标位于 `server.dll` 内。Linux 直接使用 `_Z22NextBotCreatePlayerBotI11SurvivorBotEPT_PKc` 与 `_ZN13CTerrorPlayer12RoundRespawnEv` 导出符号，尚需在实际 Linux 服务器验证。

创建顺序固定为：确认临时客户端槽与 Players 容量、调用 NextBot、将真实 Bot client 切到 team 2、从 Nick/Rochelle/Coach/Ellis round-robin 取得 character、通过 `character.inc` 唯一模型表应用并验证 `m_survivorCharacter`/model、必要时 RoundRespawn、再次应用并验证同一身份、确认 Bot 存活后返回 index。后续 human↔Bot replacement 不依赖这一次初始写入；`identity.inc` 在两个 replacement event 中继续传递并复验身份。官方 `survivor_limit` 不会被读取或写入；`sm_l4dp_survivor_limit` 只约束 Players 主动创建数量。旧 `CDirector::AddSurvivorBot` 与 Director 全局地址不再初始化或调用。

Idle 与 takeover 的 SDKCall 同步返回值只用于诊断。Linux 实机的最终状态可能在后续帧才收敛，因此插件最多验证 64 帧/1 秒。Idle 还必须收到/(或回滚时重新发现) `player_bot_replace` 的 replacement Bot，并同时验证 human team 1、Bot team 2、`m_humanSpectatorUserID` 和 original identity。超时后不会直接丢弃 session，而是以专用 rollback context 复用 takeover 验证路径恢复 Active Survivor。

Population Manager 不增加新的 SDKCall；它只在 map/round/第一名真人的初始化窗口内，最多 3 次调用同一 `LP_CreateUnlimitedSurvivorBot()` 补到 `sm_l4dp_min_survivors`。成功后立即关闭窗口，不建立循环 Timer，不响应战斗死亡。

函数名称、调用语义和签名研究参考公开的 `l4d_CreateSurvivorBot` 引擎资料；项目业务实现独立编写，不引入其 Native、include 或 runtime dependency。游戏更新后必须重新验证签名唯一性、Windows 调用目标和实际调用行为。
