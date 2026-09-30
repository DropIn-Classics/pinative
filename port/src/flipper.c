/* flipper.c - FLIPPER_RENDER (CODE:1527F): each flipper drawn at each
 * angle it can take, in video memory over the stage, and kept: the
 * pixels that changed (FLIP_DIFF) in its "flipper gfx data" block and
 * its collision mask (FLIP_MASK) in its "flipper mask data" block.
 */
#include "game.h"
#include "names.h"
#include "pmax.h"
#include "pmem.h"
#include "vga.h"

/* CODE:3D389, CODE:3D394 */
static void MAP_MASK(uint8_t ah)
{
    vga_outw(0x3C4, (uint16_t)(2 | ah << 8));
}

static void READ_MAP(uint8_t ah)
{
    vga_outw(0x3CE, (uint16_t)(4 | ah << 8));
}

/* CODE:1634F: the flipper `p`'s angle, +6 plus +1Ah (less when +0Ah is
 * not 0), around 0..77h */
static uint32_t FLIP_ANGLE(uint32_t p)
{
    uint16_t cx = (uint16_t)(rw(p + 6) + rw(p + 0x1A));

    if (rw(p + 0x0A) != 0)
        cx = (uint16_t)(cx - 2 * rw(p + 0x1A));
    if ((int16_t)cx >= 0x78)
        cx = (uint16_t)(cx - 0x78);
    if ((int16_t)cx < 0)
        cx = (uint16_t)(cx + 0x78);
    return cx;
}

/* CODE:16482: FLIP_SHAPES's entry for `angle`, and in *ax the rows of
 * flipdat before it (each angle's height + 2) */
static uint32_t FLIP_ROWS(uint32_t angle, uint16_t *ax)
{
    uint32_t sh = N_FLIP_SHAPES;

    *ax = 0;
    while (angle--) {
        *ax = (uint16_t)(*ax + rb(sh + 2) + 2);
        sh += 4;
    }
    return sh;
}

/* the video memory offset (less 1500h) of the flipper's box at its angle
 * (the y less the shape's times 54h, the x less the shape's over 4) and
 * its rows (the height + 2); CODE:153B7, 1549E and 1536E alike */
static uint32_t flip_box(uint32_t p, uint32_t *rows)
{
    uint32_t sh = N_FLIP_SHAPES + FLIP_ANGLE(p) * 4;
    uint16_t x = (uint16_t)(rw(p + 2) - rb(sh));
    uint32_t y = (uint32_t)rw(p + 4) - rb(sh + 1);

    *rows = (uint32_t)rb(sh + 2) + 2;
    return y * 0x54 + (x >> 2);
}

/* CODE:153B7 (CODE:15404): the box's 16 bytes a row in each plane (read
 * map 0..3) from video memory to the block at `blk` from `di`; the next
 * offset */
static uint32_t FLIP_BG_SAVE(uint32_t p, uint32_t blk, uint32_t di)
{
    uint32_t rows, off = flip_box(p, &rows) + 0x1500, r, i;
    uint8_t plane;

    for (plane = 0; plane < 4; plane++) {
        READ_MAP(plane);
        for (r = 0; r < rows; r++)
            for (i = 0; i < 16; i++)
                lwb(blk + di++, vga_read((uint16_t)(off + r * 0x54 + i)));
    }
    return di;
}

/* CODE:16440: the colour 0..15 of pixel bit 7 - `dl` of byte `bp` of the
 * flipdat row at `row` (four planes 8 bytes apart) */
static uint8_t FLIP_PIXEL(uint32_t row, uint32_t bp, uint8_t dl)
{
    uint8_t al = 0;
    int k;

    for (k = 0; k < 4; k++) {
        uint8_t bh = (uint8_t)(lrb(row + bp + 8 * (uint32_t)k) << dl);
        al = (uint8_t)(al >> 1 | (bh & 0x80));
    }
    return (uint8_t)(al >> 4);
}

/* CODE:16399: the flipper at its angle drawn from flipdat into video
 * memory, 40h pixels a row, height + 1 rows, colour 0 left out */
