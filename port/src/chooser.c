/* chooser.c - the chooser (CODE:4CFB): its start, up to CHOOSER.
 *
 * The start is a chain of checksummed jumps (each target a dword less the
 * byte sum of some code; the targets were computed from a run's memory
 * and followed in a -trace, docs/HANDOFF.md "The chooser's start").  The
 * captions are compiled: CAPTION_BUILD draws each into two bitmaps and
 * makes six 16-bit routines of it (GEN_CODE, GEN_CODE2) in blocks of their
 * own, which the chooser later calls to draw; the port makes the same
 * bytes, so memory compares with a run.  CHOOSER then draws the backdrop's
 * picture, hands the driver its two retrace routines and starts the music;
 * the Info and greetings pages switch to a 256-colour mode of their own.
 */
#include <stdio.h>
#include <string.h>

#include "frame.h"
#include "game.h"
#include "image.h"
#include "names.h"
#include "nosound.h"
#include "pmax.h"
#include "pmem.h"
#include "vga.h"

/* CODE:340A, IRQ 1's handler while the chooser runs: a key pressed (below
 * 80h) into the ring at CODE:1906 (16 bytes, index CODE:1903) and shifted
 * into KEY_HISTORY; releases dropped */
static void KBD_IRQ(unsigned char ah)
{
    uint32_t save = pm_ds;

    pm_ds = PI_IMAGE_BASE;
    if (ah < 0x80) {
        wb(0x1906 + (rb(0x1903) & 0x0F), ah);
        wb(0x1903, (uint8_t)(rb(0x1903) + 1));
        wd(N_KEY_HISTORY, rd(N_KEY_HISTORY) << 8 | ah);
    }
    pm_ds = save;
}

/* CODE:33B8: IRQ 1's vector kept (INT 93h AH=3 BL=1 gave 0030:00004152 in
 * the run), CODE:340A set; INT 93h AH=9 BL=1 (not looked into) and the
 * PIC leave nothing in memory */
static void KBD_INSTALL(void)
{
    wd(N_KBD_OLD, 0x4152);
    ww(N_KBD_OLD + 4, 0x30);
    frame_set_keyboard(KBD_IRQ);
}

/* CODE:23CA */
static void CHOOSER_MODE(void)
{
    static const uint16_t crtc[] = {
        0x2501, 0x2702, 0x8F03, 0x2A04, 0x9005, 0x0E06, 0x3E07, 0x4109,
        0xD710, 0x2C11, 0xBF12, 0x1713, 0x0014, 0xD615, 0xF816, 0xE317
    };
    int i;

    wb(0x191A, 1);
    vga_outw(0x3C4, 0x8901);
    vga_outb(0x3D4, 0x11);
    vga_outb(0x3D5, vga_inb(0x3D5) & 0x7F);
    for (i = 0; i < (int)(sizeof crtc / sizeof crtc[0]); i++)
        vga_outw(0x3D4, crtc[i]);
    vga_outb(0x3C2, vga_inb(0x3CC) | 0xC0);
    vga_inb(0x3DA);
    for (i = 0x10; i > 0; i--) {
        vga_outb(0x3C0, (uint8_t)(i - 1));
        vga_outb(0x3C0, (uint8_t)(i - 1));
    }
}

/* CODE:299B */
static void WRITE_MODE1(void)
{
    vga_outw(0x3CE, 0x0105);
    vga_outw(0x3C4, 0x0F02);
}

/* CODE:29B0 */
static void READ_MODE1(void)
{
    vga_outw(0x3CE, 0x1003);
    vga_outw(0x3CE, 0x0F02);
    vga_outw(0x3CE, 0x0805);
}

/* CODE:2613 (CODE:254D a RET) */
static void SCROLL_INIT(void)
{
    ww(N_SCROLL_TOP, 0xE60);
    ww(N_SCROLL_SHOWN, 0xE60);
    ww(N_CRT_START, 0xE60);
    wb(N_SCROLL_PHASE, 3);
}

/* CODE:2FE3 */
static void VIDEO_TOP_SET(void)
{
    int i;

    for (i = 0; i < 0x26; i++)
        vga_write((uint16_t)i, 0xFF);
}

/* CODE:36AE */
static void ATTRACT_START(void)
{
    wb(0x1919, 1);
    ww(0x1917, 0x12C);
    wd(0x1540, rw(N_CHOOSER_LOADED) == 1 ? 0x1371 : 0x13D7);
    wd(0x1544, 0x191B);
    wb(0x132B, 0x46);
    wb(0x132C, 2);
}

/* CODE:29D2 */
static void CHOOSER_DAC(void)
{
    int i;

    if ((int8_t)rb(0x132B) < 0)
        return;
    vga_outb(0x3C8, 0);
    for (i = 0; i < 0x33; i++)
        vga_outb(0x3C9, rb(0x13A4 + i));
}

/* CODE:24AF with CX 1BDh (the run) */
static void SPLIT_SET(uint16_t cx)
{
    uint8_t al;

    vga_outb(0x3D4, 0x11);
    vga_outb(0x3D5, vga_inb(0x3D5) & 0x7F);
    vga_outw(0x3D4, (uint16_t)(((cx & 0xFF) - 1) << 8 | 0x18));
    vga_outb(0x3D4, 7);
    al = (uint8_t)((vga_inb(0x3D5) & 0xEF) | ((cx >> 8) & 1) << 4);
    vga_outw(0x3D4, (uint16_t)(al << 8 | 7));
    vga_outb(0x3D4, 9);
    al = vga_inb(0x3D5) & 0xBF;
    vga_outw(0x3D4, (uint16_t)(al << 8 | 9));
}

/* CODE:2503 */
static void VIDEO_CLEAR(void)
{
    uint32_t i;

    vga_outw(0x3C4, 0x0F02);
    for (i = 0; i < 0x10000; i++)
        vga_write((uint16_t)i, 0);
}

/* CODE:2869 (menuchar.rix loaded by CHOOSER_LOAD): CHAR_STRIDE, CHAR_OFFS */
static void MENUCHAR_INIT(void)
{
    uint16_t bx = 0x3A, dl;
    int i;

    if (rw(N_CHOOSER_LOADED) != 1)
        pi_stop("MENUCHAR_INIT: menuchar.rix not loaded yet (CODE:2840)");
    ww(N_CHAR_STRIDE, (uint16_t)(lrw(pmax_base(rw(N_MENUCHAR_SEL)) + 4) >> 3));
    dl = rw(N_CHAR_STRIDE) & 0xFF;
    for (i = 0; i < 0x80; i++) {
        uint16_t ax = (uint16_t)(rb(N_CHAR_TAB + 3 * i + 1) * dl);

        ww(N_CHAR_OFFS + 2 * i, bx);
        bx = (uint16_t)(bx + 4 * ax);
    }
}

/* ---- the captions ---- */

static uint32_t fs;                     /* BITMAP_SEL's base */

static void or_b(uint32_t off, uint8_t v) { lwb(fs + off, lrb(fs + off) | v); }
static void or_d(uint32_t off, uint32_t v) { lwd(fs + off, lrd(fs + off) | v); }

static uint32_t swap_lo(uint32_t x) { return (x & 0xFFFF0000u) | (x & 0xFF) << 8 | (x >> 8 & 0xFF); }
static uint32_t rol(uint32_t x, int n) { n &= 31; return n ? x << n | x >> (32 - n) : x; }
static uint32_t ror(uint32_t x, int n) { n &= 31; return n ? x >> n | x << (32 - n) : x; }

/* CODE:2CE3: character `ch` into both bitmaps at bit *ebx, *ebx moved on
 * by its advance; *eax as the routine leaves EAX */
