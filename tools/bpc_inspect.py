#!/usr/bin/env python3
"""Validate and describe a Pinball Illusions BPC module and REL table."""

from __future__ import annotations

import argparse
import hashlib
import struct
import sys
from pathlib import Path


HEADER_SLOTS = 45
LOAD_ONE_FLAG = 0x80000000
LOAD_PAIR_FLAG = 0xC0000000
POINTER_MASK = 0x3FFFFFFF
AUDIO_RECORD_SIZE = 12
AUDIO_RECORD_TAG = 4
SLOT31_RECORD_SIZE = 26
SLOT31_RECORD_TAG = 2
SLOT33_RECORD_SIZE = 20
SLOT33_RECORD_COUNT = 4
SLOT33_STATE_SIZE = 40
SLOT22_RECORD_SIZE = 0x1F7
SLOT22_RECORD_COUNT = 4
SLOT22_ACTIVE_TYPE = 1
SLOT22_PLACEHOLDER_TYPE = 3
SLOT22_MASK_POINTER_OFFSET = 0x1C
OFFSET_DIRECTORY_SLOTS = (12, 13)
OBJECT_POINTER_LIST_SLOTS = (14, 15, 16, 17, 26)
SLOT23_ZONE_SIZE = 18
SLOT14_GROUP_SIZE = 10
LIGHT_STATE_SIZE = 30
LIGHT_NEXT_OFFSET = 0x10
LIGHT_COORDINATE_ALIAS_OFFSET = 0x0C
LIGHT_ID_OFFSET = 0x1C
SLOT15_EFFECT_SIZE = 0x34
SLOT15_POINTER_OFFSETS = (4, 8, 12, 16, 20, 24)
SLOT15_DISPLAY_STATE_OFFSET = 0x10
SLOT16_FIXED_SIZE = 0x50
SLOT16_ACTION_SIZE = 12
SLOT16_EVENT_OFFSETS = (0x48, 0x4C)
SLOT17_CHOICE_SIZE = 8
SLOT17_FINAL_LIMIT = 0x100
SLOT26_ACCUMULATOR_SIZE = 0x2A
SLOT26_BCD_PAIR_OFFSETS = (0x02, 0x0A, 0x12, 0x1A)
SLOT26_TRAILER_OFFSET = 0x22
EVENT_SEQUENCE_COMMAND_OFFSET = 4
EVENT_COMMAND_SIZES = (
    4, 6, 8, 6, 8, 6, 6, 14,
    6, 6, 4, 4, 6, 6, 6, 6,
    6, 6, 14, 6, 6, 8, 6, 8,
    10, 2, 6, 4, 10, 4, 4, 4,
)


def read_c_string(data: bytes, offset: int) -> tuple[str, int]:
    if offset >= len(data):
        raise ValueError(f"string offset 0x{offset:x} lies outside the module")
    end = data.find(b"\0", offset)
    if end < 0:
        raise ValueError(f"string at 0x{offset:x} is not terminated")
    try:
        text = data[offset:end].decode("ascii")
    except UnicodeDecodeError as error:
        raise ValueError(f"string at 0x{offset:x} is not ASCII") from error
    return text, end + 1


def possible_module_string(data: bytes, offset: int) -> str | None:
    """Return a conservative printable path-like string, if present."""
    end = data.find(b"\0", offset, min(offset + 128, len(data)))
    if end < 0 or end - offset < 4:
        return None
    value = data[offset:end]
    if any(byte < 0x20 or byte > 0x7E for byte in value):
        return None
    if b"\\" not in value and b"/" not in value:
        return None
    return value.decode("ascii")


def classify_header_value(data: bytes, value: int) -> tuple[str, str]:
    flags = value & LOAD_PAIR_FLAG
    offset = value & POINTER_MASK
    if value == 0:
        return "null", "-"
    if flags == LOAD_ONE_FLAG:
        name, _next = read_c_string(data, offset)
        return "resource", f"`{name}`"
    if flags == LOAD_PAIR_FLAG:
        first, next_offset = read_c_string(data, offset)
        second, _next = read_c_string(data, next_offset)
        return "joined resources", f"`{first}` + `{second}`"
    if flags != 0:
        raise ValueError(f"unsupported pointer flags in 0x{value:08x}")
    if offset >= len(data):
        raise ValueError(f"header target 0x{offset:x} lies outside the module")
    module_string = possible_module_string(data, offset)
    if module_string is not None:
        return "module string", f"`{module_string}`"
    return "module pointer", f"`0x{offset:08X}`"


def audio_record_run(
    data: bytes, header_values: list[int]
) -> tuple[int, list[tuple[int, ...]]]:
    """Return the slot-34 audio-record base and its contiguous tagged run."""
    base = header_values[34]
    if base & LOAD_PAIR_FLAG or base + AUDIO_RECORD_SIZE > len(data):
        raise ValueError("slot 34 does not point to an in-module audio-record table")

    records = []
    offset = base
    while offset + AUDIO_RECORD_SIZE <= len(data):
        record = struct.unpack_from("<6h", data, offset)
        if record[0] != AUDIO_RECORD_TAG:
            break
        records.append(record)
        offset += AUDIO_RECORD_SIZE
    if not records:
        raise ValueError("slot 34 does not begin with an audio record tagged 4")

    end = base + len(records) * AUDIO_RECORD_SIZE
    for slot in range(35, 39):
        target = header_values[slot]
        if target < base or target >= end or (target - base) % AUDIO_RECORD_SIZE:
            raise ValueError(
                f"slot {slot} does not select a record in the slot-34 audio table"
            )
    return base, records


