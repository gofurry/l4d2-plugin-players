from __future__ import annotations

import tempfile
import unittest
from pathlib import Path

from validate import parse_smc_phrases, validate_translation


class TranslationValidationTests(unittest.TestCase):
    def validate_text(self, text: str) -> None:
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "phrases.txt"
            path.write_text(text, encoding="utf-8")
            validate_translation(path)

    def test_parser_accepts_standard_sections(self) -> None:
        parsed = parse_smc_phrases(
            '"Phrases"\n{\n"Example"\n{\n"en" "English"\n"chi" "中文"\n}\n}\n'
        )
        self.assertEqual(parsed["Example"]["chi"], "中文")

    def test_parser_rejects_unclosed_section(self) -> None:
        with self.assertRaises(ValueError):
            parse_smc_phrases('"Phrases" { "Example" { "en" "English" }')

    def test_validator_rejects_printf_placeholder(self) -> None:
        text = '"Phrases" { "AutoIdleWarning" { "#format" "{1:d}" "en" "%d" "chi" "{1}" } }'
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "phrases.txt"
            path.write_text(text, encoding="utf-8")
            with self.assertRaisesRegex(ValueError, "printf placeholder"):
                validate_translation(path)

    def test_validator_rejects_wrong_format_type(self) -> None:
        sections = []
        for name in ("AutoIdleWarning", "IdleKickWarning"):
            format_type = "{1:f}" if name == "AutoIdleWarning" else "{1:d}"
            sections.append(
                f'"{name}" {{ "#format" "{format_type}" "en" "{{1}}" "chi" "{{1}}" }}'
            )
        sections.append(
            '"IdleKickBroadcast" { "#format" "{1:N}" "en" "{1}" "chi" "{1}" }'
        )
        text = '"Phrases" { ' + " ".join(sections) + " }"
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "phrases.txt"
            path.write_text(text, encoding="utf-8")
            with self.assertRaisesRegex(ValueError, "expected"):
                validate_translation(path)


if __name__ == "__main__":
    unittest.main()
