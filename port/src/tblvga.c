/* tblvga.c - TBL_VGA_INIT (CODE:911F): the table's display mode (planar,
 * by OPT_RESOLUTION), FRAME_RATE measured, the stage's picture drawn
 * into video memory and into the "Spooky" block.
 */
#include "frame.h"
#include "game.h"
#include "names.h"
#include "pmax.h"
#include "pmem.h"
#include "vga.h"

/* CODE:3D389: the sequencer's map mask */
static void MAP_MASK(uint8_t ah)
{
    vga_outw(0x3C4, (uint16_t)(2 | ah << 8));
}

/* the CRTC's register `i` with `and` and `or` applied */
static void crtc_mod(uint8_t i, uint8_t and, uint8_t or)
{
    vga_outb(0x3D4, i);
    vga_outb(0x3D5, (uint8_t)((vga_inb(0x3D5) & and) | or));
}

/* the sequencer's register 4 AND F6h (chain-4 off, odd/even on), CRTC 14h
 * bit 6 off (no doubleword mode), CR 17h E3h (byte mode): unchained */
static void unchain(void)
{
    vga_outw(0x3D4, 0x0011);
    vga_outb(0x3C4, 4);
    vga_outb(0x3C5, (uint8_t)(vga_inb(0x3C5) & 0xF6));
    crtc_mod(0x14, 0xBF, 0);
    vga_outw(0x3D4, 0xE317);
}

/* CODE:3D39F: mode 13h (INT 10h), unchained */
static void VGA_UNCHAINED(void)
{
    vga_set_mode(0x13);
    unchain();
}

/* CODE:3D3D4: CR 11h's protection off, the ten CRTC words at
 * CRTC_WORDS, the Misc register's sync polarity bits 6 and 7 set */
static void CRTC_WORDS_SET(void)
{
    int i;

    crtc_mod(0x11, 0x7F, 0);
    for (i = 0; i < 10; i++)
        vga_outw(0x3D4, rw(N_CRTC_WORDS + 2 * (uint32_t)i));
    vga_outb(0x3C2, (uint8_t)(vga_inb(0x3CC) | 0xC0));
}

/* CODE:9444: 360x350 timings (the 28 MHz clock, Misc A7h) */
static void VGA360_REGS(void)
{
    static const uint8_t crtc[][2] = {
        { 0x00, 0x6B }, { 0x01, 0x59 }, { 0x02, 0x5A }, { 0x03, 0x8E }, { 0x04, 0x5C },
        { 0x05, 0x88 }, { 0x06, 0xBF }, { 0x07, 0x1F }, { 0x09, 0x40 }, { 0x10, 0x83 },
        { 0x12, 0x5D }, { 0x13, 0x2A }, { 0x14, 0x0F }, { 0x15, 0x63 }, { 0x16, 0xBA },
        { 0x17, 0xE3 }
    };
    uint8_t al;
    size_t i;

    vga_outb(0x3C4, 4);
    vga_outb(0x3C5, 6);
    vga_outb(0x3C4, 2);
    vga_outb(0x3C5, 0x0F);
    vga_outb(0x3C2, 0xA7);
    vga_outb(0x3D4, 0x11);
    al = (uint8_t)(vga_inb(0x3D5) & 7);
    vga_outb(0x3D4, 0x11);
    vga_outb(0x3D5, al);
    for (i = 0; i < sizeof crtc / sizeof crtc[0]; i++) {
        vga_outb(0x3D4, crtc[i][0]);
        vga_outb(0x3D5, crtc[i][1]);
    }
    vga_outw(0x3D4, 0x5301);
    vga_outb(0x3D4, 0x11);
    vga_outb(0x3D5, 0xA5);
}

/* CODE:3D320: the line compare (the split) at CX - 1 */
static void TBL_SPLIT_SET(uint16_t cx)
{
    uint8_t cl = (uint8_t)cx, ch = (uint8_t)(cx >> 8);

    crtc_mod(0x11, 0x7F, 0);
    vga_outw(0x3D4, (uint16_t)(0x18 | (uint8_t)(cl - 1) << 8));
    crtc_mod(0x07, 0xEF, (uint8_t)((ch & 1) << 4));
    crtc_mod(0x09, 0xBF, (uint8_t)((ch & 2) << 5));
}

/* the cells each mode routine sets after the split */
static void mode_cells(uint16_t d890, uint16_t d892, uint32_t b8e, uint32_t b922, uint16_t rate)
{
    ww(N_TBL_D890, d890);
    ww(N_SCROLL_MAX, d892);
    wd(N_VSYNC_START_ARG, b8e);
    wd(N_TBL_B922, b922);
    ww(N_FRAME_RATE, rate);
}

/* CODE:96E7 */
static void MODE_VGA320(void)
{
    wb(N_MODE_SVGA800_FLAG, 0);
    VGA_UNCHAINED();
    CRTC_WORDS_SET();
    vga_outw(0x3D4, 0x2A13);
    TBL_SPLIT_SET(0x1A0);
    mode_cells(0x53, 0x188, 0xDDDD, 0, 0x3C);
}

/* CODE:9743 */
static void MODE_VGA360(void)
{
    wb(N_MODE_SVGA800_FLAG, 0);
    VGA_UNCHAINED();
    CRTC_WORDS_SET();
    VGA360_REGS();
    vga_outw(0x3D4, 0x2A13);
    TBL_SPLIT_SET(0x13E);
    mode_cells(0x7F, 0x11A, 0xE898, 2, 0x46);
}

