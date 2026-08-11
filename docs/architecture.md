# Architecture

`plugin/src/l4d2_players.sp` 是唯一编译入口，所有 `.inc` 模块最终组成一个 `l4d2_players.smx`。

```text
commands / menu / AFK manager / QoL
              ↓
join / idle / spectate (Session lifecycle)
              ↓
identity       population       midjoin
(transfer)     (round baseline) (new 5+ post-processing only)
              ↓
survivor_engine + engine (SDKCall) + gamedata
              ↓
L4D2 server binary
```

- `engine.inc`：只准备和调用 L4D2 内部函数。
- `state.inc`：统一识别 Active Survivor、Engine Idle、Free Spectator 和特感控制状态。
- `identity.inc`：监听 `player_bot_replace` / `bot_player_replace` 的 Pre/Post，在 human↔Bot 转换中传递 character/model、通知 pending Idle replacement Bot，并于下一帧复验。
- `population.inc`：只在回合初始化窗口内有界补足 `sm_l4dp_min_survivors`；不周期运行，不处理战斗死亡，不参与 Human Team Wipe。
- `survivor_engine.inc`：组合 bind/takeover，并在 pending takeover 中记录 existing/newly-created Bot 来源；业务模块不直接操作 SDKCall。
- `idle.inc`：以 serial/reason/original identity/replacement Bot/start time 组成事务；严格提交 Engine Idle，超时通过现有 takeover 状态机回滚。
- `join.inc`：串行化 Join 请求，避免并发请求接管同一个 Bot；只有 baseline 已满足时才能创建新的 5+ Bot。
- `spectate.inc`：将 Active/Engine Idle 状态有界转换为 Free Spectator，并验证 Idle Bot 解绑后仍被保留。
- `auto_join.inc`：每次真人连接一次的短延迟 Join 调度；必须等待 Population ready，只调用 `LP_JoinSurvivor`，最多 3 次过渡态尝试。
- `midjoin.inc`：仅处理 newly-created Bot takeover 成功后的安全近队友放置和限定 T1/近战/药品装备。
- `character.inc`：唯一的 8 角色名称/模型映射；同时供 `!csm`、新建 Bot 和 Identity Lifecycle 使用。
- `nightvision.inc`：独立 QoL 模块；管理个人 `light_dynamic`、owner-only transmit、3 项菜单、档位 Cookie 和双击 F，只在 session 最终稳定后同步实体。
- `runtime.inc`：保存每个客户端的活动、Idle、旁观、Auto Join 连接级选择和个人设置状态。
- `hud.inc`：Auto Idle 只使用独立 CenterText 可见状态；Idle Kick 独立保留 HintText，两者不共享 enum 或清理路径。
- `afk_monitor.inc`：唯一的长期 1 秒 Timer；没有 per-client 长期 Timer。
- `human_team_wipe.inc`：记录本轮真人 Survivor 参与/死亡状态，在安全延迟后让剩余 Bot 自杀并交由游戏自然判定团灭。

v1.0.0 不暴露公共 Native/Forward，也不包含对其他功能型 SourceMod 插件的可选调用。
