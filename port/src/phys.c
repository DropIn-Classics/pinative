/* phys.c - the balls' physics (BALLS_PHYSICS, CODE:1023E): each ball
 * sampled against the level's map, the normal and the surface found, the
 * bounce, the move and the slope (docs/HANDOFF.md, "The balls' physics").
 * The work cells CODE:0000..003C are kept as the original leaves them.
 * Not translated yet, the port stopping there by name: a bumper's or
 * slingshot's kick, two balls against each other.
 */
#include <stdio.h>
#include "game.h"
#include "names.h"
#include "pmem.h"

static uint32_t sx16(uint16_t v)
{
    return (uint32_t)(int32_t)(int16_t)v;
}

/* the dword cell `c` with its low word `w` */
static void cw(uint32_t c, uint16_t w)
{
    wd(c, (rd(c) & 0xFFFF0000u) | w);
}

/* the dword cell `c` with its low byte `b` */
static void cb(uint32_t c, uint8_t b)
{
    wd(c, (rd(c) & 0xFFFFFF00u) | b);
}

/* ADD WORD PTR [c],v */
static void addw(uint32_t c, uint16_t v)
{
    ww(c, (uint16_t)(rw(c) + v));
}

static uint32_t rol(uint32_t v, unsigned n)
{
    n &= 31;
    return n ? v << n | v >> (32 - n) : v;
}

static uint32_t bswap(uint32_t v)
{
    return v >> 24 | (v >> 8 & 0xFF00u) | (v << 8 & 0xFF0000u) | v << 24;
}

/* four bytes at `p` as a big-endian dword */
static uint32_t rbe(uint32_t p)
{
    return (uint32_t)rb(p) << 24 | (uint32_t)rb(p + 1) << 16 | (uint32_t)rb(p + 2) << 8 | rb(p + 3);
}

/* IDIV r16: DX:AX / d; 1 when the original would fault (#DE) */
static int idiv16(uint32_t eax, uint32_t edx, uint16_t d, uint16_t *q, uint16_t *r)
{
    int32_t n = (int32_t)((edx & 0xFFFF) << 16 | (eax & 0xFFFF)), qq;

    if ((int16_t)d == 0)
        return 1;
    qq = n / (int16_t)d;
    if (qq < -32768 || qq > 32767)
        return 1;
    *q = (uint16_t)qq;
    *r = (uint16_t)(n % (int16_t)d);
    return 0;
}

/* ---- sampling ---- */

/* 17 lines of the ring (RING_MASKS) at x AX-7, line DX of the bit map
 * at ESI (`stride` bytes a line) into BALL_RING, byte-swapped; every hit
 * bit ORed into CODE:12819 */
static void ring_sample(uint16_t ax, uint16_t dx, uint32_t esi, uint32_t stride)
{
    uint16_t cx = (uint16_t)(ax - 7), sdx;
    uint32_t eax, ebx, i;
    unsigned cl;

    ax = (uint16_t)(dx * stride);
    sdx = cx;
    cl = (unsigned)(8 - (cx & 7));
    ax = (uint16_t)(ax + (uint16_t)((int16_t)sdx >> 3));
    esi += ax;
    wd(0x12819, 0);
    for (i = 0; i < 17; i++) {
        eax = rd(N_RING_MASKS + 4 * i) << cl;
        ebx = rbe(esi + stride * i);
        eax = (eax & ebx) >> cl;
        wd(0x12819, rd(0x12819) | eax);
        wd(N_BALL_RING + 4 * i, bswap(eax));
    }
}

/* CODE:12864: the ring against the level's map (2Ah bytes a line) */
static void MAP_SAMPLE(uint16_t ax, uint16_t dx, uint32_t esi)
{
    ring_sample(ax, dx, esi, 0x2A);
}

/* CODE:1241E: the ring against the flipper `edi`'s mask at its angle:
 * the mask (16 bytes a line, [+28h] plus the angle's word at +0B0h
 * times 16) for the angle +1Ah (a negative one mirrored: -n-1), placed by
 * FLIP_SHAPES' two bytes for (+6 + +1Ah) mod 78h, less the flipper's
 * +2, +4, plus 20h */
static void FLIP_SAMPLE(uint32_t edi, uint16_t ax, uint16_t dx)
{
    int32_t n = (int16_t)rw(edi + 0x1A);
    uint16_t k = (uint16_t)((uint16_t)n + rw(edi + 6));
    uint32_t esi;

    ww(0x12815, ax);
    ww(0x12817, dx);
    if ((int16_t)k < 0)
        k = (uint16_t)(k + 0x78);
    if ((int16_t)k >= 0x78)
        k = (uint16_t)(k - 0x78);
    if (n < 0)
        n = -n - 1;
    esi = ((uint32_t)rw(edi + (uint32_t)n * 2 + 0xB0) << 4) + rd(edi + 0x28);
    ax = (uint16_t)(ax - rw(edi + 2) + rb(N_FLIP_SHAPES + (uint32_t)k * 4) + 0x20);
    dx = (uint16_t)(dx - rw(edi + 4) + rb(N_FLIP_SHAPES + (uint32_t)k * 4 + 1) + 0x20);
    ring_sample(ax, dx, esi, 0x10);
}

/* CODE:1237C: 1 (ZF) when a sample of the ring hit; 0 with the ball lost
 * (below 258h: +9 FFh, BALL_PLACE) or no hit */
static int BALL_SAMPLE(void)
{
    uint32_t ebx = rd(0x0010), st = rd(0x0014), edi, esi;
    uint16_t ax = rw(ebx + 0x12), dx = rw(ebx + 0x14);

    if ((int16_t)ax < 0)
        ax = 0;
    if ((int16_t)dx < 0)
        dx = 0;
    ax = (uint16_t)(ax + rw(st + 0x0D4A));
    dx = (uint16_t)(dx + rw(st + 0x0D4C));
    if ((int16_t)dx > 0x258) {
        wb(ebx + 9, 0xFF);
        BALL_PLACE();
        return 0;
    }
    for (edi = rd(st + 0x28FE); rb(edi) != 0; edi += 0x1F7) {     /* CODE:123C3 */
        int16_t n = (int16_t)rw(edi + 0x1A);

        esi = edi + (uint32_t)(int32_t)n * 8 + (n < 0 ? 0x1F2 : 0xF2);
        if ((int16_t)ax < (int16_t)rw(esi) || (int16_t)dx < (int16_t)rw(esi + 2) ||
            (int16_t)ax > (int16_t)rw(esi + 4) || (int16_t)dx > (int16_t)rw(esi + 6))
            continue;
        wb(edi + 0x1F6, 0xFF);
        if (rd(ebx + 0x54) != rd(edi + 0x1C))
            continue;
        FLIP_SAMPLE(edi, ax, dx);
        wd(0x0018, 0x12844);
        return rd(0x12819) != 0;
    }
    /* CODE:1281D */
    MAP_SAMPLE(ax, dx, rd(rd(0x0010) + 0x54));
    wd(0x0018, 0x12844);
    return rd(0x12819) != 0;
}

