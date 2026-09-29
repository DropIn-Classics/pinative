#!/usr/bin/env python3
"""List the event and display streams of a Pinball Illusions table module.

    event_streams.py T00n.BPC T00n.REL [--check]

The main program runs two stream languages (ILLUSION.386, see
src/ILLUSION.hints):

  * event streams, run by CODE:2D080 (the queue at EVENT_QUEUE) and by the
    mode stream runner at CODE:2CD3C: a word, the position word at +2,
    commands of 16-bit opcodes from +4, the sizes and handlers in the table
    at CODE:2D18F; opcode 0 ends the stream;
  * display streams, run by CODE:2F35C (queued through CODE:2FEDB): a flags
    word (bit 0: the background stream), a priority byte and a second byte
    at +2 and +3, the position word at +4, commands from +6, the table at
    CODE:2F621; opcode 0 ends it.

A jump operand is a position: a byte offset from the first command.
Operand kinds: w a word, d a dword, j a position, r a pointer to a record
(not followed), E an event stream, D a display stream (both followed).

Streams are found from the roots the main program is seen to use (each
with where it is read in ILLUSION.386) and from the stream operands of the
commands found.  Every pointer operand that is not zero must be a
relocated module offset (in the REL file); a stream that does not end,
leaves the module, or has an unknown opcode is an error.  The printed
listing gives each stream with the roots it hangs from."""
from __future__ import annotations

import argparse
import struct
import sys
from pathlib import Path

HEADER_SLOTS = 45
POINTER_MASK = 0x3FFFFFFF
LOAD_FLAGS = 0xC0000000

