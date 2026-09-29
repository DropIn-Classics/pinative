/* intro.c - CHOOSER_LOAD (CODE:757D): the sound started, the chooser's and
 * the intro's files loaded, then the intro.
 */
#include "game.h"
#include "image.h"
#include "names.h"
#include "pmax.h"
#include "pmem.h"
#include "vga.h"

/* INT 94h AH=1 with an empty block name (the byte before each file name in
 * CODE, which goes only into pMAX's header); the original does not look at
 * CF, the port stops */
static uint16_t load(const char *name, uint32_t *size)
{
    uint16_t sel = pmax_load(name, size);

    if (!sel)
        pi_stop("CHOOSER_LOAD: a file not loaded");
    return sel;
}

/* CODE:732D: 30h bytes from `from` (linear) to CODE:`*di`, `bl` added to
 * each, at most 3Fh */
static void PAL_ADD(uint32_t from, uint32_t *di, uint8_t bl)
{
    uint32_t i;

    for (i = 0; i < 0x30; i++) {
        uint8_t al = (uint8_t)(lrb(from + i) + bl);
        wb((*di)++, al < 0x3F ? al : 0x3F);
    }
}

/* CODE:7341: INTRO_PALS from PCSKY.FLD's first 16 colours.  EDI comes
 * from CODE:7441's checksum (8D3Ch in the run, INTRO_PALS) */
static void INTRO_PALS_MAKE(void)
{
    static const uint8_t add[12] = { 0, 0, 0, 0, 0, 3, 0x0A, 0, 0x19, 0x23, 0x2D, 0x0F };
    uint32_t from = pmax_base(rw(N_PCSKY_SEL)), di = N_INTRO_PALS;
    int i;

    for (i = 0; i < 3; i++)
        PAL_ADD(from, &di, 0);
    for (i = 0; i < 0x30; i++)
        wb(di++, 0);
    for (i = 0; i < 12; i++)
        PAL_ADD(from, &di, add[i]);
}

/* CODE:698A: mode 13h, then unchained (sequencer memory mode 06h) with the
 * intro's own CRTC timing; all of video memory cleared */
static void INTRO_MODE(void)
{
    static const uint16_t crtc[] = {
        0x0E06, 0x3E07, 0x0109, 0xC510, 0x2C11, 0x2112, 0x9C15, 0x0016,
        0x0014, 0xE317
    };
    uint32_t i;

    vga_set_mode(0x13);
    vga_outb(0x3C0, 0);                 /* the screen off */
    vga_outb(0x3C2, 0xE3);
    vga_outb(0x3D4, 0x11);              /* CR 0-7 writable */
    vga_outb(0x3D5, vga_inb(0x3D5) & 0x7F);
    for (i = 0; i < sizeof crtc / sizeof crtc[0]; i++)
        vga_outw(0x3D4, crtc[i]);
    vga_outw(0x3C4, 0x0604);
    vga_outw(0x3C4, 0x0F02);
    for (i = 0; i < 0x10000; i++)
        vga_write((uint16_t)i, 0);
    vga_inb(0x3DA);
    vga_outb(0x3C0, 0x20);              /* the screen on */
    vga_inb(0x3DA);
}

/* the 0-0Fh of a 12-bit colour's component made 0-30h (MUL 30h, DIV 0Fh;
 * above 4Fh the original's DIV faults) */
static uint8_t c12(uint8_t v)
{
    if (v > 0x4F)
        pi_stop("INTROPIX_PALS: a divide error");
    return (uint8_t)(v * 0x30 / 0x0F);
}

/* CODE:6EBE: 32 colours of two bytes (red; green and blue in the high and
 * low nibble) from `from` (linear) made DAC colours at CODE:`*di`, which
 * then skips the row's other 32 */
static void PAL12_ROW(uint32_t from, uint32_t *di)
{
    int i;

    for (i = 0; i < 0x20; i++, from += 2) {
        uint8_t a = lrb(from), b = lrb(from + 1);
        wb((*di)++, c12(a));
        wb((*di)++, c12((uint8_t)(b >> 4)));
        wb((*di)++, c12(b & 0x0F));
    }
    *di += 0x60;
}