/* ---- the surface's handlers (SURFACE_HANDLERS) ---- */

/* CODE:122C1: a bumper, +4 = the number less 0Fh (not while tilted) */
static void SURF_BUMPER(void)
{
    if (rb(rd(0x0014) + 0x2A75) != 0)
        return;
    addw(0x002C, (uint16_t)-0x0F);
    wb(rd(0x0010) + 4, rb(0x002C));
}

/* CODE:122EA, 122F8: a slingshot, +6 = 190h or FE70h, +5 = (number -
 * 16h) / 2 + 1 (not while tilted) */
static void SURF_SLING(uint16_t side)
{
    uint32_t edi;

    ww(rd(0x0010) + 6, side);
    if (rb(rd(0x0014) + 0x2A75) != 0)
        return;
    edi = rd(0x002C);
    edi = (edi & 0xFFFF0000u) | (uint16_t)(((uint16_t)(edi - 0x16) >> 1) + 1);
    wd(0x002C, edi);
    wb(rd(0x0010) + 5, (uint8_t)edi);
}

/* CODE:12359, 12336: onto the other level (BALL_BUNDLE2 / 1), the speed
 * 0, the sound record CODE:100B4 */
static void SURF_LEVEL(int bundle)
{
    if (bundle == 2)
        BALL_BUNDLE2();
    else
        BALL_BUNDLE1();
    wd(rd(0x0010) + 0x0E, 0);
    wd(0x0000, 0x100B4);
    SFX_PLAY();
}

/* CODE:11BE9 + k x DBh: flipper kind k (the flipper's +1), its table of
 * eight bytes (CODE:11CBC + k x DBh) by the ball's octant [002C]: the
 * flipper's turn [0028] doubled; turning one way (not negative) and the
 * octant's byte set, or the other way and the byte 0, the ball gets the
 * push [0020], [0024] times the turn (negated for the first) to +1Ch,
 * +1Ah, the first also [0030] + 3BBh (run 2026-09-30: only the returns
 * without a push, kinds 0, 4 and 7) */
static void FLIP_KIND(uint32_t k)
{
    uint32_t tab = 0x11CBC + k * 0xDB, b = rd(0x0010), e;
    uint16_t si = (uint16_t)(rw(0x0028) * 2), di;

    cw(0x0028, si);
    if (!(si & 0x8000)) {
        if (rb(tab + sx16(rw(0x002C))) == 0)
            return;
        addw(0x0030, 0x3BB);
        di = (uint16_t)-si;
        cw(0x0028, di);
        e = sx16(rw(0x0020)) * sx16(di);
        wd(0x0020, e);
        ww(b + 0x1C, (uint16_t)e);
        e = sx16(rw(0x0024)) * sx16(di);
        wd(0x0024, e);
        ww(b + 0x1A, (uint16_t)e);
        return;
    }
    if (rb(tab + sx16(rw(0x002C))) != 0)                /* CODE:11C62 */
        return;
    e = sx16(rw(0x0020)) * sx16(si);
    wd(0x0020, e);
    ww(b + 0x1C, (uint16_t)e);
    di = (uint16_t)-si;
    cw(0x0028, di);
    e = sx16(rw(0x0024)) * sx16(di);
    wd(0x0024, e);
    ww(b + 0x1A, (uint16_t)e);
}

/* CODE:1187F (flipper 1), 11893, 118AD, 118C7: the flipper `f` of
 * header slot 22 hit at the contact point [0020], [0024]: with its turn
 * +10h not 0, the distance's weight from CODE:21981 (by |dx| x 40h +
 * |dy| from the flipper's +2, +4) takes half of it off the turn (toward
 * 0); near the pivot (weight below 2Eh) the weight raised by an eighth of
 * the rest and [0030] from CODE:11AF9 by the flipper's angle; the
 * ball's normal's octant to [002C], [0024] 8, then FLIP_KIND */
static void SURF_FLIPPER(uint32_t f)
{
    uint16_t bp, cx, v, dx;

    wd(0x0000, f);
    wd(0x0004, 0x21981);
    bp = rw(f + 0x10);
    cw(0x0028, bp);
    if (bp == 0)
        return;
    cx = rw(f + 2);
    wd(0x002C, sx16(cx));
    wd(0x0030, sx16(rw(f + 4)));
    v = (uint16_t)(rw(0x0020) - cx);
    cw(0x0020, v);
    if (v & 0x8000)
        cw(0x0020, (uint16_t)-v);
    v = (uint16_t)(rw(0x0024) - (uint16_t)rd(0x0030));
    cw(0x0024, v);
    if (v & 0x8000)
        cw(0x0024, (uint16_t)-v);
    /* CODE:11953 */
    v = rw(0x21981 + sx16((uint16_t)((rw(0x0020) << 6) + rw(0x0024))) * 2);
    cw(0x0020, v);
    cw(0x002C, (uint16_t)(v >> 1));
    dx = rw(0x0028);
    if (dx & 0x8000) {
        uint32_t sum = (uint32_t)dx + rw(0x002C);      /* CODE:119AE */

        cw(0x0028, (uint16_t)sum);
        if (sum > 0xFFFF)
            wd(0x0028, 0);
    } else {
        dx = (uint16_t)(dx - rw(0x002C));
        cw(0x0028, dx);
        if (dx & 0x8000)
            wd(0x0028, 0);
    }
    /* CODE:119C7 */
    ww(f + 0x10, rw(0x0028));
    wd(0x0024, 8);
    cw(0x002C, rw(rd(0x0010) + 0x28));
    dx = rw(0x0020);
    cw(0x0034, (uint16_t)(dx - 0x2E));
    if ((int16_t)dx < 0x2E) {
        uint16_t a = (uint16_t)(0x2E - dx);

        cw(0x0020, (uint16_t)(dx + (a >> 3)));
        a = (uint16_t)(rw(f + 6) + rw(f + 0x1A));
        cw(0x0034, a);
        if ((int16_t)a < 0) {
            a = (uint16_t)(a + 0x78);
            cw(0x0034, a);
        }
        if ((int16_t)a >= 0x78) {
            a = (uint16_t)(a - 0x78);
            cw(0x0034, a);
        }
        cw(0x0030, rw(0x11AF9 + sx16(a) * 2));          /* CODE:11A52 */
    }
    cw(0x002C, (uint16_t)(rw(0x002C) >> 8));
    if (rb(f + 1) > 7)
        pi_stop("SURF_FLIPPER: a kind above 7 (CODE:11A91)");
    wd(0x0034, rw(0x11AE9 + (uint32_t)rb(f + 1) * 2));
    FLIP_KIND(rb(f + 1));
}