# name, operand kinds; the size is 2 plus the operands' (CODE:2D18F)
EVENT_OPS = {
    0x01: ('light', 'r'),            # CODE:2D36E
    0x02: ('light_for', 'rw'),       # CODE:2D491
    0x03: ('block', 'r'),            # CODE:2D67E
    0x04: ('object_state', 'rw'),    # CODE:2D23E
    0x05: ('take', 'r'),             # CODE:2D907
    0x06: ('ctr_value_reset', 'r'),  # CODE:2D827
    0x07: ('ctr_value', 'rdd'),      # CODE:2D8A6
    0x08: ('eject', 'r'),            # CODE:2D785
    0x09: ('mode', 'E'),             # CODE:2DA7B
    0x0A: ('goto', 'j'),             # CODE:2D5F4
    0x0B: ('ball_save', 'w'),        # CODE:2D210
    0x0C: ('unblock', 'r'),          # CODE:2DB8E
    0x0D: ('ctr_reset', 'r'),        # CODE:2D274
    0x0E: ('unlight', 'r'),          # CODE:2D326
    0x0F: ('bcd_start', 'r'),        # CODE:2DB5C
    0x10: ('bcd_stop', 'r'),         # CODE:2DB7B
    0x11: ('display', 'D'),          # CODE:2D6F9
    0x12: ('ctr_acc', 'rdd'),        # CODE:2D843
    0x13: ('music', 'r'),            # CODE:2D5E0
    0x14: ('module_call', 'r'),      # CODE:2DBAD
    0x15: ('ctr_set', 'rw'),         # CODE:2D85F
    0x16: ('ctr_restart', 'r'),      # CODE:2D7F9
    0x17: ('unless_lit_goto', 'rj'), # CODE:2D8C2
    0x18: ('eject_at', 'rr'),        # CODE:2D7BE
    0x19: ('op19', ''),              # CODE:2D358
    0x1A: ('serve_held', 'r'),       # CODE:2D60A
    0x1B: ('multiball', 'w'),        # CODE:2D70E
    0x1C: ('wait', 'rwj'),           # CODE:2DBDF
    0x1D: ('loop', 'j'),             # CODE:2D6CC
    0x1E: ('on_multiball_end', 'j'), # CODE:2D88D
    0x1F: ('loop_count', 'w'),       # CODE:2D5C7
}
EVENT_SIZES = {  # CODE:2D18F, record n at +4n: the second word
    0x01: 6, 0x02: 8, 0x03: 6, 0x04: 8, 0x05: 6, 0x06: 6, 0x07: 14, 0x08: 6,
    0x09: 6, 0x0A: 4, 0x0B: 4, 0x0C: 6, 0x0D: 6, 0x0E: 6, 0x0F: 6, 0x10: 6,
    0x11: 6, 0x12: 14, 0x13: 6, 0x14: 6, 0x15: 8, 0x16: 6, 0x17: 8, 0x18: 10,
    0x19: 2, 0x1A: 6, 0x1B: 4, 0x1C: 10, 0x1D: 4, 0x1E: 4, 0x1F: 4,
}
DISPLAY_OPS = {
    0x01: ('anim_wait', 'rwwwww'),   # CODE:2F9CF
    0x02: ('clear', ''),             # CODE:2F7A5
    0x03: ('text', 'r'),             # CODE:2FB17
    0x04: ('op04', 'd'),             # CODE:2FBA4 (a RET)
    0x05: ('number_swapped', 'rwwww'),  # CODE:2FB2C
    0x06: ('number', 'rwwww'),       # CODE:2F712
    0x07: ('wait_s', 'w'),           # CODE:2FB6F
    0x08: ('score', 'wwww'),         # CODE:2FA88
    0x09: ('player_number', 'rwwww'),  # CODE:2F812
    0x0A: ('wait_frames', 'w'),      # CODE:2F7B1
    0x0B: ('call', 'r'),             # CODE:2F7CA
    0x0C: ('anim', 'rwwwww'),        # CODE:2F68D (the fifth word not read)
    0x0D: ('clear_a', ''),           # CODE:2F7AB
    0x0E: ('number_2a70', 'wwww'),   # CODE:2F982
    0x0F: ('clear_b', ''),           # CODE:2F79F
    0x10: ('play', 'r'),             # CODE:2FB02
    0x11: ('op11', ''),              # CODE:2FA55 (a RET)
    0x12: ('score_b', 'wwww'),       # CODE:2F755
    0x13: ('loop_count', 'w'),       # CODE:2FAE9
    0x14: ('loop', 'j'),             # CODE:2FA56
    0x15: ('op15', 'd'),             # CODE:2FBA5 (a RET)
    0x16: ('op16', 'd'),             # CODE:2FBA6 (a RET)
    0x17: ('op17', 'd'),             # CODE:2FBA7 (a RET)
    0x18: ('palette_b', ''),         # CODE:2F80C
    0x19: ('palette_a', ''),         # CODE:2F9C9
    0x1A: ('text_blink', 'rw'),      # CODE:2F7DE
}
DISPLAY_SIZES = {  # CODE:2F621, record n at +4n: the second word
    0x01: 16, 0x02: 2, 0x03: 6, 0x04: 6, 0x05: 14, 0x06: 14, 0x07: 4,
    0x08: 10, 0x09: 14, 0x0A: 4, 0x0B: 6, 0x0C: 16, 0x0D: 2, 0x0E: 10,
    0x0F: 2, 0x10: 6, 0x11: 2, 0x12: 10, 0x13: 4, 0x14: 4, 0x15: 6,
    0x16: 6, 0x17: 6, 0x18: 2, 0x19: 2, 0x1A: 8,
}
OPERAND_SIZE = {'w': 2, 'j': 2, 'd': 4, 'r': 4, 'E': 4, 'D': 4}
# the first command's offset in a stream, by language
FIRST = {'E': 4, 'D': 6}
LANGUAGE = {'E': (EVENT_OPS, EVENT_SIZES), 'D': (DISPLAY_OPS, DISPLAY_SIZES)}

for _ops, _sizes in LANGUAGE.values():
    assert set(_ops) == set(_sizes)
    for _op, (_name, _kinds) in _ops.items():
        assert 2 + sum(OPERAND_SIZE[k] for k in _kinds) == _sizes[_op], _op


class StreamError(ValueError):
    pass


