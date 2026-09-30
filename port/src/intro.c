/* intro.c - CHOOSER_LOAD (CODE:757D): the sound started, the chooser's and
 * the intro's files loaded, then the intro.
 */
#include <string.h>

#include "game.h"
#include "image.h"
#include "frame.h"
#include "names.h"
#include "nosound.h"
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

/* CODE:7223: the driver's command 6 */
static void DRIVER_MIX(void)
{
    NsRegs r = { 0 };

    r.eax = 6;
    r.ds = pi_image.desc[ILLUSION_CODE].sel;
    ns_call(rw(N_DRIVER_ENTRY + 4), &r);
}

/* CODE:6E26: the end of the next vertical retrace */
static void RETRACE_WAIT(void)
{
    frame_wait();
}

/* CODE:6F43 */
static void FADE_FRAME(void)
{
    uint32_t from, to, i;
    uint8_t dl, dh;

    if ((int8_t)rb(N_FADE_LEVEL) <= 0) {
        RETRACE_WAIT();
        return;
    }
    wb(N_FADE_LEVEL, (uint8_t)(rb(N_FADE_LEVEL) - rb(N_FADE_STEP)));
    dl = rb(N_FADE_LEVEL);
    dh = (uint8_t)(0x40 - dl);
    from = rd(N_FADE_FROM);
    to = rd(N_FADE_TO);
    for (i = 0; i < 0x300; i++) {
        uint16_t ax = (uint16_t)(rb(from + i) * dl + rb(to + i) * dh);
        wb(N_FADE_OUT + i, (uint8_t)(ax >> 6));
    }
    /* colour 3Fh black */
    wb(N_FADE_OUT + 0xBD, 0);
    ww(N_FADE_OUT + 0xBE, 0);
    RETRACE_WAIT();
    vga_outb(0x3C8, 0);
    for (i = 0; i < 0x300; i++)
        vga_outb(0x3C9, rb(N_FADE_OUT + i));
}

/* CODE:6E35 */
static void INTRO_FRAME(void)
{
    DRIVER_MIX();
    FADE_FRAME();
}

/* CODE:6E49: intropix.mgl's picture (after its 32 colours) to the four
 * planes at 7D0h, 2580h bytes each */
static void INTROPIX_SHOW(void)
{
    uint32_t base = pmax_base(rw(N_INTROPIX_SEL));
    uint32_t si = base + lrd(base + 0x18) + 0x40, i;
    int plane;

    vga_outw(0x3C4, 0x0102);
    for (plane = 0; plane < 4; plane++) {
        vga_outb(0x3C5, (uint8_t)(1 << plane));
        for (i = 0; i < 0x2580; i++)
            vga_write((uint16_t)(0x7D0 + i), lrb(si++));
    }
}

/* CODE:6F1F: 300h bytes from `from` (linear) to FADE_PAL, FADE_TO set to it */
static void FADE_TO_SET(uint32_t from)
{
    uint32_t i;

    for (i = 0; i < 0x300; i++)
        wb(N_FADE_PAL + i, lrb(from + i));
    wd(N_FADE_TO, N_FADE_PAL);
}

static void fade(uint32_t from, uint32_t to, uint8_t step)
{
    wd(N_FADE_FROM, from);
    wd(N_FADE_TO, to);
    wb(N_FADE_LEVEL, 0x40);
    wb(N_FADE_STEP, step);
}

/* CODE:6A20: the start address 0, rows of one scan line (CR 9 41h) */
static void VIEW_TOP(void)
{
    vga_outw(0x3D4, 0x000D);
    vga_outw(0x3D4, 0x000C);
    INTRO_FRAME();
    vga_outw(0x3D4, 0x3E07);
    vga_outw(0x3D4, 0x4109);
    vga_outw(0x3D4, 0x2112);
    vga_outw(0x3D4, 0x000D);
    vga_outw(0x3D4, 0x000C);
}

/* CODE:6A5C: the start address 2D50h */
static void VIEW_2D50(void)
{
    vga_outw(0x3D4, 0x500D);
    vga_outw(0x3D4, 0x2D0C);
    INTRO_FRAME();
    vga_outw(0x3D4, 0x4009);
}