static void surface_handler(uint16_t n)
{
    static char why[64];

    if (n == 0 || (n >= 5 && n <= 9) || (n >= 0x0C && n <= 0x0F))
        return;                                         /* CODE:11804 */
    if (n == 0x0A)
        SURF_LEVEL(2);
    else if (n == 0x0B)
        SURF_LEVEL(1);
    else if (n >= 0x10 && n <= 0x15)
        SURF_BUMPER();
    else if (n >= 0x16 && n <= 0x1F)
        SURF_SLING((n & 1) ? 0xFE70 : 0x0190);
    else if (n >= 1 && n <= 4)
        SURF_FLIPPER(rd(rd(0x0014) + 0x28FE) + (uint32_t)(n - 1) * 0x1F7);
    else {
        snprintf(why, sizeof why, "BALL_COLLIDE: flipper surface %u (CODE:1187F)", (unsigned)n);
        pi_stop(why);
    }
}

/* ---- the normal ---- */

/* a hit sample: its angle to [0034], two of the counters [0024] (the
 * third quadrant ...), [0028], [002C], [0030] one up, a side bit into
 * [0038], its number to [003C] */
static void hit(uint16_t angle, uint32_t c1, uint32_t c2, unsigned bit, uint32_t id)
{
    addw(0x0034, angle);
    addw(c1, 1);
    addw(c2, 1);
    wd(0x0038, rd(0x0038) | 1u << bit);
    wd(0x003C, id);
}

/* a line decoded through a 64-entry table at `t` (CODE:10594 for the
 * first line, 10674 for the last); `last` swaps the counters it adds to */
static void row_table(uint32_t t, int last)
{
    uint32_t ebx = rd(0x0020), si;

    wd(0x0004, t);
    ebx = (ebx & 0xFFFF0000u) | (uint16_t)((uint16_t)ebx >> 6);
    wd(0x0020, ebx);
    si = sx16((uint16_t)ebx);
    cw(0x0034, (uint16_t)(rw(0x0034) + rw(t + si * 2)));
    if (!last) {
        cb(0x0030, (uint8_t)(rb(0x0030) + rb(t + si + 0x40)));
        cb(0x003C, rb(t + si + 0x60));
        cb(0x0028, (uint8_t)(rb(0x0028) + rb(t + si + 0x80)));
        cb(0x0038, (uint8_t)(rb(0x0038) | rb(t + si + 0xA0)));
        cb(0x0024, (uint8_t)(rb(0x0024) + rb(t + si + 0xC0)));
    } else {
        cb(0x0028, (uint8_t)(rb(0x0028) + rb(t + si + 0x40)));
        cb(0x003C, rb(t + si + 0x60));
        cb(0x0030, (uint8_t)(rb(0x0030) + rb(t + si + 0x80)));
        cb(0x0038, (uint8_t)(rb(0x0038) | rb(t + si + 0xA0)));
        cb(0x002C, (uint8_t)(rb(0x002C) + rb(t + si + 0xC0)));
    }
}

/* the dword at [0000] into [0020] (big-endian), [0000] 4 on; 0 when it
 * is 0 */
static uint32_t row_dword(void)
{
    uint32_t e = rbe(rd(0x0000));

    wd(0x0020, e);
    wd(0x0000, rd(0x0000) + 4);
    return e;
}

/* bytes `a`, `b` of [0000] + `off` into the low word of [0020] (a high,
 * b low) */
static uint16_t row_word(uint32_t p)
{
    uint32_t e = rd(0x0020);

    e = (e & 0xFFFF0000u) | (uint16_t)(rb(p) << 8 | rb(p + 1));
    wd(0x0020, e);
    return (uint16_t)e;
}

/* byte `off` of [0000] into the low byte of [0020] */
static uint8_t row_byte(uint32_t off)
{
    uint8_t b = rb(rd(0x0000) + off);

    cb(0x0020, b);
    return b;
}

/* a line of two bits: bit 0 (after SHR 1, CF) and the rest (SI not 0) */
static void row_shift(uint16_t a0, uint32_t c0a, uint32_t c0b, unsigned b0, uint32_t id0,
                      uint16_t a1, uint32_t c1a, uint32_t c1b, unsigned b1, uint32_t id1)
{
    uint32_t e;

    if (row_dword() == 0)
        return;
    e = rd(0x0020);
    wd(0x0020, e >> 1);
    if (e & 1)
        hit(a0, c0a, c0b, b0, id0);
    if ((uint16_t)rd(0x0020) != 0)
        hit(a1, c1a, c1b, b1, id1);
}

/* CODE:10754: the ring's 17 lines (BALL_RING) summed: the angles of the
 * hit samples to [0034], the counters, the sides; then the mean angle to
 * the ball's +28h, the count to +0Ch */
