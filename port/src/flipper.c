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

/* CODE:1637B: +6 plus +1Ah, around 0..77h (+0Ah not looked at: the
 * negative +1Ah of a flipper turning the other way does it) */
static uint32_t FLIP_ANGLE2(uint32_t p)
{
    uint16_t cx = (uint16_t)(rw(p + 6) + rw(p + 0x1A));

    if ((int16_t)cx >= 0x78)
        cx = (uint16_t)(cx - 0x78);
    if ((int16_t)cx < 0)
        cx = (uint16_t)(cx + 0x78);
    return cx;
}

/* the box of the angle FLIP_ANGLE2 gives (as flip_box), the angle's kept
 * pixels' offset in the gfx block (the word +30h by |+1Ah|) to *src */
static uint32_t flip_box2(uint32_t p, uint32_t *rows, uint32_t *src)
{
    int32_t n = (int16_t)rw(p + 0x1A);
    uint32_t sh = N_FLIP_SHAPES + FLIP_ANGLE2(p) * 4;
    uint16_t x = (uint16_t)(rw(p + 2) - rb(sh));
    uint32_t y = (uint32_t)rw(p + 4) - rb(sh + 1);

    if (n < 0)
        n = -n;
    *src = rw(p + 0x30 + 2 * (uint32_t)n);
    *rows = (uint32_t)rb(sh + 2) + 2;
    return y * 0x54 + (x >> 2);
}

/* the columns of one plane (CODE:157EA; CODE:15CED with `restore`): a
 * column's first byte (the row where it first changed, or row 0's pixel)
 * is how many rows are passed over; from there up to 55 rows while the
 * kept pixel is not 0, the pixel drawn (the drawing addresses with SI and
 * DI, 16 bits) or the stage's pixel from SPOOKY_SEL put back */
static void flip_columns(uint32_t blk, uint32_t src, uint32_t di, int restore, uint32_t spooky)
{
    uint32_t col;

    for (col = 0; col < 16; col++, src++, di++) {
        uint32_t skip = lrb(blk + src), s, d, r;

        if (!skip)
            continue;
        s = src + skip * 16;
        d = di + skip * 0x54;
        for (r = 0; r < 55; r++) {
            uint8_t al;

            if (restore) {
                if (!lrb(blk + s + r * 16))
                    break;
                vga_write((uint16_t)(d + 0x1500 + r * 0x54), lrb(spooky + d + r * 0x54));
            } else {
                al = lrb(blk + (uint16_t)(s + r * 16));
                if (!al)
                    break;
                vga_write((uint16_t)((uint16_t)d + 0x1500 + r * 0x54), al);
            }
        }
    }
}

/* CODE:1572D (CODE:1578B), CODE:15C1A (CODE:15C78): the flipper's kept
 * pixels at its angle drawn into video memory, or the stage put back
 * under them, plane by plane (map mask 1, 2, 4, 8) */
static void flip_blit(uint32_t p, int restore)
{
    uint32_t rows, src, di = flip_box2(p, &rows, &src);
    uint32_t blk = pmax_base(rw(p + 0x1F2)), spooky = pmax_base(rw(N_SPOOKY_SEL));
    uint8_t plane;

    for (plane = 0; plane < 4; plane++, spooky += 0xC4E0) {
        MAP_MASK((uint8_t)(1 << plane));
        flip_columns(blk, src, di, restore, spooky);
        src += rows * 16;
    }
}

/* CODE:156C4: each flipper (four; type 3 passed over) whose angle's
 * number +1Ah is not the one drawn (+1F4h): the old one put back, the new
 * one drawn; or drawn again when +1F6h asks for it */
void FLIPPERS_DRAW(void)
{
    uint32_t p = rd(rd(0x0014) + 0x28FE);
    int k;

    for (k = 0; k < 4; k++, p += 0x1F7) {
        uint16_t now;

        if (rb(p) == 3)
            continue;
        now = rw(p + 0x1A);
        if (rw(p + 0x1F4) != now) {
            ww(p + 0x1A, rw(p + 0x1F4));
            flip_blit(p, 1);
            ww(p + 0x1A, now);
            ww(p + 0x1F4, now);
        } else if (rb(p + 0x1F6)) {
            wb(p + 0x1F6, 0);
        } else {
            continue;
        }
        flip_blit(p, 0);
    }
}

