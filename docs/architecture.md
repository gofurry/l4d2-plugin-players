# Architecture

`plugin/src/l4d2_players.sp` 是唯一编译入口，所有 `.inc` 模块最终组成一个 `l4d2_players.smx`。

```text
commands / menu / AFK manager / QoL
              ↓
state + survivor_engine
              ↓
engine (SDKCall) + gamedata
              ↓
L4D2 server binary
```

- `engine.inc`：只准备和调用 L4D2 内部函数。
- `state.inc`：统一识别 Active Survivor、Engine Idle、Free Spectator 和特感控制状态。
- `survivor_engine.inc`：组合 bind/takeover，业务模块不直接操作 SDKCall。
- `join.inc`：串行化 Bot 创建，避免并发请求接管同一个 Bot。
- `runtime.inc`：保存每个客户端的活动、Idle、旁观和个人设置状态。
- `afk_monitor.inc`：唯一的长期 1 秒 Timer；没有 per-client 长期 Timer。

插件不暴露 v0.x Native/Forward，也不包含对其他功能型 SourceMod 插件的可选调用。