static void RING_SUM(void)
{
    uint32_t e, ebx, esi, edx, ecx = rd(0x0010);
    uint16_t bx, ax, dx;

    wd(0x0000, N_BALL_RING);
    wd(0x0024, 0);
    wd(0x0028, 0);
    wd(0x002C, 0);
    wd(0x0030, 0);
    wd(0x0034, 0);
    wd(0x0038, 0);
    wd(0x003C, 0);
    /* line 0 */
    if (row_dword() != 0)
        row_table(0x10594, 0);
    /* line 1 (CODE:1086F) */
    e = row_dword();
    if (e != 0) {
        if (e & 1u << 4)
            hit(0x6A9, 0x24, 0x28, 3, 0x25);
        if (e & 1u << 5)
            hit(0x684, 0x24, 0x28, 3, 0x24);
        if (e & 1u << 11)
            hit(0x57C, 0x30, 0x24, 2, 0x1E);
        if (e & 1u << 12)
            hit(0x557, 0x30, 0x24, 2, 0x1D);
    }
    /* lines 2 and 3, their fourth and third bytes (CODE:109A2) */
    if (row_byte(3))
        hit(0x6E2, 0x24, 0x28, 3, 0x26);
    if (row_byte(2))
        hit(0x51E, 0x30, 0x24, 2, 0x1C);
    if (row_byte(7))
        hit(0x71E, 0x24, 0x28, 3, 0x27);
    if (row_byte(6))
        hit(0x4E2, 0x30, 0x24, 2, 0x1B);
    /* line 4, bytes 2 and 3 */
    wd(0x0000, rd(0x0000) + 8);
    bx = row_word(rd(0x0000) + 2);
    if (bx != 0) {
        if (bx & 0x8000)
            hit(0x4A9, 0x30, 0x24, 2, 0x1A);
        if ((uint8_t)rd(0x0020) != 0)
            hit(0x757, 0x24, 0x28, 3, 0x28);
    }
    /* line 5, bytes 2 and 3 (CODE:10B6F) */
    esi = rd(0x0000) + 6;
    bx = row_word(esi);
    wd(0x0000, esi + 2);
    if (bx != 0) {
        if (bx & 0x8000)
            hit(0x484, 0x30, 0x24, 2, 0x19);
        if ((uint8_t)rd(0x0020) != 0)
            hit(0x77C, 0x24, 0x28, 3, 0x29);
    }
    /* lines 6 to 10 (CODE:10C0E) */
    row_shift(0x7B0, 0x24, 0x28, 3, 0x2A, 0x450, 0x30, 0x24, 2, 0x18);
    row_shift(0x7D7, 0x24, 0x28, 3, 0x2B, 0x429, 0x30, 0x24, 2, 0x17);
    row_shift(0x000, 0x28, 0x2C, 0, 0x00, 0x400, 0x30, 0x24, 2, 0x16);
    row_shift(0x029, 0x28, 0x2C, 0, 0x01, 0x3D7, 0x2C, 0x30, 1, 0x15);
    row_shift(0x050, 0x28, 0x2C, 0, 0x02, 0x3B0, 0x2C, 0x30, 1, 0x14);
    /* line 11, bytes 2 and 3 (CODE:10F76) */
    bx = row_word(rd(0x0000) + 2);
    if (bx != 0) {
        if (bx & 0x8000)
            hit(0x37C, 0x2C, 0x30, 1, 0x13);
        if ((uint8_t)rd(0x0020) != 0)
            hit(0x084, 0x28, 0x2C, 0, 0x03);
    }
    /* line 12, bytes 2 and 3 (CODE:1100A) */
    esi = rd(0x0000) + 6;
    bx = row_word(esi);
    wd(0x0000, esi + 2);
    if (bx != 0) {
        if (bx & 0x8000)
            hit(0x357, 0x2C, 0x30, 1, 0x12);
        if ((uint8_t)rd(0x0020) != 0)
            hit(0x0A9, 0x28, 0x2C, 0, 0x04);
    }
    /* lines 13 and 14, their fourth and third bytes (CODE:110A9) */
    if (row_byte(3))
        hit(0x0E2, 0x28, 0x2C, 0, 0x05);
    if (row_byte(2))
        hit(0x31E, 0x2C, 0x30, 1, 0x11);
    if (row_byte(7))
        hit(0x11E, 0x28, 0x2C, 0, 0x06);
    if (row_byte(6))
        hit(0x2E2, 0x2C, 0x30, 1, 0x10);
    /* line 15 (CODE:111D9) */
    wd(0x0000, rd(0x0000) + 8);
    e = row_dword();
    if (e != 0) {
        if (e & 1u << 4)
            hit(0x157, 0x28, 0x2C, 0, 0x07);
        if (e & 1u << 5)
            hit(0x17C, 0x28, 0x2C, 0, 0x08);
        if (e & 1u << 11)
            hit(0x284, 0x2C, 0x30, 1, 0x0E);
        if (e & 1u << 12)
            hit(0x2A9, 0x2C, 0x30, 1, 0x0F);
    }
    /* line 16 (CODE:1130F): [0000] not moved on */
    e = rbe(rd(0x0000));
    wd(0x0020, e);
    if (e != 0)
        row_table(0x10674, 1);
    /* CODE:113C9: hits across angle 0 (sides 0 and 3): a turn (800h) for
     * each in the fourth quadrant's count */
    ebx = rd(0x0038);
    if ((uint8_t)ebx == 0x0B || (uint8_t)ebx == 9 || (uint8_t)ebx == 0x0D) {
        e = rol(rd(0x002C), 16);
        e = rol(e, 32 - 5);
        wd(0x0020, e);
        wd(0x0034, rd(0x0034) + e);
    }
    /* CODE:113F8 */
    addw(0x0028, (uint16_t)rd(0x0024));
    addw(0x002C, (uint16_t)rd(0x0028));
    ebx = rd(0x0030);
    bx = (uint16_t)((uint16_t)(ebx + rd(0x002C)) >> 1);
    wd(0x0030, (ebx & 0xFFFF0000u) | bx);
    ww(ecx + 0x0C, bx);
    esi = rd(0x0034);
    if (bx == 0 || (esi >> 16) >= bx)
        pi_stop("RING_SUM: DIV overflow (CODE:1143C)");
    ax = (uint16_t)(esi / bx);
    esi = (uint32_t)(esi % bx) << 16 | (uint16_t)(ax & 0x7FF);
    wd(0x0034, esi);
    ww(ecx + 0x28, (uint16_t)esi);
    /* the contact point from CODE:168C9 by the last hit sample */
    wd(0x0000, 0x168C9);
    esi = sx16((uint16_t)rd(0x003C));
    ax = rw(0x168C9 + esi * 4);
    dx = rw(0x168C9 + esi * 4 + 2);
    ax = (uint16_t)(ax + rw(ecx + 0x12));
    dx = (uint16_t)(dx + rw(ecx + 0x14));
    cw(0x0024, dx);
    edx = rd(0x0014);
    ax = (uint16_t)(ax + rw(edx + 0x0D4A));
    cw(0x0020, ax);
    ww(ecx + 0x2E, ax);
    if ((int16_t)ax < 0) {
        dx = (uint16_t)(dx + rw(edx + 0x0D4C));
        cw(0x0024, dx);
        ww(ecx + 0x30, dx);
        wd(0x0028, 0);
        return;
    }
    dx = (uint16_t)(dx + rw(edx + 0x0D4C));
    cw(0x0024, dx);
    ww(rd(0x0010) + 0x30, dx);
    if ((int16_t)dx < 0 || dx >= 0x258) {
        wd(0x0028, 0);
        return;
    }
    /* CODE:11524: the surface's number from the ball's map +50h: a
     * line's four words by x in quarters of 54h, then pairs (x, number)
     * from +12C0h searched for x */
    {
        uint32_t map = rd(rd(0x0010) + 0x50), y, p;
        unsigned q = 0;

        wd(0x0008, map);
        wd(0x0028, 0x54);
        ebx = rd(0x0020);
        while (q < 3 && (uint16_t)ebx >= 0x54) {
            ebx = (ebx & 0xFFFF0000u) | (uint16_t)(ebx - 0x54);
            wd(0x0020, ebx);
            q++;
        }
        y = sx16((uint16_t)rd(0x0024));
        p = map + y * 8 + q * 2;
        if (q == 3)
            wd(0x0028, (uint16_t)(rb(p) << 8 | rb(p + 1)));
        else
            cw(0x0028, (uint16_t)(rb(p) << 8 | rb(p + 1)));
        /* CODE:1161E */
        p = map + rd(0x0028) * 2 + 0x12C0;
        wd(0x0008, p);
        while (rb(p) != (uint8_t)rd(0x0020)) {
            p += 2;
            wd(0x0008, p);
        }
        wd(0x0028, rb(p + 1));
    }
}