static void FLIP_DRAW(uint32_t p)
{
    uint16_t ax, cx, dx;
    uint32_t sh = FLIP_ROWS(FLIP_ANGLE(p), &ax), src, di, rows;
    uint32_t gs = pmax_base(rw(N_FLIPDAT_SEL));
    uint8_t m;

    src = gs + (uint32_t)ax * 0x20;
    cx = (uint16_t)(rw(p + 2) - rb(sh));
    dx = (uint16_t)(rw(p + 4) - rb(sh + 1));
    wb(N_FLIP_DRAW_MASK, 0x11);
    dx = (uint16_t)(dx * 0x54);
    m = rb(N_FLIP_DRAW_MASK);
    wb(N_FLIP_DRAW_MASK, (uint8_t)(m << (cx & 3) | m >> (8 - (cx & 3))));
    dx = (uint16_t)(dx + (cx >> 2) + 0x1500);
    di = dx;
    for (rows = (uint32_t)rb(sh + 2) + 1; rows; rows--, src += 0x20) {
        uint32_t bp;

        for (bp = 0; bp < 0x40; bp++) {
            uint8_t al = FLIP_PIXEL(src, bp >> 3, (uint8_t)(bp & 7));

            MAP_MASK(rb(N_FLIP_DRAW_MASK));
            if (al)
                vga_write((uint16_t)di, al);
            m = rb(N_FLIP_DRAW_MASK);
            wb(N_FLIP_DRAW_MASK, (uint8_t)(m << 1 | m >> 7));
            di += m >> 7;       /* ROL's carry: the fourth plane done */
        }
        di += 0x44;
    }
}

/* CODE:1549E (CODE:154EB): the box read back plane by plane and compared
 * with what FLIP_BG_SAVE kept at `di`: a pixel equal to it becomes 0, a
 * changed one stays; the first row of the plane's columns gets, where it
 * is still 0, the number of the row where the column first changed
 * (FLIP_DIFF_COL walks it, FLIP_DIFF_ROW counts the rows) */
static void FLIP_DIFF(uint32_t p, uint32_t blk, uint32_t di)
{
    uint32_t rows, off = flip_box(p, &rows) + 0x1500, r, i;
    uint8_t plane;

    for (plane = 0; plane < 4; plane++) {
        READ_MAP(plane);
        wd(N_FLIP_DIFF_COL, di);
        wb(N_FLIP_DIFF_ROW, 0);
        for (r = 0; r < rows; r++) {
            uint32_t keep = rd(N_FLIP_DIFF_COL);

            for (i = 0; i < 16; i++) {
                uint8_t al = vga_read((uint16_t)(off + r * 0x54 + i));

                if (al == lrb(blk + di)) {
                    al = 0;
                } else {
                    uint32_t s = rd(N_FLIP_DIFF_COL);
                    if (lrb(blk + s) == 0)
                        lwb(blk + s, rb(N_FLIP_DIFF_ROW));
                }
                lwb(blk + di++, al);
                wd(N_FLIP_DIFF_COL, rd(N_FLIP_DIFF_COL) + 1);
            }
            wd(N_FLIP_DIFF_COL, keep);
            wb(N_FLIP_DIFF_ROW, (uint8_t)(rb(N_FLIP_DIFF_ROW) + 1));
        }
    }
}

/* CODE:1536E (CODE:167E4): the box put back from the stage's copy in
 * SPOOKY_SEL (its planes C4E0h apart), plane by plane */
static void FLIP_BG_RESTORE(uint32_t p)
{
    uint32_t rows, off = flip_box(p, &rows), r, i;
    uint32_t sp = pmax_base(rw(N_SPOOKY_SEL)) + off;
    uint8_t plane;

    for (plane = 0; plane < 4; plane++, sp += 0xC4E0) {
        MAP_MASK((uint8_t)(1 << plane));
        for (r = 0; r < rows; r++)
            for (i = 0; i < 16; i++)
                vga_write((uint16_t)(off + 0x1500 + r * 0x54 + i), lrb(sp + r * 0x54 + i));
    }
}

/* a byte of the collision map: the word at DS:`di` rotated left by
 * FLIP_MASK_SHIFT, its low byte */