class Module:
    def __init__(self, data: bytes, relocations: set[int]):
        self.data = data
        self.relocations = relocations
        if len(data) < HEADER_SLOTS * 4:
            raise StreamError('module shorter than its 45-slot header')
        self.header = [self.pointer_at(i * 4, may_be_zero=True)
                       for i in range(HEADER_SLOTS)]

    def word(self, at: int) -> int:
        self.inside(at, 2)
        return struct.unpack_from('<H', self.data, at)[0]

    def dword(self, at: int) -> int:
        self.inside(at, 4)
        return struct.unpack_from('<I', self.data, at)[0]

    def inside(self, at: int, size: int) -> None:
        if at < 0 or at + size > len(self.data):
            raise StreamError(f'{at:X}h+{size} is outside the module')

    def pointer_at(self, at: int, may_be_zero: bool = True) -> int:
        """the module offset held at `at` (0 for none): relocated when not 0"""
        value = self.dword(at)
        if value == 0 and may_be_zero:
            return 0
        if at not in self.relocations:
            raise StreamError(f'the pointer at {at:X}h is not relocated')
        if value & LOAD_FLAGS:
            return 0  # a resource reference of the loader, not a module offset
        if value >= len(self.data):
            raise StreamError(f'the pointer at {at:X}h leaves the module')
        return value

    def pointer_list(self, slot: int) -> list[int]:
        """a header slot's zero-ended list of pointers"""
        at, out = self.header[slot], []
        if at == 0:
            return out
        while True:
            value = self.pointer_at(at)
            if value == 0:
                return out
            out.append(value)
            at += 4


def roots(module: Module) -> list[tuple[str, int, str]]:
    """(language, stream, where it hangs from) for every root found"""
    out = []
    m = module

    def add(lang, at, why):
        target = m.pointer_at(at)
        if target:
            out.append((lang, target, why))

    # zones (CODE:2C1DA): 14 bytes each, the object at +0Ah; types 0 and 1
    # queue the object's +6 (CODE:2C6F5, 2C57D), type 4 its +14h (CODE:2C7E3)
    for slot in (5, 11):
        at = m.header[slot]
        while at and m.word(at) != 0xFFFF:
            kind, obj = m.word(at + 8), m.pointer_at(at + 10)
            if obj and kind in (0, 1):
                add('E', obj + 6, f'zone {at:X}h (slot {slot}, type {kind}) object {obj:X}h +6')
            elif obj and kind == 4:
                add('E', obj + 0x14, f'zone {at:X}h (slot {slot}, type 4) object {obj:X}h +14h')
            at += 14
    # slot 14 groups: their +6 (docs/bpc-module.md)
    for group in m.pointer_list(14):
        add('E', group + 6, f'slot-14 group {group:X}h +6')
    # slot 15 records: +14h (lit, CODE:2D40E) and +18h (taken, CODE:2D97D)
    # to CODE:2FEDB; +34h queued by handler 11h (CODE:2E5A2); handler 1Ah
    # (CODE:2E51D) picks one of the 8-byte entries at +34h (a flags word,
    # a limit word, an event stream) by a number 0..FFh: a limit above
    # FFh is the last one reached
    for rec in m.pointer_list(15):
        add('D', rec + 0x14, f'slot-15 record {rec:X}h +14h')
        add('D', rec + 0x18, f'slot-15 record {rec:X}h +18h')
        handler = m.word(rec + 0x2C)
        if handler == 0x11:
            add('E', rec + 0x34, f'slot-15 record {rec:X}h +34h (handler 11h)')
        elif handler == 0x1A:
            at = m.pointer_at(rec + 0x34)
            while True:
                add('E', at + 4, f'slot-15 record {rec:X}h +34h (handler 1Ah) entry {at:X}h')
                if m.word(at + 2) > 0xFF:
                    break
                at += 8
    # slot 16 counters: +48h (CODE:2DCA1), +4Ch; each threshold's +4
    # (CODE:2DD8A) from +50h, a negative threshold ends them
    for ctr in m.pointer_list(16):
        add('E', ctr + 0x48, f'slot-16 counter {ctr:X}h +48h')
        add('E', ctr + 0x4C, f'slot-16 counter {ctr:X}h +4Ch')
        at = ctr + 0x50
        while m.word(at) < 0x8000:
            add('E', at + 4, f'slot-16 counter {ctr:X}h threshold {m.word(at)} ({at:X}h)')
            at += 12
    # slots 27 and 28: queued by zone type 0 (CODE:2C5D8)
    for slot in (27, 28):
        if m.header[slot]:
            out.append(('E', m.header[slot], f'header slot {slot}'))
    return out


