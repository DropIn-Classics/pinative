"""Synthetic regression tests for tools/cfg_inspect.py.

All fixtures are generated from scratch; no game bytes are embedded.
"""
from __future__ import annotations

import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))

import cfg_inspect  # noqa: E402


def make_cfg(
    driver: bytes = b"SB16.SDR",
    *,
    payload: bytes | None = None,
    header_tail: bytes | None = None,
) -> bytes:
    """Build a 544-byte configuration container from scratch."""
    if payload is None:
        payload = b"\x00" * cfg_inspect.PAYLOAD_SIZE
    assert len(payload) == cfg_inspect.PAYLOAD_SIZE
    header = bytearray(cfg_inspect.HEADER_SIZE)
    header[: len(driver)] = driver
    header[len(driver) : len(driver) + 1] = b"\x00"
    if header_tail is not None:
        start = len(driver) + 1
        header[start : start + len(header_tail)] = header_tail
    return bytes(header) + payload


def write_cfg(tmp: Path, data: bytes, name: str = "SYNTH.CFG") -> Path:
    path = tmp / name
    path.write_bytes(data)
    return path


class CfgInspectTest(unittest.TestCase):
    def test_valid_fixture_report(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            path = write_cfg(Path(tmp), make_cfg())
            report = cfg_inspect.inspect(path)
        self.assertIn("Size: 544 bytes (32-byte header + 512-byte payload)", report)
        self.assertIn("Configured sound driver: `SB16.SDR`", report)
        self.assertIn("# Configuration: `SYNTH.CFG`", report)

    def test_wrong_size_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            path = write_cfg(Path(tmp), make_cfg()[:-1])
            with self.assertRaisesRegex(ValueError, "expected 544"):
                cfg_inspect.inspect(path)

    def test_unterminated_driver_name_rejected(self) -> None:
        blob = b"A" * cfg_inspect.HEADER_SIZE + b"\x00" * cfg_inspect.PAYLOAD_SIZE
        with tempfile.TemporaryDirectory() as tmp:
            path = write_cfg(Path(tmp), blob)
            with self.assertRaisesRegex(ValueError, "no terminated driver name"):
                cfg_inspect.inspect(path)

    def test_invalid_driver_name_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            path = write_cfg(Path(tmp), make_cfg(driver=b"FOO.BIN"))
            with self.assertRaisesRegex(ValueError, "unexpected driver name"):
                cfg_inspect.inspect(path)

    def test_non_ascii_driver_name_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            path = write_cfg(Path(tmp), make_cfg(driver=b"\xff\xfe.BIN"))
            with self.assertRaisesRegex(ValueError, "not ASCII"):
                cfg_inspect.inspect(path)


if __name__ == "__main__":
    unittest.main()
