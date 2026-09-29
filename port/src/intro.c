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

    /* CODE:78C9: CODE:698A through CODE:4CF2's checksummed jump */
    pi_stop("CODE:698A (CHOOSER_LOAD's intro)");
}