static void CHAR_DRAW(uint8_t ch, uint32_t *ebx, uint32_t *eax)
{
    uint32_t glyph = pmax_base(rw(N_MENUCHAR_SEL));
    int cl = *ebx & 7, rows = rb(N_CHAR_TAB + 3 * ch + 1);
    uint32_t si = rw(N_CHAR_OFFS + 2 * ch);
    uint32_t di = (*ebx >> 3) + (uint16_t)(((rb(N_CHAR_TAB + 3 * ch + 2) + 1) & 0xFF) * 0x2E);
    /* EAX: the line * 2Eh (MUL AH) until a glyph line is read */
    uint32_t a = (uint16_t)(((rb(N_CHAR_TAB + 3 * ch + 2) + 1) & 0xFF) * 0x2E), d;

    (void)*eax;
    *ebx += rb(N_CHAR_TAB + 3 * ch);
    for (; rows; rows--) {
        uint32_t bp;

        a = lrw(glyph + si);
        d = lrw(glyph + si + 2);
        bp = d & a;
        a ^= bp;
        d ^= bp;
        d |= a;
        a = swap_lo(ror(rol(swap_lo(a), 16), cl));
        a = swap_lo(ror(a, 16));
        d = swap_lo(ror(rol(swap_lo(d), 16), cl));
        d = swap_lo(ror(d, 16));
        or_d(di, a);
        or_d(di + 0x2E04, d);
        si += 8;
        di += 0x2E;
    }
    *eax = a;
}

/* CODE:3013 */
static void BITMAP_CLEAR(void)
{
    uint32_t i;

    for (i = 0; i < 0x5C08; i++)
        lwb(fs + i, 0);
}

/* CODE:31BB */
static void BITMAP_FRAME(uint32_t si)
{
    int i;

    for (i = 0; i < 0x26; i++)
        lwb(fs + si++, 0xFF);
    si += 8;
    for (i = 0; i < 0xDF; i++) {
        or_b(si, 0x80);
        or_b(si + 0x25, 1);
        si += 0x2E;
    }
}

static uint32_t gen;                    /* GEN_BUF's next byte */

static void emit(uint8_t b) { lwb(gen++, b); }
static void emit_w(uint16_t w) { emit((uint8_t)w); emit((uint8_t)(w >> 8)); }

/* CODE:2D72 */
static void GEN_LATCH(uint32_t bp)
{
    if (bp == 0) {
        emit_w(0x048A);
    } else if (bp < 0x80) {
        emit_w(0x448A);
        emit((uint8_t)bp);
    } else {
        emit_w(0x848A);
        emit_w((uint16_t)bp);
    }
}

/* CODE:2D9B */
static void GEN_BYTE(uint32_t bx, uint8_t dl)
{
    if (bx == 0) {
        emit_w(0x05C6);
    } else if (bx < 0x80) {
        emit_w(0x45C6);
        emit((uint8_t)bx);
    } else {
        emit_w(0x85C6);
        emit_w((uint16_t)bx);
    }
    emit(dl);
}

/* CODE:2DCD */
static void GEN_BH(uint32_t bx)
{
    if (bx == 0) {
        emit_w(0x3D88);
    } else if (bx < 0x80) {
        emit_w(0x7D88);
        emit((uint8_t)bx);
    } else {
        emit_w(0xBD88);
        emit_w((uint16_t)bx);
    }
}

/* CODE:2DF6 */
static void GEN_SI(uint8_t *dh)
{
    emit(0x26);
    emit_w(0x368B);
    emit_w((uint16_t)(0x150C + 2 * *dh));
    emit(0x26);
    emit_w(0x3603);
    emit_w((uint16_t)(0x151C + 2 * *dh));
    (*dh)++;
}

/* CODE:2E35 (`latches` 1) and CODE:2F14 (0): a caption routine from the
 * bitmap at `si` and the one at `other`; its far pointer to `rec` */
static void GEN_CODE(uint32_t si, uint32_t other, uint32_t rec, int latches)
{
    uint32_t bx = 0, bp = 0, size, i;
    uint8_t dh = 0, ch;
    uint16_t sel;

    wd(N_GEN_OTHER, other);
    gen = rd(N_GEN_BUF);
    GEN_SI(&dh);
    for (ch = 0xE0; ch; ch--) {
        int cl;

        for (cl = 0x26; cl; cl--) {
            uint8_t dl = lrb(fs + bx + si), al = lrb(fs + rd(N_GEN_OTHER) + bx);

            if (dl != 0) {
                if (latches)
                    GEN_LATCH(bp);
                GEN_BYTE(bx, dl);
            } else if (al != 0) {
                if (!latches) {
                    GEN_BYTE(bx, dl);
                } else if (ch > 0x20) {
                    GEN_LATCH(bp);
                    GEN_BH(bx);
                }
            }
            bx++;
            bp++;
        }
        bx += 8;
        bp += 0x92;
        if (bp >= 0x1700) {
            bp = 0;
            bx -= 0x5C0;
            si += 0x5C0;
            wd(N_GEN_OTHER, rd(N_GEN_OTHER) + 0x5C0);
            /* CODE:2E23 */
            emit_w(0xC781);
            emit_w(0x05C0);
            GEN_SI(&dh);
        }
    }
    emit_w(0xCB66);
    size = gen - rd(N_GEN_BUF);
    /* INT 92h AH=4 (name CODE:2F13), the bytes copied; INT 93h AH=14h CX
     * 9Ah (a code segment) leaves nothing in memory */
    sel = pmax_alloc(size);
    if (!sel)
        pi_stop("GEN_CODE: no room (INT 92h AH=4)");
    for (i = 0; i < size; i++)
        lwb(pmax_base(sel) + i, lrb(rd(N_GEN_BUF) + i));
    wd(rec, 0);
    ww(rec + 4, sel);
}

/* CODE:302E: the caption record at `rec` from the layout at `si` */
static void CAPTION_BUILD(uint32_t si, uint32_t rec)
{
    uint32_t eax = 0, ebx = 0, edx, ebp;
    uint8_t al;

    fs = pmax_base(rw(N_BITMAP_SEL));
    BITMAP_CLEAR();                     /* leaves EAX 0 */
    for (;;) {
        al = rb(si++);
        eax = (eax & ~0xFFu) | al;
        if (al == 0 || al == 1) {
            uint8_t y = rb(si++);

            eax = (eax & 0xFFFF0000u) | (uint16_t)(y * 0x2E);
            ebx = (eax & 0xFFFF) << 3;
            if (al == 1) {
                uint32_t t = si;

                edx = 0;
                /* EAX: each width by MOVZX, then LODSB's AL */
                while ((al = rb(t++)) != 0) {
                    eax = rb(N_CHAR_TAB + 3 * al);
                    edx += eax;
                }
                eax &= ~0xFFu;
                ebx += 0x98 - (edx >> 1);
            }
            /* CODE:3059 */
            while ((al = rb(si++)) != 0) {
                eax = (eax & ~0xFFu) | al;
                CHAR_DRAW(al, &ebx, &eax);
            }
            eax &= ~0xFFu;
        } else if (al == 2) {
            uint8_t dl, cl;

            eax = (eax & 0xFFFF0000u) | rw(si);
            si += 2;
            ebp = eax & 0xFFFF;
            cl = ebp & 7;
            eax = (eax & 0xFFFF0000u) | (uint16_t)(rb(si++) * 0x2E);
            ebx = eax & 0xFFFF;
            eax = (eax & 0xFFFF0000u) | rw(si);
            si += 2;
            dl = (uint8_t)(0xFF >> cl);
            ebp >>= 3;
            or_b(ebx + ebp, dl);
            or_b(ebx + ebp + 0x2E04, dl);
            ebp++;
            cl = (uint8_t)(~eax & 7);
            dl = (uint8_t)(0xFF << cl);
            eax >>= 3;
            or_b(ebx + eax, dl);
            or_b(ebx + eax + 0x2E04, dl);
            for (; ebp < eax; ebp++) {
                or_b(ebx + ebp, 0xFF);
                or_b(ebx + ebp + 0x2E04, 0xFF);
            }
        } else if (al == 3) {
            uint16_t ax = rw(si);
            uint32_t di;
            uint8_t bh, bl, n;

            si += 2;
            bh = (uint8_t)(0x80 >> (ax & 7));
            di = ax >> 3;
            bl = rb(si++);
            di += (uint16_t)(bl * 0x2E);
            n = (uint8_t)(rb(si++) - bl + 1);
            do {
                or_b(di, bh);
                or_b(di + 0x2E04, bh);
                di += 0x2E;
            } while (--n);
            /* MOVZX EAX,AX of y1 * 2Eh, then AL counted down to 0 */
            eax = (uint16_t)(bl * 0x2E) & 0xFF00u;
        } else if (al == 0xFF) {
            break;
        } else {
            pi_stop("CAPTION_BUILD: a layout command not seen");
        }
    }
    /* CODE:3139 */
    BITMAP_FRAME(0);
    BITMAP_FRAME(0x2E04);
    GEN_CODE(0, 0x33C4, rec + 0x00, 1);
    GEN_CODE(0x2E04, 0x5C0, rec + 0x06, 1);
    GEN_CODE(0, 0x33C8, rec + 0x0C, 1);
    GEN_CODE(0x2E04, 0x5C4, rec + 0x12, 1);
    GEN_CODE(0, 0x2E04, rec + 0x18, 0);
    GEN_CODE(0x2E04, 0, rec + 0x1E, 0);
}