static uint32_t sx16(uint16_t w)
{
    return (uint32_t)(int32_t)(int16_t)w;
}

static uint16_t sar(uint16_t w, int n)
{
    return (uint16_t)((int16_t)w >> n);
}

/* the end of a move: the position in the low word of the work cell
 * `cell` to +12h, over 40h to the cell and +1Ah; the next record */
static void flip_moved(uint32_t p, uint32_t cell)
{
    uint16_t si = rw(cell);

    ww(p + 0x12, si);
    si = sar(si, 6);
    ww(cell, si);
    ww(p + 0x1A, si);
}

/* CODE:147B6 (left) and CODE:149EF (right): the flipper `p` moving up,
 * the speed +10h times 32h over FRAME_RATE added to the position +12h;
 * at the rest's side of 0 it goes back there, at the limit +14h it stops
 * (state+2A7Dh or 2A7Eh FFh), else the speed grows by +16h up to +18h */
static void flip_up(uint32_t p, int left)
{
    uint32_t st = rd(0x0014);
    uint16_t pos = rw(p + 0x12), cx;
    int16_t q;

    wd(0x0020, sx16(rw(p + 0x10)));
    wd(0x0028, sx16(rw(p + 0x14)));
    wd(0x002C, sx16(rw(p + 0x16)));
    wd(0x0030, sx16(rw(p + 0x18)));
    q = (int16_t)((int32_t)0x32 * (int16_t)rw(p + 0x10) / (int16_t)rw(N_FRAME_RATE));
    cx = (uint16_t)(pos + q);
    wd(0x0024, (sx16(pos) & 0xFFFF0000u) | cx);
    if (left) {
        if ((int16_t)cx >= 0) {
            wd(0x0024, 0xFFFFFFFFu);
            ww(p + 0x10, 0);
        } else if ((int16_t)cx <= (int16_t)rw(0x0028)) {
            ww(0x0024, rw(0x0028));
            wb(st + 0x2A7D, 0xFF);
            ww(p + 0x10, 0);
        } else {
            ww(0x0020, (uint16_t)(rw(0x0020) - rw(0x002C)));
            if ((int16_t)rw(0x0020) <= (int16_t)rw(0x0030))
                ww(0x0020, rw(0x0030));
            ww(p + 0x10, rw(0x0020));
        }
    } else {
        if ((int16_t)cx < 0) {
            wd(0x0024, 0);
            ww(p + 0x10, 0);
        } else if ((int16_t)cx >= (int16_t)rw(0x0028)) {
            ww(0x0024, rw(0x0028));
            wb(st + 0x2A7E, 0xFF);
            ww(p + 0x10, 0);
        } else {
            ww(0x0020, (uint16_t)(rw(0x0020) + rw(0x002C)));
            if ((int16_t)rw(0x0020) >= (int16_t)rw(0x0030))
                ww(0x0020, rw(0x0030));
            ww(p + 0x10, rw(0x0020));
        }
    }
    flip_moved(p, 0x0024);
}

/* CODE:148DA (left) and CODE:14AEA (right): the flipper `p` falling
 * back, the speed +10h added to the position unscaled; at the rest (0,
 * the left one -1) it stops (state+2A7Dh or 2A7Eh 0), past the limit +14h
 * it is held there, else the speed grows by +0Ch up to +0Eh */
