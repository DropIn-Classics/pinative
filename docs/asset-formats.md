# Core table asset formats

These layouts are consistent across all four tables. They were determined by
cross-table size checks, byte distributions, coherent bitplane scans, BPC
loader behavior, and internal offset invariants.

The local validation command expects decoded files in an ignored analysis
directory:

```sh
python3 tools/asset_inspect.py build/files/DATA/S001
```

## Playfield image and palette

`STAGE.M` is a headerless 320×720 image with one palette index per byte. Its
size is exactly 230,400 bytes. Rows are stored top to bottom.

`STAGE.C` contains 256 consecutive `{ red, green, blue }` triples with 8-bit
channels, for a total of 768 bytes. Values span the full `0..255` range; this
is RGB888 source data rather than already reduced VGA DAC values.

## Sparse pixel lookup

Each `LINKn` is paired with `PIXELSn`, and the loader concatenates the two
files into one allocation.

`LINKn` is exactly 4,800 bytes: 2,400 unsigned 16-bit offsets stored most
significant byte first. The offsets are monotonically nondecreasing. The last
offset always equals `PIXELSn_size / 2` exactly.

`PIXELSn` is an array of two-byte records:

```c
struct pixel_record {
    uint8_t local_pixel;  /* always 0..83 */
    uint8_t material;     /* table-defined collision/rendering class */
};
```

The offsets are cumulative ends for 2,400 spatial buckets; bucket zero begins
at record zero and bucket `i` begins at `link[i - 1]`. The natural geometry is
20×120 buckets over a 280×720 collision field. Each bucket therefore covers
14×6 pixels, exactly the 84 positions addressable by `local_pixel`.

This interpretation explains every invariant in all eight `LINK`/`PIXELS`
pairs. The exact ordering of the 84 local positions and the semantics of each
`material` value still require access-trace confirmation.

## Bitplanes

`MASK2`, `HIDE1.M`, and `HIDE2.M` are headerless 280×720 one-bit planes. Each
row occupies 35 bytes and bits are stored most-significant first within a byte.

`MASK1` is also an MSB-first bitplane, but has a distinct 384×717 geometry:
48 bytes per row and 717 rows, totaling 34,416 bytes. Its wider coordinate
space and three-row height difference are repeatable across all tables; the
meaning of the extra horizontal extent remains open.

## Angle grid

`ANGLE1` and `ANGLE2` are each 3,150 bytes arranged as a 35×90 byte grid. This
is one value per 8×8 region of the 280×720 collision field. Zero dominates;
nonzero values are a small set of orientation classes (the observed union is
within `1..14`). The main collision path at `0x134D9` uses the grid byte as an
index into BPC header slot 18. That slot is a table-specific array of signed
16-bit X/Y pairs, so it maps each orientation class directly to a collision-
response vector. The four tables export 15, 13, 14, and 10 pairs respectively;
`bpc_inspect.py` prints the complete ordered mapping.

## Relationship summary

| Layer | Geometry | Granularity |
| --- | ---: | --- |
| `STAGE.M` | 320×720 | 8-bit display pixel |
| `MASK2`, `HIDE*.M` | 280×720 | 1 bit per collision-field pixel |
| `ANGLE*` | 35×90 | 1 byte per 8×8 collision region |
| BPC slot 18 | 10-15 signed X/Y pairs | Response vector per `ANGLE*` class |
| `LINK*` | 20×120 | cumulative record end per 14×6 collision bucket |
| `PIXELS*` | variable | two-byte sparse pixel records |
| `MASK1` | 384×717 | 1 bit per expanded-mask pixel |