/* CODE:10540: 0 after a hit (BALL_BOUNCE follows), -1 without */
static int BALL_COLLIDE(void)
{
    uint32_t b = rd(0x0010), esi, ecx, ebx;
    uint16_t n;

    ww(b + 0x32, 0);
    wb(b + 4, 0);
    wb(b + 5, 0);
    if (!BALL_SAMPLE()) {
        wd(0x0020, 0xFFFFFFFFu);
        return -1;
    }
    if (rd(rd(0x0018) + 2) & 1u << 5) {
        wd(0x0020, 0xFFFFFFFFu);
        return -1;
    }
    RING_SUM();
    /* CODE:11668 */
    b = rd(0x0010);
    ww(b + 0x32, (uint16_t)rd(0x0028));
    esi = rol((uint32_t)(uint16_t)rd(0x0034) * 0x580 + 0x8000, 16);
    wd(0x0034, esi);
    ebx = sx16((uint16_t)esi);
    ecx = rd(0x0000);
    cw(0x0020, (uint16_t)(rw(ecx + ebx * 4) + rw(b + 0x12)));
    cw(0x0024, (uint16_t)(rw(ecx + ebx * 4 + 2) + rw(b + 0x14)));
    ww(b + 0x2A, (uint16_t)rd(0x0020));
    ww(b + 0x2C, (uint16_t)rd(0x0024));
    /* CODE:116E3: the gates at state+2902h (12h bytes: a box, an angle
     * range, the level; the object at +0Eh gets its +10h cleared) */
    wd(0x0000, rd(rd(0x0014) + 0x2902));
    for (;;) {
        uint32_t g = rd(0x0000);

        if (rw(g) & 0x8000)
            break;
        cb(0x003C, rb(b + 8));
        if (rb(b + 8) == rb(g + 0x0C) &&
            (uint16_t)rd(0x0020) >= rw(g) && (uint16_t)rd(0x0024) >= rw(g + 2) &&
            (uint16_t)rd(0x0020) <= rw(g + 4) && (uint16_t)rd(0x0024) <= rw(g + 6)) {
            cw(0x002C, rw(b + 0x28));
            if (rw(b + 0x28) >= rw(g + 8) && rw(b + 0x28) <= rw(g + 0x0A)) {
                g = rd(g + 0x0E);
                wd(0x0000, g);
                ww(g + 0x10, 0);
                break;
            }
        }
        wd(0x0000, g + 0x12);
    }
    /* CODE:11771: the surface's four words (CODE:C33E) to +34h..+3Ah */
    wd(b + 0x1A, 0);
    wd(0x0000, 0xC33E);
    ebx = sx16((uint16_t)rd(0x0028));
    ecx = 0xC33E + ebx * 8;
    wd(0x002C, sx16(rw(ecx)));
    wd(0x0030, sx16(rw(ecx + 2)));
    wd(0x0034, sx16(rw(ecx + 4)));
    wd(0x0038, sx16(rw(ecx + 6)));
    ww(b + 0x34, rw(ecx));
    ww(b + 0x36, rw(ecx + 2));
    ww(b + 0x38, rw(ecx + 4));
    ww(b + 0x3A, rw(ecx + 6));
    n = (uint16_t)rd(0x0028);
    if (n > 0x1F) {
        /* CODE:11812: an object for OBJECT_HITS */
        cw(0x0028, (uint16_t)(n - 0x20));
        ww(rd(0x0010) + 0x6C, (uint16_t)(n - 0x20));
        wd(0x0020, 0);
        return 0;
    }
    cw(0x002C, n);
    cw(0x0028, rw(0x1183F + (uint32_t)n * 2));
    surface_handler(n);
    wd(0x0020, 0);                                      /* CODE:11804 */
    return 0;
}

/* ---- the bounce ---- */

/* the clamp to F001h..0FFFh of the word in cell `c` (CODE:12C9E) */
static void clamp_cell(uint32_t c)
{
    int16_t v = (int16_t)rw(c);

    if (v <= (int16_t)0xF001)
        cw(c, 0xF001);
    else if (v >= 0x0FFF)
        cw(c, 0x0FFF);
}

