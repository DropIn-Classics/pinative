/* setup.c - SETUP_ARGS (CODE:32F39): the language of SETSOUND.DAT's texts
 * and the command line's options, the sound set-up and the options screen
 * when asked for (not translated yet: the port stops there).
 */
#include <stdio.h>
#include "game.h"
#include "names.h"
#include "pmax.h"
#include "pmem.h"

/* CODE:328B4: SETSOUND.DAT through pMAX (INT 94h AH=1); SETSOUND_TEXTS the
 * language's table in it, YES_KEYS a word of its text +114h */
static void SETSOUND_LOAD(void)
{
    uint16_t sel = pmax_load("SETSOUND\\SETSOUND.DAT");
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
        pi_stop("OPTIONS_RESET");
    if (rb(N_ARG_S) != 0
        || pmax_cfg_header(N_CFG_HEADER) != 0 || rd(N_CFG_HEADER) == 0xFFFFFFFFu)
        pi_stop("SOUND_SETUP");
    if (rb(N_ARG_O) == 1)
        pi_stop("OPTIONS_SCREEN");
    if (pmax_cfg_header(N_CFG_HEADER) != 0 || rd(N_CFG_HEADER) == 0xFFFFFFFFu) {
        SETSOUND_FREE();
        return 1;
    }
    SETSOUND_FREE();
    return 0;
}
