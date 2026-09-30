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
        pi_stop("SLINGS_STEP: a slingshot's picture (CODE:28E7B)");
    }
}

/* CODE:30996: a hole ejecting, else the next hole from the stack
 * state+2A64h (only the empty cases translated) */
static void HOLE_EJECT_STEP(void)
{
    uint32_t st = rd(0x0014), h = rd(st + 0x2A68);

    wd(0x0020, h);
    if (h != 0)
        pi_stop("HOLE_EJECT_STEP: a hole ejecting (CODE:309BB)");
    h = rd(st + 0x2A64);
    wd(0x0004, h);
    if (h != 0xFA48)
        pi_stop("HOLE_EJECT_STEP: a hole on the stack (CODE:30BF0)");
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

/* CODE:2B4EC: M (state+0E78h) flips CODE:9C4E's bit 0; P (state+0E5Fh)
 * the pause (not translated); state+91h FFh */
static void PLAY_KEYS(void)
{
    uint32_t st = rd(0x0014);

    if (rb(st + 0x0E78) != 0) {
        wb(st + 0x0E78, 0);
        wb(0x9C4E, (uint8_t)(rb(0x9C4E) ^ 1));
    }
    st = rd(0x0014);
    if (rb(st + 0x0E5F) != 0)
        pi_stop("PLAY_KEYS: the pause (CODE:2B51C)");
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
        snprintf(name, sizeof name, "CODE:%X", (unsigned)rd(N_PHASES + ph * 4u));
        pi_stop(name);
    }
}
