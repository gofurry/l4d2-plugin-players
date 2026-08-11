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