static void flip_down(uint32_t p, int left)
{
    uint32_t st = rd(0x0014), edx = sx16(rw(p + 0x12));
    uint16_t dx;

    wd(0x0020, sx16(rw(p + 0x0C)));
    wd(0x0024, sx16(rw(p + 0x0E)));
    wd(0x0028, sx16(rw(p + 0x10)));
    wd(0x0030, sx16(rw(p + 0x14)));
    dx = (uint16_t)(edx + rw(p + 0x10));
    wd(0x002C, (edx & 0xFFFF0000u) | dx);
    if (left) {
        if ((int16_t)dx >= 0) {
            wd(0x002C, 0xFFFFFFFFu);
            ww(p + 0x10, 0);
            wb(st + 0x2A7D, 0);
        } else if ((int16_t)dx < (int16_t)rw(0x0030)) {
            ww(0x002C, rw(0x0030));
            ww(p + 0x10, 0);
        } else {
            ww(0x0028, (uint16_t)(rw(0x0028) + rw(0x0020)));
            if ((int16_t)rw(0x0028) >= (int16_t)rw(0x0024))
                ww(0x0028, rw(0x0024));
            ww(p + 0x10, rw(0x0028));
        }
    } else {
        if ((int16_t)dx < 0) {
            wd(0x002C, 0);
            ww(p + 0x10, 0);
            wb(st + 0x2A7E, 0);
        } else if ((int16_t)dx > (int16_t)rw(0x0030)) {
            ww(0x002C, rw(0x0030));
            ww(p + 0x10, 0);
        } else {
            ww(0x0028, (uint16_t)(rw(0x0028) - rw(0x0020)));
            if ((int16_t)rw(0x0028) <= (int16_t)rw(0x0024))
                ww(0x0028, rw(0x0024));
            ww(p + 0x10, rw(0x0028));
        }
    }
    flip_moved(p, 0x002C);
}

/* a nudge key's press (CODE:144C5 and on): bit `bit` of state+2A77h set;
 * when it was set already (the key held) that of state+2A76h cleared */
static void nudge_press(uint32_t st, int bit)
{
    uint8_t was = rb(st + 0x2A77);

    wb(st + 0x2A77, (uint8_t)(was | 1 << bit));
    if (was & 1 << bit)
        wb(st + 0x2A76, (uint8_t)(rb(st + 0x2A76) & ~(1 << bit)));
}

/* CODE:144AD: the nudges, the flipper keys, each flipper's move and the
 * tilt count */
