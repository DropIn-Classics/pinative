#!/usr/bin/env python3
"""The resource archive appended to ILLUSION.EXE (docs/reverse-engineering.md).

    illfiles.py [--exe PATH] list
    illfiles.py [--exe PATH] extract NAME... [-o DIR]
    illfiles.py [--exe PATH] extract --all [-o DIR]

The archive follows the MZ image: a word count, a word directory size,
the directory (per entry a NUL-terminated name whose bytes are the
clear text rotated left by 3, then offset, unpacked size, packed size as
dwords; offsets from the archive's start), then the payloads (a dword
unpacked size, then the NLZW stream). The decoder is a translation of
retools/pi_inspect.c. Files go to build/files/ with their archive paths.
"""
import argparse, os, struct, sys

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)),
                                '..', 'doskit', 'tools'))
from kit import game_dir, build_dir

M32 = 0xFFFFFFFF
LIMIT = 0x1F3F


def ror3(b):
    return ((b >> 3) | (b << 5)) & 0xFF


class Archive:
    def __init__(self, path):
        with open(path, 'rb') as f:
            self.data = f.read()
        d = self.data
        if d[:2] != b'MZ':
            raise SystemExit(f'{path}: not an MZ program')
        last, pages = struct.unpack_from('<HH', d, 2)
        self.base = pages * 512 - ((512 - last) if last else 0)
        count, dirbytes = struct.unpack_from('<HH', d, self.base)
        self.entries = []
        p, end = self.base + 4, self.base + 4 + dirbytes
        for _ in range(count):
            z = d.index(0, p, end)
            name = bytes(ror3(b) for b in d[p:z]).decode('ascii')
            off, unpacked, packed = struct.unpack_from('<III', d, z + 1)
            self.entries.append((name, off, unpacked, packed))
            p = z + 13
        if p != end:
            raise SystemExit('directory size does not match its entries')

    def check(self):
        """Payloads follow each other from the directory's end to EOF."""
        want = 4 + struct.unpack_from('<H', self.data, self.base + 2)[0]
        for name, off, unpacked, packed in self.entries:
            if off != want:
                return f'{name}: at {off:X}, expected {want:X}'
            if struct.unpack_from('<I', self.data, self.base + off)[0] != unpacked:
                return f'{name}: size prefix differs from the directory'
            want = off + 4 + packed
        if self.base + want != len(self.data):
            return f'the last payload ends at {self.base + want:X}, the file at {len(self.data):X}'
        return None

    def find(self, name):
        for e in self.entries:
            if e[0].upper() == name.upper().replace('/', '\\'):
                return e
        raise SystemExit(f'{name}: not in the archive')

    def unpack(self, entry):
        name, off, unpacked, packed = entry
        return Decoder(self.data, self.base + off + 4).lzw(unpacked)


class Decoder:
    """NLZW: LZW codes through a uniform arithmetic coder (pi_inspect.c)."""

    def __init__(self, data, pos):
        self.data, self.pos = data, pos
        self.low, self.high, self.total = 0, M32, 256
        self.bit_count, self.pending = 0, 0
        w = [self.word() for _ in range(4)]
        self.code = (w[0] << 16) | w[1]
        self.bits = (w[2] << 16) | w[3]

    def word(self):
        p = self.pos
        self.pos += 2
        if p + 2 > len(self.data):
            return 0        # the DOS decoder reads padding bits past EOF
        return (self.data[p] << 8) | self.data[p + 1]

    def refill(self):
        self.bit_count -= 16
        self.bits = (self.bits | (self.word() << self.bit_count)) & M32

    def shift(self, n):
        if n == 0:
            return
        self.code = ((self.code << n) | (self.bits >> (32 - n))) & M32
        self.bits = (self.bits << n) & M32
        self.high = ((self.high << n) | (M32 >> (32 - n))) & M32
        self.low = (self.low << n) & M32

    def consume(self, n):
        if self.bit_count + n >= 32:
            first = 32 - self.bit_count
            self.shift(first)
            self.bit_count = 32
            self.refill()
            self.refill()
            self.bit_count = n - first
            self.shift(n - first)
        else:
            self.bit_count += n
            self.shift(n)
        if self.bit_count >= 16:
            self.refill()

    def normalize(self):
        while True:
            diff = self.high ^ self.low
            equal = 32 - diff.bit_length()
            if equal:
                self.consume(equal)
                return
            if self.pending:
                self.consume(1)
                self.pending = 0
                continue
            lo = ((~self.low) << 1) & M32
            hi = (self.high << 1) & M32
            top = max(lo.bit_length() - 1 if lo else 0, hi.bit_length() - 1 if hi else 0)
            n = 31 - top
            if n == 0:
                return
            self.pending += n
            self.consume(n)
            self.code ^= 0x80000000
            self.high ^= 0x80000000
            self.low ^= 0x80000000
            return

    def symbol(self):
        span = self.high - self.low + 1
        s = ((self.code - self.low + 1) * self.total - 1) // span
        if s >= self.total:
            raise ValueError('NLZW: symbol out of range')
        low = self.low
        self.high = (low + (span * (s + 1)) // self.total - 1) & M32
        self.low = (low + (span * s) // self.total) & M32
        self.normalize()
        self.total += 1
        return s

    def lzw(self, size):
        out = bytearray()
        parent = [-1] * LIMIT
        suffix = list(range(256)) + [0] * (LIMIT - 256)

        def string(c):
            s = bytearray()
            while c != -1:
                s.append(suffix[c])
                c = parent[c]
            s.reverse()
            return s

        while len(out) < size:
            nxt = 256
            prev = self.symbol()
            if prev >= nxt:
                raise ValueError('NLZW: bad first code')
            out.append(suffix[prev])
            while len(out) < size:
                c = self.symbol()
                if c > nxt:
                    raise ValueError('NLZW: bad code')
                if c < nxt:
                    s = string(c)
                else:
                    s = string(prev)
                    s.append(s[0])
                out += s
                parent[nxt], suffix[nxt] = prev, s[0]
                prev = c
                nxt += 1
                if nxt == LIMIT:
                    self.total = 256
                    break
        return bytes(out[:size])


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('--exe', help='ILLUSION.EXE (default: the game folder\'s)')
    sub = ap.add_subparsers(dest='cmd', required=True)
    sub.add_parser('list')
    x = sub.add_parser('extract')
    x.add_argument('names', nargs='*')
    x.add_argument('--all', action='store_true')
    x.add_argument('-o', '--out')
    a = ap.parse_args()

    ar = Archive(a.exe or os.path.join(game_dir(), 'ILLUSION.EXE'))
    if a.cmd == 'list':
        print(f'archive at {ar.base:X}, {len(ar.entries)} entries')
        for i, (name, off, unpacked, packed) in enumerate(ar.entries):
            print(f'{i:3}  {name:36}  off={off:08X}  size={unpacked:9}  packed={packed:9}')
        err = ar.check()
        print(err or 'layout ok: payloads contiguous up to the end of the file')
        return 1 if err else 0
    out = a.out or os.path.join(build_dir(), 'files')
    todo = ar.entries if a.all else [ar.find(n) for n in a.names]
    for e in todo:
        path = os.path.join(out, *e[0].split('\\'))
        os.makedirs(os.path.dirname(path), exist_ok=True)
        with open(path, 'wb') as f:
            f.write(ar.unpack(e))
        print(f'{e[0]} -> {path}')
    return 0


if __name__ == '__main__':
    sys.exit(main())