/* CODE:39C8 */
static void CAPTIONS_MAKE(void)
{
    static const uint16_t pairs[][2] = {
        {0x1AE6, 0x1554}, {0x1B5C, 0x15E4}, {0x1E96, 0x1674}, {0x1BD2, 0x1578},
        {0x1C48, 0x1608}, {0x1EF6, 0x1698}, {0x1CBE, 0x159C}, {0x1D34, 0x162C},
        {0x1F56, 0x16BC}, {0x1DAA, 0x15C0}, {0x1E20, 0x1650}, {0x1FB6, 0x16E0},
        {0x226E, 0x1728}, {0x2016, 0x1704}, {0x202B, 0x174C}, {0x2055, 0x1794},
        {0x2076, 0x17B8}, {0x20DC, 0x17DC}, {0x2124, 0x1800}, {0x2169, 0x1824},
        {0x21C5, 0x1848}, {0x220A, 0x1770}, {0x2236, 0x186C}
    };
    int i;

    /* INT 92h AH=6, 10000h bytes (name CODE:39C7); INT 93h AH=0Ch's
     * selector (80h in the run, flat) is the port's linear memory */
    wd(N_GEN_BUF, pmax_alloc_linear(0x10000));
    if (!rd(N_GEN_BUF))
        pi_stop("CAPTIONS_MAKE: no room (INT 92h AH=6)");
    for (i = 0; i < (int)(sizeof pairs / sizeof pairs[0]); i++)
        CAPTION_BUILD(pairs[i][0], pairs[i][1]);
    pmax_free_linear(rd(N_GEN_BUF));
}

/* ---- CHOOSER ---- */

static void CUBE_WAIT(uint8_t bl);
static void MUSIC_MIX(void);

/* CODE:26FD: the picture of selector `sel` from its byte 3Ah into video
 * memory 72A4h, each plane four times rotated by 0, 2, 4, 6 bits; with
 * `wait` CUBE_DRAW_WAIT (CODE:263B), a frame of the chooser every 16
 * lines */
static void CUBE_DRAW(uint16_t sel, int wait)
/* `wait` 2: CUBE_DRAW_MIX (CODE:2789), MUSIC_MIX after each rotation */
{
    uint32_t src = pmax_base(sel), esi = 0x3A, edi = 0x72A4;
    uint8_t bl = 1;

    vga_outw(0x3CE, 0x0003);            /* CODE:29C7 */
    vga_outb(0x3C4, 2);
    vga_outb(0x3C5, bl);
    do {
        int cl, ch, k;

        for (cl = 0; cl < 8; cl += 2) {
            for (ch = 0; ch < 0x20; ch++) {
                uint32_t s = esi;
                uint16_t ax;

                for (k = 0; k < 0xB7; k++, s++) {
                    ax = lrw(src + s);
                    vga_write((uint16_t)edi++, (uint8_t)(ax << cl | ax >> (16 - cl)));
                }
                /* the line's last byte with its first */
                ax = (uint16_t)(lrb(src + esi) << 8 | lrb(src + s));
                vga_write((uint16_t)edi++, (uint8_t)(ax << cl | ax >> (16 - cl)));
                esi += 0x2E0;
                if (wait == 1 && (ch & 0x0F) == 0)
                    CUBE_WAIT(bl);
            }
            esi -= 0x5C00;
            if (wait == 2)
                MUSIC_MIX();
        }
        esi += 0xB8;
        edi -= 0x5C00;
        bl <<= 1;
        vga_outb(0x3C5, bl);            /* 10h after the last plane, as the original */
    } while (bl < 0x10);
}

/* CODE:4C85, called by the driver at each retrace */
static void VSYNC_CB(void)
{
    uint32_t save = pm_ds;

    pm_ds = PI_IMAGE_BASE;
    wd(N_VSYNC_COUNT, rd(N_VSYNC_COUNT) + 1);
    pm_ds = save;
}

/* CODE:4C93, called by the driver a tenth of the display after the
 * retrace */
static void CRT_START_CB(void)
{
    uint32_t save = pm_ds;

    pm_ds = PI_IMAGE_BASE;
    if (rb(N_CRT_START_ON) == 1) {
        uint16_t bx = rw(N_CRT_START);

        vga_outw(0x3D4, (uint16_t)((bx & 0xFF00) | 0x0C));
        vga_outw(0x3D4, (uint16_t)(bx << 8 | 0x0D));
    }
    pm_ds = save;
}

void ns_far_call(uint16_t sel, uint32_t off)
{
    if (sel == pmax_code_sel()) {
        switch (off) {
        case 0x4C85:
            VSYNC_CB();
            return;
        case 0x4C93:
            CRT_START_CB();
            return;
        }
    }
    pi_stop("a driver's callback not translated");
}

static void driver(NsRegs *r)
{
    r->ds = pi_image.desc[ILLUSION_CODE].sel;
    ns_call(rw(N_DRIVER_ENTRY + 4), r);
}

/* CODE:4CB6: the driver's commands 0Eh and 0Fh */
static void VSYNC_START(void)
{
    NsRegs r = { 0 };

    r.eax = 0x0E;
    r.edx = 0x4C85;
    r.es = pmax_code_sel();
    driver(&r);
    memset(&r, 0, sizeof r);
    r.eax = 0x0F;
    r.ecx = 0x1999;
    r.edx = 0x4C93;
    r.es = pmax_code_sel();
    driver(&r);
}

/* CODE:236A: the driver's command 1 */
static void MUSIC_PLAY(void)
{
    NsRegs r = { 0 };

    r.eax = 1;
    r.ecx = 0x0F;
    driver(&r);
}

/* ---- CHOOSER_WAIT ---- */

static void MUSIC_MIX(void)
{
    NsRegs r = { 0 };

    r.eax = 6;
    driver(&r);
}

static unsigned long chooser_frames;    /* the port's count, for its stop */

/* CODE:25BB; FRAME_SPINS, the original's count of its wait, is not
 * kept */
static void FRAME_WAIT(void)
{
    uint32_t c = rd(N_VSYNC_COUNT);
    static char why[96];

    while (rd(N_VSYNC_COUNT) == c)
        if (!frame_wait()) {
            snprintf(why, sizeof why, "FRAME_WAIT (the window closed; %lu chooser frames)",
                     chooser_frames);
            pi_stop(why);
        }
    chooser_frames++;
    if (rb(0x1534) == 1)
        pi_stop("FRAME_WAIT with CODE:1534 1 (the driver's commands 8, 0Dh, 0Ch)");
}

/* CODE:381C */
static void CRT_START_PICK(void)
{
    if (rw(N_ATTRACT_TIME) == 1 && rb(N_ATTRACT_STAGE) == 9)
        ww(N_CRT_START, rw(N_SCROLL_SHOWN));
    else
        ww(N_CRT_START, rw(N_SCROLL_TOP));
}

/* A caption routine GEN_CODE or GEN_CODE2 made, at the far pointer at
 * CODE:`ptr`, run as the CPU would: DS video memory, ES CODE, SI and BX 0, DI
 * `di`; only the instructions the generators write */
