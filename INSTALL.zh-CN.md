# L4D2 Players 安装说明

## 1. 安装

解压 Release ZIP，将其中的 `left4dead2/` 合并到服务器游戏目录。最终应存在：

```text
left4dead2/addons/sourcemod/plugins/l4d2_players.smx
left4dead2/addons/sourcemod/gamedata/l4d2_players.txt
left4dead2/addons/sourcemod/translations/l4d2_players.phrases.txt
```

重启服务器或执行：

```text
sm plugins load l4d2_players
```

首次加载会生成 `left4dead2/cfg/sourcemod/l4d2_players.cfg`。若 gamedata 缺失或签名不匹配，插件会拒绝加载并在 SourceMod error log 中指出具体条目。

升级安装时，旧 `l4d2_players.cfg` 不会由 SourceMod 自动删除过期行。以下旁观踢出 CVar 已废弃，请从旧配置中手动删除：

```cfg
sm_l4dp_spectator_join_grace_seconds
sm_l4dp_spectator_kick_seconds
```

新安装自动生成的配置不包含这两项。Release 根目录的 `l4d2_players.cfg.example` 只列出 v1.0.0 仍支持的 CVar。Free Spectator 可无限旁观，不会因时间被 Players 踢出。

## 2. 推荐 server.cfg

```cfg
sm_cvar director_afk_timeout 999999
sm_cvar sb_all_bot_game 1
sm_cvar allow_all_bot_survivor_team 1
sm_cvar sv_hibernate_when_empty 1
```

插件只检查前三项并输出 warning，不会修改服务器全局 CVar。

`sb_all_bot_game 1`必须保留，以便所有真人处于 Engine Idle 时由绑定 Bot 继续游戏。默认启用的：

```cfg
sm_l4dp_human_team_wipe "1"
```

会在本轮所有参与 Survivor 的真人真正死亡后杀死剩余普通 Survivor Bot，让游戏自然进入 team wipe；它不会直接重启或换图。

## 3. Survivor 容量

默认：

```cfg
sm_l4dp_survivor_limit "4"
```

5+ 服务器可将其设置为 `5–16`。该值只是 Players 的业务上限；实际加入还受 `MaxClients` 和可用临时客户端槽影响。Idle 会同时保留真人 spectator 和 Survivor Bot，16 人服务器应为这些客户端实体预留足够容量。

官方 `survivor_limit` 应继续保持 `4`。Players 不会把它设置成 `sm_l4dp_survivor_limit`，而是在没有可接管 Bot 时直接使用内置 NextBot + RoundRespawn 创建路径，因此不会让 Director 在开局自动补出 8/12/16 个 Bot。Players 不检测或调用 L4DToolZ；服务器仍需由 L4DToolZ 等容量基础设施提供足够 `MaxClients`。

Players 创建的每个 Bot 会在 takeover 前按 Nick、Rochelle、Coach、Ellis round-robin 初始化 character/model，并在 RoundRespawn 后重新应用和验证。该步骤只存在于新 Bot 创建函数中，不会修改 Idle 返回或 existing Bot 的角色。

单真人诊断可由 root 管理员执行：

```text
sm_l4dp_survivor_limit 5
sm_l4dp_addbot 1
```

`sm_l4dp_addbot [count]` 默认创建 1 个，调用与 `!join` 完全相同的内部创建函数，不会突破 Players 上限，也不会进入普通玩家菜单。创建失败会在 SourceMod error log 中记录 SDKCall、容量、client slot、team 或 respawn 阶段。

## 4. 自动加入与 5+ 中途出生

默认配置：

```cfg
sm_l4dp_auto_join "1"
sm_l4dp_midjoin_spawn_near_player "1"
sm_l4dp_midjoin_loadout "1"
```

Auto Join 在真人完全进入游戏约 2.5 秒后复用 `!join` 路径，过渡状态最多进行 3 次有界尝试。容量已满时玩家保持 Free Spectator；主动 `!spec` 会抑制本次连接剩余时间（包括 changelevel 后）的 Auto Join，手动 `!join` 不受影响。

出生位置和装备只应用于“没有自己的 Idle Bot、没有现有空闲 Bot、由 Players 新建 Bot、takeover 最终成功”的 Join。安全位置检查失败时保留引擎出生点；近战优先复用地图中实际存在的 melee script，再尝试内置合法集合，全部失败时至少给予 pistol。该策略绝不会清理 `!afk` → `!join` 或接管现有 Bot 的武器。

## 5. 迁移旧插件

v1.0.0 已覆盖 MultiSlots 的 Survivor 加入、CreateSurvivorBot / `l4d_CreateSurvivorBot` 的无上限 Bot 创建以及旧 AFK dead-bot fix。正式部署前应停用这些重叠插件，避免它们同时换队、创建 Bot 或修复同一 Idle 关系。

Players 自身不依赖 Left4DHooks，但服务器上的其他 bugfix 可能依赖它；是否删除必须单独审计。

## 6. 更新 gamedata

游戏更新后若插件报告签名失败：

1. 不要强制加载插件；
2. 对当前 Windows/Linux `server` 二进制重新验证五个函数，并确认 Windows NextBot `E8` 调用点仍解析到有效函数入口；
3. 只更新 `gamedata/l4d2_players.txt`；
4. 重新执行 build、validate 和多人联调清单。
