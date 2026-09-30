/* play.c - the game (CODE:B928): its start and main loop, as far as they
 * are translated.
 */
#include "frame.h"
#include "game.h"
#include "names.h"
#include "pmax.h"
#include "pmem.h"
#include "vga.h"

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
    pi_stop("CODE:B976");
}