static void CAPTION_RUN(uint32_t ptr, uint16_t di)
{
    uint32_t p = pmax_base(rw(ptr + 4)) + rd(ptr);
    uint16_t si = 0, d;
    uint8_t op, m;

    for (;;) {
        op = lrb(p);
        m = lrb(p + 1);
        switch (op) {
        case 0x26:                      /* MOV SI,ES:[w] / ADD SI,ES:[w] */
            if (lrb(p + 2) != 0x36)
                goto bad;
            if (m == 0x8B)
                si = rw(lrw(p + 3));
            else if (m == 0x03)
                si = (uint16_t)(si + rw(lrw(p + 3)));
            else
                goto bad;
            p += 5;
            continue;
        case 0x81:                      /* ADD DI,imm16 */
            if (m != 0xC7)
                goto bad;
            di = (uint16_t)(di + lrw(p + 2));
            p += 4;
            continue;
        case 0x66:                      /* RETF (32-bit) */
            if (m != 0xCB)
                goto bad;
            return;
        case 0x8A:                      /* MOV AL,[SI+d] */
            if ((m & 0x3F) != 0x04)
                goto bad;
            break;
        case 0xC6:                      /* MOV BYTE PTR [DI+d],imm8 */
            if ((m & 0x3F) != 0x05)
                goto bad;
            break;
        case 0x88:                      /* MOV [DI+d],BH */
            if ((m & 0x3F) != 0x3D)
                goto bad;
            break;
        default:
            goto bad;
        }
        p += 2;
        switch (m >> 6) {
        case 0:
            d = 0;
            break;
        case 1:
            d = (uint16_t)(int8_t)lrb(p);
            p += 1;
            break;
        case 2:
            d = lrw(p);
            p += 2;
            break;
        default:
            goto bad;
        }
        if (op == 0x8A) {
            vga_read((uint16_t)(si + d));       /* the latches loaded */
        } else if (op == 0xC6) {
            vga_write((uint16_t)(di + d), lrb(p));
            p += 1;
        } else {
            vga_write((uint16_t)(di + d), 0);   /* BH: EBX is 0 */
        }
        continue;
bad:
        pi_stop("CAPTION_RUN: an instruction the generators do not write");
    }
}

/* CODE:32A2: `second` 1 for CAPTION_DRAW2 (CODE:32EE) */
static void CAPTION_DRAW(int second)
{
    uint32_t edx = rd(N_CAPTION);
    uint32_t off;

    wb(N_CAPTION_FLIP, rb(N_CAPTION_FLIP) ^ 0xFF);
    if (second)
        off = rb(N_CAPTION_FLIP) & 0x80 ? 0x1E : 0x18;
    else if (rb(N_CAPTION_FLIP) & 0x80)
        off = rb(N_SCROLL_PHASE) == 2 ? 0x12 : 0x06;
    else
        off = rb(N_SCROLL_PHASE) == 2 ? 0x0C : 0x00;
    CAPTION_RUN(edx + off, rw(N_SCROLL_SHOWN));
    if (second)
        MUSIC_MIX();
}

/* CODE:3325 */
static void PAL_FADE(void)
{
    uint8_t bl, bh;
    int i;

    if (rb(N_CH_FADE_LEVEL) == 0) {
        if (rb(N_MENU_FADE) == 1)
            wb(N_MENU_DONE, 1);
        return;
    }
    wb(N_CH_FADE_LEVEL, (uint8_t)(rb(N_CH_FADE_LEVEL) - rb(N_CH_FADE_STEP)));
    if (rb(N_CH_FADE_LEVEL) & 0x80)
        wb(N_CH_FADE_LEVEL, 0);
    bl = rb(N_CH_FADE_LEVEL);
    bh = 0x40;
    if (bl > bh)
        bl = bh;
    bh = (uint8_t)(bh - bl);
    for (i = 0x33; i > 0; i--) {
        uint16_t ax = (uint16_t)(rb(rd(N_PAL_FROM) + i - 1) * bl + rb(rd(N_PAL_TO) + i - 1) * bh);

        wb(N_CHOOSER_PAL + i - 1, (uint8_t)(ax >> 6));
    }
}

/* CODE:31DE */
static void LAG_SHIFT(void)
{
    uint16_t ax = rw(N_LAG_PIC + 0x0E), t, bx = rw(N_LAG_STEP);
    int i;

    for (i = 6; i >= 0; i--) {
        t = rw(N_LAG_PIC + 2 * i);
        ww(N_LAG_PIC + 2 * i, ax);
        ax = t;
    }
    ax = rw(N_LAG_COL + 0x0E);
    for (i = 6; i >= 0; i--) {
        if (i < 6) {
            ax = (uint16_t)(ax + bx);
            if (ax >= 0x80)
                ax = (uint16_t)(ax - 0x80);
        }
        t = rw(N_LAG_COL + 2 * i);
        ww(N_LAG_COL + 2 * i, ax);
        ax = t;
    }
}

/* CODE:29FF: `cl` columns of 32 latch copies */
static void STRIP_COPY(uint16_t si, uint16_t di, uint8_t cl)
{
    int k;

    do {
        for (k = 0; k < 0x20; k++) {
            vga_read((uint16_t)(si + 0x72A4 + 0xB8 * k));
            vga_write((uint16_t)(di + 0x2E * k), 0);
            vga_write((uint16_t)(di + 0xC9BC + 0x2E * k), 0);
        }
        di++;
        si++;
    } while (--cl);
}

/* CODE:2BE8 */
static void STRIP_DRAW(uint16_t ax)
{
    uint16_t di = (uint16_t)(rw(N_SCROLL_TOP) + ax);
    uint16_t si = (uint16_t)(rw(N_SCROLL_COL) << 2), dx;
    uint8_t w = rb(N_WAVE_TAB + rw(N_WAVE_POS));

    si = (uint16_t)(si + (w >> 3));
    dx = rw(N_SHIFT_OFFS + 2 * (w & 7));
    if (si >= 0x80)
        si = (uint16_t)(si - 0x80);
    ww(N_LAG_COL + 0x0E, si);
    ww(N_LAG_PIC + 0x0E, (uint16_t)(dx + 0x72A4));
    STRIP_COPY((uint16_t)(si + dx), di, 0x2E);
}

/* CODE:2575, after SCROLL_NEXT (CODE:259F, `add` 5C0h) or SCROLL_NEXT4
 * (CODE:255A, 5C4h) */
static void SCROLL_NEXT(uint16_t add)
{
    ww(N_SCROLL_SHOWN, rw(N_SCROLL_TOP));
    ww(N_SCROLL_TOP, (uint16_t)(rw(N_SCROLL_TOP) + add));
    if (rw(N_SCROLL_TOP) >= 0x44A4) {
        ww(N_SCROLL_TOP, (uint16_t)(rw(N_SCROLL_TOP) - 0x3644));
        wb(0x18FD, 0);
        wb(N_MENU_BLINK, rb(N_MENU_BLINK) ^ 1);
    } else {
        wb(0x18FD, (uint8_t)(rb(0x18FD) + 1));
    }
}

/* CODE:2C55 */
static void SCROLL_STEP(void)
{
    ww(N_WAVE_POS, (uint16_t)(rw(N_WAVE_POS) + 2));
    if (rw(N_WAVE_POS) >= 0x300)
        ww(N_WAVE_POS, (uint16_t)(rw(N_WAVE_POS) - 0x300));
    wb(N_SCROLL_PHASE, (uint8_t)(rb(N_SCROLL_PHASE) - 1));
    if (rb(N_SCROLL_PHASE) != 0) {
        STRIP_DRAW(0x2840);
        ww(N_LAG_STEP, 0);
        SCROLL_NEXT(0x5C0);
        return;
    }
    /* COL_NEXT (CODE:2980) */
    ww(N_SCROLL_COL, (uint16_t)(rw(N_SCROLL_COL) + 1));
    if (rw(N_SCROLL_COL) >= 0x20)
        ww(N_SCROLL_COL, 0);
    STRIP_DRAW(0x2844);
    ww(N_LAG_STEP, 4);
    SCROLL_NEXT(0x5C4);
    wb(N_SCROLL_PHASE, 3);
}

/* a new palette to fade to (MENU_KEYS' and ATTRACT_STEP's common steps) */
static void FADE_TO(uint32_t to)
{
    wd(N_PAL_FROM, rd(N_PAL_TO));
    wd(N_PAL_TO, to);
}

