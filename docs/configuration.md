# Configuration container

The installed `ILLUSION.CFG` is 544 bytes: a 32-byte setup header followed by
a 512-byte pMAX-managed payload. This layout is established from the installed
file size and the main image's explicit 512-byte clear operation at `0x32F16`.

Run the structural inspector with:

```sh
python3 tools/cfg_inspect.py PATH/ILLUSION.CFG   # the installed game's
```

## On-disk observations

The header begins with a NUL-terminated SDR resource name. In the inspected
installation this is `SB16.SDR`, matching the driver loaded by the main image.
The remaining header bytes are retained as opaque setup metadata.

The payload begins at file offset `0x20`. In the inspected file every one of
its 128 dwords is the ASCII sequence `SN95`. This is treated as an encoded or
pMAX-owned representation, not as literal runtime option values. The project
does not rewrite it directly.

## Runtime block

Startup invokes pMAX service `0x94/0x07` with `EDX=0x9D`, which populates a
512-byte block in the main image. Service `0x94/0x06` persists that block. The
reset path at `0x32F16` clears exactly `0x80` dwords before saving it.

The embedded setup UI treats runtime offsets `0x9D` through `0xA2` as six
zero-based option indices. The exclusive option counts at `0x32DE3` are:

| Runtime address | Option | Ordered values | Engine mapping |
| ---: | --- | --- | --- |
| `0x9D` | Balls per game | Three, five | `3, 5` at `0xB2C0` |
| `0x9E` | Table angle | Normal, high, very high, very low, low | `3, 4, 5, 1, 2` at `0xB2D6` |
| `0x9F` | Scrolling | Medium, smooth, fast | `3, 5, 1` at `0xB2C4` |
| `0xA0` | Multiball maximum | Six, three, four, five | Selects a capacity at `0xB13E`; `0xB0E3` clamps it into BPC descriptor slots 43 and 44 |
| `0xA1` | Tilt sensitivity | Normal, earthquake | `100, 0` at `0xB2CA` |
| `0xA2` | Resolution | VGA 360x350, SVGA 640x480, SVGA 800x600, VGA 320x240 | `1, 2, 3, 0` at `0xB2CE` and selects the display-probe path |

Byte `0xA3` is a cached BIOS display mode associated with option `0xA2`. It is
validated at startup, recomputed when necessary, and cleared when option
`0xA2` changes. Successful correction is immediately persisted.

The two BPC descriptor capacities controlled by `0xA0` have independent
per-table caps. Slots 43 and 44 respectively use caps `(6, 4)` for table 1,
neither descriptor for table 2, `(6, unused)` for table 3, and `(4, 6)` for
table 4. A zero cap skips the write rather than storing zero.

The user-facing labels and their order come from the English block of
`SETSOUND\\SETSOUND.DAT` at offsets `0xD9F` through `0x1012`. The numeric
mappings above come independently from the main image and align with those
labels, providing a cross-check on field order.