/* CODE:6EE9: as PAL12_ROW, each component halved and ch, dl, dh added */
static void PAL12_ROW_HALF(uint32_t from, uint32_t *di, uint8_t ch, uint8_t dl, uint8_t dh)
{
    int i;

    for (i = 0; i < 0x20; i++, from += 2) {
        uint8_t a = lrb(from), b = lrb(from + 1);
        wb((*di)++, (uint8_t)((c12(a) >> 1) + ch));
        wb((*di)++, (uint8_t)((c12((uint8_t)(b >> 4)) >> 1) + dl));
        wb((*di)++, (uint8_t)((c12(b & 0x0F) >> 1) + dh));
    }
    *di += 0x60;
}

/* CODE:6EAC: 64 colours of ch, dl, dh */
static void PAL_FILL(uint32_t *di, uint8_t ch, uint8_t dl, uint8_t dh)
{
    int i;

    for (i = 0; i < 0x40; i++) {
        wb((*di)++, ch);
        wb((*di)++, dl);
        wb((*di)++, dh);
    }
}

/* CODE:6FC0: INTRO_PAL1 and INTRO_PAL2 from intropix.mgl's 32 colours (at
 * its dword +18h).  EDI comes from CODE:7441's checksum (813Ch in the run,
 * INTRO_PAL1) */
static void INTROPIX_PALS(void)
{
    uint32_t base = pmax_base(rw(N_INTROPIX_SEL));
    uint32_t from = base + lrd(base + 0x18), di = N_INTRO_PAL1;
    int i;

    PAL12_ROW(from, &di);
    PAL_FILL(&di, 0x26, 0x20, 0x20);
    PAL_FILL(&di, 0x3F, 0x3F, 0x3F);
    PAL12_ROW_HALF(from, &di, 0x18, 0x12, 0x12);
    di = N_INTRO_PAL2;
    for (i = 0; i < 4; i++)
        PAL12_ROW(from, &di);
}

void CHOOSER_LOAD(void)
{
    uint32_t size;

    ww(N_HOST_DS, pi_image.desc[ILLUSION_CODE].sel);
    ww(N_VIDEO_SEL, pmax_video_sel());
    /* through CODE:4CF2's checksummed jump */
    if (SOUND_START()) {
        /* CODE:35B7D with BL 0, then exit (INT 21h AX=4CFFh) */
        pi_stop("CHOOSER_LOAD: the sound start failed");
    }

    /* CODE:75B6 */
    ww(N_CHOOSER_LOADED, 1);
    ww(N_CUBE_SEL, load("chooser\\cube.rix", NULL));
    ww(N_TUBE_SEL, load("chooser\\tube.rix", NULL));
    ww(N_TORUS_SEL, load("chooser\\torus.rix", NULL));
    ww(N_TINYFONT_SEL, load("chooser\\tinyfont.fnt", NULL));
    ww(N_INFODATA_SEL, load("chooser\\infodata.mgl", NULL));
    ww(N_MENUCHAR_SEL, load("chooser\\menuchar.rix", NULL));
    ww(N_INTROANI_SEL, load("intro\\introani.roy", &size));
    wd(N_INTROANI_SIZE, size);
    wd(N_INTROANI_POS, 0);
    ww(N_INTROPIX_SEL, load("intro\\intropix.mgl", NULL));
    ww(N_SCROLL_SEL, load("intro\\SCROLL.DLT", NULL));
    ww(N_BKGR_SEL, load("intro\\BKGR.FLD", NULL));
    ww(N_PCSKY_SEL, load("intro\\PCSKY.FLD", NULL));

    /* CODE:7885: through CODE:7438's checksummed jump */
    INTRO_PALS_MAKE();
    wd(N_INTRO_NEXT, N_INTRO_SCRIPT);
    wd(N_INTRO_TIME, 0);
    wd(0x8120, 0);              /* written only, by this name */
    wb(N_INTRO_END, 0);
    wd(0x812C, 0x7074);         /* a RET's offset (CODE:7074); not read by this name */

    /* CODE:78C9, 78DE: through CODE:4CF2's checksummed jumps */
    INTRO_MODE();
    INTROPIX_PALS();
    wd(N_FADE_FROM, N_INTRO_BLACK);
    wd(N_FADE_TO, N_INTRO_BLACK);
    wb(N_FADE_LEVEL, 0x40);
    wb(N_FADE_STEP, 4);
    vga_outw(0x3D4, 0x500D);            /* the start address 2D50h */
    vga_outw(0x3D4, 0x2D0C);
    pi_stop("INTRO_FRAME (CODE:7925, the driver's command 6)");
}