/* CODE:3492 */
static void MENU_KEYS(void)
{
    uint8_t bl = rb(N_KEY_READ) & 0x0F, bh = rb(N_KEY_READ + 1) & 0x0F, al;

    if (bl == bh)
        return;
    if (rb(N_MENU_FADE) == 1)
        goto next;
    al = rb(0x1906 + bl);
    switch (al) {
    case 0x01:                          /* Esc */
        if (rb(N_ESC_KEY) == 1)
            break;
        wb(N_WAIT_END, 1);
        wb(N_ESC_KEY, 1);
        wb(N_CH_FADE_LEVEL, 0x40);
        FADE_TO(0x13D7);
        wb(N_MENU_FADE, 1);
        wb(N_CH_FADE_STEP, 1);
        break;
    case 0x50:                          /* Down */
        al = (uint8_t)(rb(N_MENU_ROW) + 1);
        if (al >= rb(N_MENU_ROWS))
            al = (uint8_t)(rb(N_MENU_ROWS) - 1);
        wb(N_MENU_ROW, al);
        break;
    case 0x48:                          /* Up */
        al = (uint8_t)(rb(N_MENU_ROW) - 1);
        if ((int8_t)al < 0)
            al = 0;
        wb(N_MENU_ROW, al);
        break;
    case 0x4D:                          /* Right */
        wb(N_MENU_INFO, 1);
        break;
    case 0x4B:                          /* Left */
        wb(N_MENU_INFO, 0);
        break;
    case 0x1C:                          /* Enter, Space */
    case 0x39:
        wb(N_MENU_ENTER, 1);
        wb(N_WAIT_END, 1);
        if (rb(N_MENU_ON) != 1)
            break;
        wb(N_CH_FADE_STEP, 2);
        wb(N_CH_FADE_LEVEL, 0x40);
        FADE_TO(rb(N_MENU_INFO) == 1 ? 0x1371 : 0x13D7);
        wb(N_MENU_FADE, 1);
        break;
    case 0x3B: case 0x3C: case 0x3D: case 0x3E:        /* F1..F4 */
        if (rb(N_MENU_ON) != 1)
            break;
        wb(N_MENU_ROW, (uint8_t)(al - 0x3B));
        wb(N_MENU_INFO, 0);
        wb(N_MENU_ENTER, 1);
        wb(N_WAIT_END, 1);
        wb(N_CH_FADE_STEP, 2);
        wb(N_CH_FADE_LEVEL, 0x40);
        FADE_TO(0x13D7);
        wb(N_MENU_FADE, 1);
        break;
    }
next:
    wb(N_KEY_READ, (uint8_t)(rb(N_KEY_READ) + 1));
}

/* CODE:3849 */
static void CAPTION_STEP(void)
{
    uint8_t al;

    if (rb(0x18FD) != 1)
        return;
    if (rb(N_ESC_KEY) != 1 && rb(N_WAIT_END) == 1) {
        if (rd(N_CAPTION) != 0x1728)
            wd(N_CAPTION, 0x1728);
        else
            wb(N_CAPTION_OUT, (uint8_t)(rb(N_CAPTION_OUT) + 1));
        return;
    }
    wb(N_CAPTION_TIME, (uint8_t)(rb(N_CAPTION_TIME) + 1));
    al = rb(N_CAPTION_TIME) & 0x1F;
    if (al == 0) {
        uint32_t esi = rd(N_CAPTION_NEXT), eax = rd(esi);

        if (eax == 0xFFFFFFFFu) {
            esi = N_CAPTIONS;
            eax = rd(esi);
        }
        wd(N_CAPTION, eax);
        wd(N_CAPTION_NEXT, esi + 4);
    } else if (al == 0x1E) {
        wd(N_CAPTION, 0x1728);
    }
}

static void CRT_START_OUT(void)
{
    uint16_t bx = rw(N_CRT_START);

    vga_outw(0x3D4, (uint16_t)((bx & 0xFF00) | 0x0C));
    vga_outw(0x3D4, (uint16_t)(bx << 8 | 0x0D));
}

/* CUBE_DRAW_WAIT's frame, every 16 lines (CODE:2687) */
static void CUBE_WAIT(uint8_t bl)
{
    vga_outb(0x3C5, 0x0F);
    FRAME_WAIT();
    ww(N_CRT_START, rw(N_SCROLL_SHOWN));
    CAPTION_DRAW(1);
    vga_outb(0x3C5, bl);
}

/* CODE:36F4 */
static void ATTRACT_STEP(void)
{
    uint8_t al;

    ww(N_ATTRACT_TIME, (uint16_t)(rw(N_ATTRACT_TIME) - 1));
    if (rw(N_ATTRACT_TIME) != 0)
        return;
    for (;;) {
        if (rb(N_MENU_FADE) == 1)
            return;
        al = rb(N_ATTRACT_STAGE);
        if (al == 9) {
            /* CODE:376A: the next backdrop drawn over 32 frames */
            uint8_t b = rb(N_BACKDROP);

            if (b >= 3)
                b = 0;
            wb(N_BACKDROP, b);
            CUBE_DRAW(rw(N_CUBE_SEL + 2 * b), 1);
            WRITE_MODE1();
            LAG_SHIFT();
            SCROLL_STEP();
            PAL_FADE();
            MENU_KEYS();
            CAPTION_STEP();
            ww(N_CRT_START, rw(N_SCROLL_SHOWN));
            CRT_START_OUT();
            CRT_START_OUT();
            MUSIC_MIX();
            FRAME_WAIT();
            CHOOSER_DAC();
            CRT_START_PICK();
            READ_MODE1();
            CAPTION_DRAW(0);
            wb(N_CH_FADE_STEP, 2);
            wb(N_ATTRACT_STAGE, 0);
            wb(N_BACKDROP, (uint8_t)(rb(N_BACKDROP) + 1));
            continue;
        }
        if (al == 1)
            wb(N_CH_FADE_STEP, 4);
        /* CODE:3716 */
        FADE_TO(0x191B + (uint16_t)(al * 0x33));
        wb(N_CH_FADE_LEVEL, 0x40);
        ww(N_ATTRACT_TIME, 0x12C);
        wb(N_ATTRACT_STAGE, (uint8_t)(rb(N_ATTRACT_STAGE) + 1));
        if (rb(N_ATTRACT_STAGE) == 9) {
            wb(N_CH_FADE_STEP, 4);
            ww(N_ATTRACT_TIME, 0x11);
        }
        return;
    }
}

/* CODE:38C6 */
static void CHOOSER_WAIT(void)
{
    wd(N_CAPTION, 0x1728);
    wd(N_CAPTION_NEXT, N_CAPTIONS);
    wb(N_MENU_ON, 0);
    wb(N_MENU_ROW, 0);
    wb(N_CAPTION_OUT, 0);
    wb(N_WAIT_END, 0);
    wb(N_CAPTION_TIME, 0x14);
    do {
        FRAME_WAIT();
        CHOOSER_DAC();
        CRT_START_PICK();
        READ_MODE1();
        CAPTION_DRAW(0);
        ATTRACT_STEP();
        WRITE_MODE1();
        LAG_SHIFT();
        SCROLL_STEP();
        PAL_FADE();
        /* CODE:23A2 leaves only registers */
        MENU_KEYS();
        CAPTION_STEP();
        MUSIC_MIX();
        if (rd(N_KEY_HISTORY) == 0x0F3A2A1Du)
            wb(N_WAIT_END, 1);
    } while (rb(N_MENU_DONE) != 1 && rb(N_CAPTION_OUT) < 4);
}

/* CODE:364F */
static void MENU_CAPTION(void)
{
    uint32_t row = rb(N_MENU_ROW);

    if (rb(N_MENU_ON) != 1 || rb(0x18FD) != 1)
        return;
    if (rb(N_MENU_BLINK) == 1)
        wd(N_CAPTION, rd(N_MENU_PICS_NOBOX + 4 * row));
    else if (rb(N_MENU_INFO) == 1)
        wd(N_CAPTION, rd(N_MENU_INFO_PICS + 4 * row));
    else
        wd(N_CAPTION, rd(N_MENU_PICS + 4 * row));
}

/* CODE:3969 */
static void MENU_LOOP(void)
{
    wb(N_MENU_ON, 1);
    wb(N_MENU_DONE, 0);
    do {
        FRAME_WAIT();
        CHOOSER_DAC();
        CRT_START_PICK();
        READ_MODE1();
        CAPTION_DRAW(0);
        ATTRACT_STEP();
        WRITE_MODE1();
        LAG_SHIFT();
        SCROLL_STEP();
        PAL_FADE();
        MENU_KEYS();
        MENU_CAPTION();
        MUSIC_MIX();
    } while (rb(N_MENU_DONE) != 1);
}