/* CODE:12F26: a bumper's kick, [002C] its number n: the speed along
 * the normal ([0020]) less 157Ch; the bumper's record (state+28D6h, word
 * offsets from it, n - 1 the index), unless its byte +1 is running, +1
 * 6, its points (+16h, SCORE_ADD), its event stream (+6, EVENT_QUEUE)
 * and sound record (+2, SFX_PLAY); [0028], [0000], [0004], [000C] and
 * [0010] kept */
static void BUMPER_KICK(void)
{
    uint32_t k10 = rd(0x0010), k0c = rd(0x000C), k04 = rd(0x0004), k00 = rd(0x0000),
             k28 = rd(0x0028), t, r, e;

    wd(0x0020, (rd(0x0020) & 0xFFFF0000u) | (uint16_t)(rd(0x0020) - 0x157C));
    t = rd(rd(0x0014) + 0x28D6);
    r = t + sx16(rw(t + sx16((uint16_t)rd(0x002C)) * 2 - 2));
    wd(0x0004, r);
    if (rb(r + 1) == 0) {
        wb(r + 1, 6);
        wd(0x000C, r + 0x16);
        SCORE_ADD();
        r = rd(0x0004);
        e = rd(r + 6);
        wd(0x002C, e);
        if (e != 0) {
            wd(0x0000, e);
            wd(0x002C, r);
            EVENT_QUEUE();
            wd(0x0004, rd(0x002C));
        }
        r = rd(0x0004);
        e = rd(r + 2);
        wd(0x002C, e);
        if (e != 0) {
            wd(0x0000, e);
            SFX_PLAY();
        }
    }
    wd(0x0028, k28);
    wd(0x0000, k00);
    wd(0x0004, k04);
    wd(0x000C, k0c);
    wd(0x0010, k10);
}

/* CODE:1302B: a slingshot's kick, [002C] its number n: the speed along
 * the normal less DACh, the ball's +6 added to [0028]'s low word; the
 * slingshot's record (state+28DAh, as the bumpers') byte +0 FFh (its
 * picture, SLINGS_STEP), unless its +1 is running, +1 6, its points and
 * sound record; the same cells kept, [0028] as changed here */
static void SLING_KICK(void)
{
    uint32_t b = rd(0x0010), k0c = rd(0x000C), k04 = rd(0x0004), k00 = rd(0x0000),
             k28, t, r, e;

    wd(0x0020, (rd(0x0020) & 0xFFFF0000u) | (uint16_t)(rd(0x0020) - 0x0DAC));
    k28 = (rd(0x0028) & 0xFFFF0000u) | (uint16_t)(rd(0x0028) + rw(b + 6));
    wd(0x0028, k28);
    t = rd(rd(0x0014) + 0x28DA);
    r = t + sx16(rw(t + sx16((uint16_t)rd(0x002C)) * 2 - 2));
    wd(0x0000, r);
    wb(r, 0xFF);
    if (rb(r + 1) == 0) {
        wb(r + 1, 6);
        wd(0x000C, r + 0x16);
        SCORE_ADD();
        r = rd(0x0000);
        e = rd(r + 2);
        wd(0x002C, e);
        if (e != 0) {
            wd(0x0000, e);
            SFX_PLAY();
        }
    }
    wd(0x0028, k28);
    wd(0x0000, k00);
    wd(0x0004, k04);
    wd(0x000C, k0c);
    wd(0x0010, b);
}