def slot31_record_run(
    data: bytes, header_values: list[int], relocations: set[int]
) -> tuple[int, list[tuple[int, ...]]]:
    """Return the slot-31 run of tagged records with relocated data pointers."""
    base = header_values[31]
    if base == 0:
        return 0, []
    if base & LOAD_PAIR_FLAG or base + SLOT31_RECORD_SIZE > len(data):
        raise ValueError("slot 31 does not point to an in-module record table")

    records = []
    offset = base
    while offset + SLOT31_RECORD_SIZE <= len(data):
        record = struct.unpack_from("<11HI", data, offset)
        if record[0] != SLOT31_RECORD_TAG:
            break
        pointer_offset = offset + 22
        if pointer_offset not in relocations:
            raise ValueError(
                f"slot-31 record at 0x{offset:X} lacks its data relocation"
            )
        if record[-1] >= len(data):
            raise ValueError(
                f"slot-31 record at 0x{offset:X} points outside the BPC"
            )
        records.append(record)
        offset += SLOT31_RECORD_SIZE
    if not records:
        raise ValueError("slot 31 does not begin with a record tagged 2")
    return base, records


def slot22_record_run(
    data: bytes, header_values: list[int], relocations: set[int]
) -> tuple[int, list[bytes]]:
    """Return and validate the slot-22 fixed-stride flipper records."""
    base = header_values[22]
    if base == 0:
        return 0, []
    if base & LOAD_PAIR_FLAG or base >= len(data):
        raise ValueError("slot 22 does not point inside the BPC")

    records = []
    offset = base
    while offset < len(data) and data[offset] != 0:
        end = offset + SLOT22_RECORD_SIZE
        if end > len(data):
            raise ValueError("slot-22 record extends outside the BPC")
        records.append(data[offset:end])
        offset = end
    if offset >= len(data):
        raise ValueError("slot-22 record bank has no zero sentinel")
    if not records:
        raise ValueError("slot 22 points directly at a zero sentinel")

    if len(records) != SLOT22_RECORD_COUNT:
        raise ValueError(
            f"slot-22 bank has {len(records)} records, expected "
            f"{SLOT22_RECORD_COUNT}"
        )
    mask_targets = {
        header_values[slot] & POINTER_MASK: slot
        for slot in (1, 7)
        if (header_values[slot] & LOAD_PAIR_FLAG) == LOAD_ONE_FLAG
    }
    active_count = 0
    placeholder_count = 0
    for index, record in enumerate(records):
        record_offset = base + index * SLOT22_RECORD_SIZE
        internal_relocations = sorted(
            relocation - record_offset
            for relocation in relocations
            if record_offset <= relocation < record_offset + SLOT22_RECORD_SIZE
        )
        if record[0] == SLOT22_PLACEHOLDER_TYPE:
            placeholder_count += 1
            if any(record[1:]) or internal_relocations:
                raise ValueError(
                    f"slot-22 placeholder record {index} is not empty"
                )
            continue
        if record[0] != SLOT22_ACTIVE_TYPE:
            raise ValueError(
                f"slot-22 record {index} has unknown type {record[0]}"
            )

        active_count += 1
        if internal_relocations != [SLOT22_MASK_POINTER_OFFSET]:
            raise ValueError(
                f"slot-22 record {index} does not have exactly its mask "
                "pointer relocated"
            )
        mask_pointer = struct.unpack_from(
            "<I", record, SLOT22_MASK_POINTER_OFFSET
        )[0]
        if mask_pointer not in mask_targets:
            raise ValueError(
                f"slot-22 record {index} does not select MASK1 or MASK2"
            )
        start_angle, end_angle, direction = struct.unpack_from("<3H", record, 6)
        if start_angle >= 120 or end_angle >= 120 or direction not in {0, 1}:
            raise ValueError(
                f"slot-22 record {index} has invalid angular fields"
            )
        if record[-1] != 0xFF:
            raise ValueError(
                f"slot-22 record {index} lacks its trailing active marker"
            )
    if active_count != 3 or placeholder_count != 1:
        raise ValueError(
            "slot-22 bank is not three active flippers plus one placeholder"
        )
    return base, records


def offset_directory(
    data: bytes, header_values: list[int], slot: int
) -> tuple[int, list[int]]:
    """Return a self-sized directory of 16-bit offsets relative to its base."""
    base = header_values[slot]
    if base == 0:
        return 0, []
    if base & LOAD_PAIR_FLAG or base + 4 > len(data):
        raise ValueError(f"slot {slot} does not point to an offset directory")
    first_offset = struct.unpack_from("<H", data, base)[0]
    if first_offset < 4 or first_offset % 2:
        raise ValueError(f"slot-{slot} offset directory has an invalid size")
    record_count = first_offset // 2 - 1
    directory_size = (record_count + 1) * 2
    if base + directory_size > len(data):
        raise ValueError(f"slot-{slot} offset directory extends outside the BPC")
    values = list(struct.unpack_from(f"<{record_count + 1}H", data, base))
    offsets, terminator = values[:-1], values[-1]
    if terminator != 0:
        raise ValueError(f"slot-{slot} offset directory lacks its zero terminator")
    if offsets != sorted(set(offsets)) or offsets[0] != directory_size:
        raise ValueError(f"slot-{slot} offsets are not strictly increasing")
    if any(base + offset >= len(data) for offset in offsets):
        raise ValueError(f"slot-{slot} record offset lies outside the BPC")
    return base, offsets