/* CODE:237C */
static void MUSIC_STOP(void)
{
    NsRegs r = { 0 };

    r.eax = 3;
    driver(&r);
}

/* CODE:4FF9, CHOOSER, to the chooser's end */
/* ---- the Info and greetings pages ---- */

/* CODE:3CFD: DAC 0..127 from CODE:`from` */
static void PAGE_DAC(uint32_t from)
{
    int i;

    vga_outb(0x3C8, 0);
    for (i = 0; i < 0x180; i++)
        vga_outb(0x3C9, rb(from + i));
}

/* CODE:3D19 */
static void PAGE_FADE(void)
{
    uint8_t bl, bh;
    int i;

    if ((int8_t)rb(N_CH_FADE_LEVEL) < 0)
        return;
    bl = rb(N_CH_FADE_LEVEL);
    bh = 0x40;
    if (bl > bh)
        bl = bh;
    bh = (uint8_t)(bh - bl);
    for (i = 0x180; i > 0; i--) {
        uint16_t ax = (uint16_t)(rb(rd(N_PAL_FROM) + i - 1) * bl + rb(rd(N_PAL_TO) + i - 1) * bh);

        wb(N_PAGE_PAL + i - 1, (uint8_t)(ax >> 6));
    }
    wb(N_CH_FADE_LEVEL, (uint8_t)(rb(N_CH_FADE_LEVEL) - rb(N_CH_FADE_STEP)));
}

/* CODE:3B72 (`right` 0) and CHAR_PUT_R (CODE:3CCE, 1): character `ch`
 * of tinyfont.fnt at the pixel address *edi */
static void CHAR_PUT(uint8_t ch, uint32_t *edi, int right)
{
    uint32_t fs = pmax_base(rw(N_TINYFONT_SEL)), esi, end, ebx;
    uint8_t w = lrb(fs + ch * 4u + 2), al;

    if (right)
        *edi -= w;
    ebx = (uint32_t)(uint16_t)(0x4C * lrb(fs + ch * 4u + 3)) + (*edi >> 2);
    al = (uint8_t)(0x11 << (*edi & 3) | 0x11 >> (8 - (*edi & 3)));
    if (!right)
        *edi += w;
    end = lrw(fs + ch * 4u + 4);
    esi = lrw(fs + ch * 4u);
    if (esi == end)
        return;
    vga_outb(0x3C4, 2);
    do {
        uint16_t bp = lrw(fs + esi);
        uint32_t b = ebx;
        int k;

        bp = (uint16_t)(bp << 8 | bp >> 8);
        esi += 2;
        for (k = 0; k < 16; k++) {
            int carry = al >> 7;

            al = (uint8_t)(al << 1 | carry);
            b += (uint32_t)carry;
            if (bp & 0x8000) {
                vga_outb(0x3C5, al);
                vga_write((uint16_t)b, 1);
            }
            bp = (uint16_t)(bp << 1);
        }
        ebx += 0x4C;
    } while (esi < end);
}

/* CODE:3D72 */
static void PAGE_PAL_MAKE(void)
{
    uint32_t src = pmax_base(rw(N_INFODATA_SEL)) + rd(N_PAGE_BASE) + 0x0A;
    int i;

    for (i = 0; i < 0x300; i++)
        wb(N_PAGE_PAL_TO + i, lrb(src + i));
    ww(N_PAGE_PAL_TO + 0x30, 0);
    wb(N_PAGE_PAL_TO + 0x32, 0x10);
    wb(N_PAGE_PAL_TO + 3, 0x3F);
    wb(N_PAGE_PAL_TO + 4, 0x3F);
    wb(N_PAGE_PAL_TO + 5, 0x3F);
    wd(N_PAL_FROM, 0x8A3C);
    wd(N_PAL_TO, N_PAGE_PAL_TO);
    wb(N_CH_FADE_LEVEL, 0x40);
}

/* CODE:3DD7 */
static void TILE_DRAW(uint8_t al)
{
    uint32_t src = pmax_base(rw(N_INFODATA_SEL)) + rd(N_PAGE_BASE)
                   + ((al & 0xF0u) << 6) + ((al & 0x0Fu) << 3) + 0x30A;
    uint16_t di = (uint16_t)((al & 0xF0) * 0x26 + (al & 0x0F) * 2 + 0x28A);
    int row, p;

    vga_outb(0x3C4, 2);
    for (row = 0; row < 8; row++) {
        for (p = 0; p < 4; p++) {
            vga_outb(0x3C5, (uint8_t)(1 << p));
            vga_write(di, lrb(src + p));
            vga_write((uint16_t)(di + 1), lrb(src + 4 + p));
        }
        src += 0x80;
        di = (uint16_t)(di + 0x4C);
    }
}

/* CODE:3FCA */
static void TILE_STEP(void)
{
    uint32_t ebx = rd(N_TILE_ORDER);

    if (ebx == 0xFFFFFFFFu)
        return;
    TILE_DRAW(rb(ebx + rb(N_TILE_COUNT)));
    wb(N_TILE_COUNT, (uint8_t)(rb(N_TILE_COUNT) + 1));
    if (rb(N_TILE_COUNT) == 0)
        wd(N_TILE_ORDER, 0xFFFFFFFFu);
}

/* the start of the next row of 48 lines (E40h bytes), plus `add`: the
 * 16-bit DIV and MUL of CODE:4046 */
static uint32_t NEXT_ROW(uint32_t eax, uint32_t add)
{
    uint16_t q = (uint16_t)((uint16_t)eax / 0xE40);

    return ((eax & 0xFFFF0000u) | (uint16_t)((q + 1) * 0xE40)) + add;
}

/* CODE:41A8 (`base` INFODATA.MGL's) and LINE_MEASURE2 (CODE:4197, base
 * CODE's) */
static void LINE_MEASURE(uint32_t base)
{
    uint32_t fs = pmax_base(rw(N_TINYFONT_SEL)), esi = rd(N_TEXT_POS);
    uint16_t bx = 0;
    uint8_t dl = 0, dh = 0, al;

    for (;;) {
        al = lrb(base + esi++);
        if (al == 0x20) {
            dh = dl;
        } else if (al == 0x0A) {
            dh = dl;
            break;
        } else if (al == 0) {
            wb(N_LINE_LEFT, dl);
            return;
        }
        dl++;
        bx = (uint16_t)(bx + lrb(fs + al * 4u + 2));
        if (bx >= rw(N_WRAP_WIDTH))
            break;
    }
    wb(N_LINE_LEFT, (uint8_t)(dh + 1));
}

/* CODE:3FF3 (`base` INFODATA.MGL's) and TEXT_STEP2 (CODE:405E, CODE's) */
static void TEXT_STEP(uint32_t base)
{
    uint32_t esi = rd(N_TEXT_POS), edi;
    uint8_t al;

    if (esi == 0xFFFFFFFFu)
        return;
    if (rb(N_LINE_LEFT) == 0) {
        LINE_MEASURE(base);
        wd(N_TEXT_DI, NEXT_ROW(rd(N_TEXT_DI), 4));
        return;
    }
    edi = rd(N_TEXT_DI);
    al = lrb(base + esi);
    if (al != 0x0A)
        CHAR_PUT(al, &edi, 0);
    wd(N_TEXT_POS, rd(N_TEXT_POS) + 1);
    wd(N_TEXT_DI, edi);
    wb(N_LINE_LEFT, (uint8_t)(rb(N_LINE_LEFT) - 1));
}

/* CODE:40C1 (TITLE_TEXT at TITLE_DI, a newline + A8h) and SCORES_STEP
 * (CODE:412C, right-aligned, + 128h) */
static void COLUMN_STEP(uint32_t text, uint32_t di, uint32_t add, int right)
{
    uint32_t esi = rd(text), edi;
    uint8_t al;

    if (esi == 0xFFFFFFFFu)
        return;
    edi = rd(di);
    al = rb(esi);
    if (al == 0) {
        wd(text, 0xFFFFFFFFu);
    } else if (al == 0x0A) {
        wd(text, esi + 1);
        wd(di, NEXT_ROW(rd(di), add));
    } else {
        CHAR_PUT(al, &edi, right);
        wd(text, esi + 1);
        wd(di, edi);
    }
}

