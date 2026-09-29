"""Synthetic regression tests for tools/event_streams.py.

All fixtures are generated from scratch; no game bytes are embedded.
"""
from __future__ import annotations

import struct
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))

import event_streams as es  # noqa: E402

SIZE = 0x800


class Builder:
    """a module image with a REL set, pointers written as relocated"""

    def __init__(self):
        self.data = bytearray(SIZE)
        self.rel: set[int] = set()

    def word(self, at, *values):
        for i, v in enumerate(values):
            struct.pack_into("<H", self.data, at + 2 * i, v & 0xFFFF)

    def ptr(self, at, target):
        struct.pack_into("<I", self.data, at, target)
        self.rel.add(at)

    def slot(self, n, target):
        self.ptr(n * 4, target)

    def module(self):
        return es.Module(bytes(self.data), set(self.rel))


def sample() -> Builder:
    """slot 5: one hole zone; slot 15: one record with a display stream;
    slot 16: one counter whose threshold 1 starts a mode"""
    b = Builder()
    # zones: a type-4 zone whose object's +14h is event stream 0x300
    b.slot(5, 0x100)
    b.word(0x100, 10, 10, 20, 20, 4)
    b.ptr(0x10A, 0x140)
    b.word(0x10E, 0xFFFF)
    b.ptr(0x140 + 0x14, 0x300)
    # slot 15: record 0x180, +14h display stream 0x400
    b.slot(15, 0x120)
    b.ptr(0x120, 0x180)
    b.ptr(0x180 + 0x14, 0x400)
    # slot 16: counter 0x200, threshold 1 -> event 0x340, then -1
    b.slot(16, 0x130)
    b.ptr(0x130, 0x200)
    b.word(0x250, 1)
    b.ptr(0x254, 0x340)
    b.word(0x25C, 0xFFFF)
    # event 0x300: eject_at 0x140 0x140, end
    b.word(0x304, 0x18)
    b.ptr(0x306, 0x140)
    b.ptr(0x30A, 0x140)
    b.word(0x30E, 0)
    # event 0x340: mode 0x360, end
    b.word(0x344, 9)
    b.ptr(0x346, 0x360)
    b.word(0x34A, 0)
    # event 0x360 (the mode): loop_count 2; +4 display 0x400;
    # +A loop +4; +E wait 0 5 +1A; +18 end; +1A goto +18
    b.word(0x364, 0x1F, 2)
    b.word(0x368, 0x11)
    b.ptr(0x36A, 0x400)
    b.word(0x36E, 0x1D, 4)
    b.word(0x372, 0x1C, 0, 0, 5, 0x1A)
    b.word(0x37C, 0)
    b.word(0x37E, 0x0A, 0x18)
    # display 0x400: header word, priority, position; wait_s 3; clear; end
    b.word(0x406, 7, 3, 2, 0)
    return b


class EventStreamsTest(unittest.TestCase):
    def test_sizes_match_the_operands(self):
        for ops, sizes in es.LANGUAGE.values():
            for op, (_name, kinds) in ops.items():
                self.assertEqual(
                    2 + sum(es.OPERAND_SIZE[k] for k in kinds), sizes[op]
                )

    def test_walk_finds_every_stream_with_its_roots(self):
        streams = es.walk(sample().module())
        self.assertEqual(
            sorted(streams),
            [("D", 0x400), ("E", 0x300), ("E", 0x340), ("E", 0x360)],
        )
        self.assertEqual(
            streams[("D", 0x400)]["from"],
            ["slot-15 record 180h +14h", "event 360h +4h"],
        )
        mode = streams[("E", 0x360)]["commands"]
        self.assertEqual(sorted(mode), [0, 4, 0xA, 0xE, 0x18, 0x1A])
        self.assertEqual(mode[0xE], (0x1C, [0, 5, 0x1A]))

    def test_listing(self):
        text = es.listing(es.walk(sample().module()))
        self.assertIn("event 300h\n  from zone 100h (slot 5, type 4) object 140h +14h", text)
        self.assertIn("  +E    1C wait 0 5 +1A", text)
        self.assertIn("  +0    18 eject_at @140 @140", text)
        self.assertIn("display 400h", text)

    def test_unknown_opcode(self):
        b = sample()
        b.word(0x40A, 0x40)
        with self.assertRaisesRegex(es.StreamError, "unknown opcode 40h"):
            es.walk(b.module())

    def test_pointer_not_relocated(self):
        b = sample()
        b.rel.discard(0x346)
        with self.assertRaisesRegex(es.StreamError, "346h is not relocated"):
            es.walk(b.module())

    def test_jump_into_a_command(self):
        b = sample()
        b.word(0x380, 0x0F)  # goto +F: inside the wait at +E
        with self.assertRaisesRegex(es.StreamError, "overlap"):
            es.walk(b.module())

    def test_a_stream_of_both_languages(self):
        b = sample()
        b.ptr(0x36A, 0x360)  # display operand to an event stream
        with self.assertRaisesRegex(es.StreamError, "both an event and a display"):
            es.walk(b.module())


if __name__ == "__main__":
    unittest.main()