def object_pointer_list(
    data: bytes, header_values: list[int], relocations: set[int], slot: int
) -> tuple[int, list[int]]:
    """Return a zero-terminated list of relocated in-module pointers."""
    base = header_values[slot]
    if base == 0:
        return 0, []
    if base & LOAD_PAIR_FLAG or base + 4 > len(data):
        raise ValueError(f"slot {slot} does not point to an object-pointer list")
    pointers = []
    offset = base
    while offset + 4 <= len(data):
        target = struct.unpack_from("<I", data, offset)[0]
        if target == 0:
            return base, pointers
        if offset not in relocations:
            raise ValueError(
                f"slot-{slot} object pointer at 0x{offset:X} is not relocated"
            )
        if target >= len(data):
            raise ValueError(
                f"slot-{slot} object pointer at 0x{offset:X} leaves the BPC"
            )
        pointers.append(target)
        offset += 4
    raise ValueError(f"slot-{slot} object-pointer list has no zero terminator")


def event_sequence(data: bytes, offset: int) -> list[tuple[int, int]]:
    """Return the opcode and location of each command in a queued event."""
    if offset + EVENT_SEQUENCE_COMMAND_OFFSET + 2 > len(data):
        raise ValueError("event sequence header extends outside the BPC")
    commands = []
    command_offset = offset + EVENT_SEQUENCE_COMMAND_OFFSET
    while command_offset + 2 <= len(data):
        opcode = struct.unpack_from("<H", data, command_offset)[0]
        if opcode == 0:
            return commands
        if opcode >= len(EVENT_COMMAND_SIZES):
            raise ValueError(
                f"event sequence has invalid opcode 0x{opcode:X} "
                f"at 0x{command_offset:X}"
            )
        size = EVENT_COMMAND_SIZES[opcode]
        if command_offset + size > len(data):
            raise ValueError("event sequence command extends outside the BPC")
        commands.append((opcode, command_offset))
        command_offset += size
    raise ValueError("event sequence has no zero terminator")


def display_state_record(
    data: bytes, relocations: set[int], offset: int
) -> tuple[int, int]:
    """Validate a 26-byte slot-31-style mutable display state."""
    if offset + SLOT31_RECORD_SIZE > len(data):
        raise ValueError("display-state record extends outside the BPC")
    data_location = offset + 0x16
    data_pointer = struct.unpack_from("<I", data, data_location)[0]
    if data_pointer:
        if data_location not in relocations or data_pointer >= len(data):
            raise ValueError("display-state record has an invalid data pointer")
    elif data_location in relocations:
        raise ValueError("display-state record has a relocated null data pointer")
    return data[offset], data_pointer


def slot14_light_groups(
    data: bytes, header_values: list[int], relocations: set[int]
) -> tuple[int, list[tuple[int, int, int, int, tuple[int, ...]]], list[int]]:
    """Return slot-14 light groups and their linked 30-byte light states."""
    base, pointers = object_pointer_list(data, header_values, relocations, 14)
    groups = []
    all_nodes: list[int] = []
    globally_seen = set()
    for group in pointers:
        if group + SLOT14_GROUP_SIZE > len(data):
            raise ValueError("slot-14 light-group descriptor extends outside the BPC")
        first, flags, reserved, auxiliary = struct.unpack_from("<IBBI", data, group)
        if group not in relocations or first == 0 or first >= len(data):
            raise ValueError("slot-14 light group lacks its relocated first-state pointer")
        if reserved != 0:
            raise ValueError("slot-14 light group has a nonzero reserved byte")
        auxiliary_location = group + 6
        if auxiliary:
            if auxiliary_location not in relocations or auxiliary >= len(data):
                raise ValueError("slot-14 light group has an invalid auxiliary pointer")
            event_sequence(data, auxiliary)
        elif auxiliary_location in relocations:
            raise ValueError("slot-14 null auxiliary pointer is relocated")

        chain = []
        node = first
        locally_seen = set()
        while node:
            if node in locally_seen:
                raise ValueError("slot-14 light-state chain contains a cycle")
            if node in globally_seen:
                raise ValueError("slot-14 light-state chains share a node")
            if node + LIGHT_STATE_SIZE > len(data):
                raise ValueError("slot-14 light state extends outside the BPC")
            locally_seen.add(node)
            globally_seen.add(node)
            chain.append(node)
            all_nodes.append(node)

            coordinate_alias = struct.unpack_from(
                "<I", data, node + LIGHT_COORDINATE_ALIAS_OFFSET
            )[0]
            coordinate_location = node + LIGHT_COORDINATE_ALIAS_OFFSET
            if coordinate_alias:
                if (
                    coordinate_location not in relocations
                    or coordinate_alias != node + 0x14
                ):
                    raise ValueError(
                        "slot-14 light state has an invalid coordinate alias"
                    )
            elif coordinate_location in relocations:
                raise ValueError("slot-14 null coordinate alias is relocated")

            next_location = node + LIGHT_NEXT_OFFSET
            next_node = struct.unpack_from("<I", data, next_location)[0]
            if next_node:
                if next_location not in relocations or next_node >= len(data):
                    raise ValueError("slot-14 light state has an invalid next pointer")
            elif next_location in relocations:
                raise ValueError("slot-14 null next pointer is relocated")
            node = next_node
        groups.append((group, first, flags, auxiliary, tuple(chain)))
    return base, groups, all_nodes


