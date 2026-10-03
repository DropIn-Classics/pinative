/* play.c - the game (CODE:B928): its start and main loop, as far as they
 * are translated.
 */
#include <stdio.h>
#include "frame.h"
#include "game.h"
#include "names.h"
#include "nosound.h"
#include "pmax.h"
#include "pmem.h"
#include "vga.h"

static uint32_t sx16(uint16_t v)
{
    return (uint32_t)(int32_t)(int16_t)v;
}

/* the low word of the cell c */
static void cw(uint32_t c, uint16_t w)
{
    wd(c, (rd(c) & 0xFFFF0000u) | w);
}

/* CODE:2A30E: the display queue's state (state+2A26h .. 2A5Fh) cleared,
 * its pointer state+2A2Ah to DISPLAY_QBUF, whose first dword 0 */
static void DISPLAY_RESET(void)
{
    uint32_t st = rd(0x0014);

    wd(st + 0x2A2E, 0);
    wb(st + 0x2A3A, 0);
    wb(st + 0x2A3B, 0);
    ww(st + 0x2A3E, 0);
    wb(st + 0x2A3C, 0);
    ww(st + 0x2A40, 0);
    ww(st + 0x2A4E, 0);
    wd(st + 0x2A50, 0);
    wd(st + 0x2A58, 0);
    wd(st + 0x2A5C, 0);
    wd(st + 0x2A2A, N_DISPLAY_QBUF);
    wd(rd(st + 0x2A2A), 0);
    ww(st + 0x2A26, 0);
    ww(st + 0x2A28, 0);
    DM_CLEAR();
}

/* CODE:9311: the CRTC's start address at the line SCREEN_LINE (84 bytes
 * a line) from 1500h */
static void SCREEN_START(void)
{
    uint16_t bx = (uint16_t)(rw(N_SCREEN_LINE) * 0x54 + 0x1500);

    vga_outw(0x3D4, (uint16_t)(0x0C | (bx & 0xFF00)));
    vga_outw(0x3D4, (uint16_t)(0x0D | bx << 8));
}

/* CODE:29813: the next CRT start (CRT_NEXT) from the line in [0020] */
static void CRT_NEXT_SET(void)
{
    ww(N_CRT_NEXT, (uint16_t)(rw(0x0020) * 0x54 + 0x1500));
}

/* CODE:30114: the attract mode's scroll a line on (ATTRACT_DIR 0: down,
 * to SCROLL_MAX, then turning; FFh: up, to 0, then turning) into
 * ATTRACT_LINE and SCROLL_LINE, then the CRT start of that line plus
 * SCROLL_ADD (and [0024]'s low word SCROLL_X) */
static void ATTRACT_SCROLL(void)
{
    uint32_t st = rd(0x0014), v;

    v = (rd(0x0020) & 0xFFFF0000u) | rw(st + 0x9A);
    wd(0x0020, v);
    if (rb(st + 0x98) != 0) {
        v = (v & 0xFFFF0000u) | (uint16_t)(v - 1);
        wd(0x0020, v);
        if ((int16_t)v < 0) {
            wd(0x0020, 0);
            wb(st + 0x98, 0);
        }
        v = rd(0x0020);
        if ((uint16_t)v >= rw(st + 0x0D54)) {
            v = (v & 0xFFFF0000u) | rw(st + 0x0D54);
            wd(0x0020, v);
        }
    } else {
        v = (v & 0xFFFF0000u) | (uint16_t)(v + 1);
        wd(0x0020, v);
        if ((uint16_t)v >= rw(st + 0x0D54)) {
            v = (v & 0xFFFF0000u) | rw(st + 0x0D54);
            wd(0x0020, v);
            wb(st + 0x98, 0xFF);
        }
    }
    v = rd(0x0020);
    ww(st + 0x9A, (uint16_t)v);
    ww(st + 0x0D58, (uint16_t)v);
    /* CODE:302C7 */
    wd(0x0020, (v & 0xFFFF0000u) | (uint16_t)(v + rw(st + 0x0D4C)));
    wd(0x0024, (rd(0x0024) & 0xFFFF0000u) | rw(st + 0x0D4A));
    CRT_NEXT_SET();
}

/* CODE:27D0A: the dot-matrix display into video memory from TBL_B922:
 * 16 rows of A0h dots, each dot the animation area's ORed with the text
 * area's; the even dots into plane 0, the odd ones into plane 2, a row
 * every second line (A8h bytes) */
static void DM_SHOW(void)
{
    uint32_t anim = pmax_base(rw(N_DM_ANIM)), text = pmax_base(rw(N_DM_TEXT));
    uint32_t di, si, r, k;
    int plane;

    for (plane = 0; plane < 2; plane++) {
        vga_outw(0x3C4, (uint16_t)(2 | (plane ? 4 : 1) << 8));
        di = rd(N_TBL_B922);
        si = (uint32_t)plane;
        for (r = 0; r < 16; r++, di += 0x58, si += 0xA0)
            for (k = 0; k < 0x50; k++, di++)
                vga_write((uint16_t)di, (uint8_t)(lrb(anim + si + 2 * k) | lrb(text + si + 2 * k)));
    }
}

/* CODE:A704: TBL_FADE_PAL mixed from the palette at `src` and the one at
 * `dst`: (src x (20h - cl) + dst x cl) / 20h, a byte at a time */
void FADE_MIX(uint32_t src, uint32_t dst, uint8_t cl)
{
    uint32_t b;

    for (b = N_TBL_FADE_PAL; b < N_TBL_PALETTE; b++)
        wb(b, (uint8_t)((rb(src++) * (0x20 - cl) + rb(dst++) * cl) / 0x20));
}

/* CODE:B02B: TBL_FADE_PAL to the DAC from colour 0, shifted right 2 */
void FADE_PAL_SET(void)
{
    uint32_t i;

    vga_outb(0x3C8, 0);
    for (i = 0; i < 0x300; i++)
        vga_outb(0x3C9, (uint8_t)(rb(N_TBL_FADE_PAL + i) >> 2));
}

/* CODE:A654: the dot-matrix display shown, TOP_COLOURS into the stage's
 * palette (MODULE_HEADER+50h) at colour FCh, then 32 pictures from
 * TBL_PALETTE toward the stage's palette (the last one 31/32 of the way) */
static void TABLE_FADE_IN(void)
{
    uint32_t pal = rd(N_MODULE_HEADER + 0x50), i;
    uint8_t cl;

    DM_SHOW();
    for (i = 0; i < 12; i++)
        wb(pal + 3 * 0xFC + i, rb(N_TOP_COLOURS + i));
    for (cl = 0x20; cl != 0; cl--) {
        FADE_MIX(rd(N_MODULE_HEADER + 0x50), N_TBL_PALETTE, cl);
        frame_wait();                               /* CODE:3D372 */
        FADE_PAL_SET();
    }
}

/* one of the frame step's routines by its address in the table at
 * [FRAME_ROUTINES] */
static void frame_routine(uint32_t a)
{
    static char name[16];

    switch (a) {
    case N_FRAME_MUSIC:
        FRAME_MUSIC();
        break;
    case N_DM_SHOW:
        DM_SHOW();
        break;
    case N_LIGHTS_DRAW:
        LIGHTS_DRAW();
        break;
    case N_DROPS_UPDATE:
        DROPS_UPDATE();
        break;
    default:
        snprintf(name, sizeof name, "CODE:%X", (unsigned)a);
        pi_stop(name);
    }
}

/* CODE:29889: FRAME_DONE 0; the four routines of the table at
 * [FRAME_ROUTINES] while the retrace has not come (FRAME_DONE FFh, set by
 * DRV_FRAME); then the retrace waited.  The port has no interrupts: the
 * retrace comes only in the wait, so the four always run (as in the runs,
 * docs/HANDOFF.md, "The attract mode's frame") */
static unsigned long table_frames;      /* the port's count, for its stop */

static void FRAME_STEP(void)
{
    uint32_t ebx;
    static char why[96];

    wb(N_FRAME_DONE, 0);
    for (ebx = 0; ebx < 4; ebx++) {
        if (rb(N_FRAME_DONE) == 0xFF)
            break;
        frame_routine(rd(rd(N_FRAME_ROUTINES) + ebx * 4));
    }
    if (ebx == 4) {
        ww(0x2982A, 0);
        while (rb(N_FRAME_DONE) != 0xFF)
            if (!frame_wait()) {
                snprintf(why, sizeof why, "FRAME_STEP (the window closed; %lu table frames)",
                         table_frames);
                pi_stop(why);
            }
    }
    table_frames++;
    /* CODE:298C5 */
    BALLS_STEP();
    {
        uint8_t al = rb(N_FRAME_DONE);

        wb(N_FRAME_DONE, 0);
        DROPS_DRAW();
        wb(N_FRAME_DONE, al);
    }
    BALLS_SHOW();
    FLIPPERS_DRAW();
    /* the next retrace (DRV_TICK): in the runs 1.3 ms after DRV_FRAME,
     * while FLIPPERS_DRAW runs in the first frame; in a frame with less
     * work after the wait only in the next FRAME_STEP, so FRAME_COUNT
     * is one behind the port's there (docs/HANDOFF.md, "The attract
     * mode's display") */
    ns_retrace();
}

/* ---- a game's start (GAME_PHASE 2) ---- */

/* CODE:29AB3: each of the 13 balls (state+10AEh, 76h bytes) put at the
 * plunger, its bytes +1, +9 and +0Bh 0 */
static void BALLS_RESET(void)
{
    uint32_t b;
    uint16_t n;

    wd(0x0010, rd(0x0014) + 0x10AE);
    ww(0x003C, 0x0C);
    do {
        BALL_PLACE();
        b = rd(0x0010);
        wb(b + 1, 0);
        wb(b + 9, 0);
        wb(b + 0x0B, 0);
        wd(0x0010, b + 0x76);
        n = rw(0x003C);
        ww(0x003C, (uint16_t)(n - 1));
    } while (n != 0);
}

/* CODE:29E45: the event ring (state+2A1Eh to CODE:F618, its first dword
 * 0, state+2A1Ah, 2A1Ch, 2A22h), the mode stream's cells (state+0D4Fh ..
 * 0D6Bh) and the display's background stream and animation (state+2A58h,
 * 2A5Ch) cleared */
static void STREAMS_RESET(void)
{
    uint32_t st = rd(0x0014);

    wd(st + 0x2A22, 0);
    wd(st + 0x2A1E, 0xF618);
    wd(rd(st + 0x2A1E), 0);
    ww(st + 0x2A1A, 0);
    ww(st + 0x2A1C, 0);
    wd(st + 0x0D5E, 0);
    ww(st + 0x0D62, 0);
    ww(st + 0x0D64, 0);
    wd(st + 0x0D66, 0);
    ww(st + 0x0D6A, 0);
    wb(st + 0x0D4F, 0);
    wb(st + 0x0D50, 0);
    wb(st + 0x0D51, 0);
    wd(st + 0x2A5C, 0);
    wd(st + 0x2A58, 0);
}

/* CODE:29ED6: header slot 15's records (state+28E2h, a list ended by 0):
 * bytes +1, +2 0; a record with bit 1 of +0 lit (+1 FFh, +2Eh FFFFh, its
 * lamp +4, if any, on for the player's bit [003C]) and put on the lit
 * list (bit 2 of +0, state+2A32h the first, 2A36h the last, +30h the
 * next); without, bit 2 cleared and +30h 0; then STREAMS_RESET */
static void RECORDS_RESET(void)
{
    uint32_t st = rd(0x0014), p, e, x, t;

    wd(st + 0x2A32, 0);
    wd(st + 0x2A36, 0);
    wd(0x0000, rd(st + 0x28E2));
    wd(0x0038, sx16(rw(st + 0x0D72)));
    wd(0x003C, sx16(rw(st + 0x0D74)));
    for (;;) {
        p = rd(0x0000);
        e = rd(p);
        wd(0x0020, e);
        wd(0x0000, p + 4);
        if (e == 0)
            break;
        wd(0x0004, e);
        wb(e + 2, 0);
        wb(e + 1, 0);
        if (!(rb(e) & 2)) {
            /* CODE:29FE4 */
            wb(e, (uint8_t)(rb(e) & ~4));
            wd(e + 0x30, 0);
            continue;
        }
        wb(e + 1, 0xFF);
        ww(e + 0x2E, 0xFFFF);
        x = rd(e + 4);
        wd(0x0020, x);
        if (x != 0) {
            wd(0x0008, x);
            wb(x, (uint8_t)(rb(x) | (uint8_t)rd(0x003C)));
            wb(x + 2, (uint8_t)(rb(x + 2) | 2));
            wb(x + 1, 0xFF);
            wb(x + 3, 0);
            wb(x + 4, 8);
        }
        /* CODE:29F91 */
        wb(e, (uint8_t)(rb(e) | 4));
        st = rd(0x0014);
        t = rd(st + 0x2A36);
        wd(0x0020, t);
        if (t != 0) {
            wd(0x000C, t);
            wd(t + 0x30, e);
        } else {
            wd(st + 0x2A32, e);
        }
        wd(st + 0x2A36, e);
    }
    STREAMS_RESET();
}

