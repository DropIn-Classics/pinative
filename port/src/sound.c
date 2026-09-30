/* sound.c - SOUND_START (CODE:7082): the CD checked, the sound driver
 * loaded and started.
 */
#include <string.h>
#include "game.h"
#include "image.h"
#include "names.h"
#include "nosound.h"
#include "pmax.h"
#include "pmem.h"

/* CODE:61D9: each selector word of HOST_CALLBACKS set to CS */
static void CALLBACKS_CS(void)
{
    uint32_t p;

    for (p = N_HOST_CALLBACKS; rd(p) != 0xFFFFFFFFu; p += 6)
        ww(p + 4, pmax_code_sel());
}

/* CODE:7082: 1 (CF) when there is no CD or the driver fails */
int SOUND_START(void)
{
    uint16_t sel;
    NsRegs r;

    if (CD_INSTALLED() != 0)
        pi_stop("SOUND_START: no CD (CODE:716A)");
    CD_LOCK(1);
    if (CD_READ_TOC() != 0)
        pi_stop("SOUND_START: no CD (CODE:716A)");
    CD_LOCK(1);
    CD_LOCK(1);

    /* the driver named in the configuration header, loaded and given a
     * code selector (INT 94h AH=1, INT 93h AH=8 with DX 409Ah).  The port
     * has only NOSOUND.SDR (src/NOSOUND.hints) and loads it whatever the
     * header names; DRIVER_CFG keeps the player's name. */
    pmax_cfg_header(N_DRIVER_CFG);
    pmax_name(0x7075, 0x1C);         /* "Sound Driver" */
    sel = pmax_load("NOSOUND.SDR", NULL);
    if (!sel)
        pi_stop("SOUND_START: NOSOUND.SDR not loaded");
    ww(N_DRIVER_SEL, sel);
    wd(N_DRIVER_ENTRY, 0);
    ww(N_DRIVER_ENTRY + 4, pmax_alias(sel));
    CALLBACKS_CS();

    /* command 0: FS:EDI the callbacks, ES:EBX the header, DS the driver */
    memset(&r, 0, sizeof r);
    r.fs = r.es = pi_image.desc[ILLUSION_CODE].sel;
    r.edi = N_HOST_CALLBACKS;
    r.ebx = N_DRIVER_CFG;
    r.ds = rw(N_DRIVER_SEL);
    if (ns_call(rw(N_DRIVER_ENTRY + 4), &r))
        pi_stop("SOUND_START: the driver failed (CODE:7152)");

    /* command 4: INTRO_MOD_NAME into slot 0 (what it returns is not
     * looked at) */
    r.eax = 4;
    r.ebx = 0;
    r.es = pi_image.desc[ILLUSION_CODE].sel;
    r.edx = N_INTRO_MOD_NAME;
    ns_call(rw(N_DRIVER_ENTRY + 4), &r);
    wd(N_SOUND_JUMP, 0x4CF2);
    return 0;
}