/* CODE:6A7C: the start address 3CF0h, 400 lines */
static void VIEW_3CF0(void)
{
    static const uint16_t crtc[] = {
        0x4009, 0x3E07, 0x8F12, 0x0014, 0x0016, 0xE317, 0xF00D, 0x3C0E
    };
    int i;

    vga_outw(0x3D4, 0xF00D);
    vga_outw(0x3D4, 0x3C0C);
    INTRO_FRAME();
    for (i = 0; i < 8; i++)
        vga_outw(0x3D4, crtc[i]);
}

/* CODE:6B8A: one pass of an introani.roy frame from `*si` (linear) at
 * 7D0h: bytes below 10h written (XCHG: the latches read first), 10h a
 * word to skip, 11h-FEh that less 10h to skip, FFh the end */
static void ANI_PASS(uint32_t *si)
{
    uint32_t edi = 0x7D0;

    for (;;) {
        uint8_t al = lrb((*si)++);

        if (al < 0x10) {
            vga_read((uint16_t)edi);
            vga_write((uint16_t)edi, al);
            edi++;
        } else if (al == 0x10) {
            edi += lrw(*si);
            *si += 2;
        } else if (al == 0xFF) {
            return;
        } else {
            edi += (uint32_t)(al - 0x10);
        }
    }
}

/* CODE:6BB1: introani.roy's next frame, XORed into the picture in write
 * mode 2, pixel bits 6 then 7 */
static void ANI_FRAME(void)
{
    uint32_t base = pmax_base(rw(N_INTROANI_SEL)), si = base + rd(N_INTROANI_POS);

    vga_outw(0x3C4, 0x0F02);
    vga_outw(0x3CE, 0x4205);
    vga_outw(0x3CE, 0x4008);
    vga_outw(0x3CE, 0x1803);
    ANI_PASS(&si);
    vga_outw(0x3CE, 0x8008);
    ANI_PASS(&si);
    vga_outw(0x3CE, 0x4005);
    vga_outw(0x3CE, 0xFF08);
    vga_outw(0x3CE, 0x0003);
    wd(N_INTROANI_POS, si - base);
}

/* CODE:6C82 (`words` 2580h) and CODE:6D50 (3E80h): intropix.mgl's picture
 * at its dword `at` (a palette of 300h bytes, then the four planes) to
 * 3CF0h plane by plane, a frame after each, then a fade from CODE:8A3C to
 * its palette */
static void PIC_SHOW(uint32_t at, uint32_t words)
{
    uint32_t base = pmax_base(rw(N_INTROPIX_SEL)), pal = base + lrd(base + at);
    uint32_t si = pal + 0x300, i;
    int plane;

    for (plane = 0; plane < 4; plane++) {
        vga_outw(0x3C4, (uint16_t)(0x0100 << plane | 2));
        for (i = 0; i < words * 2; i++)
            vga_write((uint16_t)(0x3CF0 + i), lrb(si++));
        INTRO_FRAME();
        DRIVER_MIX();
    }
    FADE_TO_SET(pal);
    wb(N_FADE_LEVEL, 0x40);
    wb(N_FADE_STEP, 4);
    wd(N_FADE_FROM, 0x8A3C);
    if (words == 0x2580)
        VIEW_2D50();
    else
        VIEW_3CF0();
    DRIVER_MIX();
}

/* INTRO_SCRIPT's routines by their offsets */
static void script_call(uint32_t routine)
{
    switch (routine) {
    case 0x7074:                        /* a RET */
        break;
    case 0x6AC6:
        vga_outw(0x3D4, 0x000D);
        vga_outw(0x3D4, 0x000C);
        wb(N_FADE_LEVEL, 0x40);
        wd(N_FADE_FROM, N_INTRO_BLACK);
        wb(N_FADE_STEP, 4);
        FADE_TO_SET(pm_ds + N_INTRO_PAL1);
        break;
    case 0x6AF9:
    case 0x6B1C:
        fade(N_INTRO_PAL2, N_INTRO_PAL1, 2);
        break;
    case 0x6B3F:
        fade(N_INTRO_PAL1, N_INTRO_PAL2, 2);
        break;
    case 0x6B62:
        wb(N_FADE_LEVEL, 0x40);
        wd(N_FADE_FROM, 0x8A3C);
        wb(N_FADE_STEP, 4);
        FADE_TO_SET(pm_ds + N_INTRO_PAL1);
        VIEW_TOP();
        break;
    case 0x6BB1:
        ANI_FRAME();
        break;
    case 0x6C14: PIC_SHOW(0x08, 0x2580); break;
    case 0x6C1F: PIC_SHOW(0x04, 0x2580); break;
    case 0x6C2A: PIC_SHOW(0x10, 0x2580); break;
    case 0x6C35: PIC_SHOW(0x0C, 0x3E80); break;
    case 0x6C40: PIC_SHOW(0x14, 0x3E80); break;
    case 0x6C4B:
        fade(0x8A3C, 0x8A3C, 4);
        INTRO_FRAME();
        DRIVER_MIX();
        PIC_SHOW(0x1C, 0x3E80);
        break;
    case 0x6E1E:
        wb(N_INTRO_END, 1);
        break;
    default:
        pi_stop("INTRO_TICK: a script routine not translated");
    }
}