/* CODE:2A0C2 (and CODE:2A2BD alike): header slot 17's lists
 * (state+28EAh) of 8-byte entries, to one whose word +2 is 100h or
 * more: bit 1 of +0 cleared */
static void SLOT17_CLEAR(void)
{
    uint32_t p, e, q;

    wd(0x0000, rd(rd(0x0014) + 0x28EA));
    for (;;) {
        p = rd(0x0000);
        e = rd(p);
        wd(0x0020, e);
        wd(0x0000, p + 4);
        if (e == 0)
            break;
        wd(0x0004, e);
        for (;;) {
            q = rd(0x0004);
            wb(q, (uint8_t)(rb(q) & ~2));
            if (rw(q + 2) >= 0x100)
                break;
            wd(0x0004, q + 8);
        }
    }
}

/* CODE:29FFA: header slot 16's counters (state+28E6h): +30h, +34h from
 * +28h, +2Ch, the eight players' words +6 and +16h to the word +2, +38h,
 * +3Ch and +26h 0, byte +2 of each threshold (12 bytes from +50h, to a
 * negative word) 0; then SLOT17_CLEAR */
static void COUNTERS_RESET(void)
{
    uint32_t st = rd(0x0014), p, e, q;
    uint16_t si;

    wd(0x0000, rd(st + 0x28E6));
    for (;;) {
        p = rd(0x0000);
        e = rd(p);
        wd(0x0020, e);
        wd(0x0000, p + 4);
        if (e == 0)
            break;
        wd(0x0004, e);
        wd(e + 0x30, rd(e + 0x28));
        wd(e + 0x34, rd(e + 0x2C));
        wd(0x0020, (e & 0xFFFF0000u) | rw(e + 2));
        wd(0x0024, 7);
        for (;;) {
            si = rw(0x0024);
            ww(e + sx16(si) * 2 + 6, rw(0x0020));
            ww(e + sx16(si) * 2 + 0x16, rw(0x0020));
            ww(0x0024, (uint16_t)(si - 1));
            if (si == 0)
                break;
        }
        wd(e + 0x38, 0);
        wd(e + 0x3C, 0);
        ww(e + 0x26, 0);
        for (q = e + 0x50; !(rw(q) & 0x8000); q += 0x0C) {
            wd(0x0008, q);
            wb(q + 2, 0);
        }
        wd(0x0008, q);
    }
    SLOT17_CLEAR();
}

/* ---- the next ball (GAME_PHASE 5) ---- */

/* CODE:29CE3: header slot 15's records at the next ball: the lit list
 * emptied; each record's bit for the player ([0038] AND 7) cleared in +2
 * unless bit 5 of +0, in +1 unless bit 0; +1 FFh with bit 1; bit 2 and
 * +30h cleared; one lit for the player (+2Eh FFFFh, its lamp +4, if any,
 * on for the player's bit [003C]) back on the lit list; then
 * STREAMS_RESET */
static void RECORDS_BALL_RESET(void)
{
    uint32_t st = rd(0x0014), p, e, x, t;
    unsigned bit;

    wd(st + 0x2A32, 0);
    wd(st + 0x2A36, 0);
    wd(0x0038, sx16(rw(st + 0x0D72)));
    wd(0x003C, sx16(rw(st + 0x0D74)));
    wd(0x0000, rd(st + 0x28E2));
    for (;;) {
        p = rd(0x0000);
        e = rd(p);
        wd(0x0020, e);
        wd(0x0000, p + 4);
        if (e == 0)
            break;
        wd(0x0004, e);
        bit = rd(0x0038) & 7;
        if (!(rb(e) & 0x20))
            wb(e + 2, (uint8_t)(rb(e + 2) & ~(1u << bit)));
        /* CODE:29D63 */
        if (!(rb(e) & 1))
            wb(e + 1, (uint8_t)(rb(e + 1) & ~(1u << bit)));
        /* CODE:29D80 */
        if (rb(e) & 2)
            wb(e + 1, 0xFF);
        /* CODE:29D92 */
        wb(e, (uint8_t)(rb(e) & ~4));
        wd(e + 0x30, 0);
        if (!(rb(e + 1) >> bit & 1))
            continue;
        ww(e + 0x2E, 0xFFFF);
        x = rd(e + 4);
        wd(0x0020, x);
        if (x != 0) {
            wd(0x0008, x);
            wb(x, (uint8_t)(rb(x) | (uint8_t)rd(0x003C)));
            wb(x + 2, (uint8_t)(rb(x + 2) | 2));
            wb(x + 1, 0xFF);
            wb(x + 3, 0);
            wb(x + 4, 8);
        }
        /* CODE:29DED */
        wb(e, (uint8_t)(rb(e) | 4));
        st = rd(0x0014);
        t = rd(st + 0x2A36);
        wd(0x0020, t);
        if (t != 0) {
            wd(0x000C, t);
            wd(t + 0x30, e);
        } else {
            wd(st + 0x2A32, e);
        }
        wd(st + 0x2A36, e);
    }
    STREAMS_RESET();
}

/* CODE:2A113: header slot 16's counters at the next ball: +38h, +3Ch,
 * +26h 0; without bit 0 of +0 +30h, +34h from +28h, +2Ch and, without
 * bit 3, the player's words +6 and +16h to the word +2 and the
 * thresholds' bytes +2 0; with bit 0 the 12-digit number ending at +38h
 * added to the one ending at +40h the player's word +6 times; then
 * SLOT17_CLEAR */
static void COUNTERS_BALL_RESET(void)
{
    uint32_t st = rd(0x0014), p, e, q;
    uint16_t si, bx;

    wd(0x0000, rd(st + 0x28E6));
    for (;;) {
        p = rd(0x0000);
        e = rd(p);
        wd(0x0020, e);
        wd(0x0000, p + 4);
        if (e == 0)
            break;
        wd(0x0004, e);
        wd(e + 0x38, 0);
        wd(e + 0x3C, 0);
        ww(e + 0x26, 0);
        si = rw(rd(0x0014) + 0x0D72);
        ww(0x0024, si);
        if (!(rb(e) & 1)) {
            wd(e + 0x30, rd(e + 0x28));
            wd(e + 0x34, rd(e + 0x2C));
            if (rb(e) & 8)
                continue;
            wd(0x0020, (e & 0xFFFF0000u) | rw(e + 2));
            ww(e + sx16(si) * 2 + 6, rw(0x0020));
            ww(e + sx16(si) * 2 + 0x16, rw(0x0020));
            wd(0x0008, e + 0x50);
            /* CODE:2A1C6 */
            for (q = e + 0x50; !(rw(q) & 0x8000); q += 0x0C) {
                wb(q + 2, 0);
                wd(0x0008, q + 0x0C);
            }
            continue;
        }
        /* CODE:2A1E5 */
        bx = (uint16_t)(rw(e + sx16(si) * 2 + 6) - 1);
        ww(0x0020, bx);
        if (bx & 0x8000)
            continue;
        do {
            /* CODE:2A210 */
            q = rd(0x0004);
            wd(0x0008, q + 0x38);
            wd(0x000C, q + 0x40);
            bcd12_add(rd(0x000C), rd(0x0008));
            wd(0x0008, rd(0x0008) - 6);
            wd(0x000C, rd(0x000C) - 6);
            bx = rw(0x0020);
            ww(0x0020, (uint16_t)(bx - 1));
        } while (bx != 0);
    }
    SLOT17_CLEAR();
}

/* CODE:28E5D: the state [0020] of the drop target [0000] into
 * DROP_STATES (by its word +44h) with the target */
void DROP_SET(void)
{
    uint32_t b = rd(0x0000), n = rw(b + 0x44);

    wb(N_DROP_STATES + n * 8, rb(0x0020));
    wd(N_DROP_STATES + n * 8 + 2, b);
}

/* CODE:2A39A: of header slot 4's C0h objects (state+28B6h) each drop
 * target (word +0 1) up: +0Bh 0, DROP_SET with 0; the stacks state+2AD0h,
 * 2A60h, 2A64h to CODE:FA24, F920, FA48 */
static void DROPS_UP_ALL(void)
{
    uint32_t st = rd(0x0014), p, e;
    uint16_t si;

    wd(0x0010, rd(st + 0x28B6));
    ww(0x003C, 0xBF);
    do {
        p = rd(0x0010);
        e = rd(p);
        wd(0x0000, e);
        wd(0x0010, p + 4);
        if (e != 0 && rw(e) == 1) {
            wb(e + 0x0B, 0);
            wd(0x0020, 0);
            DROP_SET();
        }
        si = rw(0x003C);
        ww(0x003C, (uint16_t)(si - 1));
    } while (si != 0);
    st = rd(0x0014);
    wd(st + 0x2AD0, 0xFA24);
    wd(st + 0x2A60, 0xF920);
    wd(st + 0x2A64, 0xFA48);
}

/* CODE:2A421: as DROPS_UP_ALL at the next ball, but a drop target with
 * bit 0 of its byte +4 stays as it is */
static void DROPS_UP_BALL(void)
{
    uint32_t st = rd(0x0014), p, e;
    uint16_t si;

    wd(0x0010, rd(st + 0x28B6));
    ww(0x003C, 0xBF);
    do {
        p = rd(0x0010);
        e = rd(p);
        wd(0x0000, e);
        wd(0x0010, p + 4);
        if (e != 0 && rw(e) == 1 && !(rb(e + 4) & 1)) {
            wb(e + 0x0B, 0);
            wd(0x0020, 0);
            DROP_SET();
        }
        si = rw(0x003C);
        ww(0x003C, (uint16_t)(si - 1));
    } while (si != 0);
    st = rd(0x0014);
    wd(st + 0x2AD0, 0xFA24);
    wd(st + 0x2A60, 0xF920);
    wd(st + 0x2A64, 0xFA48);
}

/* CODE:2A4B1: the player's bonus (the number ending at +10h) cleared
 * unless byte +11h is set, the multiplier word +12h unless byte +14h is;
 * both bytes cleared */
static void BONUS_CLEAR(void)
{
    uint32_t pl = rd(rd(0x0014) + 0x0D76);

    wd(0x0000, pl);
    if (rb(pl + 0x11) == 0) {
        wd(pl + 8, 0);
        wd(pl + 0x0C, 0);
    }
    wb(pl + 0x11, 0);
    if (rb(pl + 0x14) == 0)
        ww(pl + 0x12, 0);
    wb(pl + 0x14, 0);
}

/* CODE:2A976: GAME_PHASE 2, a game's start: the balls per game to
 * state+0D36h, the ball 0 (state+0D38h), player 0 and its bit 1, the
 * eight player records (state+0D7Ah, 16h bytes) cleared; the balls, the
 * lights, the counters, the slot-15 records, the drop targets and the
 * display reset; the sound at CODE:1009A, the module's slot 40
 * (state+2946h), the tilt count 0, GAME_PHASE 6 */
static void GAME_START(void)
{
    uint32_t st = rd(0x0014), e;
    uint16_t n;

    ww(st + 0x0D36, rw(st + 0x0E38));
    wd(0x0024, 0);
    ww(st + 0x0D38, 0);
    ww(st + 0x0D72, 0);
    ww(st + 0x0D74, 1);
    wd(0x0000, st + 0x0D7A);
    wd(st + 0x0D76, st + 0x0D7A);
    ww(0x0020, 7);
    do {
        e = rd(0x0000);
        wd(e, 0);
        wd(e + 4, 0);
        wd(e + 8, 0);
        wd(e + 0x0C, 0);
        wb(e + 0x10, 0);
        ww(e + 0x12, 0);
        wb(e + 0x11, 0);
        wb(e + 0x14, 0);
        wd(0x0000, e + 0x16);
        n = rw(0x0020);
        ww(0x0020, (uint16_t)(n - 1));
    } while (n != 0);
    BALLS_RESET();
    LIGHTS_RESET();
    COUNTERS_RESET();
    RECORDS_RESET();
    DROPS_UP_ALL();
    DISPLAY_RESET();
    st = rd(0x0014);
    ww(st + 0x0D32, 1);
    wb(st + 0x0D3C, 0xFF);
    wb(st + 0x0D3D, 0);
    ww(st + 0x0D3A, 0);
    wb(st + 0x0D30, 0xFF);
    wd(0x0000, 0x1009A);
    SFX_PLAY();
    st = rd(0x0014);
    wb(st + 0x2A7F, 0xFF);
    /* the module's slot 40: a RET but on table 2 (docs/bpc-module.md) */
    MOD_CALL(rd(st + 0x2946));
    st = rd(0x0014);
    ww(st + 0x2A78, 0);
    ww(st + 0x8E, 6);
}