/* CODE:4442 */
static void TITLE_MAKE(void)
{
    uint32_t esi = rb(N_MENU_ROW) * 0x32u + N_HISCORES, edi = N_TITLE_BUF;
    int i;

    for (i = 0; i < 5; i++) {
        wb(edi++, rb(esi));
        wb(edi++, rb(esi + 1));
        wb(edi++, rb(esi + 2));
        wb(edi++, i < 4 ? 0x0A : 0);
        esi += 0x0A;
    }
}

/* CODE:448B */
static void SCORES_MAKE(void)
{
    static const int order[6] = { 6, 7, 8, 9, 4, 5 };
    uint32_t esi = rb(N_MENU_ROW) * 0x32u + N_HISCORES, edi = N_SCORES_BUF;
    int i, k, n;

    for (i = 0; i < 5; i++) {
        for (k = 0; k < 6; k++) {
            uint8_t b = rb(esi + order[k]);

            /* AAM 10h: the low nibble first; CODE:447F puts a dot after it
             * for bytes +7 and +4, CODE:4476 none; a dot too between +8
             * and +9 */
            wb(edi++, (uint8_t)((b & 0x0F) + 0x30));
            if (order[k] == 7 || order[k] == 4)
                wb(edi++, 0x2E);
            wb(edi++, (uint8_t)((b >> 4) + 0x30));
            if (order[k] == 8)
                wb(edi++, 0x2E);
        }
        for (n = 0x0F; n > 0; n--) {
            if (rb(edi - 1) != 0x30 && rb(edi - 1) != 0x2E)
                break;
            edi--;
        }
        wb(edi++, 0x0A);
        esi += 0x0A;
    }
    wb(edi - 1, 0);
}

/* CODE:4214 */
static void PAGE_MODE(void)
{
    int i, k;

    FRAME_WAIT();
    MUSIC_MIX();
    ww(N_CRT_START, 0);
    vga_outw(0x3CE, 0xFF08);
    vga_outw(0x3CE, 0x0005);
    vga_outw(0x3CE, 0x0003);
    vga_outw(0x3C4, 0x0F02);
    for (k = 0; k < 8; k++) {
        for (i = 0; i < 0x2000; i++)
            vga_write((uint16_t)(k * 0x2000 + i), 0);
        FRAME_WAIT();
        MUSIC_MIX();
    }
    FRAME_WAIT();
    PAGE_DAC(0x8A3C);
    MUSIC_MIX();
    ww(N_CRT_START, 0);
    FRAME_WAIT();
    MUSIC_MIX();
    wb(N_CRT_START_ON, 0);
    FRAME_WAIT();
    MUSIC_MIX();
    FRAME_WAIT();
    vga_outw(0x3C4, 0x0000);
    vga_outw(0x3C4, 0x0101);
    vga_outw(0x3C4, 0x0F02);
    vga_outw(0x3C4, 0x0604);
    vga_outw(0x3C4, 0x0300);
    vga_outw(0x3D4, 0x0E11);
    for (i = 0; i < 0x19; i++) {
        vga_outb(0x3D4, (uint8_t)i);
        vga_outb(0x3D5, rb(N_PAGE_CRTC + i));
    }
    vga_inb(0x3DA);
    vga_outb(0x3C0, 0x30);
    vga_outb(0x3C0, 0x41);
    for (i = 0; i < 5; i++)
        vga_outw(0x3CE, (uint16_t)i);
    vga_outw(0x3CE, 0x4005);
    vga_outw(0x3CE, 0x0506);
    vga_outw(0x3CE, 0x0F07);
    vga_outw(0x3CE, 0xFF08);
    MUSIC_MIX();
    FRAME_WAIT();
    MUSIC_MIX();
}

/* CODE:4357 */
static void PAGE_MODE_END(void)
{
    int i;

    MUSIC_MIX();
    FRAME_WAIT();
    for (i = 0; i < 9; i++)
        vga_outw(0x3D4, rw(N_CHOOSER_CRTC + 2 * i));
    vga_outw(0x3C4, 0x0901);
    vga_outw(0x3C4, 0x0F02);
    vga_outw(0x3C4, 0x0604);
    vga_outb(0x3C0, 0x30);
    vga_outb(0x3C0, 0x01);
    for (i = 0; i < 5; i++)
        vga_outw(0x3CE, (uint16_t)i);
    vga_outw(0x3CE, 0x0005);
    vga_outw(0x3CE, 0x0506);
    vga_outw(0x3CE, 0x0F07);
    vga_outw(0x3CE, 0xFF08);
    /* VIDEO_TOP_CLEAR (CODE:2522) */
    vga_outw(0x3C4, 0x0F02);
    for (i = 0; i < 0x5C; i++)
        vga_write((uint16_t)i, 0);
    wb(N_CRT_START_ON, 1);
}

/* CODE:43EF */
static void PAGE_BORDER(void)
{
    int i;

    vga_outw(0x3C4, 0x0F02);
    for (i = 0; i < 0x4C; i++) {
        vga_write((uint16_t)i, 1);
        vga_write((uint16_t)(0x4234 + i), 1);
    }
    vga_outw(0x3C4, 0x0102);
    for (i = 0; i < 0xDE; i++)
        vga_write((uint16_t)(0x4C + 0x4C * i), 1);
    vga_outw(0x3C4, 0x0802);
    for (i = 0; i < 0xDE; i++)
        vga_write((uint16_t)(0x97 + 0x4C * i), 1);
}

/* PAGE_KEYS (CODE:3465): only Esc */
static void PAGE_KEYS(void)
{
    uint8_t bl = rb(N_KEY_READ) & 0x0F, bh = rb(N_KEY_READ + 1) & 0x0F;

    if (bl == bh)
        return;
    if (rb(0x1906 + bl) == 1)
        wb(N_ESC_KEY, 1);
    wb(N_KEY_READ, (uint8_t)(rb(N_KEY_READ) + 1));
}

/* the pages' common end: the fade out (from CODE:45F5 and 4935) */
static void PAGE_LEAVE(void)
{
    wb(N_ESC_KEY, 0);
    wb(N_MENU_FADE, 0);
    wd(N_PAL_FROM, N_PAGE_PAL_TO);
    wd(N_PAL_TO, 0x8A3C);
    wb(N_CH_FADE_LEVEL, 0x40);
}

static void PAGE_FADE_OUT(void)
{
    do {
        MUSIC_MIX();
        PAGE_FADE();
        FRAME_WAIT();
        PAGE_DAC(N_PAGE_PAL);
    } while ((int8_t)rb(N_CH_FADE_LEVEL) > -1);
}

/* the chooser again, the next backdrop (CODE:4687, 49F1) */
static void PAGE_BACKDROP(void)
{
    uint8_t b;

    SPLIT_SET(0x1BD);
    CHOOSER_DAC();
    WRITE_MODE1();
    READ_MODE1();
    VIDEO_TOP_SET();
    wb(N_MENU_FADE, 0);
    wb(N_MENU_DONE, 0);
    b = (uint8_t)(rb(N_BACKDROP) + 1);
    if (b >= 3)
        b = 0;
    wb(N_BACKDROP, b);
    CUBE_DRAW(rw(N_CUBE_SEL + 2 * b), 2);
}

/* CODE:4504 */
static void INFO_PAGE(void)
{
    uint32_t info = pmax_base(rw(N_INFODATA_SEL));

    ww(N_WRAP_WIDTH, 0xA0);
    PAGE_MODE();
    MUSIC_MIX();
    wd(N_PAGE_BASE, lrd(info + 4 * rb(N_MENU_ROW) + 4));
    wd(N_TEXT_POS, 0x430A + rd(N_PAGE_BASE));
    PAGE_BORDER();
    PAGE_PAL_MAKE();
    wd(N_TILE_ORDER, N_TILE_PERM);
    wd(N_TEXT_DI, 0x394);
    wd(N_TITLE_DI, 0xABA8);
    wd(N_SCORES_DI, 0xAC28);
    wd(N_TITLE_TEXT, N_TITLE_BUF);
    wd(N_SCORES_TEXT, N_SCORES_BUF);
    wb(N_TILE_COUNT, 0);
    LINE_MEASURE(info);
    TITLE_MAKE();
    SCORES_MAKE();
    do {
        MUSIC_MIX();
        TILE_STEP();
        TEXT_STEP(info);
        COLUMN_STEP(N_TITLE_TEXT, N_TITLE_DI, 0xA8, 0);
        COLUMN_STEP(N_SCORES_TEXT, N_SCORES_DI, 0x128, 1);
        TILE_STEP();
        TEXT_STEP(info);
        TILE_STEP();
        TEXT_STEP(info);
        PAGE_KEYS();
        PAGE_FADE();
        FRAME_WAIT();
        PAGE_DAC(N_PAGE_PAL);
    } while (rb(N_ESC_KEY) != 1);
    PAGE_LEAVE();
    PAGE_FADE_OUT();
    PAGE_MODE_END();
    SCROLL_INIT();
    ATTRACT_START();
    wd(N_PAL_FROM, 0x1371);
    PAGE_BACKDROP();
}

