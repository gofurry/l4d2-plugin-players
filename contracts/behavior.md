# L4D2 Players Behavior Contract

## Player states

- `SURVIVOR_ACTIVE`：真人在 team 2。
- `SURVIVOR_IDLE`：真人在 team 1，且存在 `m_humanSpectatorUserID` 等于其 userid 的 Survivor Bot。
- `SPECTATOR_FREE`：真人在 team 1，且没有上述关联 Bot。
- 其他状态视为 `NONE`，不会被 Join/AFK 逻辑隐式换队。

## Idle

- 只有活着、未被特感控制的 Active Survivor 可以手动或自动 Idle。
- 倒地和挂边仍视为活着；允许 Idle。
- Idle 必须由 `GoAwayFromKeyboard` 建立原生关系，并通过有界多帧验证。
- 没有临时客户端槽时明确失败，不踢其他玩家。

## Join

优先级不可更改：自己的 Idle Bot、无人绑定且存活的 Survivor Bot、新创建的 Survivor Bot。自己的 Idle Bot 保留现有 human/Bot 关系，只调用内部 takeover；Free Spectator 接管普通 Bot 才执行 unassigned、bind、takeover。两条路径均以有界验证后的 Active Survivor 状态为成功依据。Bot 创建请求全局串行；超时或请求者离线不会无限重试。只有创建新 Bot 会增加 Survivor 数并受 `sm_l4dp_survivor_limit` 约束。

## Free Spectator

`!spec`/`!spectate`与`!afk`完全分离。进入 Free Spectator 后真人必须在 team 1，且不得存在指向其 userid 的 Survivor Bot 绑定。从 Engine Idle 进入时，原 Bot 的 `m_humanSpectatorUserID` 被显式清零，并在有界验证中确认 Bot 仍为 Survivor Bot。Free Spectator 不启动任何超时、警告 HUD 或 Kick，可无限旁观。

## AFK

活动会重置 Auto Idle 时间。任何合法原生 Idle 来源都进入统一 Idle Kick 计时。管理员只豁免 Idle Kick，不豁免 Auto Idle。

## Human Team Wipe

本轮成为过 Active Survivor 或 Engine Idle 的真人会被登记为参与者。活着的 Active 真人，以及绑定 Bot 仍活着的 Engine Idle 真人，均视为有效存活；普通无人绑定 Bot 不算。只有全部登记参与者均收到真正死亡事件且不存在有效存活真人时，插件才延迟杀死剩余 Survivor Bot。Free Spectator、从未加入者、倒地和挂边玩家不会单独触发判断。

## QoL

Bhop 只重放跳跃边沿，不修改速度或物理 CVar。角色切换同时修改 `m_survivorCharacter` 和模型，允许重复且不持久化。