/* ---- the ball waiting for its launch (GAME_PHASE 6) ---- */

/* CODE:2F85C: the music record at [0000]: its track (+8) to MUSIC_NEXT
 * and flags (+0Ah) to state+59h (with bit 1 while LOST_BALL_RUNOUT);
 * its word +2 not negative: the module request (order +4, slot +6, the
 * word to MOD_REQUEST); FFFFh or FFFEh: the flags and track kept in
 * state+5Ah, 2A82h first */
void MUSIC_REQUEST(void)
{
    uint32_t st = rd(0x0014), r = rd(0x0000);
    uint16_t di;

    if (rb(N_LOST_BALL_RUNOUT) == 0) {
        ww(st + 0x2A80, rw(r + 8));
        wb(st + 0x59, rb(r + 0x0A));
    } else {
        wb(st + 0x59, (uint8_t)(rb(r + 0x0A) | 2));
    }
    di = rw(r + 2);
    wd(0x0020, (rd(0x0020) & 0xFFFF0000u) | di);
    if (di & 0x8000) {
        uint16_t k = (uint16_t)~di;

        wb(st + 0x5A, rb(st + 0x59));
        ww(st + 0x2A82, rw(st + 0x2A80));
        if (k > 1)
            pi_stop("MUSIC_REQUEST: a word +2 below FFFEh (CODE:2F906)");
        wd(0x0020, (rd(0x0020) & 0xFFFF0000u) | rw(0x2F91C + 2u * k));
    }
    ww(st + 0x2A88, rw(r + 4));
    ww(st + 0x2A8A, rw(r + 6));
    ww(st + 0x2A84, di);
}

/* CODE:301C9: the scroll in play: SCROLL_LINE (state+0D58h) toward the
 * ball's line (state+0D5Ch, from BALLS_SHOW) less state+0D52h, by the
 * distance / state+0E3Ch (halved going up near the top and going down),
 * between 0 and SCROLL_MAX; to ATTRACT_LINE and the next CRT start */
static void PLAY_SCROLL(void)
{
    uint32_t st = rd(0x0014), e;
    uint16_t bp = rw(st + 0x0D58), di, q, r;
    int32_t n;

    wd(0x0024, (rd(0x0024) & 0xFFFF0000u) | bp);
    di = (uint16_t)(rw(st + 0x0D5C) - bp - rw(st + 0x0D52));
    n = (int16_t)di;
    if ((int16_t)rw(st + 0x0E3C) == 0)
        pi_stop("PLAY_SCROLL: IDIV by 0 (CODE:301F3)");
    q = (uint16_t)(n / (int16_t)rw(st + 0x0E3C));
    r = (uint16_t)(n % (int16_t)rw(st + 0x0E3C));
    e = (uint32_t)r << 16 | q;
    wd(0x0020, e);
    if ((int16_t)q < 0) {
        if ((int16_t)bp <= 0x20) {
            e = (e & 0xFFFF0000u) | (uint16_t)((int16_t)q >> 1);
            wd(0x0020, e);
        }
        ww(st + 0x0D58, (uint16_t)(rw(st + 0x0D58) + (uint16_t)e));
        if ((int16_t)rw(st + 0x0D58) < 0)
            ww(st + 0x0D58, 0);
    } else {
        di = (uint16_t)(rw(st + 0x0D54) - bp);
        wd(0x0028, (rd(0x0028) & 0xFFFF0000u) | di);
        if (di <= 0xFFCE) {
            e = (e & 0xFFFF0000u) | (uint16_t)((int16_t)q >> 1);
            wd(0x0020, e);
            ww(st + 0x0D58, (uint16_t)(rw(st + 0x0D58) + (uint16_t)e));
            di = rw(st + 0x0D54);
            wd(0x0028, (rd(0x0028) & 0xFFFF0000u) | di);
            if (di <= rw(st + 0x0D58))
                ww(st + 0x0D58, di);
        }
    }
    /* CODE:302A7 */
    e = (rd(0x0020) & 0xFFFF0000u) | rw(st + 0x0D58);
    ww(st + 0x9A, (uint16_t)e);
    wd(0x0020, (e & 0xFFFF0000u) | (uint16_t)(e + rw(st + 0x0D4C)));
    wd(0x0024, (rd(0x0024) & 0xFFFF0000u) | rw(st + 0x0D4A));
    CRT_NEXT_SET();
}

/* CODE:3086D: the next of the drop targets' queue (state+2A60h, down
 * from CODE:F920; a word, the kind, and the target): kind 1 DROP_SET
 * with the target's +0Bh */
static void DROPS_QUEUE_STEP(void)
{
    uint32_t st = rd(0x0014), q = rd(st + 0x2A60);
    uint16_t k;

    wd(0x0004, q);
    if (q == 0xF920)
        return;
    k = rw(q);
    wd(0x0000, rd(q + 2));
    wd(0x0004, q + 6);
    wd(st + 0x2A60, q + 6);
    wd(0x0020, (rd(0x0020) & 0xFFFF0000u) | rw(0x308C7 + sx16(k) * 2));
    if (k == 0)
        return;
    if (k != 1)
        pi_stop("DROPS_QUEUE_STEP: a kind above 1 (CODE:308C0)");
    wd(0x0020, (rd(0x0020) & 0xFFFFFF00u) | rb(rd(0x0000) + 0x0B));
    DROP_SET();
}

/* CODE:28E7B: the DROP_PIECES entry of the object [0000] (its dword
 * +8 the index) gets the state [0020]'s low byte and the object's x, y
 * (the low words of its dwords +0, +4); DROPS_UPDATE draws it */
static void PIECE_SET(void)
{
    uint32_t o = rd(0x0000), i = rd(o + 8) * 8;

    wb(N_DROP_PIECES + 1 + i, rb(0x0020));
    ww(N_DROP_PIECES + 2 + i, rw(o));
    ww(N_DROP_PIECES + 4 + i, rw(o + 4));
}

/* CODE:308E7: the slingshots' records (state+28DAh: word offsets from
 * it, 0 ends): a byte +0 FFh (a kick) set to 2, a count above 0 down;
 * at 0 their picture +16h drawn (CODE:28E7B) */
static void SLINGS_STEP(void)
{
    uint32_t st = rd(0x0014), base = rd(st + 0x28DA), r;
    uint16_t bx;
    uint8_t bl;

    wd(0x0010, base);
    wd(0x003C, base);
    for (;;) {
        wd(0x0000, base);
        bx = rw(rd(0x0010));
        wd(0x0020, (rd(0x0020) & 0xFFFF0000u) | bx);
        wd(0x0010, rd(0x0010) + 2);
        if (bx == 0)
            return;
        r = base + sx16(bx);
        wd(0x0000, r);
        bl = rb(r);
        wd(0x0020, (rd(0x0020) & 0xFFFFFF00u) | bl);
        if (bl == 0)
            continue;
        if (bl & 0x80) {
            wb(r, 2);
        } else {
            wb(r, (uint8_t)(bl - 1));
            if (bl != 1)
                continue;
            wd(0x0020, 0);
            wb(r, 0);
        }
        /* CODE:3096E */
        wd(0x0024, rd(r + 0x16));
        if (rd(r + 0x16) == 0)
            continue;
        wd(0x0000, rd(r + 0x16));
        PIECE_SET();
    }
}

/* the ball the hole [0000] holds (its byte +1, the ball's number) to
 * [0010], the number less 1 to [0020] */
static uint32_t hole_ball(void)
{
    uint32_t h = rd(0x0000), b;
    uint16_t bx = (uint16_t)(rb(h + 1) - 1);

    wd(0x0020, bx);
    b = rd(rd(0x0014) + 0x107A + sx16(bx) * 4);
    wd(0x0010, b);
    return b;
}

/* CODE:30CFB: the ball [0010] out of the hole [0000]: at its +6, +8 (the
 * pixels to +12h, +14h, shifted left by 0Ah to +1Eh, +22h), its speed
 * words (high bytes cleared) plus +0Ah, +0Ch, onto the level +0Eh as
 * zone type 3 (0) or 2 (1) does */
static void HOLE_BALL_OUT(void)
{
    uint32_t h = rd(0x0000), b = rd(0x0010);
    int32_t x, y;
    uint16_t lv;

    x = (int32_t)sx16(rw(h + 6));
    y = (int32_t)sx16(rw(h + 8));
    ww(b + 0x12, (uint16_t)x);
    ww(b + 0x14, (uint16_t)y);
    wd(0x0028, 0x0A);
    wd(b + 0x1E, (uint32_t)x << 10);
    wd(b + 0x22, (uint32_t)y << 10);
    wd(0x0020, sx16(rw(h + 0x0A)));
    wd(b + 0x0E, rd(b + 0x0E) & 0x00FF00FFu);
    ww(b + 0x0E, (uint16_t)(rw(b + 0x0E) + rw(h + 0x0A)));
    ww(b + 0x10, (uint16_t)(rw(b + 0x10) + rw(h + 0x0C)));
    lv = rw(h + 0x0E);
    wd(0x0024, (sx16(rw(h + 0x0C)) & 0xFFFF0000u) | lv);
    if (lv == 0)
        BALL_BUNDLE1();
    else if (lv == 1)
        BALL_BUNDLE2();
    else
        pi_stop("HOLE_EJECT_STEP: a level above 1 (CODE:30D6F)");
}

/* the hole's count [0020] at or below 3Ch: at 3Ch the ball out; below
 * 32h every 8th count the sound record +10h of [0004] (none: nothing
 * drawn either), then the picture +30h of [0004] (the second hole) or of
 * the hole flickered by count AND 4. Without bit 1 [0004] still holds
 * the drop-target queue's place, not the hole (a bug: nothing sounds);
 * with pi_fix_hole_sound the hole's own record +10h is played instead */
static void hole_flicker(int second)
{
    uint16_t di = rw(0x0020);
    uint32_t r;

    if (di > 0x3C)
        return;
    if (di == 0x3C) {                                   /* CODE:30CAD */
        hole_ball();
        if (rb(rd(0x0000)) & 2)                         /* CODE:30CDE */
            wd(0x0000, rd(rd(0x0000) + 0x34));
        HOLE_BALL_OUT();
        return;
    }
    if (di >= 0x32)
        return;
    cw(0x0024, (uint16_t)(di & 7));
    if ((di & 7) == 0) {
        r = !second && pi_fix_hole_sound ? rd(rd(0x0000) + 0x10) : rd(rd(0x0004) + 0x10);
        wd(0x0024, r);
        if (r == 0)
            return;
        if (second) {
            wd(0x0000, r);
            SFX_PLAY();
        } else {
            wd(0x0028, rd(0x0000));
            wd(0x0000, r);
            SFX_PLAY();
            wd(0x0000, rd(0x0028));
        }
    }
    cw(0x0020, (uint16_t)(rw(0x0020) & 4));             /* CODE:30A92, 30BB1 */
    wd(0x0000, rd((second ? rd(0x0004) : rd(0x0000)) + 0x30));
    PIECE_SET();
}

/* CODE:30C5C: a hole without a picture whose count ran out: the ball let
 * go and out */
static void hole_out_now(void)
{
    uint32_t h = rd(0x0000), b;

    wd(rd(0x0014) + 0x2A68, 0);
    b = hole_ball();
    wb(b + 1, (uint8_t)(rb(b + 1) & 0x7F));
    wb(h + 1, 0);
    if (rb(h) & 2) {
        wb(h, (uint8_t)(rb(h) & ~2));
        wd(0x0000, rd(h + 0x34));                       /* CODE:30CEC */
    }
    HOLE_BALL_OUT();
}

/* CODE:30996: the hole ejecting (state+2A68h): its count +4 counted up
 * from below 0 (no picture: the ball out at 0) or down from 4Ch (at 0 the
 * ball let go and the hole free, at 3Ch the ball out, below 32h the
 * picture flickered with its sound); a hole with bit 1 of its byte +0
 * set ejects at its second hole +34h, one without ends with the main
 * program's record CODE:100E8. With none ejecting the next from the
 * stack state+2A64h (up from CODE:FA48): count 4Ch when the hole the
 * ball comes out of has a picture (+30h), else FFCEh */
