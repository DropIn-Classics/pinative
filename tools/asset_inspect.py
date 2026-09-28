#!/usr/bin/env python3
"""Validate one decoded table's core rendering and collision assets."""

from __future__ import annotations

import argparse
import collections
import struct
import sys
from pathlib import Path


def require_size(name: str, data: bytes, expected: int) -> None:
    if len(data) != expected:
        raise ValueError(f"{name} is {len(data)} bytes, expected {expected}")


def bit_count(data: bytes) -> int:
    return sum(bin(byte).count("1") for byte in data)


def inspect_pair(directory: Path, number: int) -> list[str]:
    link = (directory / f"LINK{number}").read_bytes()
    pixels = (directory / f"PIXELS{number}").read_bytes()
    require_size(f"LINK{number}", link, 4800)
    if len(pixels) % 2:
        raise ValueError(f"PIXELS{number} has an incomplete record")
    offsets = [value[0] for value in struct.iter_unpack(">H", link)]
    if any(left > right for left, right in zip(offsets, offsets[1:])):
        raise ValueError(f"LINK{number} offsets are not monotonic")
    record_count = len(pixels) // 2
    if offsets[-1] != record_count:
        raise ValueError(
            f"LINK{number} ends at {offsets[-1]}, PIXELS{number} has "
            f"{record_count} records"
        )
    records = list(zip(pixels[::2], pixels[1::2]))
    if records and max(pixel for pixel, _kind in records) >= 84:
        raise ValueError(f"PIXELS{number} has a local pixel index above 83")
    kinds = collections.Counter(kind for _pixel, kind in records)
    return [
        f"| `LINK{number}` + `PIXELS{number}` | 20x120 buckets; "
        f"{record_count:,} records | offsets end at {offsets[-1]:,}; "
        f"{len(kinds)} material/type values |"
    ]


def inspect(directory: Path) -> str:
    files = {
        name: (directory / name).read_bytes()
        for name in (
            "MASK1", "MASK2", "ANGLE1", "ANGLE2", "HIDE1.M", "HIDE2.M",
            "STAGE.M", "STAGE.C",
        )
    }
    require_size("MASK1", files["MASK1"], 384 * 717 // 8)
    require_size("MASK2", files["MASK2"], 280 * 720 // 8)
    require_size("HIDE1.M", files["HIDE1.M"], 280 * 720 // 8)
    require_size("HIDE2.M", files["HIDE2.M"], 280 * 720 // 8)
    require_size("ANGLE1", files["ANGLE1"], 35 * 90)
    require_size("ANGLE2", files["ANGLE2"], 35 * 90)
    require_size("STAGE.M", files["STAGE.M"], 320 * 720)
    require_size("STAGE.C", files["STAGE.C"], 256 * 3)

    palette = list(zip(
        files["STAGE.C"][0::3],
        files["STAGE.C"][1::3],
        files["STAGE.C"][2::3],
    ))
    lines = [
        f"# Asset inventory: `{directory.name}`",
        "",
        "| Assets | Layout | Contents | Validation |",
        "| --- | --- | --- | --- |",
    ]
    for number in (1, 2):
        lines.extend(inspect_pair(directory, number))
    for name, width, height in (
        ("MASK1", 384, 717),
        ("MASK2", 280, 720),
        ("HIDE1.M", 280, 720),
        ("HIDE2.M", 280, 720),
    ):
        data = files[name]
        lines.append(
            f"| `{name}` | {width}x{height}, 1 bpp | "
            f"{bit_count(data):,} set pixels | {len(data):,} bytes |"
        )
    for name in ("ANGLE1", "ANGLE2"):
        data = files[name]
        lines.append(
            f"| `{name}` | 35x90 bytes | values {min(data)}..{max(data)}; "
            f"{len(set(data))} distinct | {len(data):,} bytes |"
        )
    lines.extend([
        f"| `STAGE.M` | 320x720, 8 bpp | {len(set(files['STAGE.M']))} "
        f"palette indices | {len(files['STAGE.M']):,} bytes |",
        f"| `STAGE.C` | 256 RGB888 triples | channel range "
        f"{min(files['STAGE.C'])}..{max(files['STAGE.C'])}; "
        f"{len(set(palette))} distinct colors | {len(files['STAGE.C']):,} bytes |",
        "",
    ])
    return "\n".join(lines)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("directory", type=Path)
    arguments = parser.parse_args()
    try:
        print(inspect(arguments.directory))
    except (OSError, ValueError) as error:
        print(f"error: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
