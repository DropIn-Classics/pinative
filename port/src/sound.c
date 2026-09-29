/* sound.c - CHOOSER_LOAD (CODE:757D) and SOUND_START (CODE:7082): the CD
 * checked, the sound driver loaded and started.
 */
#include "game.h"
#include "image.h"
#include "names.h"
#include "pmax.h"
#include "pmem.h"

/* CODE:7082: 1 (CF) when there is no CD or the driver fails */
static int SOUND_START(void)
{
    if (CD_INSTALLED() != 0)
        pi_stop("SOUND_START: no CD (CODE:716A)");
    CD_LOCK(1);
    if (CD_READ_TOC() != 0)
        pi_stop("SOUND_START: no CD (CODE:716A)");
    CD_LOCK(1);
    CD_LOCK(1);

    /* the driver named in the configuration header */
    pmax_cfg_header(N_DRIVER_CFG);
    pi_stop("SOUND_START's driver");
    return 0;
}

void CHOOSER_LOAD(void)
{
    ww(N_HOST_DS, pi_image.desc[ILLUSION_CODE].sel);
    ww(N_VIDEO_SEL, pmax_video_sel());
    /* through CODE:4CF2's checksummed jump */
    if (SOUND_START()) {
        /* CODE:35B7D with BL 0, then exit (INT 21h AX=4CFFh) */
        pi_stop("CHOOSER_LOAD: the sound start failed");
    }
    pi_stop("CHOOSER_LOAD's files");
}