static void HOLE_EJECT_STEP(void)
{
    uint32_t st = rd(0x0014), h = rd(st + 0x2A68), p, b;

    wd(0x0020, h);
    if (h != 0) {
        wd(0x0000, h);
        if (rb(h) & 2) {
            wd(0x0004, rd(h + 0x34));
            if (rw(h + 4) & 0x8000) {
                ww(h + 4, (uint16_t)(rw(h + 4) + 1));
                if (rw(h + 4) == 0)
                    hole_out_now();
                return;
            }
            ww(h + 4, (uint16_t)(rw(h + 4) - 1));       /* CODE:309E5 */
            if (rw(h + 4) == 0) {
                wd(st + 0x2A68, 0);
                b = hole_ball();
                wb(b + 1, (uint8_t)(rb(b + 1) & 0x7F));
                wb(h, (uint8_t)(rb(h) & ~2));
                wb(h + 1, 0);
            }
            h = rd(0x0000);                             /* CODE:30A33 */
            wd(0x0020, (rd(0x0020) & 0xFFFF0000u) | rw(h + 4));
            hole_flicker(1);
            return;
        }
        if (rw(h + 4) & 0x8000) {                       /* CODE:30AB7 */
            ww(h + 4, (uint16_t)(rw(h + 4) + 1));
            if (rw(h + 4) == 0)
                hole_out_now();
            return;
        }
        ww(h + 4, (uint16_t)(rw(h + 4) - 1));           /* CODE:30AD0 */
        if (rw(h + 4) == 0) {
            wd(st + 0x2A68, 0);
            wd(0x0024, h);
            wd(0x0000, 0x100E8);
            RECORD_DISPATCH();
            h = rd(0x0024);
            wd(0x0000, h);
            b = hole_ball();
            wb(b + 1, (uint8_t)(rb(b + 1) & 0x7F));
            wb(h + 1, 0);
        }
        h = rd(0x0000);                                 /* CODE:30B44 */
        wd(0x0020, (rd(0x0020) & 0xFFFF0000u) | rw(h + 4));
        hole_flicker(0);
        return;
    }
    p = rd(st + 0x2A64);                                /* CODE:30BD6 */
    wd(0x0004, p);
    if (p == 0xFA48)
        return;
    wd(st + 0x2A64, p + 4);
    h = rd(p);
    wd(0x0000, h);
    wd(0x0004, p + 4);
    if ((rb(h) & 2) ? rd(rd(h + 0x34) + 0x30) != 0 : rd(h + 0x30) != 0)
        ww(h + 4, 0x4C);
    else
        ww(h + 4, 0xFFCE);
    wd(rd(0x0014) + 0x2A68, h);
}

/* CODE:30799: a bank of drop targets raised: with none waiting (state+
 * 2AD8h 0) the next from the stack state+2AD0h (up to CODE:FA24) and 64h
 * frames; when they run out each target of the bank (a chain by +1Eh)
 * up (+0Bh 0) and queued for DROP_SET (kind 1 on state+2A60h) */
static void DROPS_RAISE_STEP(void)
{
    uint32_t st = rd(0x0014), p, t, q;

    if (rw(st + 0x2AD8) == 0) {
        p = rd(st + 0x2AD0);
        wd(0x0000, p);
        if (p == 0xFA24)
            return;
        ww(st + 0x2AD8, 0x64);
        wd(st + 0x2AD4, rd(p));
        wd(0x0000, p + 4);
        wd(st + 0x2AD0, p + 4);
        return;
    }
    ww(st + 0x2AD8, (uint16_t)(rw(st + 0x2AD8) - 1));
    if (rw(st + 0x2AD8) != 0)
        return;
    wd(0x000C, rd(st + 0x2AD4));
    q = rd(st + 0x2A60);
    wd(0x0008, q);
    t = rd(rd(0x000C));
    wd(0x0000, t);
    for (;;) {
        wb(t + 0x0B, 0);
        q -= 4;
        wd(q, t);
        q -= 2;
        wd(0x0008, q);
        ww(q, 1);
        wd(0x0020, rd(t + 0x1E));
        if (rd(t + 0x1E) == 0)
            break;
        t = rd(t + 0x1E);
        wd(0x0000, t);
    }
    wd(st + 0x2A60, q);
}

/* CODE:2FBBF: a take's points: the 12-digit packed-BCD number [000C]
 * points past added to the player's +10h (the bonus), the one 8 bytes
 * before it to the player's +8 (the score); [000C] 14 back after */
void TAKE_PAY(void)
{
    uint32_t keep10 = rd(0x0010), keep0 = rd(0x0000), pl;
    int k;

    wb(N_DISPLAY_BUSY, 0xFF);
    pl = rd(rd(0x0014) + 0x0D76);
    wd(0x0010, pl);
    wd(0x0000, pl + 0x10);
    for (k = 0; k < 2; k++) {
        uint32_t src = rd(0x000C), dst = rd(0x0000);
        int cf = 0, i;

        for (i = 0; i < 4; i++)
            adc_daa(dst - 4 + (uint32_t)i, src - 4 + (uint32_t)i, &cf);
        for (i = 0; i < 2; i++)
            adc_daa(dst - 8 + (uint32_t)i, src - 8 + (uint32_t)i, &cf);
        wd(0x000C, rd(0x000C) - 6);
        wd(0x0000, rd(0x0000) - 6);
        if (k == 0) {
            wd(0x000C, rd(0x000C) - 2);
            wd(0x0000, rd(0x0000) - 2);
        }
    }
    wd(0x0000, keep0);
    wd(0x0010, keep10);
}

/* CODE:2C3E7: the zone record the ball is inside ([0010]+68h) left: its
 * byte +0 0 */
static void ZONE_LEAVE(void)
{
    uint32_t b = rd(0x0010), z = rd(b + 0x68);

    wd(0x0020, z);
    if (z == 0)
        return;
    wd(0x0000, z);
    wb(z, 0);
    wd(b + 0x68, 0);
}

/* CODE:2C4DF (2C65E for type 0): the zone object's pay, [0004] the
 * object: with a light state at +0Ah the player's bit set in it; when it
 * was set already the light flashed 8 times and the points at +26h to
 * the score only (SCORE_ADD), else flashed 0Ch times and, as without a
 * light, the points at +1Eh (TAKE_PAY); the record +2 (RECORD_DISPATCH)
 * and the event stream +6 */
static void zone_pay(void)
{
    uint32_t r = rd(rd(0x0004) + 0x0A);

    wd(0x0020, r);
    wd(0x0008, r);
    if (r != 0) {
        uint16_t cx = rw(rd(0x0014) + 0x0D72);
        unsigned b = cx & 7;
        int was = rb(r) >> b & 1;

        wd(0x0020, (r & 0xFFFF0000u) | cx);
        wb(r, (uint8_t)(rb(r) | 1u << b));
        if (was) {
            wd(0x0020, (r & 0xFFFF0000u) | 8);
            LIGHT_QUEUE();
            wd(0x000C, rd(0x0004) + 0x26);
            SCORE_ADD();
            goto rest;
        }
        wd(0x0020, (rd(0x0020) & 0xFFFF0000u) | 0x0C);
        LIGHT_QUEUE();
    }
    wd(0x000C, rd(0x0004) + 0x1E);
    TAKE_PAY();
rest:
    r = rd(rd(0x0004) + 2);
    wd(0x0020, r);
    if (r != 0) {
        wd(0x0000, r);
        RECORD_DISPATCH();
    }
    r = rd(rd(0x0004) + 6);
    wd(0x0020, r);
    if (r != 0) {
        wd(0x0000, r);
        EVENT_QUEUE();
    }
}

/* CODE:2C4B5: zone type 1: on entry (the object's byte +0 0) the ball's
 * number into it and the object to the ball's +68h, then its pay */
static void ZONE_TYPE1(void)
{
    uint32_t o = rd(rd(0x0000) + 0x0A), b = rd(0x0010);

    wd(0x0004, o);
    wd(b + 0x68, o);
    if (rb(o) != 0)
        return;
    wb(o, rb(b + 0x0A));
    zone_pay();
}

/* CODE:2C59C: zone type 0, the end of the plunger lane (presumably): on
 * entry (the object's byte +0 not the ball's number) while the skill
 * shot is armed (state+0D2Fh) header slot 27's event stream (state+2912h)
 * when state+0E38h equals the balls per game (state+0D36h), else slot
 * 28's (2916h); then the tilt count, the skill shot, state+0D3Ch and
 * 0D3Dh (the ball no longer waits) and the ball's +0Bh cleared, the
 * ball's number into the object, its pay; ZONE_LEAVE in any case */
static void ZONE_TYPE0(void)
{
    uint32_t o = rd(rd(0x0000) + 0x0A), b = rd(0x0010), st = rd(0x0014);

    wd(0x0004, o);
    wd(0x0020, (rd(0x0020) & 0xFFFFFF00u) | rb(b + 0x0A));
    if (rb(b + 0x0A) != rb(o)) {
        if (rb(st + 0x0D2F) != 0) {
            wd(0x0000, rd(st + 0x2912));
            wd(0x0024, (rd(0x0024) & 0xFFFF0000u) | rw(st + 0x0E38));
            if (rw(st + 0x0E38) != rw(st + 0x0D36))
                wd(0x0000, rd(st + 0x2916));
            EVENT_QUEUE();
            wd(0x0004, o);
        }
        /* CODE:2C61F */
        st = rd(0x0014);
        ww(st + 0x2A78, 0);
        wb(st + 0x0D2F, 0);
        b = rd(0x0010);
        wb(o, rb(b + 0x0A));
        wb(st + 0x0D3C, 0);
        wb(st + 0x0D3D, 0);
        wb(b + 0x0B, 0);
        zone_pay();
    }
    ZONE_LEAVE();                                       /* CODE:2C713 */
}

/* CODE:2C719: zone type 4, a hole: unless the ball is held (bit 7 of its
 * byte +1), when the hole's byte +1 is 0 (none held): with its byte +2
 * set, the player's bit (state+0D74h) in the light state at +2Ch, which
 * then stands for the hole in what follows, its byte +3 and state+2A6Ch
 * counted up; the ball's number into its byte +1, the ball held, the
 * points before +2Ch (TAKE_PAY), the main program's record CODE:100CE
 * (RECORD_DISPATCH) and the event stream +14h */
static void ZONE_TYPE4(void)
{
    uint32_t b = rd(0x0010), h, r, st;

    if (rb(b + 1) & 0x80)
        return;
    h = rd(rd(0x0000) + 0x0A);
    wd(0x0004, h);
    wd(0x0020, (rd(0x0020) & 0xFFFFFF00u) | rb(h + 1));
    if (rb(h + 1) != 0)
        return;
    if (rb(h + 2) != 0) {
        r = rd(h + 0x2C);
        wd(0x0020, r);
        if (r != 0) {
            uint16_t dx;

            wd(0x0004, r);
            st = rd(0x0014);
            dx = rw(st + 0x0D74);
            wd(0x0024, (rd(0x0024) & 0xFFFF0000u) | dx);
            wb(r, (uint8_t)(rb(r) | (uint8_t)dx));
        }
        r = rd(0x0004);                                 /* CODE:2C78B */
        wb(r + 3, (uint8_t)(rb(r + 3) + 1));
        st = rd(0x0014);
        ww(st + 0x2A6C, (uint16_t)(rw(st + 0x2A6C) + 1));
    }
    b = rd(0x0010);                                     /* CODE:2C7A3 */
    wb(rd(0x0004) + 1, rb(b + 0x0A));
    wb(b + 1, (uint8_t)(rb(b + 1) | 0x80));
    wd(0x000C, rd(0x0004) + 0x2C);
    TAKE_PAY();
    wd(0x0000, 0x100CE);
    RECORD_DISPATCH();
    r = rd(rd(0x0004) + 0x14);
    wd(0x0020, r);
    if (r != 0) {
        wd(0x0000, r);
        EVENT_QUEUE();
    }
}

static void zone_handler(uint16_t type)
{
    static char why[48];

    switch (type) {
    case 0:
        ZONE_TYPE0();
        break;
    case 1:
        ZONE_TYPE1();
        break;
    case 2:
        ZONE_LEAVE();
        BALL_BUNDLE2();
        break;
    case 3:
        ZONE_LEAVE();
        BALL_BUNDLE1();
        break;
    case 4:
        ZONE_TYPE4();
        break;
    default:
        snprintf(why, sizeof why, "ZONES_CHECK: zone type %u", (unsigned)type);
        pi_stop(why);
    }
}

/* the object of the zone at [0000] left by the ball in [0038]'s low
 * byte: the ball's number cleared from its byte +0 (types 0, 1) */
static void zone_unmark(uint32_t z)
{
    uint32_t o;

    if (rw(z + 8) > 1)
        return;
    o = rd(z + 0x0A);
    wd(0x0004, o);
    if ((uint8_t)rd(0x0038) == rb(o))
        wb(o, 0);
}

/* CODE:2C1DA: each ball in play's centre (+12h, +14h plus 8) against
 * the zones at its +64h (0Eh bytes: x0, y0, x1, y1, the type, the
 * object; a negative x0 ends them): the first one it is inside handled
 * by its type; the ball's marks taken off the objects of the others */
