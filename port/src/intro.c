/* intro.c - CHOOSER_LOAD (CODE:757D): the sound started, the chooser's and
 * the intro's files loaded, then the intro.
 */
#include "game.h"
#include "image.h"
#include "names.h"
#include "pmax.h"
#include "pmem.h"

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

    /* CODE:7885: CODE:7341 through CODE:7438's checksummed jump */
    pi_stop("CODE:7341 (CHOOSER_LOAD's intro)");
}
