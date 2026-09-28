"""Synthetic regression tests for tools/sdr_inspect.py.

All fixtures are generated from scratch; no game bytes are embedded.
"""
from __future__ import annotations

import struct
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))

import sdr_inspect  # noqa: E402

SIZE = 0x200
ENTRY = 0x100
DISPATCH = ENTRY + 0x10
TABLE = 0x150
BANNER = b"SYNTH TEST\r\n"


def make_sdr(
    *,
    entry: int = ENTRY,
    table: int = TABLE,
    handlers: list[int] | None = None,
    guard_max: int = 0x12,
) -> bytes:
    """Build a minimal valid synthetic SDR image from scratch."""
    data = bytearray(SIZE)
    data[0] = sdr_inspect.ENTRY_OPCODE
    struct.pack_into("<i", data, 1, entry - 5)
    data[5 : 5 + len(BANNER)] = BANNER
    data[5 + len(BANNER)] = 0x1A
    if 0 <= entry < SIZE:
        data[entry : entry + 3] = bytes([0x83, 0xF8, guard_max])
    if 0 <= DISPATCH + 8 <= SIZE:
        data[DISPATCH : DISPATCH + 4] = sdr_inspect.DISPATCH_OPCODE
        struct.pack_into("<I", data, DISPATCH + 4, table)
    if handlers is None:
        handlers = [entry] * sdr_inspect.COMMAND_COUNT
    for index, handler in enumerate(handlers):
        offset = table + index * 4
        if 0 <= offset + 4 <= SIZE:
            struct.pack_into("<I", data, offset, handler & 0xFFFFFFFF)
    return bytes(data)


def write_sdr(tmp: Path, data: bytes, name: str = "SYNTH.SDR") -> Path:
    path = tmp / name
    path.write_bytes(data)
    return path


class SdrInspectTest(unittest.TestCase):
    def test_valid_fixture_report(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            path = write_sdr(Path(tmp), make_sdr())
            report = sdr_inspect.inspect(path)
        self.assertIn("Shared banner: `SYNTH TEST`", report)
        self.assertIn("Offset-zero jump target: `0x00000100`", report)
        self.assertIn("Command table: `0x00000150`", report)

    def test_entry_target_outside_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            path = write_sdr(Path(tmp), make_sdr(entry=SIZE + 0x10))
            with self.assertRaisesRegex(ValueError, "entry target .* lies outside"):
                sdr_inspect.inspect(path)

    def test_handler_target_outside_rejected(self) -> None:
        bad = [ENTRY] * sdr_inspect.COMMAND_COUNT
        bad[3] = SIZE + 5
        with tempfile.TemporaryDirectory() as tmp:
            path = write_sdr(Path(tmp), make_sdr(handlers=bad))
            with self.assertRaisesRegex(ValueError, "command 0x03 target .* lies outside"):
                sdr_inspect.inspect(path)

    def test_table_outside_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            path = write_sdr(Path(tmp), make_sdr(table=SIZE - 10))
            with self.assertRaisesRegex(ValueError, "command table .* lies outside"):
                sdr_inspect.inspect(path)


if __name__ == "__main__":
    unittest.main()
