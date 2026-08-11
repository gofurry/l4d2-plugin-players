from __future__ import annotations

import re
import sys
from pathlib import Path


REQUIRED_GAMEDATA_KEYS = (
    '"CDirector"',
    '"CDirectorMusicBanks::OnRoundStart"',
    '"CTerrorPlayer::GoAwayFromKeyboard"',
    '"SurvivorBot::SetHumanSpectator"',
    '"CTerrorPlayer::TakeOverBot"',
    '"CDirector::AddSurvivorBot"',
    '"@TheDirector"',
)

FORBIDDEN_SOURCE_PATTERNS = (
    "#include <left4dhooks",
    "#include <l4dmultislots",
    "L4D_SetHumanSpec(",
    "L4D_TakeOverBot(",
    "L4DMultiSlots_Join(",
    "#include <createsurvivorbot",
    'CreateNative("CreateSurvivorBot"',
)

WINDOWS_SIGNATURES = {
    "CDirectorMusicBanks::OnRoundStart": "55 8B EC 83 EC ?? 56 57 8B F9 8B 0D ?? ?? ?? ?? E8 ?? ?? ?? ?? 84 C0 0F",
    "CTerrorPlayer::GoAwayFromKeyboard": "?? ?? ?? ?? ?? ?? 53 56 57 8B F1 8B 06 8B 90 C8 08 00 00",
    "SurvivorBot::SetHumanSpectator": "?? ?? ?? ?? ?? ?? 83 BE ?? ?? ?? ?? 00 7E 07 32 C0 5E 5D C2 04 00 8B 0D",
    "CTerrorPlayer::TakeOverBot": "?? ?? ?? ?? ?? ?? ?? ?? ?? A1 ?? ?? ?? ?? 33 C5 89 45 FC 53 56 8D 85",
    "CDirector::AddSurvivorBot": "55 8B EC 8B 89 ?? ?? ?? ?? 83 EC ?? 56 8D 45 FF",
}

EXPECTED_PLUGIN_VERSION = "0.3.4"

PARAMETERIZED_PHRASES = {
    "AutoIdleWarning": "{1:d}",
    "IdleKickWarning": "{1:d}",
    "IdleKickBroadcast": "{1:N}",
}


def tokenize_smc(text: str) -> list[str]:
    """Tokenize the quoted strings and braces used by SourceMod SMC files."""
    tokens: list[str] = []
    index = 0
    length = len(text)
    while index < length:
        if text[index].isspace():
            index += 1
            continue
        if text.startswith("//", index):
            newline = text.find("\n", index + 2)
            index = length if newline < 0 else newline + 1
            continue
        if text[index] in "{}":
            tokens.append(text[index])
            index += 1
            continue
        if text[index] != '"':
            raise ValueError(f"unexpected SMC character at offset {index}: {text[index]!r}")

        index += 1
        value: list[str] = []
        while index < length:
            character = text[index]
            if character == '"':
                index += 1
                tokens.append("".join(value))
                break
            if character == "\\":
                if index + 1 >= length:
                    raise ValueError("unterminated SMC escape sequence")
                value.append(character)
                value.append(text[index + 1])
                index += 2
                continue
            value.append(character)
            index += 1
        else:
            raise ValueError("unterminated SMC quoted string")
    return tokens


def parse_smc_phrases(text: str) -> dict[str, dict[str, str]]:
    tokens = tokenize_smc(text)
    position = 0

    def take(expected: str | None = None) -> str:
        nonlocal position
        if position >= len(tokens):
            raise ValueError(f"expected {expected or 'token'}, reached end of SMC file")
        token = tokens[position]
        position += 1
        if expected is not None and token != expected:
            raise ValueError(f"expected {expected!r}, found {token!r}")
        return token

    take("Phrases")
    take("{")
    phrases: dict[str, dict[str, str]] = {}
    while position < len(tokens) and tokens[position] != "}":
        phrase_name = take()
        if phrase_name in ("{", "}") or phrase_name in phrases:
            raise ValueError(f"invalid or duplicate phrase section: {phrase_name!r}")
        take("{")
        fields: dict[str, str] = {}
        while position < len(tokens) and tokens[position] != "}":
            key = take()
            value = take()
            if key in ("{", "}") or value in ("{", "}"):
                raise ValueError(f"phrase {phrase_name!r} must contain quoted key/value pairs")
            if key in fields:
                raise ValueError(f"phrase {phrase_name!r} contains duplicate field {key!r}")
            fields[key] = value
        take("}")
        phrases[phrase_name] = fields
    take("}")
    if position != len(tokens):
        raise ValueError("unexpected tokens after the Phrases section")
    return phrases


