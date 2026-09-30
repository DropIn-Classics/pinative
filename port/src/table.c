/* table.c - TABLE (CODE:A323): a table loaded and played, up to the
 * chooser again.
 *
 * TABLE runs on aliases of the program's DS and CS (INT 93h AH=8; the same
 * base, so the port's memory accesses are unchanged, only the selectors
 * kept in memory differ).  It loads the table (TABLE_LOAD: the driver
 * started again with the table's music; TABLE_LOAD2: the module and the
 * rest), runs the game with IRQ 1 unmasked (CODE:B928) and frees it all.
 * Translated so far: TABLE_LOAD2 up to CODE:911F, the module loaded
 * (docs/HANDOFF.md, "The table's start", "The dot-matrix display's
 * blocks", "The module's load").
 */
#include <string.h>

#include "frame.h"
#include "game.h"
#include "image.h"
#include "names.h"
#include "nosound.h"
#include "pmax.h"
#include "pmem.h"

/* CODE:A076, IRQ 1's handler while the table runs: not translated yet */
static void KBD_IRQ(unsigned char ah)
{
    (void)ah;
    pi_stop("KBD_IRQ (CODE:A076)");
}

/* CODE:A593, CODE:A5BF: a list's end pointer to its start, its C8h bytes
 * cleared */
static void list_init(uint32_t end, uint32_t list)
{
    uint32_t i;

    wd(end, list);
    for (i = 0; i < 0xC8; i++)
        wb(list + i, 0);
}

/* CODE:A03B: IRQ 1's vector kept (INT 93h AH=3 BL=1 gave 0030:00004152
 * in the run, pMAX's own: the chooser's handler was taken back at its
 * end), KBD_IRQ set */
static void TBL_KBD_INSTALL(void)
{
    wd(N_TBL_KBD_OLD, 0x4152);
    ww(N_TBL_KBD_OLD + 4, 0x30);
    frame_set_keyboard(KBD_IRQ);
    wb(N_KEY_RELEASED, 0xFF);
}

/* CODE:A48F: EDI less the ECX bytes from ESI */
static uint32_t BYTE_SUB(uint32_t esi, uint32_t ecx, uint32_t edi)
{
    while (ecx--)
        edi -= rb(esi++);
    return edi;
}

/* CODE:A2C8: 1 (CF) unless AL is 1..4 */
static int TABLE_DIGITS(uint8_t al)
{
    static const uint32_t names[] = {
        0xBBAD, 0x98DD, 0x98F1, 0x27927, 0x28CE9, 0x28E31, 0x28E4B, 0xB1A3
    };
    uint8_t c = (uint8_t)(al + '0');
    size_t i;

    if (c < '1' || c > '4')
        return 1;
    for (i = 0; i < sizeof names / sizeof names[0]; i++)
        wb(names[i], c);
    ww(0xCC22, (uint16_t)(c - '1'));
    wb(N_TABLE_NUM, (uint8_t)(c - '1' + 1));
    return 0;
}

/* CODE:A624: 258h words at state+21E6h, 0 and 2Ah more each */
static void STATE_21E6_INIT(void)
{
    uint32_t di = rd(0x14) + 0x21E6;
    uint16_t ax = 0;
    int i;

    for (i = 0; i < 0x258; i++, ax += 0x2A)
        ww(di + 2 * (uint32_t)i, ax);
}

/* CODE:994A */
static void TBL_CALLBACKS_CS(void)
{
    uint32_t p;

    for (p = N_TBL_CALLBACKS; rd(p) != 0xFFFFFFFFu; p += 6)
        ww(p + 4, pmax_code_sel());
}

/* the driver through TBL_DRIVER_ENTRY */
static int tbl_driver(NsRegs *r)
{
    return ns_call(rw(N_TBL_DRIVER_ENTRY + 4), r);
}

/* CODE:9AB9: as SOUND_START's driver start (sound.c), with the table's
 * cells; the port loads NOSOUND.SDR whatever the header names */