/* CODE:7029: the driver's position (command 0Dh); the script's routines
 * whose position has come, asking the driver again after each */
static void INTRO_TICK(void)
{
    for (;;) {
        NsRegs r = { 0 };
        uint32_t esi;

        r.eax = 0x0D;
        r.ds = pi_image.desc[ILLUSION_CODE].sel;
        ns_call(rw(N_DRIVER_ENTRY + 4), &r);
        if (!((int32_t)rd(N_INTRO_TIME) > (int32_t)r.eax))
            wd(N_INTRO_TIME, r.eax);
        esi = rd(N_INTRO_NEXT);
        if (esi >= N_INTRO_MOD_NAME || rd(N_INTRO_TIME) < rd(esi))
            return;
        script_call(rd(esi + 4));
        wd(N_INTRO_NEXT, rd(N_INTRO_NEXT) + 8);
    }
}

/* port 60h, which the intro reads with IRQ 1 masked: the last byte the
 * keyboard gave */
static uint8_t port60;

static void key_byte(unsigned char b)
{
    port60 = b;
}

/* Esc and space (the MOV AL,1Ch before the third JE sets no flags: Enter
 * is not a key here) */
static int intro_key(void)
{
    return port60 == 0x01 || port60 == 0x39;
}

/* CODE:72BB: BKGR.FLD and PCSKY.FLD (from +300h) to 3CF0h, plane by
 * plane, 12C0h dwords each: PCSKY's dword ORed with BKGR's shifted left 4 */
static void INTRO_BKGR(void)
{
    uint32_t bk = pmax_base(rw(N_BKGR_SEL)), sky = pmax_base(rw(N_PCSKY_SEL));
    uint32_t si = 0x300, bx = 0, di, i;
    int plane;

    for (plane = 0; plane < 4; plane++) {
        vga_outw(0x3C4, (uint16_t)(0x0100 << plane | 2));
        for (di = 0x3CF0, i = 0; i < 0x12C0; i++, si += 4, bx += 4) {
            uint32_t d = lrd(sky + si) | lrd(bk + bx) << 4;
            int k;

            for (k = 0; k < 4; k++)
                vga_write((uint16_t)di++, (uint8_t)(d >> 8 * k));
        }
    }
}

/* CODE:7230: SCROLL.DLT's next frame at 3CF0h, pixel bit 7 XORed (write
 * mode 2, function XOR, bit mask 80h): pairs of an offset and the plane
 * bits, FFh a line's end, 240 lines; SCROLL_POS moved on one line */
static void SCROLL_FRAME(void)
{
    uint32_t base = pmax_base(rw(N_SCROLL_SEL)), si = base + rd(N_SCROLL_POS);
    uint32_t di = 0x3CF0, next = 0;
    int line;

    vga_outw(0x3C4, 0x0F02);
    vga_outw(0x3CE, 0x4205);
    vga_outw(0x3CE, 0x8008);
    vga_outw(0x3CE, 0x1803);
    for (line = 0; line < 0xF0; line++) {
        uint8_t bl;

        while ((bl = lrb(si)) != 0xFF) {
            vga_read((uint16_t)(di + bl));
            vga_write((uint16_t)(di + bl), lrb(si + 1));
            si += 2;
        }
        si++;
        di += 0x50;
        if (line == 0)
            next = si;
    }
    wd(N_SCROLL_POS, next - base);
    wd(N_SCROLL_LEFT, rd(N_SCROLL_LEFT) - 1);
}