static void ZONES_CHECK(void)
{
    uint32_t st = rd(0x0014), b, z;
    uint16_t n = rw(st + 0x0D32);

    wd(0x0008, st + 0x1046);
    ww(st + 0x0D34, n);
    if (n == 0)
        return;
    do {
        b = rd(rd(0x0008));
        wd(0x0010, b);
        wd(0x0008, rd(0x0008) + 4);
        if (rb(b + 9) != 0) {
            ZONE_LEAVE();
            goto next;
        }
        wd(0x0020, (sx16(rw(b + 0x12)) & 0xFFFF0000u) | (uint16_t)(rw(b + 0x12) + 8));
        wd(0x0024, (sx16(rw(b + 0x14)) & 0xFFFF0000u) | (uint16_t)(rw(b + 0x14) + 8));
        wd(0x0000, rd(b + 0x64));
        wd(0x0038, (rd(0x0038) & 0xFFFFFF00u) | rb(b + 0x0A));
        for (;;) {                                      /* CODE:2C258 */
            uint16_t x = (uint16_t)rd(0x0020), y = (uint16_t)rd(0x0024);

            z = rd(0x0000);
            wd(0x0028, (rd(0x0028) & 0xFFFF0000u) | rw(z));
            if (rw(z) & 0x8000) {
                ZONE_LEAVE();
                goto next;
            }
            wd(0x002C, sx16(rw(z + 2)));
            wd(0x0030, sx16(rw(z + 4)));
            wd(0x0034, sx16(rw(z + 6)));
            if (x >= rw(z) && y >= rw(z + 2) && x <= rw(z + 4) && y <= rw(z + 6))
                break;
            zone_unmark(z);                             /* CODE:2C3A6 */
            wd(0x0000, z + 0x0E);
        }
        /* inside: the type's handler (CODE:2C3DD), then the rest of the
         * zones only unmarked */
        {
            uint16_t type = rw(z + 8);
            uint32_t keep8 = rd(0x0008);

            wd(0x0028, (rd(0x0028) & 0xFFFF0000u) | rw(0x2C3DD + sx16(type) * 2));
            zone_handler(type);
            wd(0x0000, z);
            wd(0x0008, keep8);
            wd(0x0038, (rd(0x0038) & 0xFFFFFF00u) | rb(rd(0x0010) + 0x0A));
        }
        for (;;) {                                      /* CODE:2C31C */
            z = rd(0x0000) + 0x0E;
            wd(0x0000, z);
            if (rw(z) & 0x8000)
                break;
            if (rw(z + 8) == 4) {
                uint32_t o = rd(z + 0x0A);

                wd(0x0004, o);
                if ((uint8_t)rd(0x0038) == rb(o + 1))
                    wb(o + 1, 0);
            } else {
                zone_unmark(z);
            }
        }
next:
        st = rd(0x0014);
        n = (uint16_t)(rw(st + 0x0D34) - 1);
        ww(st + 0x0D34, n);
    } while (n != 0);
}

/* CODE:2F2AF: a ball to serve (state+0D3Ah) while none waits (state+
 * 0D3Ch): on the table (state+0D32h) and waiting, the sound record
 * CODE:1009A; with state+0D3Dh set or Enter (KEY_DOWN+1Ch) the waiting
 * ball's speed up by 1770h (the plunger, CODE:2F300) */
static void SERVE(void)
{
    uint32_t st = rd(0x0014), p, b;
    uint8_t n;

    if (rb(st + 0x0D3D) == 0) {
        if (rw(st + 0x0D3A) != 0 && rb(st + 0x0D3C) == 0) {
            ww(st + 0x0D32, (uint16_t)(rw(st + 0x0D32) + 1));
            ww(st + 0x0D3A, (uint16_t)(rw(st + 0x0D3A) - 1));
            wb(st + 0x0D3D, 0xFF);
            wb(st + 0x0D3C, 0xFF);
            wd(0x0000, 0x1009A);
            SFX_PLAY();
            return;
        }
        if (rb(st + 0x0E62) == 0)
            return;
        wb(st + 0x0E62, 0);
    }
    /* CODE:2F31C */
    p = rd(st + 0x2906);
    wd(0x0000, p);
    n = rb(p);
    wd(0x0020, n);
    if (n == 0)
        return;
    wb(p, 0);
    b = rd(st + (uint32_t)n * 4 + 0x1076);
    wd(0x0000, b);
    ww(b + 0x10, (uint16_t)(rw(b + 0x10) - 0x1770));
}

/* CODE:2B3BE: while the players can still be chosen (state+0D30h), F1..F8
 * set the count state+0D70h to 1..8, keypad Enter adds one; at most 8 */
static void PLAYERS_KEYS(void)
{
    uint32_t st = rd(0x0014), keys = st + 0x0E81;
    uint16_t si;

    if (rb(st + 0x0D30) != 0) {
        wd(0x0020, 7);
        wd(0x0000, keys);
        for (si = 7; ; ) {
            if (rb(keys + sx16(si)) != 0) {
                wb(keys + sx16(si), 0);
                si = (uint16_t)(si + 1);
                wd(0x0020, si);
                ww(st + 0x0D70, si);
                break;
            }
            wb(keys + sx16(si), 0);
            if (si == 0) {
                wd(0x0020, 0);
                if (rb(st + 0x0EE2) != 0) {
                    wb(st + 0x0EE2, 0);
                    wd(0x0020, 1);
                    ww(st + 0x0D70, (uint16_t)(rw(st + 0x0D70) + 1));
                }
                break;
            }
            si--;
            wd(0x0020, si);
        }
    }
    if (rw(st + 0x0D70) > 8)
        ww(st + 0x0D70, 8);
}

/* CODE:2B49A: the scroll, the lights and CODE:30784's steps (CODE:156C3,
 * 14DE1 and 14C45 are RETs) */
static void PLAY_STEP(void)
{
    PLAY_SCROLL();
    LIGHTS_STEP();
    DROPS_QUEUE_STEP();
    SLINGS_STEP();
    HOLE_EJECT_STEP();
    DROPS_RAISE_STEP();
}

/* a number into a text record's digits and the record drawn: [0020]'s
 * low word `n` by DEC_TEXT ending at `end`, then the record `rec` */
static void number_text(uint16_t n, uint32_t end, uint32_t rec)
{
    wd(0x0000, end);
    wd(0x0020, (rd(0x0020) & 0xFFFF0000u) | n);
    DEC_TEXT();
    wd(0x0000, rec);
    DM_TEXT_DRAW();
}

/* CODE:2B1DC: GAME_PHASE 6, the ball waiting for its launch: the ball
 * save (SERVE_SECONDS x FRAME_RATE), header slot 34's music record; then
 * frames of the physics, the zones, the serve and "PLAYER n" (or
 * "PLAYERS n" while they can be chosen) "BALL n" until the ball leaves
 * (state+0D3Ch 0: GAME_PHASE 4) or Esc (GAME_PHASE 1, the attract
 * mode's music) */
static void BALL_WAIT(void)
{
    uint32_t st = rd(0x0014), e;

    e = (uint32_t)rw(st + 0x0E42) * rw(st + 0x50);
    wd(0x0020, e);
    ww(st + 0x0D3E, (uint16_t)e);
    wb(st + 0x0D2F, 0xFF);
    wd(0x0000, rd(st + 0x292E));
    MUSIC_REQUEST();
    for (;;) {
        FRAME_STEP();
        PLAY_STEP();
        BALLS_PHYSICS();
        ZONES_CHECK();
        SERVE();
        DM_CLEAR();
        st = rd(0x0014);
        if (rb(st + 0x0D30) == 0)
            number_text((uint16_t)(rw(st + 0x0D72) + 1), 0x2B39A, 0x2B38A);
        else
            number_text(rw(st + 0x0D70), 0x2B3AD, 0x2B39C);
        st = rd(0x0014);
        number_text((uint16_t)(rw(st + 0x0D38) + 1), 0x2B3BC, 0x2B3AE);
        DM_SCORE_IDLE();
        FLASH_STEP();
        PLAYERS_KEYS();
        st = rd(0x0014);
        if (rb(st + 0x0E47) != 0) {
            /* CODE:2B345 */
            wb(st + 0x0E47, 0);
            wb(st + 0x0E35, 1);
            ww(st + 0x8E, 1);
            ww(st + 0x2A88, 1);
            ww(st + 0x2A8A, 0);
            ww(st + 0x2A84, 0xFFFE);
            return;
        }
        if (rb(st + 0x0D3C) == 0)
            break;
    }
    ww(st + 0x2A78, 0);
    wb(st + 0x2A75, 0);
    ww(st + 0x8E, 4);
}

/* ---- play (GAME_PHASE 4) ---- */

/* CODE:2B4B9: a frame's rules */
static void PLAY_EVENTS(void)
{
    ZONES_CHECK();
    OBJECT_HITS();
    EVENT_RUN();
    SERVE();
    FLASH_STEP();
    LIT_LIST_STEP();
    MODE_RUN();
    BCD_COUNTERS_STEP();
    COUNTER_TIMERS();
    OBJECT_TIMERS();
}

/* CODE:2B63B: Esc in the pause: "REALLY QUIT TABLE?" (CODE:2A93E) with
 * the flippers stepped, until Y (state+8Dh FFh; 1 back) or another key
 * (the display cleared; 0 back, to the pause) */
static int PAUSE_QUIT(void)
{
    uint32_t st = rd(0x0014);

    wb(st + 0x0E47, 0);
    wb(st + 0x0E5B, 0);
    wb(N_LAST_KEY, 0);
    for (;;) {
        FRAME_STEP();
        PLAY_SCROLL();
        /* CODE:156C3, a RET */
        FLIPPERS_STEP();
        DM_CLEAR();
        wd(0x0000, 0x2A93E);
        DM_TEXT_DRAW();
        st = rd(0x0014);
        if (rb(st + 0x0E5B) != 0) {
            wb(st + 0x8D, 0xFF);
            return 1;
        }
        if (rb(N_LAST_KEY) != 0)
            break;
    }
    DM_CLEAR();
    st = rd(0x0014);
    wb(st + 0x0E5B, 0);
    wb(N_LAST_KEY, 0);
    return 0;
}

/* CODE:2B4EC: M (state+0E78h) flips CODE:9C4E's bit 0; P (state+0E5Fh)
 * the pause (CODE:2B51C): the display's buffers and the sound's levels
 * kept, frames of "GAME PAUSED" until a key (put back) or Esc
 * (PAUSE_QUIT; on Y out without state+91h); state+91h FFh */
static void PLAY_KEYS(void)
{
    uint32_t st = rd(0x0014), s14, s18;

    if (rb(st + 0x0E78) != 0) {
        wb(st + 0x0E78, 0);
        wb(0x9C4E, (uint8_t)(rb(0x9C4E) ^ 1));
    }
    st = rd(0x0014);
    if (rb(st + 0x0E5F) != 0) {
        ww(N_PAUSE_FRAMES, 0);
        wb(N_LAST_KEY, 0);
        wb(rd(0x0014) + 0x91, 0);
        DM_SAVE();
        SOUND_PAUSE();
        for (;;) {                                      /* CODE:2B543 */
            ww(N_PAUSE_FRAMES, (uint16_t)(rw(N_PAUSE_FRAMES) + 1));
            st = rd(0x0014);
            if (rb(st + 0x0EC5) != 0) {
                wb(st + 0x0EC5, 0);
                /* CODE:26A6D, a RET */
                wb(N_LAST_KEY, 0);
            }
            st = rd(0x0014);
            if (rb(st + 0x0E47) != 0) {
                if (PAUSE_QUIT())
                    return;
                continue;
            }
            if (rb(N_LAST_KEY) != 0)
                break;
            s18 = rd(0x0018);
            s14 = rd(0x0014);
            /* CODE:26A6D, a RET */
            wd(0x0014, s14);
            wd(0x0018, s18);
            FRAME_STEP();
            PLAY_SCROLL();
            /* CODE:156C3, a RET */
            DM_CLEAR();
            wd(0x0000, 0x2B6C5);                        /* "GAME PAUSED" */
            DM_TEXT_DRAW();
            /* [CODE:0020]'s low word PAUSE_FRAMES AND 100h */
            wd(0x0020, (rd(0x0020) & 0xFFFF0000u)
                       | (rw(N_PAUSE_FRAMES) & 0x100));
            wd(0x0000, (rw(N_PAUSE_FRAMES) & 0x100) == 0
                       ? 0x2B6DA    /* "PRESS ANY BUTTON TO PLAY" */
                       : 0x2B6FC);  /* "PRESS ESC TO QUIT" */
            DM_TEXT_DRAW();
        }
        DM_RESTORE();                                   /* CODE:2B616 */
        SOUND_RESUME();
        wb(rd(0x0014) + 0x0E5F, 0);
    }
    wb(rd(0x0014) + 0x91, 0xFF);
}

/* CODE:2994A: 150h bytes of video memory at D9E0h cleared in all four
 * planes */
static void VIDEO_D9E0_CLEAR(void)
{
    uint16_t i;

    vga_outw(0x3C4, 0x0F02);
    for (i = 0; i < 0x150; i++)
        vga_write((uint16_t)(0xD9E0 + i), 0);
}