def validate_translation(path: Path) -> dict[str, dict[str, str]]:
    phrases = parse_smc_phrases(path.read_text(encoding="utf-8"))
    if not phrases:
        raise ValueError("translation file contains no phrase sections")

    for phrase_name, fields in phrases.items():
        for language in ("en", "chi"):
            if language not in fields or not fields[language]:
                raise ValueError(f"phrase {phrase_name!r} is missing language {language!r}")
            if re.search(r"%(?:d|N)\b", fields[language]):
                raise ValueError(f"phrase {phrase_name!r} uses a printf placeholder instead of {{1}}")

        expected_format = PARAMETERIZED_PHRASES.get(phrase_name)
        actual_format = fields.get("#format")
        if expected_format is None:
            if actual_format is not None:
                raise ValueError(f"non-parameterized phrase {phrase_name!r} unexpectedly declares #format")
            continue
        if actual_format != expected_format:
            raise ValueError(
                f"phrase {phrase_name!r} has #format {actual_format!r}; expected {expected_format!r}"
            )
        for language in ("en", "chi"):
            if "{1}" not in fields[language]:
                raise ValueError(f"phrase {phrase_name!r} language {language!r} does not use {{1}}")

    missing_parameterized = set(PARAMETERIZED_PHRASES) - set(phrases)
    if missing_parameterized:
        raise ValueError(f"missing parameterized phrase sections: {sorted(missing_parameterized)}")
    return phrases


def validate_state_machine_sources(root: Path) -> None:
    definitions = (root / "plugin" / "include" / "l4d2_players" / "definitions.inc").read_text(
        encoding="utf-8"
    )
    survivor_engine = (
        root / "plugin" / "include" / "l4d2_players" / "survivor_engine.inc"
    ).read_text(encoding="utf-8")
    idle = (root / "plugin" / "include" / "l4d2_players" / "idle.inc").read_text(encoding="utf-8")
    join = (root / "plugin" / "include" / "l4d2_players" / "join.inc").read_text(encoding="utf-8")
    config = (root / "plugin" / "include" / "l4d2_players" / "config.inc").read_text(encoding="utf-8")
    human_wipe = (
        root / "plugin" / "include" / "l4d2_players" / "human_team_wipe.inc"
    ).read_text(encoding="utf-8")

    if f'#define LP_VERSION "{EXPECTED_PLUGIN_VERSION}"' not in definitions:
        raise ValueError(f"LP_VERSION must be {EXPECTED_PLUGIN_VERSION}")
    required_engine_fragments = (
        "LP_TAKEOVER_PATH_RETURN_IDLE",
        "LP_TAKEOVER_PATH_BIND_FREE_BOT",
        "LP_IsActiveSurvivor(stateClient)",
        "LP_STATE_VERIFY_MAX_FRAMES",
        "LP_STATE_VERIFY_TIMEOUT",
    )
    for fragment in required_engine_fragments:
        if fragment not in survivor_engine:
            raise ValueError(f"takeover state verification fragment missing: {fragment}")

    return_path = survivor_engine.split("bool LP_ReturnFromIdleBot", 1)[1].split(
        "bool LP_BindAndTakeOverBot", 1
    )[0]
    if "ChangeClientTeam" in return_path or "LP_BindHumanToBot" in return_path:
        raise ValueError("return-from-idle wrapper must not change team or rebind the existing Idle Bot")
    if "LP_BeginTakeover(client, bot, LP_TAKEOVER_PATH_RETURN_IDLE)" not in return_path:
        raise ValueError("return-from-idle wrapper does not select the dedicated takeover path")

    for fragment in ("LP_IsEngineIdle(stateClient)", "LP_STATE_VERIFY_MAX_FRAMES", "LP_STATE_VERIFY_TIMEOUT"):
        if fragment not in idle:
            raise ValueError(f"bounded Idle verification fragment missing: {fragment}")
    if "LP_ReturnFromIdleBot(client, bot)" not in join:
        raise ValueError("join flow does not use the dedicated return-from-idle path")
    if "LP_BindAndTakeOverBot(client, bot)" not in join:
        raise ValueError("join flow does not retain the Free Spectator bind path")

    if 'CreateConVar("sm_l4dp_human_team_wipe", "1"' not in config:
        raise ValueError("human team wipe ConVar is missing or not enabled by default")
    required_wipe_fragments = (
        "LP_IsEffectiveHumanSurvivorAlive",
        "LP_IsEngineIdle(client)",
        "LP_FindIdleHumanForBot(bot) == 0",
        "CreateTimer(LP_HUMAN_WIPE_CHECK_DELAY",
        "ForcePlayerSuicide(bot)",
        'LP_Log("Human team wipe: no participating human survivor remains alive.")',
    )
    for fragment in required_wipe_fragments:
        if fragment not in human_wipe:
            raise ValueError(f"human team wipe invariant missing: {fragment}")
    for forbidden in ("ForceChangeLevel", "ServerCommand(\"restart", "ServerCommand(\"changelevel"):
        if forbidden in human_wipe:
            raise ValueError(f"human team wipe must not directly restart/change maps: {forbidden}")