void FLIPPERS_MOVE(void)
{
    uint32_t st = rd(0x0014), p;
    uint16_t di;
    uint8_t bl;
    int nudge_x = 1;

    wb(st + 0x2A76, (uint8_t)(rb(st + 0x2A76) | 7));
    if (rb(st + 0x0E7F) != 0) {                         /* Space */
        nudge_press(st, 0);
        ww(st + 0x0D44, 0x258);
        di = (uint16_t)(rw(st + 0x0D48) + rw(st + 0x0D44));
        ww(0x0020, di);
        ww(st + 0x0D48, di);
        if (di >= 0x3E8) {
            ww(st + 0x0D44, 0);
            ww(st + 0x0D48, 0x3E8);
            wb(st + 0x0E7F, 0);
        }
    } else {
        wb(st + 0x2A77, (uint8_t)(rb(st + 0x2A77) & ~1));
        ww(st + 0x0D44, 0xFF38);
        di = (uint16_t)(rw(st + 0x0D48) + rw(st + 0x0D44));
        ww(0x0020, di);
        ww(st + 0x0D48, di);
        if ((int16_t)di < 0) {
            ww(st + 0x0D44, 0);
            ww(st + 0x0D48, 0);
        }
    }
    /* CODE:14578 */
    di = sar(rw(st + 0x0D48), 8);
    ww(0x0020, di);
    ww(st + 0x0D4C, di);

    if (rb(st + 0x0E7E) != 0) {                         /* Left Alt */
        nudge_press(st, 1);
        ww(st + 0x0D42, 0xFDA8);
        di = (uint16_t)(rw(st + 0x0D46) + rw(st + 0x0D42));
        ww(0x0020, di);
        ww(st + 0x0D46, di);
        if ((int16_t)di <= (int16_t)0xFC18) {
            ww(st + 0x0D42, 0);
            ww(st + 0x0D46, 0xFC18);
            wb(st + 0x0E7E, 0);
        }
    } else if (rb(st + 0x0EFE) != 0) {                  /* Right Alt */
        nudge_press(st, 2);
        ww(st + 0x0D42, 0x258);
        di = (uint16_t)(rw(st + 0x0D46) + rw(st + 0x0D42));
        ww(0x0020, di);
        ww(st + 0x0D46, di);
        if ((int16_t)di >= 0x3E8) {
            ww(st + 0x0D42, 0);
            ww(st + 0x0D46, 0x3E8);
            wb(st + 0x0EFE, 0);
        }
    } else {                                            /* CODE:14692 */
        wb(st + 0x2A77, (uint8_t)(rb(st + 0x2A77) & 0xF9));
        di = rw(st + 0x0D46);
        ww(0x0020, di);
        if (di == 0) {
            nudge_x = 0;
        } else {
            uint32_t sum;

            ww(st + 0x0D42, (int16_t)di < 0 ? 0xC8 : 0xFF38);
            sum = (uint32_t)di + rw(st + 0x0D42);
            ww(0x0020, (uint16_t)sum);
            if (sum > 0xFFFF) {
                /* the carry: both cleared, +0D46h then overwritten */
                ww(st + 0x0D42, 0);
                ww(st + 0x0D46, 0);
            }
            ww(st + 0x0D46, rw(0x0020));
        }
    }
    if (nudge_x) {                                      /* CODE:14717 */
        di = sar(rw(st + 0x0D46), 8);
        ww(0x0020, di);
        ww(st + 0x0D4A, di);
    }

    /* CODE:1473F: the flipper keys */
    wd(0x0000, rd(st + 0x28FE));
    wb(st + 0x2A7B, (uint8_t)(rb(st + 0x0E70) | rb(st + 0x0E63)));
    bl = (uint8_t)(rb(st + 0x0E7C) | rb(st + 0x0EE3));
    wb(0x0024, bl);
    wb(st + 0x2A7C, bl);

    for (;;) {                                          /* CODE:14781 */
        p = rd(0x0000);
        wb(0x0020, rb(p));
        if (rb(p) == 0)
            break;
        if (rb(p) != 3) {
            int left = rw(p + 0x0A) != 0;

            if (rb(st + 0x2A7F) != 0 && rb(st + (left ? 0x2A7B : 0x2A7C)) != 0)
                flip_up(p, left);
            else
                flip_down(p, left);
        }
        wd(0x0000, rd(0x0000) + 0x1F7);
    }

    /* CODE:14BD7: the tilt count */
    if (rb(st + 0x0FC5) != 0)
        return;
    bl = (uint8_t)(rb(st + 0x2A77) & rb(st + 0x2A76));
    wb(0x0020, bl);
    if (bl != 0) {
        ww(0x0020, rw(st + 0x0E3E));
        ww(st + 0x2A78, (uint16_t)(rw(st + 0x2A78) + rw(0x0020)));
        if (rw(st + 0x2A78) >= 0xC8)
            wb(st + 0x2A75, 0xFF);
    }
    if (rw(st + 0x2A78) != 0)
        ww(st + 0x2A78, (uint16_t)(rw(st + 0x2A78) - 1));
}

/* CODE:104A5: a flipper's sound when state+2A7Dh (left) or 2A7Eh (right)
 * changed: FLIP_UP_SFX when it went to FFh, FLIP_DOWN_SFX to 0 */
void FLIPPER_SOUNDS(void)
{
    uint32_t st = rd(0x0014);
    int k;

    for (k = 0; k < 2; k++) {
        uint32_t was = k ? N_FLIP_R_WAS : N_FLIP_L_WAS;
        uint8_t b = rb(was), now = rb(st + 0x2A7D + k);

        wb(0x0020, b);
        if (b != now) {
            wd(0x0000, (int8_t)(b - now) < 0 ? N_FLIP_DOWN_SFX : N_FLIP_UP_SFX);
            SFX_PLAY();
        }
        wb(was, rb(st + 0x2A7D + k));
    }
}

/* CODE:1048B: four moves, the sounds */
void FLIPPERS_STEP(void)
{
    FLIPPERS_MOVE();
    FLIPPERS_MOVE();
    FLIPPERS_MOVE();
    FLIPPERS_MOVE();
    FLIPPER_SOUNDS();
}
