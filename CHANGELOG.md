# Changelog

## 1.0.0 - 2026-08-12

- Added isolated personal night vision in `nightvision.inc`, using one parented pure-white `light_dynamic` per enabled living human Survivor and owner-only `SDKHook_SetTransmit` visibility.
- Added eight fixed distance levels from 125 through 1000 while keeping Source brightness fixed; ClientPrefs stores only the 1–8 level, never the enabled state.
- Added `!ysy`, the main-menu Night Vision entry, and a three-action submenu for toggle/increase/decrease with immediate in-place distance updates.
- Added double-F menu open/close detection around flashlight impulse 100 with a 0.30-second window and 0.45-second cooldown, without consuming or changing the original flashlight input.
- Added independent light lifecycle cleanup/restoration for Idle, Join, Free Spectator, death/respawn, round/map end, disconnect, and configuration disable without adding night vision state to Population, Identity, or session transitions.
- Added `sm_l4dp_nightvision_enabled` and `sm_l4dp_nightvision_default_level`, defaulting to `1` and `2` respectively.
- Added a unified Survivor Identity Lifecycle around `player_bot_replace` and `bot_player_replace`; character/model identity now transfers in both directions and is verified again on the next frame for Idle, existing-Bot, newly-created-Bot, and Auto Join takeovers.
- Added `sm_l4dp_min_survivors` (default 4) and a bounded, initialization-only Population Manager for map start, round start, and the first human client. It uses the existing unlimited creation engine, never writes Valve `survivor_limit`, and does not replenish combat deaths or Human Team Wipe.
- Ordered Auto Join behind successful baseline reconciliation so the first standard Coop human receives one controlled Survivor plus three normal Survivor Bots; newly-created 5+ mid-join behavior remains isolated to post-baseline expansion.
- Made manual and automatic Idle one transactional session transition that records serial/reason/original identity/replacement Bot/start time and commits only after team, binding, and identity all converge.
- Added Idle timeout rollback through the existing takeover state machine. Rollback never runs mid-join placement/loadout and emits fatal-stage diagnostics if recovery itself cannot restore Active Survivor state.
- Separated lifecycle ownership: `population.inc` owns initial Survivor body count, `identity.inc` owns human↔Bot identity transfer, session modules own Active/Idle/Free Spectator/takeover, and `midjoin.inc` owns only verified newly-created 5+ placement/loadout.
- Initializes every Players-created Bot with a round-robin Nick/Rochelle/Coach/Ellis identity before takeover, reapplies it after `RoundRespawn`, and verifies both `m_survivorCharacter` and the shared Survivor model mapping.
- Reuses the same identity/model helper as `!csm`, preventing newly-created Bots from retaining infected models or first-person arms without introducing a second model table.
- Added default-on, once-per-connection Auto Join with a 2.5-second initial delay and at most three bounded attempts; it calls the existing `LP_JoinSurvivor`/Join queue exclusively.
- Preserves an explicit `!spec` choice across changelevel for the rest of the connection while keeping manual `!join` available.
- Records whether a `bind_free_bot` takeover uses an existing Bot or a Bot newly created by Players.
- Applies placement and loadout only after a newly-created Bot takeover is verified as an Active Survivor; Idle returns and existing-Bot takeovers remain untouched.
- Added hull/ground/out-of-world checked placement around a randomly selected healthy human teammate, with living Bot fallback and safe failure that preserves the engine origin.
- Added the restricted mid-join loadout: random T1 pump/chrome/SMG/silenced SMG, map-observed melee with maintained legal fallback and pistol safety fallback, plus pills/adrenaline.
- Added `sm_l4dp_auto_join`, `sm_l4dp_midjoin_spawn_near_player`, and `sm_l4dp_midjoin_loadout`, all enabled by default.
- Includes Human Team Wipe, Auto Idle/Idle Kick, `!zs`, ClientPrefs-backed `!bhop`, and eight-character `!csm` in the stable v1.0.0 feature set.
- Declared replacement of overlapping MultiSlots, CreateSurvivorBot, and legacy AFK dead-bot fix behavior without adding a third-party runtime dependency.

## 0.3.5 - 2026-08-11

