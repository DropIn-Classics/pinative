#!/usr/bin/env python3
"""Validate and describe a decoded Pinball Illusions SDR sound driver."""

from __future__ import annotations

import argparse
import hashlib
import struct
import sys
from pathlib import Path


ENTRY_OPCODE = 0xE9
DISPATCH_OPCODE = bytes.fromhex("2e ff 24 85")
COMMAND_COUNT = 0x13


def inspect(path: Path) -> str:
    data = path.read_bytes()
    if len(data) < 5 or data[0] != ENTRY_OPCODE:
        raise ValueError("driver does not begin with a near jump")

    entry = 5 + struct.unpack_from("<i", data, 1)[0]
    if not 0 <= entry < len(data):
        raise ValueError(f"entry target 0x{entry:X} lies outside the driver")

    banner_end = data.find(b"\x1a", 5, entry)
    if banner_end < 0:
        raise ValueError("driver banner has no 0x1A terminator")
    banner = data[5:banner_end].strip(b"\r\n")
    try:
        banner_text = banner.decode("ascii").replace("\r\n", " / ")
    except UnicodeDecodeError as error:
        raise ValueError("driver banner is not ASCII") from error

    dispatch = data.find(DISPATCH_OPCODE, entry, min(entry + 96, len(data)))
    if dispatch < 0 or dispatch + 8 > len(data):
        raise ValueError(
            f"no EAX jump-table dispatch near entry 0x{entry:X}"
        )
    table = struct.unpack_from("<I", data, dispatch + 4)[0]
    table_size = COMMAND_COUNT * 4
    if table + table_size > len(data):
        raise ValueError(f"command table 0x{table:X} lies outside the driver")

    handlers = struct.unpack_from(f"<{COMMAND_COUNT}I", data, table)
    for command, handler in enumerate(handlers):
        if handler >= len(data):
            raise ValueError(
                f"command 0x{command:02X} target 0x{handler:X} lies outside "
                "the driver"
            )

    guard = data.find(b"\x83\xf8", entry, dispatch)
    if guard < 0 or guard + 3 > len(data):
        raise ValueError("dispatch has no immediate EAX upper-bound check")
    accepted_maximum = data[guard + 2]

    lines = [
        f"# SDR driver: `{path.name}`",
        "",
        f"- Size: {len(data)} bytes (`0x{len(data):X}`)",
        f"- SHA-256: `{hashlib.sha256(data).hexdigest().upper()}`",
        f"- Shared banner: `{banner_text}`",
        f"- Offset-zero jump target: `0x{entry:08X}`",
        f"- Dispatch instruction: `0x{dispatch:08X}`",
        f"- Command table: `0x{table:08X}`",
        f"- Valid table entries: `0x00` through `0x{COMMAND_COUNT - 1:02X}`",
        "",
    ]
    if accepted_maximum >= COMMAND_COUNT:
        lines.extend([
            "Warning: the dispatcher accepts command "
            f"`0x{accepted_maximum:02X}`, but the table ends at "
            f"`0x{COMMAND_COUNT - 1:02X}`. Treat the extra value as invalid.",
            "",
        ])
    lines.extend([
        "| Command | Handler |",
        "| ---: | ---: |",
    ])
    for command, handler in enumerate(handlers):
        lines.append(f"| `0x{command:02X}` | `0x{handler:08X}` |")
    lines.append("")
    return "\n".join(lines)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("driver", type=Path)
    arguments = parser.parse_args()
    try:
        print(inspect(arguments.driver))
    except (OSError, ValueError) as error:
        print(f"error: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