/* CODE:4886 */
static void GREETINGS_PAGE(void)
{
    int i;

    ww(N_WRAP_WIDTH, 0x128);
    PAGE_MODE();
    MUSIC_MIX();
    wd(N_TEXT_POS, N_GREETINGS_TEXT);
    for (i = 0; i < 0x300; i++)
        wb(N_PAGE_PAL_TO + i, 0);
    ww(N_PAGE_PAL_TO + 0x30, 0);
    wb(N_PAGE_PAL_TO + 0x32, 0x10);
    wb(N_PAGE_PAL_TO + 3, 0x3F);
    wb(N_PAGE_PAL_TO + 4, 0x3F);
    wb(N_PAGE_PAL_TO + 5, 0x3F);
    wd(N_PAL_FROM, 0x8A3C);
    wd(N_PAL_TO, N_PAGE_PAL_TO);
    wb(N_CH_FADE_LEVEL, 0x40);
    PAGE_BORDER();
    wd(N_TEXT_DI, 0x394);
    LINE_MEASURE(PI_IMAGE_BASE);
    do {
        MUSIC_MIX();
        TEXT_STEP(PI_IMAGE_BASE);
        TEXT_STEP(PI_IMAGE_BASE);
        PAGE_KEYS();
        PAGE_FADE();
        FRAME_WAIT();
        PAGE_DAC(N_PAGE_PAL);
    } while (rb(N_ESC_KEY) != 1);
    PAGE_LEAVE();
    wb(N_CH_FADE_STEP, 2);
    PAGE_FADE_OUT();
    PAGE_MODE_END();
    SCROLL_INIT();
    wb(N_ATTRACT_STAGE, 1);
    ww(N_ATTRACT_TIME, 0x12C);
    wd(N_PAL_FROM, 0x1371);
    wd(N_PAL_TO, 0x191B);
    wb(N_CH_FADE_LEVEL, 0x46);
    wb(N_CH_FADE_STEP, 2);
    PAL_FADE();
    PAGE_BACKDROP();
}

static uint8_t CHOOSER(void)
{
    uint8_t al;
    int i;

    /* the checksummed calls in their order (targets from the run's
     * memory, docs/HANDOFF.md "The chooser's timer") */
    CUBE_DRAW(rw(N_CUBE_SEL), 0);
    VSYNC_START();
    MUSIC_PLAY();
    wb(N_CHOOSER_LOADED, 0);
    CHOOSER_WAIT();
    /* CODE:505C, the table menu */
    if (rb(N_MENU_DONE) != 1) {
        wb(N_MENU_ROW, 0);
        if (rd(N_KEY_HISTORY) == 0x0F3A2A1Du) {
            GREETINGS_PAGE();
            wb(N_MENU_INFO, 0);
        }
        for (;;) {
            MENU_LOOP();
            if (rb(N_MENU_INFO) == 0 || rb(N_ESC_KEY) == 1)
                break;
            INFO_PAGE();
            wb(N_MENU_INFO, 0);
        }
    }
    /* CODE:50A9 */
    if (rb(N_ESC_KEY) != 1) {
        static const char azerty[] = "BEFR", qwertz[] = "UHUYFSGSZCLSLPRG";
        uint16_t ax = pmax_country();
        uint32_t ebx = 0x5E21, esi;

        for (i = 0; i < 4; i += 2)
            if (ax == (uint16_t)(azerty[i] << 8 | azerty[i + 1]))
                ebx = 0x6023;
        if (ebx == 0x5E21)
            for (i = 0; i < 16; i += 2)
                if (ax == (uint16_t)(qwertz[i + 1] << 8 | qwertz[i]))
                    ebx = 0x5F22;
        wd(0x5E1D, ebx);
        for (esi = 0; esi < 0x100; esi++) {
            al = rb(ebx + esi);
            if (al < 0x20 || al > 0x7A)
                al = 0xFF;
            else if (al > 0x40)
                al &= 0xDF;
            wb(N_KEY_CHARS + esi, al);
        }
    }
    /* CODE:5198 */
    MUSIC_STOP();
    if (rb(N_ESC_KEY) == 1) {
        vga_outw(0x3C4, 0x0F02);
        vga_outw(0x3CE, 0x0005);
        for (i = 0; i < 0x10000; i++)
            vga_write((uint16_t)i, 0);
        /* INT 10h mode 3: the port's window keeps its picture; the
         * text follows on the console (ENTRY) */
    }
    /* CODE:51D6 */
    {
        NsRegs r = { 0 };

        r.eax = 5;
        driver(&r);
        memset(&r, 0, sizeof r);
        r.eax = 0x0B;
        driver(&r);
    }
    pmax_free_sel(rw(N_DRIVER_ENTRY + 4));
    CD_STOP();
    /* CHOOSER_FREE (CODE:28F5), CAPTIONS_FREE first */
    for (i = 0x1554; i < N_CAPTION; i += 6)
        if (rw(i + 4))
            pmax_free(rw(i + 4));
    pmax_free(rw(N_MENUCHAR_SEL));
    pmax_free(rw(N_CUBE_SEL));
    pmax_free(rw(N_TUBE_SEL));
    pmax_free(rw(N_TORUS_SEL));
    pmax_free(rw(N_TINYFONT_SEL));
    pmax_free(rw(N_INFODATA_SEL));
    pmax_free(rw(N_DRIVER_SEL));
    if (rb(0x037C) == 1)
        pi_stop("CODE:523B (the self-patched call written)");
    /* KBD_RESTORE (CODE:33F5): IRQ 1 masked, its vector back */
    frame_set_keyboard(NULL);
    al = rb(N_ESC_KEY) == 1 ? 0xFF : rb(N_MENU_ROW);
    if (rb(0x5E1C) == 1)
        pi_stop("CODE:5277 (INT 94h AH=6)");
    return al;
}

/* CODE:4CFB, the chooser: the table chosen, or FFh */
uint8_t CHOOSER_START(void)
{
    ww(N_CHOOSER_DS, PI_SEL_CODE);
    wb(0x5E1C, 0);
    ww(0x8A3C + 0x30, 0);
    wb(0x8A3C + 0x32, 0x10);
    wb(N_MENU_DONE, 0);
    wb(N_MENU_FADE, 0);
    wb(0x132D, 1);
    /* the checksummed jumps in their order (the -trace) */
    KBD_INSTALL();
    vga_set_mode(0x0D);
    CHOOSER_MODE();
    ATTRACT_START();
    CHOOSER_DAC();
    SPLIT_SET(0x1BD);
    ww(N_CHOOSER_VSEL, pmax_video_sel());
    VIDEO_CLEAR();
    SCROLL_INIT();
    MENUCHAR_INIT();
    wd(N_GEN_BUF, 0);                   /* CODE:28B8 */
    /* CODE:4E20: the overscan colour 10h */
    vga_outb(0x3C0, 0x31);
    vga_outb(0x3C0, 0x10);
    WRITE_MODE1();
    READ_MODE1();
    VIDEO_TOP_SET();
    /* BITMAP_ALLOC (INT 92h AH=7, name CODE:226F) */
    ww(N_BITMAP_SEL, pmax_alloc_top(0x5C08));
    if (!rw(N_BITMAP_SEL))
        pi_stop("BITMAP_ALLOC: no room (INT 92h AH=7)");
    CAPTIONS_MAKE();
    pmax_free(rw(N_BITMAP_SEL));        /* BITMAP_FREE */
    wb(0x1534, 0);
    if (rw(N_CHOOSER_LOADED) != 1)
        pi_stop("CODE:4E98 (the chooser's files loaded again)");
    return CHOOSER();
}