def validate_afk_hint_sources(root: Path) -> None:
    definitions = (root / "plugin" / "include" / "l4d2_players" / "definitions.inc").read_text(
        encoding="utf-8"
    )
    runtime = (root / "plugin" / "include" / "l4d2_players" / "runtime.inc").read_text(
        encoding="utf-8"
    )
    hud = (root / "plugin" / "include" / "l4d2_players" / "hud.inc").read_text(
        encoding="utf-8"
    )
    afk = (root / "plugin" / "include" / "l4d2_players" / "afk_monitor.inc").read_text(
        encoding="utf-8"
    )
    idle = (root / "plugin" / "include" / "l4d2_players" / "idle.inc").read_text(
        encoding="utf-8"
    )
    takeover = (
        root / "plugin" / "include" / "l4d2_players" / "survivor_engine.inc"
    ).read_text(encoding="utf-8")
    join = (root / "plugin" / "include" / "l4d2_players" / "join.inc").read_text(
        encoding="utf-8"
    )
    spectate = (root / "plugin" / "include" / "l4d2_players" / "spectate.inc").read_text(
        encoding="utf-8"
    )
    main_source = (root / "plugin" / "src" / "l4d2_players.sp").read_text(encoding="utf-8")

    required_fragments = {
        "runtime.inc": ("bool autoIdleHudVisible;", "bool idleKickWarning;", "LP_StopAutoIdleHud(client);"),
        "hud.inc": (
            "void LP_StopAutoIdleHud(int client)",
            "!g_LPPlayers[client].autoIdleHudVisible",
            'PrintCenterText(client, " ");',
            "void LP_ShowAutoIdleCenterText(int client, int seconds)",
            "!LP_IsActiveSurvivor(client)",
            "LP_IsAutomaticIdleVerificationPending(client)",
            'PrintCenterText(client, "%T", "AutoIdleWarning", client, seconds);',
            "void LP_ClearIdleKickHint(int client)",
            "!g_LPPlayers[client].idleKickWarning",
            'PrintHintText(client, "%T", "IdleKickWarning", client, seconds);',
        ),
        "afk_monitor.inc": (
            'HookEvent("player_team", LP_EventPlayerTeam',
            "LP_StopAllAutoIdleHuds();",
            "LP_IsAutomaticIdleVerificationPending(client)",
            "LP_ShowAutoIdleCenterText(client",
            "LP_GoIdle(client, LP_IDLE_REASON_AUTOMATIC, false)",
            "default:",
            "LP_StopAutoIdleHud(client);",
        ),
        "idle.inc": (
            "LP_IsAutomaticIdleVerificationPending",
            "if (reason == LP_IDLE_REASON_AUTOMATIC)",
            "LP_StopAutoIdleHud(client);",
            "LP_StopAutoIdleHud(stateClient);",
        ),
        "survivor_engine.inc": ("if (success)", "LP_StopAutoIdleHud(client);", "LP_ClearIdleKickHint(client);"),
        "join.inc": ("LP_StopAutoIdleHud(client);",),
        "spectate.inc": ("LP_StopAutoIdleHud(client);",),
        "l4d2_players.sp": (
            "public void OnMapEnd()",
            "public void OnClientDisconnect(int client)",
            "LP_StopAutoIdleHud(client);",
            "LP_ClearIdleKickHint(client);",
        ),
    }
    contents = {
        "definitions.inc": definitions,
        "runtime.inc": runtime,
        "hud.inc": hud,
        "afk_monitor.inc": afk,
        "idle.inc": idle,
        "survivor_engine.inc": takeover,
        "join.inc": join,
        "spectate.inc": spectate,
        "l4d2_players.sp": main_source,
    }
    for filename, fragments in required_fragments.items():
        for fragment in fragments:
            if fragment not in contents[filename]:
                raise ValueError(f"AFK hint lifecycle fragment missing from {filename}: {fragment}")

    all_source = "\n".join(contents.values())
    for removed in (
        "LPPlayersHint",
        "playersHint",
        "LP_PLAYERS_HINT_",
        "LP_ClearAfkHint",
        "LP_ClearAllAfkHints",
        "LP_ShowAfkHint",
        "LP_ShowAutoIdleHint",
        "autoIdleWarning",
    ):
        if removed in all_source:
            raise ValueError(f"removed unified Hint lifecycle fragment remains: {removed}")

    auto_hud_path = hud.split("void LP_StopAutoIdleHud", 1)[1].split(
        "void LP_ClearIdleKickHint", 1
    )[0]
    for forbidden in ("HintText", "PrintHintText", "BfWrite", "StartMessage", 'PrintCenterText(client, "")'):
        if forbidden in auto_hud_path:
            raise ValueError(f"Auto Idle CenterText path must not touch HintText or use an empty clear: {forbidden}")
    if auto_hud_path.count('PrintCenterText(client, " ");') != 1:
        raise ValueError("Auto Idle CenterText may be covered only once with a guarded single space")

    if all_source.count("PrintHintText(") != 1:
        raise ValueError("only the preserved Idle Kick warning may use PrintHintText")
    if "PrintCenterText(" not in auto_hud_path:
        raise ValueError("Auto Idle warning must use PrintCenterText")

    active_path = afk.split("void LP_UpdateActiveClient", 1)[1].split(
        "void LP_UpdateIdleClient", 1
    )[0]
    if active_path.find("LP_StopAutoIdleHud(client);") > active_path.find(
        "LP_GoIdle(client, LP_IDLE_REASON_AUTOMATIC, false);"
    ):
        raise ValueError("automatic Idle request must stop CenterText before LP_GoIdle")


