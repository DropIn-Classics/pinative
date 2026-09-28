#!/usr/bin/env python3
"""Describe the installed Pinball Illusions configuration container."""

from __future__ import annotations

import argparse
import collections
import hashlib
import sys
from pathlib import Path


HEADER_SIZE = 32
PAYLOAD_SIZE = 512


def inspect(path: Path) -> str:
    data = path.read_bytes()
    expected = HEADER_SIZE + PAYLOAD_SIZE
    if len(data) != expected:
        raise ValueError(f"file is {len(data)} bytes, expected {expected}")

    name_end = data.find(b"\0", 0, HEADER_SIZE)
    if name_end < 0:
        raise ValueError("header has no terminated driver name")
    try:
        driver = data[:name_end].decode("ascii")
    except UnicodeDecodeError as error:
        raise ValueError("driver name is not ASCII") from error
    if not driver.upper().endswith(".SDR"):
        raise ValueError(f"unexpected driver name {driver!r}")

    payload = data[HEADER_SIZE:]
    words = [payload[offset:offset + 4] for offset in range(0, len(payload), 4)]
    common_word, common_count = collections.Counter(words).most_common(1)[0]
    printable_word = (
        common_word.decode("ascii")
        if all(0x20 <= byte <= 0x7E for byte in common_word)
        else common_word.hex().upper()
    )
    lines = [
        f"# Configuration: `{path.name}`",
        "",
        f"- Size: {len(data)} bytes (32-byte header + 512-byte payload)",
        f"- SHA-256: `{hashlib.sha256(data).hexdigest().upper()}`",
        f"- Configured sound driver: `{driver}`",
        f"- Header bytes after name: `{data[name_end + 1:HEADER_SIZE].hex(' ').upper()}`",
        f"- Most frequent payload dword: `{printable_word}` "
        f"({common_count} of {len(words)})",
        "",
    ]
    return "\n".join(lines)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("config", type=Path)
    arguments = parser.parse_args()
    try:
        print(inspect(arguments.config))
    except (OSError, ValueError) as error:
        print(f"error: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