/* CODE:9265: one picture timed with PIT channel 0 (mode 0 from 0); FRAME_RATE
 * 1234DCh / the ticks, at most 3Dh.  The port takes the ticks of one
 * picture of the mode set (vga_refresh_hz, as the retraces are timed);
 * the two retraces it waits for are two pictures. */
static void MEASURE_RATE(void)
{
    uint32_t ticks, ax;

    frame_wait();
    frame_wait();
    ticks = (uint32_t)(1193182.0 / vga_refresh_hz() + 0.5) & 0xFFFF;
    if (!ticks)
        pi_stop("MEASURE_RATE: no ticks");
    ax = 0x1234DCu / ticks;
    if (ax > 0xFFFF)
        pi_stop("MEASURE_RATE: DIV overflow");
    ww(N_FRAME_RATE, (uint16_t)(ax > 0x3D ? 0x3D : ax));
}

/* CODE:92FA: 300h bytes at DS:`esi` to the DAC, each shifted right 2 */
static void DAC_LOAD(uint32_t esi)
{
    int i;

    vga_outb(0x3C8, 0);
    for (i = 0; i < 0x300; i++)
        vga_outb(0x3C9, (uint8_t)(rb(esi + (uint32_t)i) >> 2));
}

/* CODE:9392: a byte of eight rows 30h bytes apart at DS:`esi`, the
 * bit 7 - dl of each; the first row's bit to bit 0 */
static uint8_t STAGE_BITS(uint32_t esi, uint8_t dl)
{
    uint8_t al = 0;
    int k;

    for (k = 0; k < 8; k++, esi += 0x30) {
        uint8_t bh = (uint8_t)(rb(esi) << (dl & 0x1F));
        al = (uint8_t)(al >> 1 | (bh & 0x80));
    }
    return al;
}

/* the words of plane `dl` of the stage at DS:`ebp`, 258h lines of 2Ah,
 * each to `put` in turn */
static void stage_plane(uint32_t ebp, uint8_t dl, void (*put)(uint32_t *di, uint8_t v),
                        uint32_t *di)
{
    uint32_t esi = ebp;
    int line, col;

    for (line = 0; line < 0x258; line++) {
        for (col = 0; col < 0x2A; col++, esi++) {
            put(di, STAGE_BITS(esi, dl));
            put(di, STAGE_BITS(esi, (uint8_t)(dl ^ 4)));
        }
        esi += 0x156;
    }
}

static void put_vram(uint32_t *di, uint8_t v)
{
    vga_write((uint16_t)(*di)++, v);
}

static uint32_t spooky;

static void put_spooky(uint32_t *di, uint8_t v)
{
    lwb(spooky + (*di)++, v);
}

/* CODE:933F: the stage at DS:`ebp` into the four planes from 1500h, plane
 * by plane with STAGE_MASK (11h, rotated a bit a plane) */
static void STAGE_TO_VRAM(uint32_t ebp)
{
    uint8_t dl;

    for (dl = 0; dl < 4; dl++) {
        uint32_t di = 0x1500;
        MAP_MASK(rb(N_STAGE_MASK));
        stage_plane(ebp, dl, put_vram, &di);
        wb(N_STAGE_MASK, (uint8_t)(rb(N_STAGE_MASK) << 1 | rb(N_STAGE_MASK) >> 7));
    }
}

/* CODE:93FB: the same into SPOOKY_SEL's block, the planes one after the
 * other from 0 */
static void STAGE_TO_SPOOKY(uint32_t ebp)
{
    uint32_t di = 0;
    uint8_t dl;

    spooky = pmax_base(rw(N_SPOOKY_SEL));
    for (dl = 0; dl < 4; dl++) {
        stage_plane(ebp, dl, put_spooky, &di);
        wb(N_STAGE_MASK, (uint8_t)(rb(N_STAGE_MASK) << 1 | rb(N_STAGE_MASK) >> 7));
    }
}

/* 10000h bytes of video memory 0, the map mask as it is */
static void vram_clear(void)
{
    uint32_t i;

    for (i = 0; i < 0x10000; i++)
        vga_write((uint16_t)i, 0);
}

void TBL_VGA_INIT(void)
{
    uint32_t mode;

    frame_wait();
    DAC_LOAD(N_TBL_PALETTE);
    unchain();
    MAP_MASK(0x0F);
    vram_clear();
    mode = rd(N_MODE_ROUTINES + 4 * (uint32_t)rb(N_OPT_RESOLUTION));
    if (mode == N_MODE_VGA360)
        MODE_VGA360();
    else if (mode == N_MODE_VGA320)
        MODE_VGA320();
    else
        pi_stop("TBL_VGA_INIT: an SVGA mode (CODE:97A4, CODE:9822)");
    MEASURE_RATE();
    MAP_MASK(0x0F);
    vram_clear();
    DAC_LOAD(N_TBL_PALETTE);    /* the stage's palette, MODULE_HEADER+50h, read and not used */
    STAGE_TO_VRAM(rd(N_MODULE_HEADER + 0x4C));
    MAP_MASK(0x0F);
    pmax_policy(0);
    pmax_name(0x921D, rw(N_TABLE_DS));       /* "Spooky" */
    ww(N_SPOOKY_SEL, pmax_alloc(0x32DC0));
    if (!rw(N_SPOOKY_SEL))
        pi_stop("TBL_VGA_INIT: no room for Spooky (its CF is not looked at)");
    STAGE_TO_SPOOKY(rd(N_MODULE_HEADER + 0x4C));
}