/* CODE:2C0C7: the balls on the table and to serve (state+0D32h, 0D3Ah);
 * each lost one (+9 set) cleared, the sound record CODE:10080, swapped
 * with the list's last (state+1046h), taken off, its erase place +70h
 * D8BDh; one fewer on the table, and while the ball save runs one more
 * to serve.  1 (SF) when none is left on the table */
static int BALLS_LOST(void)
{
    uint32_t st = rd(0x0014), p, b, list;
    uint16_t di = (uint16_t)(rw(st + 0x0D32) + rw(st + 0x0D3A));

    wd(0x0020, (rd(0x0020) & 0xFFFF0000u) | di);
    if (di == 0)
        goto none;
    ww(st + 0x0D34, di);
    wd(0x0020, (rd(0x0020) & 0xFFFF0000u) | (uint16_t)(di - 1));
    wd(0x000C, st + 0x1046);
    wd(0x0008, st + 0x1046);
    do {                                                /* CODE:2C110 */
        p = rd(0x000C);
        b = rd(p);
        wd(0x0010, b);
        wd(0x000C, p + 4);
        if (rb(b + 9) != 0) {
            uint32_t k;

            wb(b + 9, 0);
            wd(0x0000, 0x10080);
            SFX_PLAY();
            list = rd(0x0008);
            k = list + sx16(rw(0x0020)) * 4;
            p = rd(0x000C) - 4;
            wd(0x000C, p);
            wd(p, rd(k));
            wd(k, rd(0x0010));
            BALL_HIDE();
            VIDEO_D9E0_CLEAR();
            ww(rd(0x0010) + 0x70, 0xDBD8);              /* CODE:2996F */
            st = rd(0x0014);
            ww(st + 0x0D32, (uint16_t)(rw(st + 0x0D32) - 1));
            if (rw(st + 0x0D32) == 0)
                goto none;
            if (rw(st + 0x0D3E) != 0)
                ww(st + 0x0D3A, (uint16_t)(rw(st + 0x0D3A) + 1));
        }
        st = rd(0x0014);                                /* CODE:2C1A5 */
        ww(st + 0x0D34, (uint16_t)(rw(st + 0x0D34) - 1));
    } while (rw(st + 0x0D34) != 0);
    wd(0x0020, 0);
    return 0;
none:
    wd(0x0020, 0xFFFFFFFFu);
    return 1;
}

/* a frame of play without the flippers' keys and the ball save: as the
 * lost ball's loops at CODE:2B95A and 2BA0A step */
static void PLAY_FRAME_LOST(void)
{
    FRAME_STEP();
    PLAY_STEP();
    BALLS_PHYSICS();
    PLAY_EVENTS();
    PLAY_KEYS();
    DISPLAY_RUN();
    ANIMS_STEP();
}

/* 1 while the display's queue (state+2A2Ah at index state+2A28h) has an
 * entry or a display record runs (state+2A2Eh) */
static int display_busy(void)
{
    uint32_t st = rd(0x0014), c;
    uint16_t di = rw(st + 0x2A28);

    wd(0x0024, (rd(0x0024) & 0xFFFF0000u) | di);
    c = rd(rd(st + 0x2A2A) + sx16(di) * 4);
    wd(0x0020, c);
    return c != 0 || rd(st + 0x2A2E) != 0;
}

/* CODE:2B9F6: the last ball lost without the ball save: LOST_BALL_RUNOUT
 * and state+0D51h set, frames until the display, the event queue and the
 * mode stream are done, then GAME_PHASE 5 */
static void LOST_RUNOUT(void)
{
    uint32_t st = rd(0x0014);

    wb(st + 0x0D51, 0xFF);
    wb(N_LOST_BALL_RUNOUT, 0xFF);
    for (;;) {
        uint16_t di;
        uint32_t c;

        PLAY_FRAME_LOST();
        if (display_busy())
            continue;
        st = rd(0x0014);
        di = rw(st + 0x2A1C);
        wd(0x0024, (rd(0x0024) & 0xFFFF0000u) | di);
        c = rd(rd(st + 0x2A1E) + sx16(di) * 4);
        wd(0x0020, c);
        if (c != 0 || rd(st + 0x0D5E) != 0)
            continue;
        break;
    }
    wb(st + 0x0D51, 0);
    wb(N_LOST_BALL_RUNOUT, 0);
    ww(st + 0x8E, 5);
}

/* CODE:2B76E: GAME_PHASE 4, a frame of play: Esc ends the game at once
 * (GAME_PHASE 3); state+0EC5h puts the first ball back at the plunger;
 * the physics and the lost balls, the rules, the keys, the display, the
 * ball save counted down with its lamp (header slot 25's second light
 * state) blinking faster in its last frames; a lost last ball served
 * again while the ball save runs (GAME_PHASE 7), else LOST_RUNOUT; the
 * tilt (state+2A75h) GAME_PHASE 9 */
static void PLAY(void)
{
    uint32_t st, l;

    FRAME_STEP();
    PLAY_STEP();
    st = rd(0x0014);
    if (rb(st + 0x0E47) != 0) {
        /* CODE:2BAB5 */
        wb(st + 0x8D, 0xFF);
        wb(N_LOST_BALL_RUNOUT, 0);
        ww(st + 0x8E, 3);
        return;
    }
    if (rb(st + 0x0EC5) != 0) {
        wb(st + 0x0EC5, 0);
        if (rb(st + 0x0D3C) == 0) {
            wd(0x0010, rd(st + 0x1046));
            BALL_PLACE();
            wb(rd(0x0014) + 0x0D3C, 0xFF);
        }
    }
    st = rd(0x0014);                                    /* CODE:2B7C8 */
    if (rb(st + 0x0EC5) != 0)
        pi_stop("PLAY: CODE:2B7D7");
    if (rb(st + 0x0FC5) == 0) {                         /* CODE:2B81E */
        BALLS_PHYSICS();
        if (BALLS_LOST())
            goto lost;
    }
    PLAY_EVENTS();                                      /* CODE:2B83D */
    PLAY_KEYS();
    DISPLAY_RUN();
    ANIMS_STEP();
    st = rd(0x0014);
    if (rb(st + 0x0FC5) != 0) {
        ww(st + 0x0D3E, 1);
    } else {
        if (rw(st + 0x0D3E) == 0)
            goto keys;
        ww(st + 0x0D3E, (uint16_t)(rw(st + 0x0D3E) - 1));
    }
    /* CODE:2B883: the ball save's lamp */
    st = rd(0x0014);
    l = rd(rd(st + 0x290A) + 4);
    wd(0x0020, l);
    if (l != 0) {
        uint16_t bp = rw(st + 0x0D3E);
        int nz;

        wd(0x0004, l);
        wd(0x0020, (l & 0xFFFF0000u) | bp);
        if (bp > 0x64) {
            wd(0x0020, (l & 0xFFFF0000u) | (bp & 4));
            nz = (bp & 4) != 0;
        } else if (bp > 0x32) {
            wd(0x0020, (l & 0xFFFF0000u) | (bp & 1));
            nz = (bp & 1) != 0;
        } else {
            wb(l, 0);
            goto keys;
        }
        wb(rd(0x0004), (uint8_t)nz);
    }
keys:                                                   /* CODE:2B8ED */
    PLAYERS_KEYS();
    st = rd(0x0014);
    if (rb(st + 0x2A75) != 0) {
        wd(0x0000, rd(st + 0x293E));
        MUSIC_REQUEST();
        ww(rd(0x0014) + 0x8E, 9);
    }
    return;
lost:                                                   /* CODE:2B922 */
    st = rd(0x0014);
    l = rd(rd(st + 0x290A) + 4);
    wd(0x0020, l);
    if (l != 0) {
        wd(0x0004, l);
        wb(l, 0);
    }
    if (rw(st + 0x0D3E) == 0) {
        LOST_RUNOUT();
        return;
    }
    do                                                  /* CODE:2B95A */
        PLAY_FRAME_LOST();
    while (display_busy());
    st = rd(0x0014);
    l = rd(st + 0x2A5C);
    wd(0x0024, l);
    if (l != 0) {
        wd(0x0000, l);
        ww(l + 0x12, rw(l + 0x22));
        wd(l + 0x16, 0);
    }
    st = rd(0x0014);                                    /* CODE:2B9DE */
    ww(st + 0x0D3A, (uint16_t)(rw(st + 0x0D3A) + 1));
    ww(st + 0x8E, 7);
}

/* CODE:2BAD5: GAME_PHASE 7, a lost ball served again under the ball
 * save: frames of play without the flippers' keys and the display's
 * streams, "DON'T MOVE" (CODE:2BB2B), until the served ball has left the
 * lane (state+0D32h not 0, state+0D3Ch 0: GAME_PHASE 4) */
static void SAVE_SERVE(void)
{
    uint32_t st;

    FRAME_STEP();
    PLAY_STEP();
    BALLS_PHYSICS();
    PLAY_EVENTS();
    PLAY_KEYS();
    DM_CLEAR();
    wd(0x0000, 0x2BB2B);
    DM_TEXT_DRAW();
    PLAYERS_KEYS();
    st = rd(0x0014);
    if (rw(st + 0x0D32) != 0 && rb(st + 0x0D3C) == 0)
        ww(st + 0x8E, 4);
}

/* CODE:2B716: GAME_PHASE 9, the tilt: state+2A7Fh 0 (the flippers go
 * down and stay down), state+0D3Ah 0, a frame of play without the keys
 * and the display's streams, "TILT" (CODE:2B761); a lost ball to
 * LOST_RUNOUT */
static void TILT(void)
{
    uint32_t st = rd(0x0014);

    wb(st + 0x2A7F, 0);
    ww(st + 0x0D3A, 0);
    FRAME_STEP();
    PLAY_STEP();
    BALLS_PHYSICS();
    if (BALLS_LOST()) {
        LOST_RUNOUT();
        return;
    }
    PLAY_EVENTS();
    DM_CLEAR();
    wd(0x0000, 0x2B761);
    DM_TEXT_DRAW();
}

/* CODE:2C037, host vector +0Ch: [0020] fiftieths of a second of frames
 * (x FRAME_RATE / 50, CODE:2C0C3) with the scroll, the flippers, the
 * flashes and the lights stepped; after the first half second
 * (CODE:2C0C5) a key (LAST_KEY) ends it; [0010] kept */
void FRAMES_WAIT(void)
{
    uint32_t keep10 = rd(0x0010), st = rd(0x0014);
    uint16_t n;

    ww(0x2C0C3, (uint16_t)((uint32_t)rw(0x0020) * rw(st + 0x50) / 0x32));
    ww(0x2C0C5, (uint16_t)(rw(st + 0x50) >> 1));
    for (;;) {
        FRAME_STEP();
        PLAY_SCROLL();
        /* CODE:156C3, a RET */
        FLIPPERS_STEP();
        FLASH_STEP();
        LIGHTS_STEP();
        if (rw(0x2C0C5) != 0)
            ww(0x2C0C5, (uint16_t)(rw(0x2C0C5) - 1));
        else if (rb(N_LAST_KEY) != 0)
            break;
        n = (uint16_t)(rw(0x2C0C3) - 1);
        ww(0x2C0C3, n);
        if (n == 0)
            break;
    }
    wd(0x0010, keep10);
}

/* CODE:2BD95: the bonus at a lost ball (nothing while the tilt,
 * state+2A75h, is set): the player's bonus (the number ending at +10h)
 * to the one ending at state+2AD0h once, or its multiplier (word +12h)
 * times; a frame; the table module's slot 32 (state+2926h) with the
 * host vector CODE:2CD10 in [0010], which shows it and leaves the total
 * in the number ending at state+2AC8h; that added to the player's score
 * (ending at +8), "PLAYER n" and the score drawn; with more than one
 * player 1.5 s of frames (FRAMES_WAIT) */