def validate_free_spectator_sources(root: Path, phrases: dict[str, dict[str, str]]) -> None:
    commands = (root / "plugin" / "include" / "l4d2_players" / "commands.inc").read_text(
        encoding="utf-8"
    )
    spectate = (root / "plugin" / "include" / "l4d2_players" / "spectate.inc").read_text(
        encoding="utf-8"
    )
    join = (root / "plugin" / "include" / "l4d2_players" / "join.inc").read_text(
        encoding="utf-8"
    )
    hud = (root / "plugin" / "include" / "l4d2_players" / "hud.inc").read_text(
        encoding="utf-8"
    )

    for fragment in (
        'RegConsoleCmd("sm_spec", LP_CommandSpectatePublic)',
        'RegConsoleCmd("sm_spectate", LP_CommandSpectatePublic)',
        'menu.AddItem("spectate", text)',
        'StrEqual(info, "spectate")',
    ):
        if fragment not in commands:
            raise ValueError(f"explicit Free Spectator command/menu fragment missing: {fragment}")

    required_spectate_fragments = (
        'SetEntProp(bot, Prop_Send, "m_humanSpectatorUserID", 0)',
        "ChangeClientTeam(client, LP_TEAM_SPECTATOR)",
        "LP_FindIdleBotForHuman(stateClient) == 0",
        "originalBotPreserved",
        "LP_STATE_VERIFY_MAX_FRAMES",
        "LP_STATE_VERIFY_TIMEOUT",
        "LP_SPECTATE_STABLE_FRAMES",
        "LP_HumanTeamWipeLeaveForFreeSpectator(client)",
        "LP_StopAutoIdleHud(client)",
    )
    for fragment in required_spectate_fragments:
        if fragment not in spectate:
            raise ValueError(f"Free Spectator state invariant missing: {fragment}")
    if "LP_CommandAfk" in spectate or "LP_GoIdle" in spectate:
        raise ValueError("!spec must not alias or route through the native Idle command")

    if "LP_ReturnFromIdleBot(client, bot)" not in join or "LP_BindAndTakeOverBot(client, bot)" not in join:
        raise ValueError("!join must retain distinct Engine Idle and Free Spectator takeover paths")

    prefix = phrases.get("ChatPrefix", {})
    if prefix.get("en") != "[Notice]" or prefix.get("chi") != "[提示]":
        raise ValueError("chat prefix must be localized as [Notice] / [提示]")
    if '"ChatPrefix", client' not in hud or "LP_PREFIX" in hud:
        raise ValueError("player chat messages must use the localized ChatPrefix phrase")