static int TABLE_SOUND(void)
{
    uint16_t sel, ds = rw(N_TABLE_DS);
    NsRegs r;

    pmax_cfg_header(N_TBL_DRIVER_CFG);
    sel = pmax_load("NOSOUND.SDR", NULL);
    if (!sel)
        pi_stop("TABLE_SOUND: NOSOUND.SDR not loaded");
    ww(N_TBL_DRIVER_SEL, sel);
    wd(N_TBL_DRIVER_ENTRY, 0);
    ww(N_TBL_DRIVER_ENTRY + 4, pmax_alias(sel));
    TBL_CALLBACKS_CS();
    hcb_use_table(1);

    /* command 0: FS:EDI the callbacks, ES:EBX the header, DS the
     * driver's; its CF not looked at */
    memset(&r, 0, sizeof r);
    r.fs = r.es = ds;
    r.edi = N_TBL_CALLBACKS;
    r.ebx = N_TBL_DRIVER_CFG;
    r.ds = sel;
    tbl_driver(&r);

    /* command 11h: JINGLE_END_CB */
    r.es = pmax_code_sel();
    r.edx = N_JINGLE_END_CB;
    r.eax = 0x11;
    tbl_driver(&r);

    /* command 4: the table's two modules into slots 0 and 1 (table 4 has
     * one) */
    r.es = ds;
    r.eax = 4;
    r.ebx = 0;
    r.edx = N_MUSIC_MOD_NAME;
    tbl_driver(&r);
    if (rb(N_TABLE_NUM) != 4) {
        r.eax = 4;
        r.ebx = 1;
        r.edx = N_MUSIC2_MOD_NAME;
        tbl_driver(&r);
    }

    /* command 8: order 1 of slot 0 */
    r.eax = 8;
    r.ebx = 1;
    r.ecx = 0;
    tbl_driver(&r);
    return 0;
}

/* CODE:A5EB */
static int TABLE_LOAD(void)
{
    STATE_21E6_INIT();
    ww(0xA2B9, 0);
    ww(0xBAD2, 0);
    return TABLE_SOUND();
}

/* CODE:B1D0 */
static int VM_DATA_LOAD(void)
{
    uint32_t n = 0;
    uint16_t sel = pmax_load_ds(N_VM_DATA_NAME, &n);

    ww(N_VM_DATA_SEL, sel);
    wd(N_VM_DATA_SIZE, n);
    return !sel;
}

/* CODE:B048 */
static int TABLE_LOAD2(void)
{
    uint16_t sel = pmax_alloc(0x33450);

    ww(N_HIDELIGHTS_SEL, sel);
    if (!sel)
        return 1;
    if (rb(N_TABLE_NUM) != 3 && VM_DATA_LOAD())
        return 1;
    if (DM_LOAD() || TABLE_MODULE())
        return 1;
    pi_stop("CODE:911F");
    return 1;
}

int TABLE(uint8_t al)
{
    uint16_t ds;

    ww(N_TABLE_OLD_DS, pi_image.desc[ILLUSION_CODE].sel);
    wb(N_TABLE_ARG, al);
    ds = pmax_alias(pi_image.desc[ILLUSION_CODE].sel);
    ww(N_TABLE_DS, ds);
    wd(N_TABLE_BASE, pmax_base(ds));
    ww(N_TABLE_OLD_CS, pmax_code_sel());
    /* the far jump through TABLE_JUMP to CS's alias */
    wd(N_TABLE_JUMP, N_TABLE_CS);
    ww(N_TABLE_JUMP + 4, pmax_alias(pmax_code_sel()));
    pmax_set_code_sel(rw(N_TABLE_JUMP + 4));

    ww(N_TABLE_VIDEO_SEL, pmax_video_sel());
    list_init(N_BLOCKS_END, N_BLOCKS);
    list_init(N_ALLOCS_END, N_ALLOCS);
    TBL_KBD_INSTALL();
    wd(N_TABLE_CHECK, BYTE_SUB(N_INTRO_MODE, N_INTRO_SCRIPT - N_INTRO_MODE, rd(0x90A3)));
    wd(0x14, 0xCB3E);
    if (TABLE_DIGITS(rb(N_TABLE_ARG)))
        pi_stop("TABLE: a table not 1..4 (CODE:BA7D)");
    if (TABLE_LOAD())
        pi_stop("TABLE: TABLE_LOAD failed");
    if (TABLE_LOAD2())
        pi_stop("TABLE: TABLE_LOAD2 failed");
    return 1;
}