static void BONUS_ADD(void)
{
    uint32_t st = rd(0x0014), pl, v;
    uint16_t n;

    if (rb(st + 0x2A75) == 0) {
        wd(st + 0x2AC0, 0);
        wd(st + 0x2AC4, 0);
        pl = rd(st + 0x0D76);
        wd(0x0000, pl);
        n = rw(pl + 0x12);
        ww(0x0020, n);
        if (n != 0)
            ww(0x0020, (uint16_t)(n - 1));
        st = rd(0x0014);
        wd(st + 0x2AC8, 0);
        wd(st + 0x2ACC, 0);
        do {
            /* CODE:2BE01 */
            wd(0x0004, rd(0x0014) + 0x2AD0);
            wd(0x0008, rd(0x0000) + 0x10);
            bcd12_add(rd(0x0004), rd(0x0008));
            wd(0x0008, rd(0x0008) - 6);
            wd(0x0004, rd(0x0004) - 6);
            n = rw(0x0020);
            ww(0x0020, (uint16_t)(n - 1));
        } while (n != 0);
        FRAME_STEP();
        PLAY_SCROLL();
        /* CODE:156C3, a RET */
        FLIPPERS_STEP();
        FLASH_STEP();
        LIGHTS_STEP();
        DM_CLEAR();
        wb(N_LAST_KEY, 0);
        wd(0x0010, 0x2CD10);
        MOD_CALL(rd(rd(0x0014) + 0x2926));
        DM_CLEAR();
        st = rd(0x0014);
        pl = rd(st + 0x0D76);
        wd(0x0000, pl);
        wd(0x0004, pl + 8);
        wd(0x0008, st + 0x2AC8);
        bcd12_add(rd(0x0004), rd(0x0008));
        wd(0x0008, rd(0x0008) - 6);
        wd(0x0004, rd(0x0004) - 6);
        /* "PLAYER n" (CODE:2A95A, its digit at CODE:2A965) */
        st = rd(0x0014);
        v = (rd(0x0020) & 0xFFFF0000u) | rw(st + 0x0D72);
        v = (v & 0xFFFFFF00u) | (uint8_t)(v + 0x31);
        wd(0x0020, v);
        wb(0x2A965, (uint8_t)v);
        wd(0x0000, 0x2A95A);
        DM_TEXT_DRAW();
        st = rd(0x0014);
        wd(0x0000, rd(st + 0x0D76) + 8);
        ww(0x002C, 0x140);
        ww(0x0030, 2);
        ww(0x0034, 1);
        ww(0x0038, 1);
        DM_SCORE_DRAW();
    }
    /* CODE:2C018 */
    if (rw(rd(0x0014) + 0x0D70) != 1) {
        ww(0x0020, 0x4B);
        FRAMES_WAIT();
    }
}

/* CODE:2BBC6: GAME_PHASE 5, a ball lost: BONUS_ADD; an extra ball (the
 * player's byte +10h) taken (GAME_PHASE 8), else the next player (the
 * player count state+0D70h), after the last one the next ball
 * (state+0D38h) or, when the balls (state+0D36h) are out, LIGHTS_RESET
 * and GAME_PHASE 3; the next ball's resets; with an extra ball still
 * held the first light state of header slot 25 (state+290Ah; its second
 * is the ball save's lamp) gets byte +5 FFh; the ball waiting (state+0D3Ch), the sound CODE:1009A and the
 * table module's slot 41 (state+294Ah) */
static void BALL_END(void)
{
    uint32_t st = rd(0x0014), pl, e;
    uint16_t di, n;

    wb(st + 0x92, 0xFF);
    wb(st + 0x2A7F, 0);
    BONUS_ADD();
    st = rd(0x0014);
    wb(st + 0x2A75, 0);
    ww(st + 0x2A78, 0);
    pl = rd(st + 0x0D76);
    wd(0x0000, pl);
    if (rb(pl + 0x10) != 0) {
        wb(pl + 0x10, (uint8_t)(rb(pl + 0x10) - 1));
        ww(st + 0x8E, 8);
    } else {
        /* CODE:2BC19 */
        wb(st + 0x0D30, 0);
        di = (uint16_t)(rw(st + 0x0D72) + 1);
        ww(0x0020, di);
        ww(st + 0x0D74, (uint16_t)(rw(st + 0x0D74) << 1));
        if (di >= rw(st + 0x0D70)) {
            wd(0x0020, 0);
            ww(st + 0x0D74, 1);
            n = (uint16_t)(rw(st + 0x0D36) - 1);
            ww(st + 0x0D36, n);
            if (n == 0) {
                /* CODE:2BD79 */
                LIGHTS_RESET();
                st = rd(0x0014);
                ww(st + 0x8E, 3);
                wb(st + 0x2A7F, 0);
                return;
            }
            ww(st + 0x0D38, (uint16_t)(rw(st + 0x0D38) + 1));
        }
        /* CODE:2BC82 */
        di = rw(0x0020);
        ww(st + 0x0D72, di);
        wd(0x0020, (uint32_t)di * 0x16);
        e = st + sx16((uint16_t)(di * 0x16)) + 0x0D7A;
        wd(0x0000, e);
        wd(st + 0x0D76, e);
        ww(st + 0x8E, 6);
    }
    /* CODE:2BCC0 */
    BALLS_RESET();
    LIGHTS_BALL_RESET();
    COUNTERS_BALL_RESET();
    RECORDS_BALL_RESET();
    DISPLAY_RESET();
    DROPS_UP_BALL();
    BONUS_CLEAR();
    if (rb(rd(0x0000) + 0x10) != 0) {
        e = rd(rd(rd(0x0014) + 0x290A));
        wd(0x0020, e);
        if (e != 0) {
            wd(0x0004, e);
            wb(e + 5, 0xFF);
        }
    }
    /* CODE:2BD15 */
    st = rd(0x0014);
    ww(st + 0x0D32, 1);
    wb(st + 0x0D3C, 0xFF);
    ww(st + 0x0D3E, 0);
    wb(st + 0x0D3D, 0);
    ww(st + 0x0D3A, 0);
    wb(st + 0x92, 0);
    wd(0x0000, 0x1009A);
    SFX_PLAY();
    MOD_CALL(rd(rd(0x0014) + 0x294A));
    wb(rd(0x0014) + 0x2A7F, 0xFF);
}

/* CODE:2BB3E: GAME_PHASE 8, an extra ball: the skill shot armed
 * (state+0D2Fh), header slot 34's music record; then frames of the
 * physics, the zones, the serve and "EXTRA BALL" (CODE:2BBB2) until the
 * ball leaves (state+0D3Ch 0: GAME_PHASE 4). No ball save is set here */
static void EXTRA_BALL(void)
{
    uint32_t st = rd(0x0014);

    wb(st + 0x0D2F, 0xFF);
    wd(0x0000, rd(st + 0x292E));
    MUSIC_REQUEST();
    do {
        FRAME_STEP();
        PLAY_STEP();
        BALLS_PHYSICS();
        ZONES_CHECK();
        SERVE();
        FLASH_STEP();
        LIT_LIST_STEP();
        PLAYERS_KEYS();
        DM_CLEAR();
        wd(0x0000, 0x2BBB2);
        DM_TEXT_DRAW();
        st = rd(0x0014);
    } while (rb(st + 0x0D3C) != 0);
    ww(st + 0x8E, 4);
}

/* CODE:2B012: [0020]'s low word frames (CODE:2B03E counts) of the
 * scroll and the flippers */
static void OVER_FRAMES(void)
{
    uint16_t n;

    ww(0x2B03E, rw(0x0020));
    do {
        FRAME_STEP();
        PLAY_SCROLL();
        /* CODE:156C3, a RET */
        FLIPPERS_STEP();
        n = (uint16_t)(rw(0x2B03E) - 1);
        ww(0x2B03E, n);
    } while (n != 0);
}

/* CODE:2AFF5: [0020]'s low word seconds of OVER_FRAMES (x FRAME_RATE) */
static void OVER_WAIT(void)
{
    wd(0x0020, (uint32_t)rw(0x0020) * rw(rd(0x0014) + 0x50));
    OVER_FRAMES();
}

/* CODE:2ABA4: the player's score ([0020]'s low word, [0024]) goes into
 * the high-score entry [0004], [0038] the entries below it (moved down
 * one, the last one lost); header slot 37's music record the first time
 * in a game over (CODE:2ADF0); "PLAYER n GOT A HIGHSCORE" and three
 * letters typed (KEY_CHARS by LAST_KEY; Backspace, state+0E54h, takes
 * one back; Enter, state+0E62h, ends with those typed) into CODE:2B09D;
 * [0038], [003C], [0000] and [0004] kept across it */
static void HISCORE_ENTER(void)
{
    uint32_t st = rd(0x0014), s0, s24, s20, a, v, c;
    uint32_t k38, k3c, k0, k4;
    uint16_t si, n;

    wb(st + 0x93, 0xFF);
    if (rb(0x2ADF0) == 0) {
        s0 = rd(0x0000);
        s24 = rd(0x0024);
        s20 = rd(0x0020);
        wd(0x0000, rd(st + 0x293A));
        MUSIC_REQUEST();
        wd(0x0020, s20);
        wd(0x0024, s24);
        wd(0x0000, s0);
    }
    /* CODE:2ABF5 */
    wb(0x2ADF0, 0xFF);
    si = rw(0x0038);
    ww(0x0034, si);
    if (si != 0) {
        v = (uint32_t)si * 10;
        wd(0x0034, v);
        wd(0x0008, rd(0x0004) + sx16((uint16_t)v));
        ww(0x0038, (uint16_t)(si - 1));
        do {
            /* CODE:2AC3E */
            a = rd(0x0008);
            wd(a, rd(a - 10));
            ww(a + 4, rw(a - 6));
            wd(a + 6, rd(a - 4));
            wd(0x0008, a - 10);
            n = rw(0x0038);
            ww(0x0038, (uint16_t)(n - 1));
        } while (n != 0);
    }
    /* CODE:2AC72 */
    a = rd(0x0004);
    ww(a + 4, rw(0x0020));
    wd(a + 6, rd(0x0024));
    st = rd(0x0014);
    v = (rd(0x0020) & 0xFFFF0000u) | rw(st + 0x0D72);
    v = (v & 0xFFFFFF00u) | (uint8_t)(v + 0x30);
    wd(0x0020, v);
    wb(0x2B04F, (uint8_t)v);
    wb(0x2B079, (uint8_t)v);
    k4 = a;
    k0 = rd(0x0000);
    k3c = rd(0x003C);
    k38 = rd(0x0038);
    wd(0x0020, 3);
    OVER_WAIT();
    DM_CLEAR();
    wd(0x0000, 0x2B040);
    DM_TEXT_DRAW();
    wd(0x0000, 0x2B058);
    DM_TEXT_DRAW();
    wd(0x0020, 3);
    OVER_WAIT();
    ww(0x2AFE0, 0);
    wb(0x2B09D, 0x20);
    wb(0x2B09E, 0x20);
    wb(0x2B09F, 0x20);
    DM_CLEAR();
    wd(0x0000, 0x2B06A);
    DM_TEXT_DRAW();
    wd(0x0000, 0x2B07C);
    DM_TEXT_DRAW();
    wb(N_LAST_KEY, 0);
    for (;;) {
        /* CODE:2AD59 */
        wd(0x0000, 0x2B094);
        DM_TEXT_DRAW();
        st = rd(0x0014);
        if (rb(st + 0x0E54) != 0) {
            wb(st + 0x0E54, 0);
            n = rw(0x2AFE0);
            ww(0x0020, n);
            if (n != 0) {
                ww(0x2AFE0, (uint16_t)(n - 1));
                wd(0x0010, 0x2B09D);
                wb(0x2B09D + sx16(n) - 1, 0x20);
            }
        }
        /* CODE:2ADB9 */
        st = rd(0x0014);
        if (rb(st + 0x0E62) != 0) {
            wb(st + 0x0E62, 0);
            break;
        }
        /* CODE:2AE19 */
        wd(0x0020, 0);
        c = rb(N_LAST_KEY);
        wd(0x0020, c);
        if (c != 0) {
            c = rb(N_KEY_CHARS + c);
            wd(0x0020, c);
            if (!(c & 0x80)) {
                wd(0x0010, 0x2B09D);
                n = rw(0x2AFE0);
                ww(0x0028, n);
                wb(0x2B09D + sx16(n), (uint8_t)c);
                ww(0x2AFE0, (uint16_t)(n + 1));
            }
        }
        /* CODE:2AE79 */
        wb(N_LAST_KEY, 0);
        wd(0x0020, 1);
        OVER_FRAMES();
        DM_CLEAR();
        wd(0x0000, 0x2B06A);
        DM_TEXT_DRAW();
        wd(0x0000, 0x2B07C);
        DM_TEXT_DRAW();
        if (rw(0x2AFE0) >= 3) {
            wd(0x0000, 0x2B094);
            DM_TEXT_DRAW();
            wd(0x0020, 3);
            OVER_WAIT();
            break;
        }
    }
    wd(0x0038, k38);
    wd(0x003C, k3c);
    wd(0x0000, k0);
    wd(0x0004, k4);
}

/* CODE:2AA90: GAME_PHASE 3, game over: header slot 36's music record,
 * the current player's score; each player's score (the player record's
 * dwords +0 and +4, as its word +0 and dword +4) against the table's
 * five high scores (TABLE_HISCORES, state+0CFCh, 10 bytes: the initials,
 * a 0, the score's word, its dword), at or above one HISCORE_ENTER. The
 * initials of CODE:2B09D go into the entry [0004] also when no entry was
 * passed: [0004] is then state+0D2Eh (MULTIBALL_ON and the next two
 * bytes); with pi_fix_gameover_names that write is left out. Then the players' scores in the attract mode (state+0E34h
 * FFh, the count state+0E2Ah and CODE:2A928, twice: CODE:2A92A) and
 * GAME_PHASE 1 */