static uint8_t map_byte(uint32_t di)
{
    uint16_t ax = rw(di);
    uint8_t cl = rb(N_FLIP_MASK_SHIFT);

    return (uint8_t)(cl ? (ax << cl | ax >> (16 - cl)) : ax);
}

/* CODE:16494: the flipper's collision mask at its angle, 16 bytes (128
 * pixels) a row: 20h rows of the collision map above it, height + 1
 * rows with the flipper's pixels (any plane of flipdat) ORed into the
 * middle 8 bytes, 20h rows below; from the angle's start (word +B0h by
 * +1Ah, in 16-byte units) in the block at +28h, the next angle's start
 * to +B2h */
static void FLIP_MASK(uint32_t p)
{
    uint16_t ax, cx, dx;
    uint32_t sh = FLIP_ROWS(FLIP_ANGLE(p), &ax), ebp, di, gs, r, i;

    ebp = ((uint32_t)rw(p + 0xB0 + 2 * (uint32_t)rw(p + 0x1A)) << 4) + rd(p + 0x28);
    cx = (uint16_t)(rw(p + 2) - rb(sh) - 0x20);
    dx = (uint16_t)(rw(p + 4) - rb(sh + 1) - 0x20);
    dx = (uint16_t)(dx * 0x2A);
    dx = (uint16_t)(dx + (uint16_t)((int16_t)cx >> 3));
    wb(N_FLIP_MASK_SHIFT, (uint8_t)(cx & 7));
    di = (uint32_t)dx + rd(p + 0x1C);
    gs = pmax_base(rw(N_FLIPDAT_SEL)) + (uint32_t)ax * 0x20;

    for (r = 0; r < 0x20; r++, di += 0x1A)
        for (i = 0; i < 16; i++)
            wb(ebp++, map_byte(di++));
    for (r = (uint32_t)rb(sh + 2) + 1; r; r--) {
        for (i = 0; i < 4; i++) {
            wb(ebp + i, map_byte(di + i));
            wb(ebp + 0x0C + i, map_byte(di + 0x0C + i));
        }
        for (i = 0; i < 8; i++, ebp++, gs++, di++)
            wb(ebp + 4, (uint8_t)(map_byte(di + 4) | lrb(gs) | lrb(gs + 8) | lrb(gs + 0x10)
                                  | lrb(gs + 0x18)));
        gs += 0x18;
        ebp += 8;
        di += 0x22;
    }
    for (r = 0; r < 0x20; r++, di += 0x1A)
        for (i = 0; i < 16; i++)
            wb(ebp++, map_byte(di++));
    ww(p + 0xB2 + 2 * (uint32_t)rw(p + 0x1A), (uint16_t)((ebp - rd(p + 0x28)) >> 4));
}

void FLIPPER_RENDER(void)
{
    uint32_t p = rd(rd(0x0014) + 0x28FE);
    int k;

    for (k = 0; k < 4; k++, p += 0x1F7) {
        uint32_t edx, di = 0, ebx = 0, blk;
        uint16_t keep;
        int step;

        if (rb(p) == 3)
            continue;
        keep = rw(p + 0x1A);
        ww(p + 0x1A, 0);
        edx = rw(p + 6);
        blk = pmax_base(rw(p + 0x1F2));
        step = rw(p + 0x0A) != 0 ? -1 : 1;
        for (;;) {
            ww(p + 0x30 + 2 * ebx, (uint16_t)di);
            ww(p + 0x1A, (uint16_t)ebx);
            wd(N_FLIP_NEXT_DI, FLIP_BG_SAVE(p, blk, di));
            FLIP_DRAW(p);
            FLIP_DIFF(p, blk, di);
            FLIP_BG_RESTORE(p);
            FLIP_MASK(p);
            di = rd(N_FLIP_NEXT_DI);
            if ((uint16_t)edx == rw(p + 8))
                break;
            edx += (uint32_t)step;
            if ((int16_t)edx >= 0x78)
                edx = 0;
            if ((int16_t)edx < 0)
                edx = 0x77;
            ebx++;
        }
        ww(p + 0x1A, keep);
    }
}
