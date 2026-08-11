# Changelog

## 1.0.0 - 2026-08-11

- Initializes every Players-created Bot with a round-robin Nick/Rochelle/Coach/Ellis identity before takeover, reapplies it after `RoundRespawn`, and verifies both `m_survivorCharacter` and the shared Survivor model mapping.
- Reuses the same identity/model helper as `!csm`, preventing newly-created Bots from retaining infected models or first-person arms without introducing a second model table.
- Added default-on, once-per-connection Auto Join with a 2.5-second initial delay and at most three bounded attempts; it calls the existing `LP_JoinSurvivor`/Join queue exclusively.
- Preserves an explicit `!spec` choice across changelevel for the rest of the connection while keeping manual `!join` available.
- Records whether a `bind_free_bot` takeover uses an existing Bot or a Bot newly created by Players.
- Applies placement and loadout only after a newly-created Bot takeover is verified as an Active Survivor; Idle returns and existing-Bot takeovers remain untouched.
- Added hull/ground/out-of-world checked placement around a randomly selected healthy human teammate, with living Bot fallback and safe failure that preserves the engine origin.
- Added the restricted mid-join loadout: random T1 pump/chrome/SMG/silenced SMG, map-observed melee with maintained legal fallback and pistol safety fallback, plus pills/adrenaline.
- Added `sm_l4dp_auto_join`, `sm_l4dp_midjoin_spawn_near_player`, and `sm_l4dp_midjoin_loadout`, all enabled by default.
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