/* CODE:79C3: the intro played to its end: a fade to INTRO_PALS, the
 * background, the scroller until SCROLL_LEFT is 21h (or a key), a fade to
 * INTRO_BLACK */
static void INTRO_SCROLL(void)
{
    wd(N_FADE_FROM, 0x8A3C);
    wb(N_FADE_STEP, 2);
    FADE_TO_SET(pm_ds + N_INTRO_PALS);
    wb(N_FADE_LEVEL, 0x40);
    INTRO_FRAME();
    wd(N_SCROLL_LEFT, 0x780);
    VIEW_2D50();
    /* CODE:7A0F: through CODE:4CF2's checksummed jump ([CODE:907F]: 72BBh) */
    INTRO_BKGR();
    /* the sequencer's and graphics controller's index left at 2 and 4 */
    vga_outb(0x3C4, 2);
    vga_outb(0x3CE, 4);
    wd(N_SCROLL_POS, 0);
    do {
        SCROLL_FRAME();
        INTRO_FRAME();
        if (intro_key())
            break;
    } while (rd(N_SCROLL_LEFT) > 0x21);
    /* CODE:7A4D */
    wb(N_FADE_LEVEL, 0x40);
    wb(N_FADE_STEP, 2);
    wd(N_FADE_FROM, rd(N_FADE_TO));
    wd(N_FADE_TO, N_INTRO_BLACK);
    do {
        SCROLL_FRAME();
        INTRO_FRAME();
    } while ((int8_t)rb(N_FADE_LEVEL) > 0);
}

/* CODE:7A82: IRQ 1 unmasked, the driver's commands 3 (the stop) and 8
 * (slot 0 from order 12h), then through CODE:7438's checksummed jump
 * ([CODE:235E]: 73D8h) INTRO_FREE: the intro's five blocks freed */
static void INTRO_LEAVE(void)
{
    NsRegs r = { 0 };

    /* the keyboard is the game's again (which handler takes it is not
     * followed) */
    frame_set_keyboard(NULL);
    pic_out21((uint8_t)(pic_in21() & 0xFD));
    r.eax = 3;
    r.ds = pi_image.desc[ILLUSION_CODE].sel;
    ns_call(rw(N_DRIVER_ENTRY + 4), &r);
    memset(&r, 0, sizeof r);
    r.eax = 8;
    r.ebx = 0x12;
    r.ecx = 0;
    r.ds = pi_image.desc[ILLUSION_CODE].sel;
    ns_call(rw(N_DRIVER_ENTRY + 4), &r);
    /* CODE:73D8 */
    pmax_free(rw(N_INTROANI_SEL));
    pmax_free(rw(N_INTROPIX_SEL));
    pmax_free(rw(N_SCROLL_SEL));
    pmax_free(rw(N_BKGR_SEL));
    pmax_free(rw(N_PCSKY_SEL));
}

void CHOOSER_LOAD(void)
{
    uint32_t size, i;

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
    INTRO_FRAME();
    for (i = 0; i < 0x10000; i++)
        vga_write((uint16_t)i, 0x3F);
    vga_outb(0x3C0, 0x31);              /* the overscan colour 3Fh */
    vga_outb(0x3C0, 0x3F);
    /* CODE:795A: through CODE:7438's checksummed jump ([CODE:9043]: 6E49h) */
    INTROPIX_SHOW();

    /* CODE:795F: command 1 with 0Fh buffers (a failure: text mode, the
     * text at CODE:80DA, the end; not seen) */
    {
        NsRegs r = { 0 };

        r.eax = 1;
        r.ecx = 0x0F;
        r.ds = pi_image.desc[ILLUSION_CODE].sel;
        if (ns_call(rw(N_DRIVER_ENTRY + 4), &r))
            pi_stop("CODE:7972 (the driver's command 1 failed)");
    }
    /* CODE:798B: IRQ 1 masked, port 60h read in the loop */
    pic_out21((uint8_t)(pic_in21() | 2));
    frame_set_keyboard(key_byte);
    DRIVER_MIX();
    for (;;) {
        INTRO_FRAME();
        INTRO_TICK();
        if (intro_key())
            break;
        if (rb(N_INTRO_END) == 1) {
            INTRO_SCROLL();
            break;
        }
    }
    INTRO_LEAVE();
}