/* CODE:12C20 */
static void BALL_BOUNCE(void)
{
    uint32_t b = rd(0x0010), st = rd(0x0014), ecx, edx, ebp, ebx, esi, edi, cosp, sinp;
    uint16_t cx, q = 0, r = 0;

    cosp = N_COSINES;
    sinp = N_SINES;
    wd(0x0000, cosp);
    wd(0x0004, sinp);
    ecx = sx16(rw(b + 0x0E));
    edx = sx16(rw(b + 0x10));
    esi = sx16(rw(st + 0x0D42));
    wd(0x0028, esi);
    edi = sx16(rw(st + 0x0D44));
    wd(0x002C, edi);
    wd(0x0020, (ecx & 0xFFFF0000u) | (uint16_t)(ecx + esi));
    wd(0x0024, (edx & 0xFFFF0000u) | (uint16_t)(edx + edi));
    ww(0x0030, 0xF001);
    ww(0x0034, 0x0FFF);
    clamp_cell(0x0020);
    clamp_cell(0x0024);
    /* CODE:12CEE: turned by 800h - the normal */
    ww(0x0028, (uint16_t)rd(0x0020));
    ww(0x002C, (uint16_t)rd(0x0024));
    cx = (uint16_t)((0x800 - rw(b + 0x28)) & 0x7FF);
    cw(0x0030, cx);
    esi = sx16((uint16_t)rd(0x0020)) * sx16(rw(cosp + cx * 2u));
    wd(0x0020, esi);
    ebx = sx16((uint16_t)rd(0x0024)) * sx16(rw(sinp + cx * 2u));
    wd(0x0024, ebx);
    ebp = sx16((uint16_t)rd(0x0028)) * sx16(rw(sinp + cx * 2u));
    wd(0x0028, ebp);
    esi = sx16((uint16_t)rd(0x002C)) * sx16(rw(cosp + cx * 2u));
    wd(0x002C, esi);
    edx = (uint32_t)-(int32_t)rd(0x0024);
    ebp = rd(0x0020) + edx;
    ecx = rd(0x0028) + esi;
    ebp = rol(ebp << 3, 16);
    wd(0x0020, ebp);
    ecx = rol(ecx << 3, 16);
    wd(0x0028, ecx);
    /* the push +1Ah, +1Ch */
    edx = (edx & 0xFFFF0000u) | rw(b + 0x1C);
    wd(0x0024, edx);
    if ((uint16_t)edx != 0) {
        uint16_t d = (uint16_t)((uint16_t)edx - (uint16_t)ebp);

        if ((int16_t)d < 0)
            d = (uint16_t)-d;
        edx = (edx & 0xFFFF0000u) | d;
        wd(0x0024, edx);
        if (d <= 0x0FA0) {
            cw(0x0028, (uint16_t)(rd(0x0028) + rw(b + 0x1A)));
            cw(0x0020, (uint16_t)(rd(0x0020) + rw(b + 0x1C)));
        } else {
            cw(0x0020, (uint16_t)(rd(0x0020) + rw(b + 0x1C)));
        }
    }
    /* CODE:12E3D */
    wd(b + 0x1A, 0);
    edi = rd(0x0020);
    if ((int16_t)edi > 0) {
        uint16_t di = (uint16_t)-(int16_t)(uint16_t)edi, dxv, lim;

        edi = (edi & 0xFFFF0000u) | di;
        wd(0x0020, edi);
        ebx = (rd(0x0024) & 0xFFFF0000u) | (uint16_t)rd(0x0028);
        ebx = sx16((uint16_t)ebx) << 4;
        dxv = (uint16_t)(ebx >> 16);
        lim = (int16_t)dxv > 0 ? (uint16_t)-dxv : dxv;
        if ((int16_t)(uint16_t)(((int16_t)di >> 1) + 1) >= (int16_t)lim) {
            wd(0x0020, 0);                              /* CODE:12EC7 */
            goto l130f9;
        }
        if (idiv16(ebx, ebx >> 16, di, &q, &r))
            pi_stop("BALL_BOUNCE: IDIV fault (CODE:12E97)");
        edx = (uint32_t)r << 16 | q;
        if ((int16_t)q < 0)
            edx = (edx & 0xFFFF0000u) | (uint16_t)-q;
        wd(0x0024, edx);
        if ((int16_t)(uint16_t)edx >= (int16_t)rw(b + 0x34)) {
            wd(0x0020, 0);
            goto l130f9;
        }
        if ((int16_t)(uint16_t)rd(0x0020) >= (int16_t)rw(b + 0x38)) {
            wd(0x0020, 0);
            goto l130f9;
        }
        /* CODE:12EF8: a bumper, then a slingshot */
        wd(0x002C, rb(b + 4));
        if (rb(b + 4) != 0 && (int16_t)(uint16_t)rd(0x0020) <= (int16_t)0xFFCE) {
            BUMPER_KICK();
            goto l130d9;
        }
        wd(0x002C, rb(b + 5));
        if (rb(b + 5) != 0 && (int16_t)(uint16_t)rd(0x0020) <= (int16_t)0xFF9C)
            SLING_KICK();
l130d9:
        /* CODE:130D9: the speed along the normal times +36h / 100h */
        esi = (uint32_t)((int32_t)sx16((uint16_t)rd(0x0020)) * (int16_t)rw(b + 0x36) >> 8);
        wd(0x0020, esi);
l130f9:
        /* friction across the normal (+3Ah) with the spin +26h */
        cw(0x0034, rw(b + 0x3A));
        edi = (rd(0x002C) & 0xFFFF0000u) | (uint16_t)(rw(b + 0x26) - (uint16_t)rd(0x0028));
        edi = sx16((uint16_t)edi) << 8;
        if (idiv16(edi, edi >> 16, (uint16_t)rd(0x0034), &q, &r))
            pi_stop("BALL_BOUNCE: IDIV fault (CODE:1313B)");
        ww(0x0030, q);
        {
            uint16_t dx16 = q, di = (uint16_t)(q << 2), bp;

            di = (uint16_t)(di + dx16);
            di = (uint16_t)((int16_t)di >> 3);
            bp = (uint16_t)((uint16_t)rd(0x0028) + di);
            cw(0x0028, bp);
            ww(b + 0x26, (uint16_t)(rw(b + 0x26) - dx16));
            /* MOV DI,BP: EDI's high word the remainder (MOV EDI,EDX) */
            wd(0x002C, (uint32_t)r << 16 | bp);
            if (bp != 0) {
                if ((int16_t)bp < 0) {
                    di = (uint16_t)-bp;
                    di = (uint16_t)(((uint16_t)(di << 4 | di >> 12)) & 0x0F);
                    di = (uint16_t)(di + 1);
                    cw(0x002C, di);
                    cw(0x0028, (uint16_t)(bp + di));
                } else {
                    uint16_t si = (uint16_t)rd(0x002C);

                    si = (uint16_t)(((uint16_t)(si << 4 | si >> 12)) & 0x0F);
                    si = (uint16_t)(si + 1);
                    cw(0x002C, si);
                    addw(0x0028, (uint16_t)-si);
                }
            }
        }
    }
    /* CODE:131BA: turned back */
    {
        uint16_t a = rw(b + 0x28), si = (uint16_t)rd(0x0020);
        uint32_t c, d, e;

        ww(0x0024, si);
        ww(0x002C, (uint16_t)rd(0x0028));
        cw(0x0030, a);
        e = sx16(si) * sx16(rw(cosp + a * 2u));
        wd(0x0020, e);
        e = sx16((uint16_t)rd(0x0028)) * sx16(rw(sinp + a * 2u));
        wd(0x0028, e);
        e = sx16((uint16_t)rd(0x0024)) * sx16(rw(sinp + a * 2u));
        wd(0x0024, e);
        e = sx16((uint16_t)rd(0x002C)) * sx16(rw(cosp + a * 2u));
        wd(0x002C, e);
        c = rol((uint32_t)-(int32_t)rd(0x0028) + rd(0x0020), 17);
        d = rol(rd(0x0024) + e, 17);
        c = (c & 0xFFFF0000u) | (uint16_t)((uint16_t)c - rw(st + 0x0D42));
        wd(0x0028, c);
        d = (d & 0xFFFF0000u) | (uint16_t)((uint16_t)d - rw(st + 0x0D44));
        wd(0x0024, d);
        /* MOV AX / MOV BP: the high words of the nudge's sign-extended
         * words */
        wd(0x0034, (sx16(rw(st + 0x0D42)) & 0xFFFF0000u) | 0xF001);
        wd(0x0038, (sx16(rw(st + 0x0D44)) & 0xFFFF0000u) | 0x0FFF);
        clamp_cell(0x0028);
        clamp_cell(0x0024);
        ww(b + 0x0E, (uint16_t)rd(0x0028));
        ww(b + 0x10, (uint16_t)rd(0x0024));
    }
    /* CODE:13316: deep in (+0Ch 6 or more): put out by 200h along the
     * normal */
    if ((int16_t)rw(b + 0x0C) >= 6) {
        uint16_t a = rw(b + 0x28);
        int32_t px = (int32_t)0xFFFFFE00 * (int16_t)rw(cosp + a * 2u);
        int32_t py = (int32_t)0xFFFFFE00 * (int16_t)rw(sinp + a * 2u);

        cw(0x0030, a);
        wd(0x0028, 0x0E);
        wd(0x0020, (uint32_t)(px >> 14));
        wd(b + 0x1E, rd(b + 0x1E) + (uint32_t)(px >> 14));
        wd(0x0024, (uint32_t)(py >> 14));
        wd(b + 0x22, rd(b + 0x22) + (uint32_t)(py >> 14));
    }
}