def validate_spectator_kick_removed(root: Path, phrases: dict[str, dict[str, str]]) -> None:
    source_root = root / "plugin"
    source = "\n".join(path.read_text(encoding="utf-8") for path in source_root.rglob("*.*"))
    forbidden = (
        "sm_l4dp_spectator_join_grace_seconds",
        "sm_l4dp_spectator_kick_seconds",
        "g_LPCvarSpectatorGrace",
        "g_LPCvarSpectatorKickSeconds",
        "g_LPSpectatorGraceSeconds",
        "g_LPSpectatorKickSeconds",
        "spectatorSince",
        "spectatorKickWarning",
        "LP_PLAYERS_HINT_SPECTATOR_KICK",
        "LP_ShowSpectatorKickHint",
        "LP_UpdateFreeSpectator",
        'KickClient(client, "Spectator timeout")',
        "SpectatorKickWarning",
    )
    for fragment in forbidden:
        if fragment in source:
            raise ValueError(f"removed Spectator Kick source fragment remains: {fragment}")
    if "SpectatorKickWarning" in phrases:
        raise ValueError("removed SpectatorKickWarning translation remains")
    if source.count("KickClient(") != 1 or 'KickClient(client, "Idle timeout")' not in source:
        raise ValueError("only the existing Idle Kick path may call KickClient")

    config_source = (root / "plugin" / "include" / "l4d2_players" / "config.inc").read_text(
        encoding="utf-8"
    )
    example_config = (root / "l4d2_players.cfg.example").read_text(encoding="utf-8")
    declared_cvars = set(re.findall(r'CreateConVar\("(sm_l4dp_[^"]+)"', config_source))
    example_cvars = set(re.findall(r"(?m)^(sm_l4dp_[a-z0-9_]+)\s", example_config))
    if declared_cvars != example_cvars:
        raise ValueError(
            f"example config CVar mismatch; missing={sorted(declared_cvars - example_cvars)}, "
            f"stale={sorted(example_cvars - declared_cvars)}"
        )

    checklist = (root / "docs" / "v0.3-test-checklist.md").read_text(encoding="utf-8")
    for stale in (
        "sm_l4dp_spectator_join_grace_seconds",
        "sm_l4dp_spectator_kick_seconds",
        "Free Spectator Kick",
    ):
        if stale in checklist:
            raise ValueError(f"removed Spectator Kick test remains: {stale}")


def compile_signature(spec: str) -> re.Pattern[bytes]:
    chunks: list[bytes] = []
    for token in spec.split():
        chunks.append(b"." if token == "??" else re.escape(bytes([int(token, 16)])))
    return re.compile(b"".join(chunks), re.DOTALL)


def main() -> int:
    if len(sys.argv) not in (2, 3):
        print("usage: validate.py <project-root> [server.dll]", file=sys.stderr)
        return 2

    root = Path(sys.argv[1]).resolve()
    source_root = root / "plugin"
    gamedata_path = root / "gamedata" / "l4d2_players.txt"
    translation_path = root / "translations" / "l4d2_players.phrases.txt"

    source = "\n".join(path.read_text(encoding="utf-8") for path in source_root.rglob("*.*"))
    lowered = source.lower()
    for forbidden in FORBIDDEN_SOURCE_PATTERNS:
        if forbidden.lower() in lowered:
            raise SystemExit(f"forbidden runtime dependency reference found: {forbidden}")

    gamedata = gamedata_path.read_text(encoding="utf-8")
    for key in REQUIRED_GAMEDATA_KEYS:
        if key not in gamedata:
            raise SystemExit(f"required gamedata key missing: {key}")
    for name, spec in WINDOWS_SIGNATURES.items():
        encoded = "".join("\\x2A" if token == "??" else f"\\x{token}" for token in spec.split())
        if encoded not in gamedata:
            raise SystemExit(f"gamedata Windows signature does not match validator baseline: {name}")

    phrases = validate_translation(translation_path)
    validate_state_machine_sources(root)
    validate_afk_hint_sources(root)
    validate_free_spectator_sources(root, phrases)
    validate_spectator_kick_removed(root, phrases)
    print(f"SourceMod translation SMC validation passed ({len(phrases)} phrase sections).")
    print("Idle/takeover state-machine source invariants passed.")
    print("Auto Idle CenterText and Idle Kick HintText lifecycle invariants passed.")
    print("Explicit Free Spectator and localized chat-prefix invariants passed.")
    print("Spectator Kick removal and current configuration invariants passed.")
    print("Static dependency and gamedata key validation passed.")

    if len(sys.argv) == 3 and sys.argv[2]:
        binary_path = Path(sys.argv[2]).resolve()
        binary = binary_path.read_bytes()
        for name, spec in WINDOWS_SIGNATURES.items():
            count = sum(1 for _ in compile_signature(spec).finditer(binary))
            if count != 1:
                raise SystemExit(f"Windows signature {name!r} matched {count} locations; expected exactly 1")
            print(f"Windows signature {name}: unique match")
        print(f"Windows gamedata signatures validated against {binary_path}")
    else:
        print("GameServerBinaryPath is not configured; skipped Windows binary signature validation.")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
