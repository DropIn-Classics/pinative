"""Synthetic regression tests for tools/asset_inspect.py.

All fixtures are generated from scratch (mostly zero-filled); no game
bytes are embedded.
"""
from __future__ import annotations

import struct
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))

import asset_inspect  # noqa: E402

FIXED_SIZES = {
    "MASK1": 384 * 717 // 8,
    "MASK2": 280 * 720 // 8,
    "HIDE1.M": 280 * 720 // 8,
    "HIDE2.M": 280 * 720 // 8,
    "ANGLE1": 35 * 90,
    "ANGLE2": 35 * 90,
    "STAGE.M": 320 * 720,
    "STAGE.C": 256 * 3,
}


def pack_link(offsets: list[int]) -> bytes:
    assert len(offsets) == 2400
    return struct.pack(f">{len(offsets)}H", *offsets)


def make_valid_tree(directory: Path) -> None:
    """Create a minimal valid synthetic asset directory from scratch."""
    for name, size in FIXED_SIZES.items():
        (directory / name).write_bytes(b"\x00" * size)
    # Sparse zero-filled LINK/PIXELS pairs: all offsets zero, zero records.
    for number in (1, 2):
        (directory / f"LINK{number}").write_bytes(pack_link([0] * 2400))
        (directory / f"PIXELS{number}").write_bytes(b"")


class AssetInspectTest(unittest.TestCase):
    def test_valid_fixture_report(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp) / "synth"
            root.mkdir()
            make_valid_tree(root)
            report = asset_inspect.inspect(root)
        self.assertIn("# Asset inventory: `synth`", report)
        self.assertIn("20x120 buckets; 0 records", report)
        self.assertIn("`STAGE.M` | 320x720, 8 bpp", report)

    def test_non_monotonic_offsets_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp) / "synth"
            root.mkdir()
            make_valid_tree(root)
            (root / "LINK1").write_bytes(pack_link([1, 0] + [0] * 2398))
            with self.assertRaisesRegex(ValueError, "not monotonic"):
                asset_inspect.inspect(root)

    def test_count_mismatch_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp) / "synth"
            root.mkdir()
            make_valid_tree(root)
            # LINK1 ends at 0 but PIXELS1 claims one record.
            (root / "PIXELS1").write_bytes(bytes([0, 1]))
            with self.assertRaisesRegex(ValueError, "ends at 0.*has 1 records"):
                asset_inspect.inspect(root)

    def test_local_pixel_mismatch_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp) / "synth"
            root.mkdir()
            make_valid_tree(root)
            (root / "LINK1").write_bytes(pack_link([0] * 2399 + [1]))
            # Local pixel index 84 is out of the valid 0..83 range.
            (root / "PIXELS1").write_bytes(bytes([84, 0]))
            with self.assertRaisesRegex(ValueError, "above 83"):
                asset_inspect.inspect(root)


if __name__ == "__main__":
    unittest.main()
