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
static void FADE_MIX(uint32_t src, uint32_t dst, uint8_t cl)
{
    uint32_t b;

    for (b = N_TBL_FADE_PAL; b < N_TBL_PALETTE; b++)
        wb(b, (uint8_t)((rb(src++) * (0x20 - cl) + rb(dst++) * cl) / 0x20));
}

/* CODE:B02B: TBL_FADE_PAL to the DAC from colour 0, shifted right 2 */
static void FADE_PAL_SET(void)
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

/* CODE:29FFA: header slot 16's counters (state+28E6h): +30h, +34h from
 * +28h, +2Ch, the eight players' words +6 and +16h to the word +2, +38h,
 * +3Ch and +26h 0, byte +2 of each threshold (12 bytes from +50h, to a
 * negative word) 0; then header slot 17's lists (state+28EAh) of 8-byte
 * entries, to one whose word +2 is 100h or more: bit 1 of +0 cleared */
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
    /* CODE:2A0C2 */
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

/* CODE:28E5D: the state [0020] of the drop target [0000] into
 * DROP_STATES (by its word +44h) with the target */
static void DROP_SET(void)
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

/* CODE:2A976: GAME_PHASE 2, a game's start: the balls per game to
 * state+0D36h, the ball 0 (state+0D38h), player 0 and its bit 1, the
 * eight player records (state+0D7Ah, 16h bytes) cleared; the balls, the
 * lights, the counters, the slot-15 records, the drop targets and the
 * display reset; the sound at CODE:1009A, the module's slot 40
 * (state+2946h), the tilt count 0, GAME_PHASE 6 */
static void GAME_START(void)
{
    uint32_t st = rd(0x0014), e, hook;
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
    hook = rd(st + 0x2946);
    if (rb(hook) != 0xC3)
        pi_stop("GAME_START: the module's slot 40 is not a RET (CODE:2AA6D)");
    st = rd(0x0014);
    ww(st + 0x2A78, 0);
    ww(st + 0x8E, 6);
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
    if (rb(st + 0x0E34) != 0)
        pi_stop("CODE:2A557");
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
        snprintf(name, sizeof name, "CODE:%X", (unsigned)rd(N_PHASES + ph * 4u));
        pi_stop(name);
    }
}