def slot15_effect_records(
    data: bytes, header_values: list[int], relocations: set[int]
) -> tuple[int, list[tuple[int, int, tuple[int, ...]]]]:
    """Return the fixed host-visible part of slot-15 timed-effect records."""
    base, pointers = object_pointer_list(data, header_values, relocations, 15)
    records = []
    for pointer in pointers:
        if pointer + SLOT15_EFFECT_SIZE > len(data):
            raise ValueError("slot-15 timed-effect record extends outside the BPC")
        internal = tuple(
            offset - pointer
            for offset in sorted(relocations)
            if pointer <= offset < pointer + SLOT15_EFFECT_SIZE
        )
        if any(offset not in SLOT15_POINTER_OFFSETS for offset in internal):
            raise ValueError("slot-15 timed-effect record has an unknown relocation")
        for offset in SLOT15_POINTER_OFFSETS:
            target = struct.unpack_from("<I", data, pointer + offset)[0]
            location = pointer + offset
            if target:
                if location not in relocations or target >= len(data):
                    raise ValueError("slot-15 timed-effect pointer is invalid")
            elif location in relocations:
                raise ValueError("slot-15 null pointer is relocated")
        display_state = struct.unpack_from(
            "<I", data, pointer + SLOT15_DISPLAY_STATE_OFFSET
        )[0]
        if display_state:
            display_state_record(data, relocations, display_state)
        flags = struct.unpack_from("<I", data, pointer)[0]
        records.append((pointer, flags, internal))
    return base, records


def slot16_progress_records(
    data: bytes, header_values: list[int], relocations: set[int]
) -> tuple[
    int,
    list[tuple[int, list[tuple[int, int, int]], tuple[int, ...]]],
]:
    """Return slot-16 per-player counters and their threshold actions."""
    base, pointers = object_pointer_list(data, header_values, relocations, 16)
    records = []
    for pointer in pointers:
        if pointer + SLOT16_FIXED_SIZE + 2 > len(data):
            raise ValueError("slot-16 progression record extends outside the BPC")
        event_targets = []
        for event_offset in SLOT16_EVENT_OFFSETS:
            location = pointer + event_offset
            target = struct.unpack_from("<I", data, location)[0]
            if target:
                if location not in relocations or target >= len(data):
                    raise ValueError("slot-16 record has an invalid event pointer")
                event_sequence(data, target)
                event_targets.append(target)
            elif location in relocations:
                raise ValueError("slot-16 record has a relocated null event pointer")
        actions = []
        action_offset = pointer + SLOT16_FIXED_SIZE
        previous_threshold = -1
        while action_offset + 2 <= len(data):
            threshold = struct.unpack_from("<h", data, action_offset)[0]
            if threshold < 0:
                break
            if action_offset + SLOT16_ACTION_SIZE > len(data):
                raise ValueError("slot-16 threshold action extends outside the BPC")
            if threshold <= previous_threshold:
                raise ValueError("slot-16 action thresholds are not increasing")
            presentation = struct.unpack_from("<I", data, action_offset + 4)[0]
            light_state = struct.unpack_from("<I", data, action_offset + 8)[0]
            if (
                presentation == 0
                or action_offset + 4 not in relocations
                or presentation >= len(data)
            ):
                raise ValueError("slot-16 action has an invalid presentation pointer")
            if light_state:
                if action_offset + 8 not in relocations or light_state >= len(data):
                    raise ValueError("slot-16 action has an invalid light-state pointer")
            elif action_offset + 8 in relocations:
                raise ValueError("slot-16 action has a relocated null light-state pointer")
            actions.append((threshold, presentation, light_state))
            previous_threshold = threshold
            action_offset += SLOT16_ACTION_SIZE
        else:
            raise ValueError("slot-16 threshold action list has no terminator")
        records.append((pointer, actions, tuple(event_targets)))
    return base, records


def slot17_weighted_choices(
    data: bytes, header_values: list[int], relocations: set[int]
) -> tuple[int, list[tuple[int, list[tuple[int, int, int]]]]]:
    """Return slot-17 cumulative-limit presentation-choice arrays."""
    base, pointers = object_pointer_list(data, header_values, relocations, 17)
    arrays = []
    for pointer in pointers:
        choices = []
        offset = pointer
        previous_limit = 0
        while offset + SLOT17_CHOICE_SIZE <= len(data):
            flags, limit, presentation = struct.unpack_from("<HHI", data, offset)
            if limit <= previous_limit:
                raise ValueError("slot-17 choice limits are not increasing")
            if (
                presentation == 0
                or offset + 4 not in relocations
                or presentation >= len(data)
            ):
                raise ValueError("slot-17 choice has an invalid presentation pointer")
            choices.append((flags, limit, presentation))
            if limit >= SLOT17_FINAL_LIMIT:
                break
            previous_limit = limit
            offset += SLOT17_CHOICE_SIZE
        else:
            raise ValueError("slot-17 choice array has no final limit")
        if choices[-1][1] != SLOT17_FINAL_LIMIT:
            raise ValueError("slot-17 choice array does not end at limit 0x100")
        arrays.append((pointer, choices))
    return base, arrays


def slot26_bounded_accumulators(
    data: bytes, header_values: list[int], relocations: set[int]
) -> tuple[
    int,
    list[
        tuple[
            int,
            int,
            int,
            tuple[int, int],
            tuple[int, int],
            tuple[int, int],
            tuple[int, int],
            bytes,
        ]
    ],
]:
    """Return slot-26 bounded packed-BCD accumulator states."""
    base, pointers = object_pointer_list(data, header_values, relocations, 26)
    records = []
    for pointer in pointers:
        end = pointer + SLOT26_ACCUMULATOR_SIZE
        if end > len(data):
            raise ValueError("slot-26 bounded accumulator extends outside the BPC")
        if any(pointer <= relocation < end for relocation in relocations):
            raise ValueError("slot-26 bounded accumulator contains a relocation")
        active, flags = struct.unpack_from("<BB", data, pointer)
        pairs = []
        for pair_offset in SLOT26_BCD_PAIR_OFFSETS:
            high, padding, low = struct.unpack_from("<HHI", data, pointer + pair_offset)
            if padding != 0:
                raise ValueError("slot-26 BCD pair has nonzero alignment padding")
            pairs.append((high, low))
        current, start, limit, step = pairs
        trailer = data[
            pointer + SLOT26_TRAILER_OFFSET:pointer + SLOT26_ACCUMULATOR_SIZE
        ]
        records.append(
            (pointer, active, flags, current, start, limit, step, trailer)
        )
    return base, records