/* ---- the move ---- */

/* CODE:133BF: each ball in play moved by its speed (x 32h / FRAME_RATE
 * / 2, in 1/400h pixels), the slope under it and SLOPE_X, SLOPE_Y into
 * its speed, its spin a step toward 0 */
static void BALLS_MOVE(void)
{
    uint32_t st = rd(0x0014), list, b, ebx, ecx, esi;
    uint16_t n = rw(st + 0x0D32), bx, si;

    ww(st + 0x0D34, n);
    if (n == 0)
        return;
    wd(0x0000, st + 0x1046);
    do {
        list = rd(0x0000);
        b = rd(list);
        wd(0x0010, b);
        wd(0x0000, list + 4);
        if (rb(b + 9) == 0 && !(rb(b + 1) & 0x80)) {
            int32_t fr = rw(N_FRAME_RATE);
            uint32_t sp;

            ebx = (uint32_t)((int32_t)sx16(rw(b + 0x0E)) * 0x32 / fr);
            ecx = (uint32_t)((int32_t)sx16(rw(b + 0x10)) * 0x32 / fr);
            ebx = (ebx & 0xFFFF0000u) | (uint16_t)((int16_t)ebx >> 1);
            ecx = (ecx & 0xFFFF0000u) | (uint16_t)((int16_t)ecx >> 1);
            ebx += rd(b + 0x1E);
            ecx += rd(b + 0x22);
            wd(0x0024, ecx);
            wd(b + 0x1E, ebx);
            wd(b + 0x22, ecx);
            ebx = (uint32_t)((int32_t)ebx >> 10);
            esi = (uint32_t)((int32_t)ecx >> 10);
            ww(b + 0x12, (uint16_t)ebx);
            ww(b + 0x14, (uint16_t)esi);
            bx = (uint16_t)((int16_t)(uint16_t)(ebx + 8) >> 3);
            si = (uint16_t)((int16_t)(uint16_t)(esi + 8) >> 3);
            wd(0x0028, 3);
            si = (uint16_t)(rw(st + sx16(si) * 2 + 0x21E6) + bx);
            wd(0x0024, (esi & 0xFFFF0000u) | si);
            wd(0x0004, rd(b + 0x5C));
            ebx = rb(rd(b + 0x5C) + sx16(si));
            wd(0x0008, rd(st + 0x28EE));
            sp = rd(st + 0x28EE) + ebx * 4;
            ebx = sx16(rw(sp));
            esi = sx16(rw(sp + 2));
            bx = (uint16_t)(ebx + rw(st + 0x0E40));
            wd(0x0020, (ebx & 0xFFFF0000u) | bx);
            si = (uint16_t)(esi + rw(st + 0x0E3A));
            wd(0x0024, (esi & 0xFFFF0000u) | si);
            ww(b + 0x3C, bx);
            ww(b + 0x3E, si);
            if (rb(b + 3) == 0) {
                ww(b + 0x0E, (uint16_t)(rw(b + 0x0E) + bx));
                ww(b + 0x10, (uint16_t)(rw(b + 0x10) + si));
            }
            /* CODE:1352D: the spin */
            {
                uint16_t s = rw(b + 0x26);

                cw(0x0020, s);
                if ((int16_t)s > 0)
                    ww(b + 0x26, (uint16_t)(s - 1));
                else if ((int16_t)s < 0)
                    ww(b + 0x26, (uint16_t)((int16_t)(s + 1) < 0 ? s + 1 : 0));
            }
            /* CODE:1356C: against the later balls */
            wd(0x0008, rd(0x0000));
            ww(st + 0x0D40, (uint16_t)(rw(st + 0x0D34) - 1));
            if (rw(st + 0x0D40) != 0)
                pi_stop("BALLS_MOVE: two balls (CODE:13597)");
        }
        /* CODE:14477 */
        n = (uint16_t)(rw(st + 0x0D34) - 1);
        ww(st + 0x0D34, n);
    } while (n != 0);
}

/* CODE:1023E: four passes of each ball in play (BALL_COLLIDE, and
 * BALL_BOUNCE after a hit), each followed by FLIPPERS_MOVE and
 * BALLS_MOVE twice; one pass while state+0FC5h is set; the flippers'
 * sounds at the end */
void BALLS_PHYSICS(void)
{
    uint32_t st = rd(0x0014), list, b;
    uint16_t n;
    int pass;

    wd(0x000C, st + 0x21E6);
    for (pass = 0; pass < 4; pass++) {
        st = rd(0x0014);
        n = rw(st + 0x0D32);
        ww(st + 0x0D34, n);
        if (n == 0) {
            for (; pass < 4; pass++)
                FLIPPERS_MOVE();
            FLIPPER_SOUNDS();
            return;
        }
        wd(0x0000, st + 0x1046);
        do {
            list = rd(0x0000);
            b = rd(list);
            wd(0x0010, b);
            wd(0x0000, list + 4);
            if (rb(b + 9) == 0 && !(rb(b + 1) & 0x80)) {
                wb(b + 3, 0);
                if (pass == 0)
                    ww(b + 0x6C, 0xFFFF);
                if (BALL_COLLIDE() >= 0)
                    BALL_BOUNCE();
            }
            wd(0x0000, list + 4);
            st = rd(0x0014);
            n = (uint16_t)(rw(st + 0x0D34) - 1);
            ww(st + 0x0D34, n);
        } while (n != 0);
        FLIPPERS_MOVE();
        BALLS_MOVE();
        BALLS_MOVE();
        if (pass == 0 && rb(rd(0x0014) + 0x0FC5) != 0)
            break;
    }
    FLIPPER_SOUNDS();
}
