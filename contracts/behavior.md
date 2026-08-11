# L4D2 Players Behavior Contract

## Player states

- `SURVIVOR_ACTIVE`：真人在 team 2。
- `SURVIVOR_IDLE`：真人在 team 1，且存在 `m_humanSpectatorUserID` 等于其 userid 的 Survivor Bot。
- `SPECTATOR_FREE`：真人在 team 1，且没有上述关联 Bot。
- 其他状态视为 `NONE`，不会被 Join/AFK 逻辑隐式换队。

## Idle

- 只有活着、未被特感控制的 Active Survivor 可以手动或自动 Idle。
- 倒地和挂边仍视为活着；允许 Idle。
- Idle 必须由 `GoAwayFromKeyboard` 建立原生关系，并在下一帧验证。
- 没有临时客户端槽时明确失败，不踢其他玩家。

## Join

优先级不可更改：自己的 Idle Bot、无人绑定且存活的 Survivor Bot、新创建的 Survivor Bot。Bot 创建请求全局串行；超时或请求者离线不会无限重试。只有创建新 Bot 会增加 Survivor 数并受 `sm_l4dp_survivor_limit` 约束。

## AFK

活动会重置 Auto Idle 时间，但不会延长 Free Spectator 占槽时间。任何合法原生 Idle 来源都进入统一 Idle Kick 计时。管理员只豁免 Kick，不豁免 Auto Idle。

## QoL

Bhop 只重放跳跃边沿，不修改速度或物理 CVar。角色切换同时修改 `m_survivorCharacter` 和模型，允许重复且不持久化。