- Replaced the Director-based Bot request with self-contained `NextBotCreatePlayerBot<SurvivorBot>` creation followed by team assignment and `CTerrorPlayer::RoundRespawn` when needed.
- Added Windows call-target resolution and Linux symbols for both required engine functions; all five Windows signatures are uniquely matched against local build `23990068`.
- Kept Valve `survivor_limit` untouched at 4; `sm_l4dp_survivor_limit` remains only the Players-managed 1–16 capacity.
- Routed `!join` through the unlimited creation path when no owned or available Bot exists.
- Added root-only `sm_l4dp_addbot [count]` diagnostics using exactly the same internal creation function as `!join`.
- Added explicit creation-stage logging for SDKCall, client-slot, capacity, client validation, team transition, and respawn failures.

## 0.3.4 - 2026-08-11

- Replaced Auto Idle `HintText` warnings with independently managed `PrintCenterText` countdowns, avoiding the native L4D2 spectator/takeover HintText channel entirely.
- Removed the shared Players Hint enum and split Auto Idle CenterText visibility from the unchanged Idle Kick HintText lifecycle.
- Stops Auto Idle CenterText immediately on activity, automatic Idle request, Idle success/failure, `!join`, `!spec`, death, team changes, round/map end, and disconnect.
- Allows only one guarded single-space CenterText cover after a visible Auto Idle countdown; Auto Idle never sends empty strings, HintText messages, or formatting arguments to clear its display.

## 0.3.3 - 2026-08-11

- Removed the Spectator Kick feature, its two ConVars, warning HUD, runtime state, translations, and kick path; Free Spectators may now remain indefinitely.
- Preserved explicit `!spec` / `!spectate`, the player-menu entry, Free Spectator recognition, and Free Spectator `!join` takeover/Bot-creation behavior.
- Fixed occasional numeric remnants after Auto Idle by clearing the L4D2 `HintText` usermessage directly without format strings or countdown arguments.
- Documented removal of stale `sm_l4dp_spectator_join_grace_seconds` and `sm_l4dp_spectator_kick_seconds` entries from upgraded configuration files.

## 0.3.2 - 2026-08-11

- Added configurable Human Team Wipe behavior while retaining `sb_all_bot_game 1` for all-human Idle play.
- Tracks humans who participated as Survivors during the round and distinguishes Active/Idle effective survival from ordinary Bots.
- Performs a delayed post-death check and kills remaining Survivor Bots only after every participating human has truly died.
- Added `sm_l4dp_human_team_wipe`, enabled by default.
- Fixed stale AFK and Idle Kick hint boxes by explicitly clearing Players-owned hints when activity or player state leaves the current countdown phase.
- Added explicit `!spec` / `!spectate` Free Spectator transitions, including verified release of Engine Idle Bot ownership and a player-menu entry.
- Localized the chat prefix as `[Notice]` for English and `[提示]` for Simplified Chinese clients.

## 0.3.1 - 2026-08-11

- Fixed Linux engine Idle verification by waiting for bounded multi-frame state convergence.
- Added a dedicated return-from-idle takeover path that preserves the existing human/Bot relationship.
- Kept Free Spectator takeover on the separate bind-and-takeover path.
- Made Active Survivor state, rather than the immediate SDKCall return value, authoritative for takeover success.
- Added structured timeout diagnostics for Idle and takeover state failures.

## 0.3.0 - 2026-08-11

- Added one-second AFK manager with activity detection and automatic engine Idle.
- Added Idle timeout HUD and kick handling.
- Added administrator kick immunity and server policy diagnostics.
- Added full English and Simplified Chinese translations.
- Fixed translation resources to use standard multi-line SMC sections and typed `#format` placeholders.

## 0.2.0 - 2026-08-11

- Added personal automatic bunny hop with ClientPrefs persistence.
- Added immediate selection of all eight Survivor characters without persistence.

## 0.1.0 - 2026-08-11

- Added self-contained gamedata and SDKCall engine abstraction.
- Added Survivor Bot creation, human/bot binding, takeover and serialized join requests.
- Added player menu, `!afk`, `!join`, `!zs` and 5+ Survivor policy.
- Added recovery for a dying Bot associated with an Idle human.
