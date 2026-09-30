/* dotmatrix.c - the dot-matrix display's blocks (DM_LOAD, CODE:2833E):
 * its text and animation areas, the five fonts expanded, the table's
 * animations.  Drawing on it is not translated yet.
 */
#include "game.h"
#include "names.h"
#include "pmax.h"
#include "pmem.h"

/* a file loaded as the original's INT 94h AH=1 stubs do, its name at
 * CODE:`path`: its selector to the word `sel`, its size to the dword
 * `size`; 1 (CF) when it is not there */
static int load(uint32_t path, uint32_t sel, uint32_t size)
{
    uint32_t n = 0;
    uint16_t s = pmax_load_ds(path, &n);

    ww(sel, s);
    wd(size, n);
    return !s;
}

/* CODE:27259 */
void DM_TEXT_CLEAR(void)
{
    uint32_t base = pmax_base(rw(N_DM_TEXT)), i;

    for (i = 0; i < 0x1400; i++)
        lwb(base + i, 0);
}

/* CODE:27CEC */
void DM_ANIM_CLEAR(void)
{
    uint32_t base = pmax_base(rw(N_DM_ANIM)), i;

    for (i = 0; i < 0x1400; i++)
        lwb(base + i, 0xFC);
}

/* CODE:271BC */
static int DM_TEXT_ALLOC(void)
{
    uint16_t sel = pmax_alloc(0x1400);

    if (!sel)
        return 1;
    ww(N_DM_TEXT, sel);
    sel = pmax_alloc(0x1400);
    ww(N_DM_TEXT_TEMP, sel);
    if (!sel)
        return 1;
    DM_TEXT_CLEAR();
    return 0;
}

/* CODE:27976 */
static int DM_ANIM_ALLOC(void)
{
    uint16_t sel = pmax_alloc(0x1400);

    ww(N_DM_ANIM, sel);
    if (!sel)
        return 1;
    sel = pmax_alloc(0x1400);
    ww(N_DM_ANIM_TEMP, sel);
    if (!sel)
        return 1;
    DM_ANIM_CLEAR();
    return 0;
}

/* CODE:26EE0: the five fonts, from the top of the heap */
static int DM_FONTS_READ(void)
{
    /* "data\misc\font1a.m", font1b, font2a, font2b, font3 */
    static const uint32_t files[] = { 0x26EF7, 0x26F4C, 0x26FA1, 0x26FF6, 0x27046 };
    int i;

    pmax_policy(1);
    for (i = 0; i < 5; i++) {
        if (load(files[i], N_DM_FONT_FILES + 2 * (uint32_t)i,
                 N_DM_FONT_SIZES + 4 * (uint32_t)i)) {
            pmax_policy(0);
            return 1;
        }
    }
    pmax_policy(0);
    return 0;
}

/* CODE:270AD: the block of `src` (`n` bytes) as 4n bytes, each of a
 * byte's bits 7, 5, 3 and 1 as FFh or 0 (the other four are passed
 * over); the source freed.  The new selector, 0 (CF) when there is no
 * room. */
static uint16_t DM_FONT_EXPAND(uint16_t src, uint32_t n)
{
    uint32_t from, to, i;
    uint16_t sel;
    int k;

    ww(N_DM_EXPAND_SRC, src);
    wd(N_DM_EXPAND_COUNT, n);
    sel = pmax_alloc(n * 4);
    ww(N_DM_EXPAND_SEL, sel);
    if (!sel)
        return 0;
    from = pmax_base(src);
    to = pmax_base(sel);
    for (i = 0; i < n; i++) {
        uint8_t b = lrb(from + i);
        for (k = 0; k < 4; k++, b = (uint8_t)(b << 2))
            lwb(to++, (b & 0x80) ? 0xFF : 0);
    }
    pmax_free(src);
    return sel;
}

/* CODE:27491: 1 (CF) when one failed; the last one's selector is kept
 * even then */
static int DM_FONTS_EXPAND(void)
{
    uint16_t sel = 0;
    int i;

    for (i = 0; i < 5; i++) {
        sel = DM_FONT_EXPAND(rw(N_DM_FONT_FILES + 2 * (uint32_t)i),
                             rd(N_DM_FONT_SIZES + 4 * (uint32_t)i));
        if (!sel && i < 4)
            return 1;
        ww(N_DM_FONTS + 2 * (uint32_t)i, sel);
    }
    return !sel;
}

/* CODE:26ED3 */
static int DM_FONTS_LOAD(void)
{
    return DM_FONTS_READ() || DM_FONTS_EXPAND();
}

/* CODE:278D8: its CF from the load */
static int DM_ANIMS_LOAD(void)
{
    return load(N_DM_ANIMS_NAME, N_DM_ANIMS_SEL, N_DM_ANIMS_SIZE);
}

/* CODE:2FBA8 */
void DM_CLEAR(void)
{
    DM_TEXT_CLEAR();
    DM_ANIM_CLEAR();
}

int DM_LOAD(void)
{
    ww(N_DM_VIDEO_SEL, pmax_video_sel());
    if (DM_TEXT_ALLOC() || DM_ANIM_ALLOC() || DM_FONTS_LOAD() || DM_ANIMS_LOAD())
        return 1;
    DM_CLEAR();
    DM_TEXT_CLEAR();
    DM_ANIM_CLEAR();
    return 0;
}
