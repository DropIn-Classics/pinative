/* entry.c - ENTRY (CODE:02A3): the start of ILLUSION.386 up to the
 * command-line options.
 */
#include <stdio.h>
#include <stdlib.h>
#include "game.h"
#include "image.h"
#include "names.h"
#include "platform.h"
#include "pmax.h"
#include "vga.h"

const char *pi_stop_mem, *pi_stop_vram;

void pi_stop(const char *name)
{
    char msg[128];
    int r = 0;

    if (pi_stop_mem && pm_write(pi_stop_mem) != 0) {
        fprintf(stderr, "pinative: %s cannot be written\n", pi_stop_mem);
        r = 1;
    }
    if (pi_stop_vram && vga_write_planes(pi_stop_vram) != 0) {
        fprintf(stderr, "pinative: %s cannot be written\n", pi_stop_vram);
        r = 1;
    }
    snprintf(msg, sizeof msg, "Stopped before %s (not translated yet, or -entry).", name);
    plat_message(msg);
    plat_shutdown();
    exit(r);
}

void pi_print_dos(uint32_t at)
{
    for (; lrb(at) != '$'; at++)
        putchar(lrb(at));
    fflush(stdout);
}

void pi_exit_text(const char *text)
{
    fputs(text, stdout);
    plat_shutdown();
    exit(0);
}

void ENTRY(void)
{
    const char *s = pmax_tail;
    uint32_t di = N_CFG_NAME;

    /* DS the program's own selector, kept */
    ww(N_CODE_SEL, pi_image.desc[ILLUSION_CODE].sel);
    /* INT 92h AH=0Bh: at least 2F0800h bytes free, else "has failed to
     * load due to a lack of free memory"; the port has them */

    /* INT 93h AH=11h: the command tail's first word, to a NUL, a space or
     * a '/', is the configuration file */
    while (*s == ' ')
        s++;
    while (*s && *s != ' ' && *s != '/')
        wb(di++, (uint8_t)*s++);
    wb(di, 0);
    /* INT 94h AH=8 opens it (pmax_cfg_open, main.c), AH=7 the options */
    pmax_cfg_read(N_OPTIONS);

    if (SETUP_ARGS())
        return;
    SVGA_CHECK();
    /* IRQ 1 masked at the PIC (the keyboard is read otherwise; not
     * followed): nothing in the port's memory */
    VGA_INIT();
    HISCORE_INIT();
    pi_stop("L757D");
}
