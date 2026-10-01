/* dotmatrix.c - the dot-matrix display's blocks (DM_LOAD, CODE:2833E):
 * its text and animation areas, the five fonts expanded, the table's
 * animations; the text drawn on it (DM_TEXT_DRAW).
 */
#include "game.h"
#include "names.h"
#include "pmax.h"
#include "pmem.h"

/* a file loaded as the original's INT 94h AH=1 stubs do, its name at
 * CODE:`path`: its selector to the word `sel`, its size to the dword
 * `size`; 1 (CF) when it is not there */
static int load(uint32_t esi, uint32_t path, uint32_t sel, uint32_t size)
{
    uint32_t n = 0;
    uint16_t s;

    pmax_name(esi, rw(N_TABLE_DS));
    s = pmax_load_ds(path, &n);

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

/* 1400h bytes from the block of the selector at `from` to the one at
 * `to` (CODE:2721D and the three like it) */
static void dm_copy(uint32_t to, uint32_t from)
{
    uint32_t d = pmax_base(rw(to)), s = pmax_base(rw(from)), i;

    for (i = 0; i < 0x1400; i++)
        lwb(d + i, lrb(s + i));
}

/* CODE:28388: DM_ANIM_SAVE, DM_TEXT_SAVE */
void DM_SAVE(void)
{
    dm_copy(N_DM_ANIM_TEMP, N_DM_ANIM);
    dm_copy(N_DM_TEXT_TEMP, N_DM_TEXT);
}

/* CODE:283A3: DM_ANIM_RESTORE, DM_TEXT_RESTORE */
void DM_RESTORE(void)
{
    dm_copy(N_DM_ANIM, N_DM_ANIM_TEMP);
    dm_copy(N_DM_TEXT, N_DM_TEXT_TEMP);
}

/* CODE:271BC */
static int DM_TEXT_ALLOC(void)
{
    uint16_t sel;

    pmax_name(0x27208, rw(N_TABLE_DS));      /* "dot matrix text area" */
    sel = pmax_alloc(0x1400);
    if (!sel)
        return 1;
    ww(N_DM_TEXT, sel);
    pmax_name(0x271D4, rw(N_TABLE_DS));      /* "Temp Text area" */
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
    uint16_t sel;

    pmax_name(0x27978, rw(N_TABLE_DS));      /* "Animarea" */
    sel = pmax_alloc(0x1400);
    ww(N_DM_ANIM, sel);
    if (!sel)
        return 1;
    pmax_name(0x2799F, rw(N_TABLE_DS));      /* "Temp Animarea" */
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
    /* their blocks' names ("font1a" ..) */
    static const uint32_t names[] = { 0x26EF0, 0x26F45, 0x26F9A, 0x26FEF, 0x27040 };
    int i;

    pmax_policy(1);
    for (i = 0; i < 5; i++) {
        if (load(names[i], files[i], N_DM_FONT_FILES + 2 * (uint32_t)i,
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
    pmax_name(0x270BE, rw(N_TABLE_DS));      /* "dot matrix font" */
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
    return load(0x278DF, N_DM_ANIMS_NAME, N_DM_ANIMS_SEL, N_DM_ANIMS_SIZE);
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

/* The text drawing (CODE:27783) and its glyph routine (CODE:27289): the
 * font's tables and numbers in CODE:27321..27337 as the original keeps
 * them.  `dl` is DL, a glyph's width in bytes, carried from glyph to
 * glyph as in the original; where it would be taken before any glyph set
 * it (a character missing from the width map) the port stops */
static uint8_t dl;
static int dl_set;

/* CODE:2726D: the character's width (the map at CODE:27321, the widths
 * at CODE:27329) halved to DL; 1 (CF) when the map has none for it */
static int glyph_width(uint8_t al)
{
    int8_t i = (int8_t)rb(rd(0x27321) + al);

    if (i < 0) {
        if (!dl_set)
            pi_stop("CODE:27287");
        return 1;
    }
    dl = (uint8_t)(rb(rd(0x27329) + (uint8_t)i) >> 1);
    dl_set = 1;
    return 0;
}

/* CODE:27289: the character `al` drawn at `*edi` into DM_TEXT, from
 * right to left when `ah` is FFh; a space moves by the word before the
 * glyph map (CODE:27325), a glyph drawn moves by its width and 1, a
 * character with no glyph does not move (CF) */
static void glyph_draw(uint8_t al, uint8_t ah, uint32_t *edi)
{
    uint32_t es, fs, esi, ecx, edx, d, r;
    int8_t g;

    wb(0x27337, ah);
    if (al == 0x20) {
        /* CODE:27305 */
        d = rw(rd(0x27325) - 2);
        *edi += ah == 0xFF ? (uint32_t)-(int32_t)d : d;
        return;
    }
    glyph_width(al);
    g = (int8_t)rb(rd(0x27325) + (uint8_t)(al - 0x20));
    if (g < 0)
        return;
    esi = (uint32_t)(uint8_t)g << 3;
    edx = dl;
    d = *edi;
    if (ah == 0xFF)
        d -= edx;
    es = pmax_base(rw(N_DM_TEXT));
    fs = pmax_base(rw(0x2732D));
    for (ecx = rd(0x27333); ecx != 0; ecx--) {
        for (r = 0; r < edx; r++)
            lwb(es + d++, lrb(fs + esi++));
        d += 0xA0 - edx;
        esi += rd(0x2732F) - edx;
    }
    if (ah == 0xFF)
        *edi -= edx + 1;
    else
        *edi += edx + 1;
}

/* CODE:27723: the five fonts' routines (27338, 2737D, 273C2, 27407,
 * 2744C, the last twice), each setting CODE:27321..27333 */
static void font_glyph(uint32_t routine, uint8_t al, uint8_t ah, uint32_t *edi)
{
    uint32_t map, glyphs, widths, sel, stride, rows;

    switch (routine) {
    case 0x27338:
        map = 0x26BA0, glyphs = 0x26A70, widths = 0x26AAB;
        sel = N_DM_FONTS, stride = 0x178, rows = 0x0F;
        break;
    case 0x2737D:
        map = 0x26BA0, glyphs = 0x26A70, widths = 0x26AAB;
        sel = N_DM_FONTS + 2, stride = 0x178, rows = 0x0C;
        break;
    case 0x273C2:
        map = 0x26CA1, glyphs = 0x26AD7, widths = 0x26B12;
        sel = N_DM_FONTS + 4, stride = 0x150, rows = 0x0F;
        break;
    case 0x27407:
        map = 0x26CA1, glyphs = 0x26AD7, widths = 0x26B12;
        sel = N_DM_FONTS + 6, stride = 0x150, rows = 0x0C;
        break;
    case 0x2744C:
        map = 0x26DA2, glyphs = 0x26B3E, widths = 0x26B79;
        sel = N_DM_FONTS + 8, stride = 0x138, rows = 5;
        break;
    default:
        pi_stop("CODE:27723");
        return;
    }
    wd(0x27321, map);
    wd(0x27325, glyphs);
    wd(0x27329, widths);
    ww(0x2732D, rw(sel));
    wd(0x2732F, stride);
    wd(0x27333, rows);
    glyph_draw(al, ah, edi);
}

/* CODE:27783: the text record at [0000] (words x, y, font, alignment,
 * then the text, 0-ended) into DM_TEXT, 0A0h bytes a line and a byte
 * two dots across: alignment 0 from x, 2 centred on x (the widths
 * summed with the font's tables at CODE:2773B, 27753, 2776B), 1 ending
 * at x from the line below, drawn from the last character back; another
 * alignment draws nothing (CF, 1) */
int DM_TEXT_DRAW(void)
{
    uint32_t a = rd(0x0000), font, s, p, edi, ecx;
    uint16_t align = rw(a + 6), di;

    font = (uint32_t)rw(a + 4) << 2;
    s = a + 8;
    if (align == 0) {
        di = (uint16_t)(rw(a + 2) * 0xA0 + (rw(a) >> 1));
        edi = di;
    } else if (align == 2) {
        /* CODE:277AE */
        wd(0x27321, rd(0x2773B + font));
        wd(0x27329, rd(0x27753 + font));
        wd(0x27325, rd(0x2776B + font));
        ecx = 0;
        for (p = s; rb(p) != 0; p++) {
            if (rb(p) == 0x20) {
                dl = (uint8_t)rw(rd(0x27325) - 2);
                dl_set = 1;
            } else {
                glyph_width(rb(p));
                dl++;
            }
            ecx += dl;
        }
        ecx >>= 1;
        di = (uint16_t)(rw(a + 2) * 0xA0 + (rw(a) >> 1));
        edi = di - ecx;
    } else if (align == 1) {
        /* CODE:27879 */
        di = (uint16_t)((rw(a + 2) + 1) * 0xA0 - ((uint16_t)(0x140 - rw(a)) >> 1));
        edi = di;
        p = s;
        do
            p++;
        while (rb(p) != 0);
        do {
            p--;
            font_glyph(rd(0x27723 + (uint32_t)rw(rd(0x0000) + 4) * 4), rb(p), 0xFF, &edi);
        } while (p > s);
        return 0;
    } else {
        return 1;
    }
    /* CODE:2781E, CODE:27859 */
    for (p = s; rb(p) != 0; p++)
        font_glyph(rd(0x27723 + (uint32_t)rw(rd(0x0000) + 4) * 4), rb(p), 0, &edi);
    return 0;
}

/* CODE:275EF: the 12-digit packed-BCD number in the 8 bytes at CODE:26ECB
 * (two 0 bytes first) as text, its leading zeros left out (one digit at
 * least) and a comma before each group of three, built backwards from
 * CODE:27710 (0-ended at CODE:27711); the text record's words before it
 * from CODE:002C, 0030, 0034, 0038 and DM_TEXT_DRAW; 1 (CF) at a nibble
 * above 9, nothing drawn then */
static int number_draw(void)
{
    uint32_t edi, esi, ebp = 0;
    uint8_t al;

    wd(0x0000, 0x26ED3);
    for (edi = 0x26ECB; ; edi++) {
        if (rb(edi) & 0xF0)
            break;
        ebp++;
        if (rb(edi) & 0x0F)
            break;
        ebp++;
        if (ebp >= 0x10)
            break;
    }
    esi = rd(0x0000) - 1;
    wb(0x2771A, 0xFF);
    edi = 0x27711;
    wb(edi, 0);
    edi--;
    wd(0x2771F, 0);
    wd(0x2771B, 0x10 - ebp);
    ebp = 0;
    do {
        if (rd(0x2771F) >= 3) {
            wb(edi--, ',');
            wd(0x2771F, 0);
        }
        wd(0x2771F, rd(0x2771F) + 1);
        al = rb(esi);
        if (rb(0x2771A) != 0xFF) {
            al >>= 4;
            if (al > 9)
                return 1;
            wb(edi--, (uint8_t)(al + 0x30));
            esi--;
            wb(0x2771A, 0xFF);
        } else {
            al &= 0x0F;
            if (al > 9)
                return 1;
            wb(edi--, (uint8_t)(al + 0x30));
            wb(0x2771A, 0);
        }
        ebp++;
    } while (ebp < rd(0x2771B));
    edi = edi + 1 - 8;
    wd(0x0000, edi);
    ww(edi, rw(0x002C));
    ww(edi + 2, rw(0x0030));
    ww(edi + 4, rw(0x0034));
    ww(edi + 6, rw(0x0038));
    DM_TEXT_DRAW();
    return 0;
}

/* CODE:275D7: the number of a high-score entry, [0000] its end (the
 * high word at -6, the low dword at -4), by CODE:2754C */
int DM_HISCORE_DRAW(void)
{
    uint32_t e = rd(0x0000);

    wb(0x26ECB, 0);
    wb(0x26ECC, 0);
    wb(0x26ECD, rb(e - 5));
    wb(0x26ECE, rb(e - 6));
    wb(0x26ECF, rb(e - 1));
    wb(0x26ED0, rb(e - 2));
    wb(0x26ED1, rb(e - 3));
    wb(0x26ED2, rb(e - 4));
    return number_draw();
}

/* CODE:275E4: a player's number, [0000] its record's +8 (the high word
 * at -8, the low dword at -4), by CODE:2750D */
int DM_SCORE_DRAW(void)
{
    uint32_t e = rd(0x0000);

    wb(0x26ECB, 0);
    wb(0x26ECC, 0);
    wb(0x26ECD, rb(e - 7));
    wb(0x26ECE, rb(e - 8));
    wb(0x26ECF, rb(e - 1));
    wb(0x26ED0, rb(e - 2));
    wb(0x26ED1, rb(e - 3));
    wb(0x26ED2, rb(e - 4));
    return number_draw();
}

/* CODE:2758B: the current player's score (the record at state+0D76h)
 * centred on the display's second line, in font 1, or font 3 when the
 * record's word +0 is not 0 */
void DM_SCORE_IDLE(void)
{
    uint32_t a;

    wd(0x002C, 0x140);
    wd(0x0030, 2);
    wd(0x0034, 1);
    wd(0x0038, 1);
    a = rd(rd(0x0014) + 0x0D76) + 8;
    wd(0x0000, a);
    if (rw(a - 8) != 0)
        wd(0x0034, rd(0x0034) + 2);
    DM_SCORE_DRAW();
}
