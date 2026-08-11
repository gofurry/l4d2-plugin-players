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

## 2. 推荐 server.cfg

```cfg
sm_cvar director_afk_timeout 999999
sm_cvar sb_all_bot_game 1
sm_cvar allow_all_bot_survivor_team 1
sm_cvar sv_hibernate_when_empty 1
```

插件只检查前三项并输出 warning，不会修改服务器全局 CVar。

## 3. Survivor 容量

默认：

```cfg
sm_l4dp_survivor_limit "4"
```

5+ 服务器可将其设置为 `5–16`。该值只是 Players 的业务上限；实际加入还受 `MaxClients` 和可用临时客户端槽影响。Idle 会同时保留真人 spectator 和 Survivor Bot，16 人服务器应为这些客户端实体预留足够容量。

Players 不检测或调用 L4DToolZ。若服务器容量层仍限制为原版人数，调高本 ConVar 不会突破底层限制。

## 4. 迁移旧插件

开发和首次测试期间继续保留现有 MultiSlots、CreateSurvivorBot 与 AFK dead-bot fix。只有完成 `docs/v0.1-test-checklist.md` 中的 4/5/8/16 人测试后，才逐个卸载旧插件并重新测试。

Players 自身不依赖 Left4DHooks，但服务器上的其他 bugfix 可能依赖它；是否删除必须单独审计。

## 5. 更新 gamedata

游戏更新后若插件报告签名失败：

1. 不要强制加载插件；
2. 对当前 Windows/Linux `server` 二进制重新验证四个函数和 `CDirector` 地址；
3. 只更新 `gamedata/l4d2_players.txt`；
4. 重新执行 build、validate 和多人联调清单。