def collision_vectors(
    data: bytes, header_values: list[int]
) -> tuple[int, list[tuple[int, int]]]:
    """Return the signed X/Y collision vectors between slots 18 and 22."""
    base = header_values[18]
    end = header_values[22]
    if base == 0:
        return 0, []
    if base & LOAD_PAIR_FLAG or end <= base or end > len(data) or (end - base) % 4:
        raise ValueError("slot 18 does not form a vector table ending at slot 22")
    vectors = [
        item for item in struct.iter_unpack("<2h", data[base:end])
    ]
    if not vectors:
        raise ValueError("slot 18 contains no collision vectors")
    return base, vectors


def slot23_zone_run(
    data: bytes, header_values: list[int], relocations: set[int]
) -> tuple[int, list[tuple[int, ...]]]:
    """Return flipper-interaction zones terminated by a negative first word."""
    base = header_values[23]
    if base == 0:
        return 0, []
    if base & LOAD_PAIR_FLAG or base + 2 > len(data):
        raise ValueError("slot 23 does not point to a flipper-zone table")
    flipper_base = header_values[22]
    flipper_end = flipper_base + SLOT22_RECORD_COUNT * SLOT22_RECORD_SIZE
    zones = []
    offset = base
    while offset + 2 <= len(data):
        first = struct.unpack_from("<h", data, offset)[0]
        if first < 0:
            if first != -1:
                raise ValueError("slot-23 zone table has an unknown terminator")
            return base, zones
        if offset + SLOT23_ZONE_SIZE > len(data):
            raise ValueError("slot-23 zone extends outside the BPC")
        zone = struct.unpack_from("<6HBBI", data, offset)
        pointer_offset = offset + 14
        if pointer_offset not in relocations:
            raise ValueError("slot-23 zone lacks its flipper-record relocation")
        target = zone[-1]
        if (
            target < flipper_base
            or target >= flipper_end
            or (target - flipper_base) % SLOT22_RECORD_SIZE
        ):
            raise ValueError("slot-23 zone does not select a slot-22 record")
        zones.append(zone)
        offset += SLOT23_ZONE_SIZE
    raise ValueError("slot-23 zone table has no negative terminator")


def slot25_object_pair(
    data: bytes,
    header_values: list[int],
    relocations: set[int],
    slot14_base: int,
    slot14_count: int,
) -> tuple[int, tuple[int, int] | None]:
    """Return the two special object pointers stored after the slot-14 list."""
    base = header_values[25]
    if base == 0:
        return 0, None
    if base & LOAD_PAIR_FLAG or base + 8 > len(data):
        raise ValueError("slot 25 does not point to an object-pointer pair")
    if slot14_base and base != slot14_base + (slot14_count + 1) * 4:
        raise ValueError("slot 25 does not immediately follow the slot-14 list")
    pointers = struct.unpack_from("<2I", data, base)
    for index, target in enumerate(pointers):
        pointer_offset = base + index * 4
        if target == 0:
            if pointer_offset in relocations:
                raise ValueError("slot-25 null object pointer is relocated")
            continue
        if pointer_offset not in relocations:
            raise ValueError("slot-25 object pointer is not relocated")
        if target >= len(data):
            raise ValueError("slot-25 object pointer leaves the BPC")
    return base, pointers


def slot33_record_bank(
    data: bytes, header_values: list[int], relocations: set[int]
) -> tuple[int, list[tuple[int, ...]]]:
    """Return the four slot-33 descriptors and validate their state blocks."""
    base = header_values[33]
    if base == 0:
        return 0, []
    end = base + SLOT33_RECORD_COUNT * SLOT33_RECORD_SIZE
    if base & LOAD_PAIR_FLAG or end > len(data):
        raise ValueError("slot 33 does not point to an in-module record bank")

    records = []
    state_targets = []
    for index in range(SLOT33_RECORD_COUNT):
        offset = base + index * SLOT33_RECORD_SIZE
        record = struct.unpack_from("<4HI4H", data, offset)
        pointer_offset = offset + 8
        if pointer_offset not in relocations:
            raise ValueError(
                f"slot-33 record {index} lacks its state-block relocation"
            )
        target = record[4]
        if target + SLOT33_STATE_SIZE > len(data):
            raise ValueError(f"slot-33 state block {index} lies outside the BPC")
        state_targets.append(target)
        records.append(record)

    expected_targets = [
        state_targets[0] + index * SLOT33_STATE_SIZE
        for index in range(SLOT33_RECORD_COUNT)
    ]
    if state_targets != expected_targets:
        raise ValueError("slot-33 state blocks are not contiguous")
    for index, target in enumerate(state_targets):
        words = struct.unpack_from("<20H", data, target)
        expected = [0] * 20
        expected[8] = (index + 1) * 0x100
        expected[18] = 100 + index
        if list(words) != expected:
            raise ValueError(f"slot-33 state block {index} has an unknown layout")
    return base, records