static void GAME_OVER(void)
{
    uint32_t st = rd(0x0014), e, p, ent;
    uint16_t n;

    e = rd(st + 0x2936);
    wd(0x0020, e);
    if (e != 0) {
        wd(0x0000, e);
        MUSIC_REQUEST();
    }
    DM_CLEAR();
    st = rd(0x0014);
    e = rd(st + 0x0D76);
    wd(0x0000, e);
    wd(0x0000, e + 8);
    ww(0x002C, 0xA0);
    wd(0x0030, 0);
    wd(0x0034, 0);
    wd(0x0038, 2);
    DM_SCORE_DRAW();
    st = rd(0x0014);
    ww(st + 0x0D72, 1);
    ww(0x003C, rw(st + 0x0D70));
    ww(0x003C, (uint16_t)(rw(0x003C) - 1));
    wd(0x0000, st + 0x0D7A);
    wb(0x2ADF0, 0);
    do {
        /* CODE:2AB48 */
        p = rd(0x0000);
        wd(0x0020, rd(p));
        wd(0x0024, rd(p + 4));
        wd(0x0004, rd(0x0014) + 0x0CFC);
        wd(0x0038, 4);
        for (;;) {
            /* CODE:2AB7D */
            ent = rd(0x0004);
            if (rw(0x0020) > rw(ent + 4)
                || (rw(0x0020) == rw(ent + 4) && rd(0x0024) >= rd(ent + 6))) {
                HISCORE_ENTER();
                break;
            }
            /* CODE:2AF01 */
            wd(0x0004, ent + 10);
            n = rw(0x0038);
            ww(0x0038, (uint16_t)(n - 1));
            if (n == 0)
                break;
        }
        /* CODE:2AF26 */
        ent = rd(0x0004);
        if (!pi_fix_gameover_names || ent < st + 0x0D2E) {
            wb(ent, rb(0x2B09D));
            wb(ent + 1, rb(0x2B09E));
            wb(ent + 2, rb(0x2B09F));
        }
        st = rd(0x0014);
        ww(st + 0x0D72, (uint16_t)(rw(st + 0x0D72) + 1));
        wd(0x0000, rd(0x0000) + 0x16);
        n = rw(0x003C);
        ww(0x003C, (uint16_t)(n - 1));
    } while (n != 0);
    st = rd(0x0014);
    ww(st + 0x0E2A, rw(st + 0x0D70));
    ww(0x2A928, rw(st + 0x0D70));
    ww(0x2A92A, 2);
    ww(st + 0x0D70, 0);
    ww(st + 0x0D72, 0);
    wb(st + 0x0E34, 0xFF);
    ww(st + 0x0E36, 0x64);
    wb(st + 0x0E35, 1);
    ww(st + 0x8E, 1);
    wb(st + 0x92, 0);
}

/* CODE:2A8AB: Esc in the attract mode (its key and Y's, KEY_DOWN+1 and
 * +15h, and LAST_KEY cleared): frames with "REALLY QUIT TABLE?"
 * (CODE:2A93E) on the display and no display stream, until Y (QUIT_TABLE,
 * state+8Dh, FFh) or another key (the display cleared, state+0E35h 1:
 * the attract record queued again) */
static void ATTRACT_QUIT(void)
{
    uint32_t st = rd(0x0014);

    wb(st + 0x0E47, 0);
    wb(st + 0x0E5B, 0);
    wb(N_LAST_KEY, 0);
    for (;;) {
        FRAME_STEP();
        ATTRACT_SCROLL();
        /* CODE:156C3, a RET */
        FLIPPERS_STEP();
        DM_CLEAR();
        wd(0x0000, 0x2A93E);
        DM_TEXT_DRAW();
        st = rd(0x0014);
        if (rb(st + 0x0E5B) != 0) {
            wb(st + 0x8D, 0xFF);
            return;
        }
        if (rb(N_LAST_KEY) != 0)
            break;
    }
    DM_CLEAR();
    wb(rd(0x0014) + 0x0E35, 1);
}

/* CODE:2A7FD: Esc (state+0E47h) to CODE:2A8AB; else the first of the
 * keys F8..F1 (state+0E88h down to +0E81h) set, or keypad Enter
 * (state+0EE2h) as F1: cleared, GAME_PHASE 2 and its number (1..8) added
 * to state+0D70h */
static void ATTRACT_KEYS(void)
{
    uint32_t st = rd(0x0014);
    uint16_t si;

    if (rb(st + 0x0E47) != 0) {
        ATTRACT_QUIT();
        return;
    }
    wd(0x0020, 7);
    wd(0x0000, st + 0x0E81);
    for (si = 7; ; si--) {
        if (rb(rd(0x0000) + sx16(si)) != 0) {
            wb(rd(0x0000) + sx16(si), 0);
            break;
        }
        wd(0x0020, (uint16_t)(si - 1));
        if (si == 0) {
            wd(0x0020, 0);
            st = rd(0x0014);
            if (rb(st + 0x0EE2) == 0)
                return;
            wb(st + 0x0EE2, 0);
            break;
        }
    }
    /* CODE:2A886 */
    st = rd(0x0014);
    ww(st + 0x8E, 2);
    ww(0x0020, (uint16_t)(rw(0x0020) + 1));
    ww(st + 0x0D70, (uint16_t)(rw(st + 0x0D70) + rw(0x0020)));
}

/* CODE:2A6FA: the attract mode's high-score pages, one of the table's
 * five entries (TABLE_HISCORES, state+0CFCh, 10 bytes each) a page for
 * 3 x FRAME_RATE frames (CODE:2A6E6), CODE:2A6E4 the entry: the display
 * cleared, the entry's number drawn centred on the second line, its
 * place and initials (the text record CODE:2A968, "1 ICE" and so on)
 * from the left; after the fifth state+0E35h 1, the attract record next */
static void HISCORE_PAGES(void)
{
    uint32_t st, ecx, ebx, esi;
    uint16_t si;

    if (rw(0x2A6E6) != 0) {
        /* CODE:2A7D4 */
        ww(0x2A6E6, (uint16_t)(rw(0x2A6E6) - 1));
        if (rw(0x2A6E6) != 0)
            return;
        ww(0x2A6E4, (uint16_t)(rw(0x2A6E4) + 1));
        if (rw(0x2A6E4) < 5)
            return;
        wb(rd(0x0014) + 0x0E35, 1);
        return;
    }
    DM_CLEAR();
    si = rw(0x2A6E4);
    ebx = (rd(0x0024) & 0xFFFF0000u) | (si & 0xFF00u) | (uint8_t)(si + 0x31);
    wd(0x0024, ebx);
    wb(0x2A970, (uint8_t)ebx);
    esi = (uint32_t)si * 10;
    wd(0x0020, esi);
    ecx = rd(0x0014) + sx16((uint16_t)esi) + 0x0CFC;
    wb(0x2A972, rb(ecx));
    wb(0x2A973, rb(ecx + 1));
    wb(0x2A974, rb(ecx + 2));
    wd(0x0000, ecx + 10);
    ww(0x002C, 0x140);
    wd(0x0030, 2);
    wd(0x0034, 1);
    wd(0x0038, 1);
    DM_HISCORE_DRAW();
    wd(0x0000, 0x2A968);
    DM_TEXT_DRAW();
    st = rd(0x0014);
    esi = 3u * rw(st + 0x50);
    wd(0x0020, esi);
    ww(0x2A6E6, (uint16_t)esi);
}

/* CODE:2A557: after a game over (state+0E34h not 0) the attract mode
 * shows, in place of its display stream, "GAME OVER" (CODE:2A92C) while
 * state+0E34h is negative and player n's score ("PLAYER n", state+0D72h)
 * while positive, each for 100 frames (state+0E36h); the sign flips at
 * each change, the player advanced at each "GAME OVER"; after the
 * players (state+0E2Ah) the round again from player 1, twice
 * (CODE:2A92A; CODE:2A928 the count), then state+0E34h 0 */
static void ATTRACT_SCORES(void)
{
    uint32_t st = rd(0x0014), v;
    uint16_t n;
    uint8_t b;

    DM_CLEAR();
    if (rb(st + 0x0E34) & 0x80) {
        wd(0x0000, 0x2A92C);
        DM_TEXT_DRAW();
    } else {
        /* CODE:2A573 */
        st = rd(0x0014);
        v = (rd(0x0020) & 0xFFFF0000u) | rw(st + 0x0D72);
        v = (v & 0xFFFFFF00u) | (uint8_t)(v + 0x31);
        wd(0x0020, v);
        wb(0x2A965, (uint8_t)v);
        wd(0x0000, 0x2A95A);
        DM_TEXT_DRAW();
        st = rd(0x0014);
        v = (uint32_t)rw(st + 0x0D72) * 0x16;
        wd(0x0020, v);
        wd(0x0000, st + 0x0D7A + sx16((uint16_t)v) + 8);
        ww(0x002C, 0x140);
        ww(0x0030, 2);
        ww(0x0034, 1);
        ww(0x0038, 1);
        DM_SCORE_DRAW();
    }
    /* CODE:2A607 */
    st = rd(0x0014);
    n = (uint16_t)(rw(st + 0x0E36) - 1);
    ww(st + 0x0E36, n);
    if (n != 0)
        return;
    b = (uint8_t)-rb(st + 0x0E34);
    wb(st + 0x0E34, b);
    if (b & 0x80) {
        ww(st + 0x0D72, (uint16_t)(rw(st + 0x0D72) + 1));
        n = (uint16_t)(rw(st + 0x0E2A) - 1);
        ww(st + 0x0E2A, n);
        if (n == 0) {
            /* CODE:2A649 */
            n = (uint16_t)(rw(0x2A92A) - 1);
            ww(0x2A92A, n);
            if (n == 0) {
                wb(st + 0x0E34, 0);
                return;
            }
            ww(st + 0x0E36, 0x64);
            ww(st + 0x0E2A, rw(0x2A928));
            ww(st + 0x0D72, 0);
            return;
        }
    }
    /* CODE:2A635 */
    ww(st + 0x0E36, 0x64);
}

/* CODE:2A4F8: GAME_PHASE 1, the attract mode, a frame */
static void ATTRACT(void)
{
    uint32_t st = rd(0x0014);

    wb(st + 0x2A7F, 0);
    ww(st + 0x0D70, 0);
    if (rb(st + 0x0EC5) != 0) {
        wb(st + 0x0EC5, 0);
        /* CODE:26A6D, a RET */
        wb(N_LAST_KEY, 0);
    }
    FRAME_STEP();
    ATTRACT_SCROLL();
    /* CODE:156C3, a RET */
    FLIPPERS_STEP();
    st = rd(0x0014);
    if (rb(st + 0x0E34) != 0) {
        ATTRACT_SCORES();
        ATTRACT_KEYS();
        return;
    }
    /* CODE:2A690 */
    DISPLAY_RUN();
    ANIMS_STEP();
    st = rd(0x0014);
    if (rb(st + 0x0E35) & 0x80) {
        HISCORE_PAGES();
    } else if (rb(st + 0x0E35) != 0) {
        /* the attract mode's display record (state+292Ah) queued */
        wb(st + 0x0E35, 0);
        wd(0x0000, rd(st + 0x292A));
        DISPLAY_QUEUE();
    } else if (rd(st + 0x2A2E) == 0) {
        wb(st + 0x0E35, 0xFF);
        ww(0x2A6E4, 0);
        ww(0x2A6E6, 0);
        HISCORE_PAGES();
    }
    ATTRACT_KEYS();
}

void TABLE_GAME(void)
{
    DISPLAY_RESET();
    LIGHTS_RESET();
    DM_CLEAR();
    SCREEN_START();
    ATTRACT_SCROLL();
    FLIPPERS_DRAW();
    FLIPPERS_STEP();
    LIGHTS_STEP();
    FLASH_STEP();
    TABLE_FADE_IN();
    wb(0xD973, 1);             /* CODE:D973, not followed */
    ww(N_GAME_PHASE, 1);
    wb(N_QUIT_TABLE, 0);
    GAME_SOUND();
    /* CODE:B976 */
    while (rb(N_QUIT_TABLE) == 0) {
        static char name[16];
        uint16_t ph = rw(N_GAME_PHASE);

        if (ph >= 0x0A)
            pi_stop("CODE:BA7D");
        if (ph == 1) {
            ATTRACT();
            continue;
        }
        if (ph == 2) {
            GAME_START();
            continue;
        }
        if (ph == 6) {
            BALL_WAIT();
            continue;
        }
        if (ph == 4) {
            PLAY();
            continue;
        }
        if (ph == 7) {
            SAVE_SERVE();
            continue;
        }
        if (ph == 5) {
            BALL_END();
            continue;
        }
        if (ph == 3) {
            GAME_OVER();
            continue;
        }
        if (ph == 8) {
            EXTRA_BALL();
            continue;
        }
        if (ph == 9) {
            TILT();
            continue;
        }
        snprintf(name, sizeof name, "CODE:%X", (unsigned)rd(N_PHASES + ph * 4u));
        pi_stop(name);
    }
}
