/* setup.c - SETUP_ARGS (CODE:32F39): the language of SETSOUND.DAT's texts
 * and the command line's options.  The original's own screens behind
 * them (the options screen, the sound set-up, the options' reset) are
 * not in the port: the setup screen replaced them (its Game page keeps
 * the same option bytes, its Sound page the volume; the sound is always
 * NOSOUND).  Asked for, the port says so and ends.  SVGA_CHECK.
 */
#include <stdio.h>
#include <stdlib.h>
#include "game.h"
#include "names.h"
#include "platform.h"
#include "pmax.h"
#include "pmem.h"

/* CODE:328B4: SETSOUND.DAT through pMAX (INT 94h AH=1); SETSOUND_TEXTS the
 * language's table in it, YES_KEYS a word of its text +114h */
static void SETSOUND_LOAD(void)
{
    uint16_t sel;

    pmax_name(0x3290F, 0x1C);        /* ESI: the file name itself */
    sel = pmax_load("SETSOUND\\SETSOUND.DAT", NULL);
    uint32_t base, texts;

    if (!sel)       /* INT 21h AH=9, then AH=4Ch */
        pi_exit_text("Error loading the Setsound language file\r\n");
    base = pmax_base(sel);
    ww(N_SETSOUND_TEXTS + 4, sel);
    texts = lrd(base + (uint32_t)rb(N_LANGUAGE) * 4);
    wd(N_SETSOUND_TEXTS, texts);
    ww(N_YES_KEYS, lrw(base + lrd(base + texts + 0x114)));
}

/* CODE:32903: SETSOUND.DAT freed (INT 92h AH=5) */
static void SETSOUND_FREE(void)
{
    pmax_free(rw(N_SETSOUND_TEXTS + 4));
}

/* the original's own screen `what` asked for: not in the port, which
 * has the setup screen instead; said and ended, as the original ends
 * after its screens */
static void not_in_port(const char *what)
{
    plat_message(what);
    plat_shutdown();
    exit(0);
}

/* CF as the return value: 1 when the program is to end */
int SETUP_ARGS(void)
{
    const char *s = pmax_tail;
    int n;

    /* INT 93h AH=15h, the country: 'SV' Swedish, 'GR' or 'SG' German */
    switch (pmax_country()) {
    case 0x5356: wb(N_LANGUAGE, 2); break;
    case 0x4752: case 0x5347: wb(N_LANGUAGE, 1); break;
    default: wb(N_LANGUAGE, 0); break;
    }
    SETSOUND_LOAD();
    wb(N_ARG_S, 0);
    wb(N_ARG_O, 0);
    wb(N_ARG_R, 0);

    /* the tail up to its first '/' (at most 80h characters), then at most
     * 20h letters after it, upper-cased; a NUL or a space (20h AND DFh)
     * ends them, '?' prints the help */
    for (n = 0x80;;) {
        char c = *s++;
        if (c == 0)
            goto done;
        if (c == '/' || --n == 0)
            break;
    }
    if (*s) {
        uint8_t al;
        n = 0x20;
        do {
            al = (uint8_t)*s;
            if (al == '?') {
                /* the texts' entry +0D4h by INT 21h AH=9 */
                uint32_t base = pmax_base(rw(N_SETSOUND_TEXTS + 4));
                pi_print_dos(base + lrd(base + rd(N_SETSOUND_TEXTS) + 0xD4));
                SETSOUND_FREE();
                return 1;
            }
            al &= 0xDF;
            if (al == 'S')
                wb(N_ARG_S, 1);
            if (al == 'O')
                wb(N_ARG_O, 1);
            if (al == 'R')
                wb(N_ARG_R, 1);
            s++;
        } while (--n && al);
    }
done:

    if (rb(N_ARG_R) == 1)
        not_in_port("The options' reset is not in the port (its setup screen keeps the options).");
    if (rb(N_ARG_S) != 0
        || pmax_cfg_header(N_CFG_HEADER) != 0 || rd(N_CFG_HEADER) == 0xFFFFFFFFu)
        not_in_port("The sound set-up is not in the port (its Sound page; the sound is always NOSOUND).");
    if (rb(N_ARG_O) == 1)
        not_in_port("The options screen is not in the port (its setup screen's Game page).");
    if (pmax_cfg_header(N_CFG_HEADER) != 0 || rd(N_CFG_HEADER) == 0xFFFFFFFFu) {
        SETSOUND_FREE();
        return 1;
    }
    SETSOUND_FREE();
    return 0;
}

/* CODE:0753: the SVGA mode checked and found for OPT_RESOLUTION 1 and 2;
 * nothing for VGA (0, 3).  The port is a VESA card with no S3 BIOS and
 * the modes 101h and 103h (doskit's vga.c, as the runner's):
 * - a mode kept in OPT_SVGA_MODE is set with INT 10h AH=0 (AL the
 *   number, a BIOS text mode for 1 or 3) and checked (CODE:0649): the
 *   BIOS's state block (AH=1Bh, CODE:065C) the runner does not fill, and
 *   the registers (CODE:06B1) want chain-4, which a text mode has not, so
 *   the check fails as in the runs (the port sets no mode there: the
 *   next mode set comes before a picture, presumably; not traced);
 * - SVGA_FIND (CODE:05FF): no "S3" at C0000h, VESA answers (AX=4F00h,
 *   "VESA"): CL - 2, 1 for 640x480 or 3 for 800x600, into OPT_SVGA_MODE;
 * - the options written back (INT 94h AH=6). */
void SVGA_CHECK(void)
{
    uint8_t r = rb(N_OPT_RESOLUTION);

    if (r == 0 || r == 3)
        return;
    wb(N_OPT_SVGA_MODE, (uint8_t)(2 * r + 1 - 2));
    pmax_cfg_write(N_OPTIONS);
}
