"""Synthetic regression tests for tools/bpc_inspect.py.

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

import bpc_inspect  # noqa: E402

BPC_SIZE = 512
AUDIO_BASE = 0x100
RECORD_COUNT = 5


def make_bpc(
    *,
    record_count: int = RECORD_COUNT,
    selector_indices: tuple[int, int, int, int] = (0, 1, 2, 3),
 ) -> tuple[bytes, bytes]:
    """Build a minimal valid synthetic (BPC, REL) pair from scratch.

    Only header slots 34-38 are non-null, pointing at a contiguous run of
    tagged 12-byte audio records. Every non-null header slot has a REL entry.
    """
    bpc = bytearray(BPC_SIZE)
    for index in range(record_count):
        struct.pack_into(
            "<6h", bpc, AUDIO_BASE + index * 12, 4, index, index + 1, index + 2, 10 + index, 20 + index
        )
    header = [0] * bpc_inspect.HEADER_SLOTS
    header[34] = AUDIO_BASE
    for slot, record_index in zip(range(35, 39), selector_indices):
        header[slot] = AUDIO_BASE + record_index * 12
    struct.pack_into(f"<{bpc_inspect.HEADER_SLOTS}I", bpc, 0, *header)
    offsets = [slot * 4 for slot in range(34, 39)]
    rel = struct.pack(f"<{len(offsets)}I", *offsets)
    return bytes(bpc), rel


def write_pair(tmp: Path, bpc: bytes, rel: bytes) -> tuple[Path, Path]:
    bpc_path = tmp / "SYNTH.BPC"
    rel_path = tmp / "SYNTH.REL"
    bpc_path.write_bytes(bpc)
    rel_path.write_bytes(rel)
    return bpc_path, rel_path


class BpcInspectTest(unittest.TestCase):
    def test_valid_fixture_report(self) -> None:
        bpc, rel = make_bpc()
        with tempfile.TemporaryDirectory() as tmp:
            bpc_path, rel_path = write_pair(Path(tmp), bpc, rel)
            report = bpc_inspect.inspect(bpc_path, rel_path)
        self.assertIn(
            "Slot 34 begins 5 contiguous 12-byte records tagged 4.", report
        )
        self.assertIn("REL entries: 5 (20 bytes)", report)
        self.assertIn("BPC size: 512 bytes", report)

    def test_duplicate_relocation_rejected(self) -> None:
        bpc, _rel = make_bpc()
        dup = struct.pack("<6I", 34 * 4, 34 * 4, 35 * 4, 36 * 4, 37 * 4, 38 * 4)
        with tempfile.TemporaryDirectory() as tmp:
            bpc_path, rel_path = write_pair(Path(tmp), bpc, dup)
            with self.assertRaisesRegex(ValueError, "duplicate"):
                bpc_inspect.inspect(bpc_path, rel_path)

    def test_out_of_range_relocation_rejected(self) -> None:
        bpc, rel = make_bpc()
        bad = rel + struct.pack("<I", BPC_SIZE)
        with tempfile.TemporaryDirectory() as tmp:
            bpc_path, rel_path = write_pair(Path(tmp), bpc, bad)
            with self.assertRaisesRegex(ValueError, "lies outside the BPC"):
                bpc_inspect.inspect(bpc_path, rel_path)

    def test_invalid_audio_selector_rejected(self) -> None:
        # Misaligned selector: one byte past the base, not on a record.
        bpc = bytearray(make_bpc()[0])
        struct.pack_into("<I", bpc, 35 * 4, AUDIO_BASE + 1)
        _bpc, rel = make_bpc()
        with tempfile.TemporaryDirectory() as tmp:
            bpc_path, rel_path = write_pair(Path(tmp), bytes(bpc), rel)
            with self.assertRaisesRegex(ValueError, "does not select a record"):
                bpc_inspect.inspect(bpc_path, rel_path)

    def test_slot31_record_run(self) -> None:
        data = bytearray(256)
        header = [0] * bpc_inspect.HEADER_SLOTS
        header[31] = 0x40
        record = (2, 80, 64, 17, 1234, 567, 1, 0, 0, 0, 0, 0xC0)
        struct.pack_into("<11HI", data, 0x40, *record)
        base, records = bpc_inspect.slot31_record_run(
            bytes(data), header, {0x40 + 22}
        )
        self.assertEqual(base, 0x40)
        self.assertEqual(records, [record])

    def test_slot22_fixed_stride_and_sentinel(self) -> None:
        data = bytearray(0x900)
        header = [0] * bpc_inspect.HEADER_SLOTS
        header[22] = 0x80
        header[1] = bpc_inspect.LOAD_ONE_FLAG | 0x60
        relocations = set()
        for index in (0, 2, 3):
            offset = 0x80 + index * bpc_inspect.SLOT22_RECORD_SIZE
            struct.pack_into("<6H", data, offset, 1, 100, 200, 10, 28, 0)
            struct.pack_into("<I", data, offset + 0x1C, 0x60)
            data[offset + bpc_inspect.SLOT22_RECORD_SIZE - 1] = 0xFF
            relocations.add(offset + 0x1C)
        data[0x80 + bpc_inspect.SLOT22_RECORD_SIZE] = 3
        sentinel = 0x80 + 4 * bpc_inspect.SLOT22_RECORD_SIZE
        base, records = bpc_inspect.slot22_record_run(
            bytes(data), header, relocations
        )
        self.assertEqual(base, 0x80)
        self.assertEqual([record[0] for record in records], [1, 3, 1, 1])
        self.assertEqual(data[sentinel], 0)

    def test_slot22_missing_sentinel_is_rejected(self) -> None:
        size = 0x40 + bpc_inspect.SLOT22_RECORD_SIZE
        data = bytearray(size)
        header = [0] * bpc_inspect.HEADER_SLOTS
        header[22] = 0x40
        data[0x40] = 1
        with self.assertRaisesRegex(ValueError, "no zero sentinel"):
            bpc_inspect.slot22_record_run(bytes(data), header, set())

    def test_slot22_active_mask_relocation_is_required(self) -> None:
        data = bytearray(0x900)
        header = [0] * bpc_inspect.HEADER_SLOTS
        header[22] = 0x80
        header[1] = bpc_inspect.LOAD_ONE_FLAG | 0x60
        for index in (0, 2, 3):
            offset = 0x80 + index * bpc_inspect.SLOT22_RECORD_SIZE
            struct.pack_into("<6H", data, offset, 1, 100, 200, 10, 28, 0)
            struct.pack_into("<I", data, offset + 0x1C, 0x60)
            data[offset + bpc_inspect.SLOT22_RECORD_SIZE - 1] = 0xFF
        data[0x80 + bpc_inspect.SLOT22_RECORD_SIZE] = 3
        with self.assertRaisesRegex(ValueError, "mask pointer relocated"):
            bpc_inspect.slot22_record_run(bytes(data), header, set())

    def test_offset_directory(self) -> None:
        data = bytearray(128)
        header = [0] * bpc_inspect.HEADER_SLOTS
        header[12] = 0x20
        struct.pack_into("<4H", data, 0x20, 8, 20, 42, 0)
        base, offsets = bpc_inspect.offset_directory(bytes(data), header, 12)
        self.assertEqual(base, 0x20)
        self.assertEqual(offsets, [8, 20, 42])

    def test_offset_directory_terminator_is_required(self) -> None:
        data = bytearray(128)
        header = [0] * bpc_inspect.HEADER_SLOTS
        header[13] = 0x20
        struct.pack_into("<3H", data, 0x20, 6, 20, 24)
        with self.assertRaisesRegex(ValueError, "zero terminator"):
            bpc_inspect.offset_directory(bytes(data), header, 13)

    def test_object_pointer_list(self) -> None:
        data = bytearray(128)
        header = [0] * bpc_inspect.HEADER_SLOTS
        header[14] = 0x20
        struct.pack_into("<3I", data, 0x20, 0x50, 0x60, 0)
        base, pointers = bpc_inspect.object_pointer_list(
            bytes(data), header, {0x20, 0x24}, 14
        )
        self.assertEqual(base, 0x20)
        self.assertEqual(pointers, [0x50, 0x60])

    def test_object_pointer_relocation_is_required(self) -> None:
        data = bytearray(128)
        header = [0] * bpc_inspect.HEADER_SLOTS
        header[16] = 0x20
        struct.pack_into("<2I", data, 0x20, 0x50, 0)
        with self.assertRaisesRegex(ValueError, "not relocated"):
            bpc_inspect.object_pointer_list(bytes(data), header, set(), 16)

    def test_slot14_light_group_and_chain(self) -> None:
        data = bytearray(512)
        header = [0] * bpc_inspect.HEADER_SLOTS
        header[14] = 0x20
        struct.pack_into("<2I", data, 0x20, 0x80, 0)
        struct.pack_into("<IBBI", data, 0x80, 0x100, 2, 0, 0)
        struct.pack_into("<I", data, 0x100 + 0x0C, 0x100 + 0x14)
        struct.pack_into("<I", data, 0x100 + 0x10, 0x140)
        struct.pack_into("<H", data, 0x100 + 0x1C, 7)
        struct.pack_into("<H", data, 0x140 + 0x1C, 8)
        relocations = {0x20, 0x80, 0x10C, 0x110}
        base, groups, states = bpc_inspect.slot14_light_groups(
            bytes(data), header, relocations
        )
        self.assertEqual(base, 0x20)
        self.assertEqual(groups[0][-1], (0x100, 0x140))
        self.assertEqual(states, [0x100, 0x140])

    def test_slot14_next_pointer_relocation_is_required(self) -> None:
        data = bytearray(512)
        header = [0] * bpc_inspect.HEADER_SLOTS
        header[14] = 0x20
        struct.pack_into("<2I", data, 0x20, 0x80, 0)
        struct.pack_into("<IBBI", data, 0x80, 0x100, 0, 0, 0)
        struct.pack_into("<I", data, 0x100 + 0x10, 0x140)
        with self.assertRaisesRegex(ValueError, "invalid next pointer"):
            bpc_inspect.slot14_light_groups(bytes(data), header, {0x20, 0x80})

    def test_slot14_auxiliary_event_sequence(self) -> None:
        data = bytearray(512)
        header = [0] * bpc_inspect.HEADER_SLOTS
        header[14] = 0x20
        struct.pack_into("<2I", data, 0x20, 0x80, 0)
        struct.pack_into("<IBBI", data, 0x80, 0x100, 0, 0, 0x140)
        struct.pack_into("<HIH", data, 0x144, 1, 0x180, 0)
        _base, groups, _states = bpc_inspect.slot14_light_groups(
            bytes(data), header, {0x20, 0x80, 0x86}
        )
        self.assertEqual(groups[0][3], 0x140)

    def test_invalid_event_opcode_is_rejected(self) -> None:
        data = bytearray(32)
        struct.pack_into("<H", data, 4, len(bpc_inspect.EVENT_COMMAND_SIZES))
        with self.assertRaisesRegex(ValueError, "invalid opcode"):
            bpc_inspect.event_sequence(bytes(data), 0)

    def test_slot15_timed_effect_pointer_fields(self) -> None:
        data = bytearray(512)
        header = [0] * bpc_inspect.HEADER_SLOTS
        header[15] = 0x20
        struct.pack_into("<2I", data, 0x20, 0x80, 0)
        struct.pack_into("<I", data, 0x80, 8)
        struct.pack_into("<I", data, 0x84, 0x100)
        base, records = bpc_inspect.slot15_effect_records(
            bytes(data), header, {0x20, 0x84}
        )
        self.assertEqual(base, 0x20)
        self.assertEqual(records, [(0x80, 8, (4,))])

    def test_slot15_unknown_relocation_is_rejected(self) -> None:
        data = bytearray(512)
        header = [0] * bpc_inspect.HEADER_SLOTS
        header[15] = 0x20
        struct.pack_into("<2I", data, 0x20, 0x80, 0)
        with self.assertRaisesRegex(ValueError, "unknown relocation"):
            bpc_inspect.slot15_effect_records(
                bytes(data), header, {0x20, 0x80 + 0x1C}
            )

    def test_slot15_display_state_is_validated(self) -> None:
        data = bytearray(512)
        header = [0] * bpc_inspect.HEADER_SLOTS
        header[15] = 0x20
        struct.pack_into("<2I", data, 0x20, 0x80, 0)
        struct.pack_into("<I", data, 0x90, 0x100)
        struct.pack_into("<I", data, 0x116, 0x180)
        _base, records = bpc_inspect.slot15_effect_records(
            bytes(data), header, {0x20, 0x90, 0x116}
        )
        self.assertEqual(records[0][2], (0x10,))

        with self.assertRaisesRegex(ValueError, "display-state.*data pointer"):
            bpc_inspect.slot15_effect_records(
                bytes(data), header, {0x20, 0x90}
            )

    def test_slot16_progression_threshold_actions(self) -> None:
        data = bytearray(512)
        header = [0] * bpc_inspect.HEADER_SLOTS
        header[16] = 0x20
        struct.pack_into("<2I", data, 0x20, 0x80, 0)
        action = 0x80 + bpc_inspect.SLOT16_FIXED_SIZE
        struct.pack_into("<hBBII", data, action, 3, 0, 0, 0x180, 0x100)
        struct.pack_into("<h", data, action + bpc_inspect.SLOT16_ACTION_SIZE, -1)
        base, records = bpc_inspect.slot16_progress_records(
            bytes(data), header, {0x20, action + 4, action + 8}
        )
        self.assertEqual(base, 0x20)
        self.assertEqual(records, [(0x80, [(3, 0x180, 0x100)], ())])

    def test_slot16_event_sequence_pointer(self) -> None:
        data = bytearray(512)
        header = [0] * bpc_inspect.HEADER_SLOTS
        header[16] = 0x20
        struct.pack_into("<2I", data, 0x20, 0x80, 0)
        struct.pack_into("<I", data, 0x80 + 0x48, 0x140)
        struct.pack_into("<h", data, 0x80 + bpc_inspect.SLOT16_FIXED_SIZE, -1)
        struct.pack_into("<HIH", data, 0x144, 5, 0x180, 0)
        _base, records = bpc_inspect.slot16_progress_records(
            bytes(data), header, {0x20, 0x80 + 0x48}
        )
        self.assertEqual(records[0][2], (0x140,))

    def test_slot16_presentation_relocation_is_required(self) -> None:
        data = bytearray(512)
        header = [0] * bpc_inspect.HEADER_SLOTS
        header[16] = 0x20
        struct.pack_into("<2I", data, 0x20, 0x80, 0)
        action = 0x80 + bpc_inspect.SLOT16_FIXED_SIZE
        struct.pack_into("<hBBIIh", data, action, 1, 0, 0, 0x180, 0, -1)
        with self.assertRaisesRegex(ValueError, "invalid presentation pointer"):
            bpc_inspect.slot16_progress_records(bytes(data), header, {0x20})

    def test_slot17_weighted_choices(self) -> None:
        data = bytearray(512)
        header = [0] * bpc_inspect.HEADER_SLOTS
        header[17] = 0x20
        struct.pack_into("<2I", data, 0x20, 0x80, 0)
        struct.pack_into("<HHI", data, 0x80, 1, 64, 0x180)
        struct.pack_into("<HHI", data, 0x88, 0, 256, 0x190)
        base, arrays = bpc_inspect.slot17_weighted_choices(
            bytes(data), header, {0x20, 0x84, 0x8C}
        )
        self.assertEqual(base, 0x20)
        self.assertEqual(
            arrays, [(0x80, [(1, 64, 0x180), (0, 256, 0x190)])]
        )

    def test_slot17_nonincreasing_limit_is_rejected(self) -> None:
        data = bytearray(512)
        header = [0] * bpc_inspect.HEADER_SLOTS
        header[17] = 0x20
        struct.pack_into("<2I", data, 0x20, 0x80, 0)
        struct.pack_into("<HHI", data, 0x80, 0, 64, 0x180)
        struct.pack_into("<HHI", data, 0x88, 0, 64, 0x190)
        with self.assertRaisesRegex(ValueError, "not increasing"):
            bpc_inspect.slot17_weighted_choices(
                bytes(data), header, {0x20, 0x84, 0x8C}
            )

    def test_slot26_bounded_accumulator(self) -> None:
        data = bytearray(512)
        header = [0] * bpc_inspect.HEADER_SLOTS
        header[26] = 0x20
        struct.pack_into("<2I", data, 0x20, 0x80, 0)
        struct.pack_into("<BB", data, 0x80, 1, 1)
        for offset, pair in zip(
            bpc_inspect.SLOT26_BCD_PAIR_OFFSETS,
            ((2, 3), (4, 5), (6, 7), (8, 9)),
        ):
            struct.pack_into("<HHI", data, 0x80 + offset, pair[0], 0, pair[1])
        data[0x80 + 0x22:0x80 + 0x2A] = bytes.fromhex(
            "01 02 03 04 05 06 07 08"
        )
        base, records = bpc_inspect.slot26_bounded_accumulators(
            bytes(data), header, {0x20}
        )
        self.assertEqual(base, 0x20)
        self.assertEqual(
            records[0][1:],
            (1, 1, (2, 3), (4, 5), (6, 7), (8, 9), bytes(range(1, 9))),
        )

    def test_slot26_internal_relocation_is_rejected(self) -> None:
        data = bytearray(512)
        header = [0] * bpc_inspect.HEADER_SLOTS
        header[26] = 0x20
        struct.pack_into("<2I", data, 0x20, 0x80, 0)
        with self.assertRaisesRegex(ValueError, "contains a relocation"):
            bpc_inspect.slot26_bounded_accumulators(
                bytes(data), header, {0x20, 0x84}
            )

    def test_collision_vectors_end_at_slot22(self) -> None:
        data = bytearray(128)
        header = [0] * bpc_inspect.HEADER_SLOTS
        header[18] = 0x20
        header[22] = 0x2C
        struct.pack_into("<6h", data, 0x20, 0, 0, -2, 4, 5, -7)
        base, vectors = bpc_inspect.collision_vectors(bytes(data), header)
        self.assertEqual(base, 0x20)
        self.assertEqual(vectors, [(0, 0), (-2, 4), (5, -7)])

    def test_slot23_flipper_zone(self) -> None:
        data = bytearray(0x900)
        header = [0] * bpc_inspect.HEADER_SLOTS
        header[22] = 0x100
        header[23] = 0x80
        target = 0x100 + 2 * bpc_inspect.SLOT22_RECORD_SIZE
        zone = (10, 20, 30, 40, 50, 60, 7, 0, target)
        struct.pack_into("<6HBBI", data, 0x80, *zone)
        struct.pack_into("<h", data, 0x80 + bpc_inspect.SLOT23_ZONE_SIZE, -1)
        base, zones = bpc_inspect.slot23_zone_run(
            bytes(data), header, {0x80 + 14}
        )
        self.assertEqual(base, 0x80)
        self.assertEqual(zones, [zone])

    def test_slot23_flipper_relocation_is_required(self) -> None:
        data = bytearray(0x900)
        header = [0] * bpc_inspect.HEADER_SLOTS
        header[22] = 0x100
        header[23] = 0x80
        struct.pack_into("<6HBBIh", data, 0x80, 1, 2, 3, 4, 5, 6, 0, 0,
                         0x100, -1)
        with self.assertRaisesRegex(ValueError, "flipper-record relocation"):
            bpc_inspect.slot23_zone_run(bytes(data), header, set())

    def test_slot25_pair_follows_slot14_list(self) -> None:
        data = bytearray(128)
        header = [0] * bpc_inspect.HEADER_SLOTS
        header[14] = 0x20
        header[25] = 0x2C
        struct.pack_into("<3I", data, 0x20, 0x60, 0x64, 0)
        struct.pack_into("<2I", data, 0x2C, 0x68, 0)
        base, pointers = bpc_inspect.slot25_object_pair(
            bytes(data), header, {0x2C}, 0x20, 2
        )
        self.assertEqual(base, 0x2C)
        self.assertEqual(pointers, (0x68, 0))

    def test_slot31_data_relocation_is_required(self) -> None:
        data = bytearray(256)
        header = [0] * bpc_inspect.HEADER_SLOTS
        header[31] = 0x40
        struct.pack_into(
            "<11HI", data, 0x40,
            2, 80, 64, 17, 1234, 567, 1, 0, 0, 0, 0, 0xC0,
        )
        with self.assertRaisesRegex(ValueError, "lacks its data relocation"):
            bpc_inspect.slot31_record_run(bytes(data), header, set())

    def test_slot33_record_bank(self) -> None:
        data = bytearray(512)
        header = [0] * bpc_inspect.HEADER_SLOTS
        header[33] = 0x40
        relocations = set()
        for index, leading in enumerate(((0, 128, 0, 1), (0, 7, 1, 1),
                                         (0, 7, 1, 1), (0, 7, 0, 1))):
            record_offset = 0x40 + index * 20
            state_offset = 0x100 + index * 40
            struct.pack_into("<4HI4H", data, record_offset, *leading, state_offset,
                             0, 0, 0, 0)
            struct.pack_into("<H", data, state_offset + 16, (index + 1) * 0x100)
            struct.pack_into("<H", data, state_offset + 36, 100 + index)
            relocations.add(record_offset + 8)
        base, records = bpc_inspect.slot33_record_bank(
            bytes(data), header, relocations
        )
        self.assertEqual(base, 0x40)
        self.assertEqual(len(records), 4)
        self.assertEqual(records[3][4], 0x178)


if __name__ == "__main__":
    unittest.main()