def inspect(bpc_path: Path, rel_path: Path) -> str:
    bpc = bpc_path.read_bytes()
    rel = rel_path.read_bytes()
    if len(bpc) < HEADER_SLOTS * 4:
        raise ValueError("BPC is shorter than its 45-slot header")
    if len(rel) % 4 != 0:
        raise ValueError("REL size is not a multiple of four")

    offsets = [item[0] for item in struct.iter_unpack("<I", rel)]
    if len(offsets) != len(set(offsets)):
        raise ValueError("REL contains duplicate relocation offsets")
    for offset in offsets:
        if offset + 4 > len(bpc):
            raise ValueError(f"relocation 0x{offset:x} lies outside the BPC")

    relocation_values = [struct.unpack_from("<I", bpc, offset)[0] for offset in offsets]
    relocated_value_counts: dict[int, int] = {}
    for value in relocation_values:
        relocated_value_counts[value] = relocated_value_counts.get(value, 0) + 1
    header_values = list(struct.unpack_from(f"<{HEADER_SLOTS}I", bpc))
    missing_header_relocations = [
        slot for slot, value in enumerate(header_values)
        if value != 0 and slot * 4 not in offsets
    ]
    if missing_header_relocations:
        slots = ", ".join(str(slot) for slot in missing_header_relocations)
        raise ValueError(f"non-null header slots lack relocations: {slots}")

    relocation_set = set(offsets)
    slot22_base, slot22_records = slot22_record_run(
        bpc, header_values, relocation_set
    )
    offset_directories = {
        slot: offset_directory(bpc, header_values, slot)
        for slot in OFFSET_DIRECTORY_SLOTS
    }
    object_pointer_lists = {
        slot: object_pointer_list(bpc, header_values, relocation_set, slot)
        for slot in OBJECT_POINTER_LIST_SLOTS
    }
    _slot14_base, light_groups, light_states = slot14_light_groups(
        bpc, header_values, relocation_set
    )
    _slot15_base, timed_effects = slot15_effect_records(
        bpc, header_values, relocation_set
    )
    _slot16_base, progress_records = slot16_progress_records(
        bpc, header_values, relocation_set
    )
    _slot17_base, weighted_choices = slot17_weighted_choices(
        bpc, header_values, relocation_set
    )
    _slot26_base, bounded_accumulators = slot26_bounded_accumulators(
        bpc, header_values, relocation_set
    )
    vector_base, vectors = collision_vectors(bpc, header_values)
    slot23_base, slot23_zones = slot23_zone_run(
        bpc, header_values, relocation_set
    )
    slot14_base, slot14_pointers = object_pointer_lists[14]
    slot25_base, slot25_pointers = slot25_object_pair(
        bpc,
        header_values,
        relocation_set,
        slot14_base,
        len(slot14_pointers),
    )
    slot31_base, slot31_records = slot31_record_run(
        bpc, header_values, relocation_set
    )
    slot33_base, slot33_records = slot33_record_bank(
        bpc, header_values, relocation_set
    )
    audio_base, audio_records = audio_record_run(bpc, header_values)

    flag_counts = {
        "plain": sum((value & LOAD_PAIR_FLAG) == 0 for value in relocation_values),
        "resource": sum((value & LOAD_PAIR_FLAG) == LOAD_ONE_FLAG for value in relocation_values),
        "joined": sum((value & LOAD_PAIR_FLAG) == LOAD_PAIR_FLAG for value in relocation_values),
    }
    lines = [
        f"# BPC module: `{bpc_path.name}`",
        "",
        f"- BPC size: {len(bpc)} bytes (`0x{len(bpc):X}`)",
        f"- BPC SHA-256: `{hashlib.sha256(bpc).hexdigest().upper()}`",
        f"- REL entries: {len(offsets)} ({len(rel)} bytes)",
        f"- REL SHA-256: `{hashlib.sha256(rel).hexdigest().upper()}`",
        f"- Unaligned relocation words: {sum(offset % 4 != 0 for offset in offsets)}",
        f"- Relocated values: {flag_counts['plain']} plain, "
        f"{flag_counts['resource']} resource, {flag_counts['joined']} joined",
        "",
        "## Header",
        "",
        "| Slot | File offset | Raw value | Kind | Target | Other relocated aliases |",
        "| ---: | ---: | ---: | --- | --- | ---: |",
    ]
    for slot, value in enumerate(header_values):
        kind, target = classify_header_value(bpc, value)
        flags = value & LOAD_PAIR_FLAG
        if value == 0:
            aliases = 0
        elif flags in {LOAD_ONE_FLAG, LOAD_PAIR_FLAG}:
            # The loader clears the header flag bits before walking the REL
            # locations, so it replaces ordinary pointers to the resource-name
            # string rather than other copies of the flagged header value.
            aliases = relocated_value_counts.get(value & POINTER_MASK, 0)
        else:
            aliases = relocated_value_counts[value] - 1
        lines.append(
            f"| {slot} | `0x{slot * 4:02X}` | `0x{value:08X}` | "
            f"{kind} | {target} | {aliases} |"
        )
    if any(offsets for _base, offsets in offset_directories.values()):
        lines.extend([
            "",
            "## Slot-12/13 offset directories",
            "",
            "Each directory starts with its first record offset and ends with "
            "a zero word.",
            "",
            "| Slot | File offset | Record count | Relative record offsets |",
            "| ---: | ---: | ---: | --- |",
        ])
        for slot, (base, offsets) in offset_directories.items():
            values = ", ".join(f"0x{offset:X}" for offset in offsets)
            lines.append(
                f"| {slot} | `0x{base:08X}` | {len(offsets)} | `{values}` |"
            )
    if any(pointers for _base, pointers in object_pointer_lists.values()):
        lines.extend([
            "",
            "## Slot-14 through slot-17 and slot-26 object lists",
            "",
            "Each entry is a relocated in-module pointer; a null dword "
            "terminates the list.",
            "",
            "| Slot | File offset | Object count |",
            "| ---: | ---: | ---: |",
        ])
        for slot, (base, pointers) in object_pointer_lists.items():
            lines.append(f"| {slot} | `0x{base:08X}` | {len(pointers)} |")
    if light_groups:
        chain_lengths = [len(group[-1]) for group in light_groups]
        multi_node_groups = sum(length > 1 for length in chain_lengths)
        dynamic_coordinates = sum(
            struct.unpack_from(
                "<I", bpc, state + LIGHT_COORDINATE_ALIAS_OFFSET
            )[0] != 0
            for state in light_states
        )
        light_ids = {
            struct.unpack_from("<H", bpc, state + LIGHT_ID_OFFSET)[0]
            for state in light_states
        }
        event_groups = sum(group[3] != 0 for group in light_groups)
        lines.extend([
            "",
            "## Slot-14 light groups",
            "",
            f"Slot 14 contains {len(light_groups)} ten-byte group descriptors "
            f"reaching {len(light_states)} linked {LIGHT_STATE_SIZE}-byte light "
            "states. The host renderer indexes its light-status buffers by the "
            "word at state offset `+0x1C`.",
            "",
            f"- Multi-state groups: {multi_node_groups}",
            f"- Distinct light IDs: {len(light_ids)}",
            f"- States whose `+0x0C` field aliases their own coordinates at "
            f"`+0x14`: {dynamic_coordinates}",
            f"- Groups with an event sequence at `+0x06`: {event_groups}",
        ])
    if timed_effects:
        relocation_counts = {
            offset: sum(offset in record[2] for record in timed_effects)
            for offset in SLOT15_POINTER_OFFSETS
        }
        relocation_text = ", ".join(
            f"+0x{offset:X}: {count}"
            for offset, count in relocation_counts.items()
        )
        display_states = sum(
            struct.unpack_from(
                "<I", bpc, record[0] + SLOT15_DISPLAY_STATE_OFFSET
            )[0] != 0
            for record in timed_effects
        )
        lines.extend([
            "",
            "## Slot-15 timed-effect states",
            "",
            f"Slot 15 contains {len(timed_effects)} records with a fixed "
            f"{SLOT15_EFFECT_SIZE}-byte host-visible state. The host uses "
            "`+0x04`/`+0x08` for light-state links, `+0x0C` for an optional "
            "audio-control record, `+0x14`/`+0x18` for presentation records, "
            "`+0x2E` as a signed countdown, and `+0x30` as the runtime next "
            "pointer. The module-only `+0x10` link selects an optional "
            "slot-31-style display state.",
            "",
            f"Relocated pointer-field occurrences: `{relocation_text}`.",
            f"Records with a display-state link at `+0x10`: {display_states}.",
        ])
    if progress_records:
        lines.extend([
            "",
            "## Slot-16 progression counters",
            "",
            f"Slot 16 contains {len(progress_records)} per-player progression "
            "records. Each begins with flags and an initial word, two arrays "
            "of eight player words at `+0x06` and `+0x16`, runtime/configured "
            "BCD values at `+0x38` and `+0x40`, and a terminated run of "
            f"{SLOT16_ACTION_SIZE}-byte threshold actions at `+0x50`. Optional "
            "pointers at `+0x48/+0x4C` select queued event sequences.",
            "",
            "| File offset | Flags | Initial word | Thresholds | Event sequences | BCD value (+0x40) |",
            "| ---: | ---: | ---: | --- | --- | --- |",
        ])
        for pointer, actions, event_targets in progress_records:
            flags, initial = struct.unpack_from("<HH", bpc, pointer)
            thresholds = ", ".join(str(item[0]) for item in actions) or "-"
            bcd_value = bpc[pointer + 0x40:pointer + 0x46].hex(" ").upper()
            events = ", ".join(
                f"`0x{target:08X}`" for target in event_targets
            ) or "-"
            lines.append(
                f"| `0x{pointer:08X}` | `0x{flags:04X}` | {initial} | "
                f"{thresholds} | {events} | `{bcd_value}` |"
            )
    if weighted_choices:
        lines.extend([
            "",
            "## Slot-17 weighted presentation choices",
            "",
            "Each array contains eight-byte `(flags, cumulative limit, "
            "presentation pointer)` choices. Limits increase strictly and "
            "the last choice ends the range at `0x100`.",
            "",
            "| File offset | Choices | Cumulative limits |",
            "| ---: | ---: | --- |",
        ])
        for pointer, choices in weighted_choices:
            limits = ", ".join(str(item[1]) for item in choices)
            lines.append(f"| `0x{pointer:08X}` | {len(choices)} | `{limits}` |")
    if bounded_accumulators:
        state_word = "state" if len(bounded_accumulators) == 1 else "states"
        lines.extend([
            "",
            "## Slot-26 bounded BCD accumulators",
            "",
            f"Slot 26 contains {len(bounded_accumulators)} "
            f"{SLOT26_ACCUMULATOR_SIZE}-byte mutable {state_word}. The update loop "
            "moves the packed-BCD pair at `+0x02/+0x06` by the step at "
            "`+0x1A/+0x1E` toward the limit at `+0x12/+0x16`; event opcode "
            "`0x0F` first copies the configured start pair at `+0x0A/+0x0E` "
            "into the current pair. Flag bit zero selects addition or "
            "subtraction, and the eight-byte trailer at `+0x22` remains "
            "module-private.",
            "",
            "| File offset | Active | Flags | Current | Start | Limit | Step |",
            "| ---: | ---: | ---: | --- | --- | --- | --- |",
        ])
        for (
            pointer,
            active,
            flags,
            current,
            start,
            limit,
            step,
            _trailer,
        ) in bounded_accumulators:
            def pair_text(pair: tuple[int, int]) -> str:
                return f"{pair[0]:04X}:{pair[1]:08X}"

            lines.append(
                f"| `0x{pointer:08X}` | {active} | `0x{flags:02X}` | "
                f"`{pair_text(current)}` | `{pair_text(start)}` | "
                f"`{pair_text(limit)}` | `{pair_text(step)}` |"
            )
    if vectors:
        vector_text = ", ".join(f"({x},{y})" for x, y in vectors)
        lines.extend([
            "",
            "## Slot-18 collision vectors",
            "",
            f"Slot 18 contains {len(vectors)} signed X/Y pairs from "
            f"`0x{vector_base:08X}` up to the slot-22 flipper bank.",
            "",
            f"`{vector_text}`",
        ])
    if slot23_base:
        record_word = "record" if len(slot23_zones) == 1 else "records"
        lines.extend([
            "",
            "## Slot-23 flipper-interaction zones",
            "",
            f"Slot 23 contains {len(slot23_zones)} "
            f"{SLOT23_ZONE_SIZE}-byte zone {record_word} followed by a `-1` word.",
            "",
            "| Index | Bounds (+0..+6) | Range (+8,+A) | Selector | Flipper index |",
            "| ---: | --- | --- | ---: | ---: |",
        ])
        for index, zone in enumerate(slot23_zones):
            flipper_index = (zone[-1] - slot22_base) // SLOT22_RECORD_SIZE
            bounds = ", ".join(str(value) for value in zone[:4])
            range_ = ", ".join(str(value) for value in zone[4:6])
            lines.append(
                f"| {index} | `{bounds}` | `{range_}` | {zone[6]} | "
                f"{flipper_index} |"
            )
    if slot25_pointers is not None:
        pointer_text = ", ".join(
            "null" if value == 0 else f"0x{value:08X}"
            for value in slot25_pointers
        )
        lines.extend([
            "",
            "## Slot-25 special object pair",
            "",
            f"Slot 25 at `0x{slot25_base:08X}` immediately follows the "
            f"slot-14 list and contains `{pointer_text}`.",
        ])
    if slot22_records:
        lines.extend([
            "",
            "## Slot-22 flipper records",
            "",
            f"Slot 22 begins {len(slot22_records)} fixed-stride "
            f"`0x{SLOT22_RECORD_SIZE:X}`-byte records followed by a zero "
            "type sentinel. Three records are active flippers and one is an "
            "empty type-3 placeholder.",
            "",
            "| Index | File offset | Type | Pivot (+2,+4) | Angles (+6,+8) | Direction | Mask |",
            "| ---: | ---: | ---: | --- | --- | ---: | --- |",
        ])
        for index, record in enumerate(slot22_records):
            offset = slot22_base + index * SLOT22_RECORD_SIZE
            if record[0] == SLOT22_PLACEHOLDER_TYPE:
                lines.append(
                    f"| {index} | `0x{offset:08X}` | placeholder | - | - | - | - |"
                )
                continue
            x, y = struct.unpack_from("<2H", record, 2)
            start_angle, end_angle, direction = struct.unpack_from("<3H", record, 6)
            mask_pointer = struct.unpack_from(
                "<I", record, SLOT22_MASK_POINTER_OFFSET
            )[0]
            mask_name = (
                "MASK1"
                if mask_pointer == (header_values[1] & POINTER_MASK)
                else "MASK2"
            )
            lines.append(
                f"| {index} | `0x{offset:08X}` | active | `{x}, {y}` | "
                f"`{start_angle}, {end_angle}` | {direction} | `{mask_name}` |"
            )
    if slot31_records:
        lines.extend([
            "",
            "## Slot-31 records",
            "",
            f"Slot 31 begins {len(slot31_records)} contiguous "
            f"{SLOT31_RECORD_SIZE}-byte records tagged {SLOT31_RECORD_TAG}.",
            "",
            "| Index | File offset | Words 1-5 | Data pointer |",
            "| ---: | ---: | --- | ---: |",
        ])
        for index, record in enumerate(slot31_records):
            offset = slot31_base + index * SLOT31_RECORD_SIZE
            words = ", ".join(str(value) for value in record[1:6])
            lines.append(
                f"| {index} | `0x{offset:08X}` | `{words}` | "
                f"`0x{record[-1]:08X}` |"
            )
    if slot33_records:
        lines.extend([
            "",
            "## Slot-33 records",
            "",
            f"Slot 33 contains {len(slot33_records)} "
            f"{SLOT33_RECORD_SIZE}-byte descriptors. Their relocated pointers "
            f"select contiguous {SLOT33_STATE_SIZE}-byte state blocks.",
            "",
            "| Index | File offset | Four leading words | State block |",
            "| ---: | ---: | --- | ---: |",
        ])
        for index, record in enumerate(slot33_records):
            offset = slot33_base + index * SLOT33_RECORD_SIZE
            words = ", ".join(str(value) for value in record[:4])
            lines.append(
                f"| {index} | `0x{offset:08X}` | `{words}` | "
                f"`0x{record[4]:08X}` |"
            )
    lines.extend([
        "",
        "## Audio records",
        "",
        f"Slot 34 begins {len(audio_records)} contiguous "
        f"{AUDIO_RECORD_SIZE}-byte records tagged {AUDIO_RECORD_TAG}.",
        "",
        "| Header slot | Record index | File offset | Six signed words |",
        "| ---: | ---: | ---: | --- |",
    ])
    for slot in range(34, 39):
        target = header_values[slot]
        index = (target - audio_base) // AUDIO_RECORD_SIZE
        words = ", ".join(str(value) for value in audio_records[index])
        lines.append(f"| {slot} | {index} | `0x{target:08X}` | `{words}` |")
    lines.append("")
    return "\n".join(lines)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("bpc", type=Path)
    parser.add_argument("rel", type=Path)
    arguments = parser.parse_args()
    try:
        print(inspect(arguments.bpc, arguments.rel))
    except (OSError, ValueError) as error:
        print(f"error: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
