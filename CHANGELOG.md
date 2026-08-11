# Changelog

## 0.3.2 - 2026-08-11

- Added configurable Human Team Wipe behavior while retaining `sb_all_bot_game 1` for all-human Idle play.
- Tracks humans who participated as Survivors during the round and distinguishes Active/Idle effective survival from ordinary Bots.
- Performs a delayed post-death check and kills remaining Survivor Bots only after every participating human has truly died.
- Added `sm_l4dp_human_team_wipe`, enabled by default.
- Fixed stale AFK, Idle Kick, and Free Spectator Kick hint boxes by explicitly clearing Players-owned hints when activity or player state leaves the current countdown phase.
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
- Added Idle and Free Spectator timeout HUDs and kicks.
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