def decode(module: Module, lang: str, start: int):
    """{position: (opcode, operands)} of the stream at `start`, and the
    streams its operands point to as (language, offset, position)"""
    ops, sizes = LANGUAGE[lang]
    base = start + FIRST[lang]
    commands, found, todo = {}, [], [0]
    while todo:
        pos = todo.pop()
        while pos not in commands:
            opcode = module.word(base + pos)
            if opcode == 0:
                commands[pos] = (0, [])
                break
            if opcode not in ops:
                raise StreamError(f'stream {start:X}h: unknown opcode {opcode:X}h at +{pos:X}h')
            _name, kinds = ops[opcode]
            at, operands = base + pos + 2, []
            for k in kinds:
                if k in 'wj':
                    operands.append(module.word(at))
                elif k == 'd':
                    operands.append(module.dword(at))
                else:
                    target = module.pointer_at(at)
                    operands.append(target)
                    if k in 'ED' and target:
                        found.append((k, target, pos))
                    elif k in 'ED':
                        raise StreamError(f'stream {start:X}h: no stream at +{pos:X}h')
                if k == 'j':
                    todo.append(operands[-1])
                at += OPERAND_SIZE[k]
            commands[pos] = (opcode, operands)
            if lang == 'E' and opcode == 0x0A:
                break  # goto: what follows is reached by a jump only
            pos += sizes[opcode]
    # a jump into the middle of a command shows as two that overlap
    end = 0
    for pos in sorted(commands):
        if pos < end:
            raise StreamError(f'stream {start:X}h: commands overlap at +{pos:X}h')
        opcode = commands[pos][0]
        end = pos + (sizes[opcode] if opcode else 2)
    return commands, found


def walk(module: Module):
    """{(language, stream): {'from': [...], 'commands': {...}}}"""
    streams = {}
    todo = [(lang, at, why) for lang, at, why in roots(module)]
    while todo:
        lang, at, why = todo.pop(0)
        other = 'D' if lang == 'E' else 'E'
        if (other, at) in streams:
            raise StreamError(f'{at:X}h is both an event and a display stream ({why})')
        entry = streams.get((lang, at))
        if entry is None:
            commands, found = decode(module, lang, at)
            entry = streams[(lang, at)] = {'from': [], 'commands': commands}
            for k, target, pos in found:
                todo.append((k, target, f'{"event" if lang == "E" else "display"} {at:X}h +{pos:X}h'))
        entry['from'].append(why)
    return streams


def operand_text(kind: str, value: int) -> str:
    if kind in 'rED':
        return f'@{value:X}' if value else '0'
    if kind == 'j':
        return f'+{value:X}'
    return f'{value:X}h' if value > 9 else str(value)


def listing(streams) -> str:
    lines = []
    for (lang, at) in sorted(streams, key=lambda s: (s[1], s[0])):
        entry = streams[(lang, at)]
        ops = LANGUAGE[lang][0]
        lines.append(f'{"event" if lang == "E" else "display"} {at:X}h')
        for why in entry['from']:
            lines.append(f'  from {why}')
        for pos in sorted(entry['commands']):
            opcode, operands = entry['commands'][pos]
            if opcode == 0:
                lines.append(f'  +{pos:<4X} 0 end')
                continue
            name, kinds = ops[opcode]
            args = ' '.join(operand_text(k, v) for k, v in zip(kinds, operands))
            lines.append(f'  +{pos:<4X} {opcode:X} {name} {args}'.rstrip())
        lines.append('')
    return '\n'.join(lines)


def load(bpc: Path, rel: Path) -> Module:
    data, reldata = bpc.read_bytes(), rel.read_bytes()
    if len(reldata) % 4:
        raise StreamError('REL size is not a multiple of four')
    return Module(data, {v for (v,) in struct.iter_unpack('<I', reldata)})


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument('bpc', type=Path)
    ap.add_argument('rel', type=Path)
    ap.add_argument('--check', action='store_true', help='counts only')
    args = ap.parse_args()
    try:
        streams = walk(load(args.bpc, args.rel))
    except StreamError as e:
        print(f'{args.bpc.name}: {e}', file=sys.stderr)
        return 1
    if not args.check:
        print(listing(streams))
    events = sum(1 for lang, _ in streams if lang == 'E')
    print(f'{args.bpc.name}: {events} event streams, {len(streams) - events} display streams')
    return 0


if __name__ == '__main__':
    sys.exit(main())
