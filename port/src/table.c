/* table.c - TABLE (CODE:A323): a table loaded and played, up to the
 * chooser again.
 *
 * TABLE runs on aliases of the program's DS and CS (INT 93h AH=8; the same
 * base, so the port's memory accesses are unchanged, only the selectors
 * kept in memory differ).  It loads the table (TABLE_LOAD: the driver
 * started again with the table's music; TABLE_LOAD2: the module and the
 * rest), runs the game with IRQ 1 unmasked (CODE:B928) and frees it all.
 * Translated so far: TABLE_LOAD2, up to the game (docs/HANDOFF.md,
 * "The table's start" and the sections after it).
 */
#include <string.h>

#include "frame.h"
#include "game.h"
#include "image.h"
#include "names.h"
#include "nosound.h"
#include "pmax.h"
#include "pmem.h"
#include "vga.h"

/* CODE:A076, IRQ 1's handler while the table runs (DS from TABLE_DS,
 * CODE's base; port 61h's acknowledge and the PIC's EOI leave nothing in
 * memory): E0h sets KEY_E0; a press sets KEY_DOWN (+80h after E0h) FFh,
 * inverts KEY_TOGGLE, goes to LAST_KEY and CODE:CC2F and KEY_RELEASED
 * 0, a release clears KEY_DOWN and sets KEY_RELEASED FFh.  A press of
 * Space or Alt (39h, 38h) while KEY_RELEASED is 0 is dropped, KEY_E0
 * left as it is (after E0 38h, Right Alt, the next key then counts as
 * an E0 key, as in the original) */
static void KBD_IRQ(unsigned char al)
{
    uint32_t save = pm_ds, ebx = 0;

    pm_ds = PI_IMAGE_BASE;
    if (!(al & 0x80) && (al == 0x39 || al == 0x38) && rb(N_KEY_RELEASED) == 0)
        goto out;
    if (al == 0xE0) {
        wb(N_KEY_E0, 0xFF);
        goto out;
    }
    if (rb(N_KEY_E0) == 0xFF)
        ebx = 0x80;
    wb(N_KEY_E0, 0);
    if (!(al & 0x80)) {
        wb(N_LAST_KEY, al);
        wb(0xCC2F, al);
        wb(N_KEY_DOWN + al + ebx, 0xFF);
        wb(N_KEY_TOGGLE + al + ebx, (uint8_t)~rb(N_KEY_TOGGLE + al + ebx));
        wb(N_KEY_RELEASED, 0);
    } else {
        wb(N_KEY_DOWN + (al & 0x7F) + ebx, 0);
        wb(N_KEY_RELEASED, 0xFF);
    }
out:
    pm_ds = save;
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

/* CODE:9F5C: command 12h, then 1 (CF) when channel 4 still plays a sound
 * and the record's priority +2 is below SFX_PRIO; SFX_PRIO set to it
 * either way */
static int SFX_CHECK(void)
{
    NsRegs r;
    uint16_t ax;
    int cf;

    memset(&r, 0, sizeof r);
    r.eax = 0x12;
    r.edx = 4;
    tbl_driver(&r);
    ax = rw(rd(0x0000) + 2);
    cf = r.eax != 0 && ax < rw(N_SFX_PRIO);
    ww(N_SFX_PRIO, ax);
    return cf;
}

/* CODE:9F88: the record at [CODE:0000] as a note of a module's sample on
 * channel 4 (command 7: the slot +12h, the sample +14h, the note +6, the
 * volume +4) unless SFX_CHECK refuses */
void SFX_NOTE(void)
{
    uint32_t p;
    NsRegs r;

    if (SFX_CHECK())
        return;
    p = rd(0x0000);
    memset(&r, 0, sizeof r);
    r.eax = 7;
    r.edx = 4;
    r.ecx = (uint32_t)rb(p + 0x12) << 8 | rb(p + 0x14);
    r.ebx = (uint32_t)rb(p + 4) << 8 | rb(p + 6);
    tbl_driver(&r);
}

/* CODE:9F2C: the sound record at [CODE:0000] on channel 4 (command 9) */
void SFX_PLAY(void)
{
    uint32_t p;
    NsRegs r;

    if (SFX_CHECK())
        return;
    p = rd(0x0000);
    memset(&r, 0, sizeof r);
    r.eax = 9;
    r.edx = 4;
    r.fs = rw(N_TABLE_DS);
    r.esi = rd(p + 0x16);
    r.ecx = rw(p + 8);
    r.ebx = (uint32_t)rb(p + 4) << 8 | rb(p + 6);
    tbl_driver(&r);
}

/* CODE:B99C, called by the driver at each retrace (command 0Eh): the
 * frame count at CODE:BAD4 */
void DRV_TICK(void)
{
    uint32_t save = pm_ds;

    pm_ds = PI_IMAGE_BASE;
    ww(N_FRAME_COUNT, (uint16_t)(rw(N_FRAME_COUNT) + 1));
    pm_ds = save;
}

/* CODE:9CD4, handed with command 11h, called by the driver's command 6
 * once a jingle has ended: JINGLE_ENDED FFh */
void JINGLE_END_CB(void)
{
    uint32_t save = pm_ds;

    pm_ds = PI_IMAGE_BASE;
    wb(N_JINGLE_ENDED, 0xFF);
    pm_ds = save;
}

/* the CRTC's start address from BX */
static void crt_start(uint16_t bx)
{
    vga_outw(0x3D4, (uint16_t)((bx & 0xFF00) | 0x0C));
    vga_outw(0x3D4, (uint16_t)(bx << 8 | 0x0D));
}

/* CODE:B9B9, called by the driver after the retrace (command 0Fh):
 * CODE:BAD1 FFh, the CRT start from CRT_NEXT; in VGA 320x240 the pel
 * panning 2, 4 or 6 as the word at CODE:D884 is negative, 0 or positive,
 * elsewhere the start a line (54h) on while that word is not 0 */
void DRV_FRAME(void)
{
    uint32_t save = pm_ds;
    int16_t d;

    pm_ds = PI_IMAGE_BASE;
    wb(N_FRAME_DONE, 0xFF);
    d = (int16_t)rw(0xD884);
    if (rb(N_OPT_RESOLUTION) == 3) {
        vga_inb(0x3DA);
        vga_outb(0x3C0, 0x33);
        vga_outb(0x3C0, d > 0 ? 6 : d < 0 ? 2 : 4);
        crt_start(rw(N_CRT_NEXT));
    } else {
        crt_start((uint16_t)(rw(N_CRT_NEXT) + (d != 0 ? 0x54 : 0)));
    }
    pm_ds = save;
}

/* CODE:9C1A: MOD_LEVEL 0 and the driver's command 0Ch with it: the
 * module music silent (while a CD track plays) */
static void MOD_SILENT(void)
{
    NsRegs r;

    ww(N_MOD_LEVEL, 0);
    memset(&r, 0, sizeof r);
    r.eax = 0x0C;
    tbl_driver(&r);
}

/* CODE:9D83: the CD stopped and track EBX played; with MUSIC_FLAGS bit 0
 * its frames times FRAME_RATE / 75 to TRACK_LEFT */
static void MUSIC_TRACK(uint32_t ebx)
{
    uint64_t t;
    uint32_t ecx;

    wd(N_TRACK_NO, ebx);
    CD_STOP();
    ecx = CD_PLAY(rd(N_TRACK_NO));
    if (rb(N_MUSIC_FLAGS) & 1) {
        t = (uint64_t)rw(N_FRAME_RATE) * ecx / 0x4B;
        if (t > 0xFFFFFFFFu)
            pi_stop("MUSIC_TRACK: DIV overflow (CODE:9DB5)");
        wd(N_TRACK_LEFT, (uint32_t)t);
    }
}

/* CODE:9B92: the driver's retrace routines (commands 0Eh and 0Fh), the
 * module music silent, the table's first music record's track played,
 * the driver's player started (command 1, CX 3) */
void GAME_SOUND(void)
{
    uint32_t rec;
    NsRegs r;

    memset(&r, 0, sizeof r);
    r.eax = 0x0E;
    r.edx = 0xB99C;
    r.es = pmax_code_sel();
    tbl_driver(&r);
    memset(&r, 0, sizeof r);
    r.eax = 0x0F;
    r.ecx = rd(N_VSYNC_START_ARG);
    r.edx = 0xB9B9;
    r.es = pmax_code_sel();
    tbl_driver(&r);
    MOD_SILENT();
    rec = rd(N_MODULE_HEADER + 0x8C);
    wb(N_MUSIC_FLAGS, rb(rec + 0x0A));
    ww(N_MUSIC_RET_ORDER, 0);
    MUSIC_TRACK(rw(rec + 8));
    ww(N_SFX_PRIO, 0);
    wb(N_SOUND_PAUSED, 0);
    memset(&r, 0, sizeof r);
    r.eax = 1;
    r.ecx = 3;
    tbl_driver(&r);
}

/* CODE:9C34: MOD_LEVEL 100h, command 0Ch with it */
static void MOD_FULL(void)
{
    NsRegs r;

    ww(N_MOD_LEVEL, 0x100);
    memset(&r, 0, sizeof r);
    r.eax = 0x0C;
    r.ebx = 0x100;
    tbl_driver(&r);
}

/* CODE:9C4F and CODE:9C5E, the same: the driver's command 2 (a toggle:
 * pause, then resume) */
static void SND_PAUSE(void)
{
    NsRegs r;

    memset(&r, 0, sizeof r);
    r.eax = 2;
    tbl_driver(&r);
}

/* the driver's command 8: the module of slot `cx` from order `bx` */
static void drv_order(uint16_t bx, uint16_t cx)
{
    NsRegs r;

    memset(&r, 0, sizeof r);
    r.eax = 8;
    r.ebx = bx;
    r.ecx = cx;
    tbl_driver(&r);
}

/* CD_LEVEL = `bl` and CD_VOLUME with it */
static void cd_level(uint8_t bl)
{
    wb(N_CD_LEVEL, bl);
    CD_VOLUME(bl);
}

/* CODE:9CEA: TRACK_LEFT counted down; at 0 (held at 1 while
 * SOUND_PAUSED) the track again, or with MUSIC_RET_ORDER the module
 * music from there */
static void MUSIC_COUNTDOWN(void)
{
    uint16_t bx;

    /* the TEST at CODE:9CEA jumps to CODE:9CF6 either way */
    wd(N_TRACK_LEFT, rd(N_TRACK_LEFT) - 1);
    if (rd(N_TRACK_LEFT) != 0)
        return;
    if (rb(N_SOUND_PAUSED) == 0xFF) {
        wd(N_TRACK_LEFT, 1);
        return;
    }
    if (rw(N_MUSIC_RET_ORDER) == 0) {
        SND_PAUSE();
        CD_STOP();
        MUSIC_TRACK(rd(N_TRACK_NO));
        SND_PAUSE();
        return;
    }
    wb(N_TRACK_ON, 0);                  /* CODE:9D3C */
    SND_PAUSE();
    cd_level(0);
    bx = rw(N_MUSIC_RET_ORDER);
    ww(N_MUSIC_RET_ORDER, 0);
    drv_order(bx, rw(N_MUSIC_RET_SLOT));
    MOD_FULL();
    SND_PAUSE();
}

/* CODE:9DC7: MUSIC_COUNTDOWN; a jingle's end; a track asked for in
 * MUSIC_NEXT; the module request MOD_REQUEST (positive: a jingle, the
 * driver's command 0Ah; negative: the module from MOD_REQ_ORDER) */
static void MUSIC_UPDATE(void)
{
    NsRegs r;
    uint32_t ebx;

    MUSIC_COUNTDOWN();
    if (rb(N_JINGLE_ENDED) == 0xFF) {
        wb(N_JINGLE_ENDED, 0);
        wb(N_JINGLE_ON, 0);
        if (rb(N_TRACK_ON) == 0xFF) {
            wb(N_MUSIC_FLAGS, rb(N_MUSIC_FLAGS_KEPT));
            SND_PAUSE();
            cd_level(0xC0);
            SND_PAUSE();
            MOD_SILENT();
        }
    }
    if (rw(N_MUSIC_NEXT) != 0 && rb(N_JINGLE_ON) != 0xFF) {    /* CODE:9E13 */
        wb(N_TRACK_ON, 0xFF);
        SND_PAUSE();
        ww(N_MUSIC_RET_ORDER, rw(N_MOD_REQ_ORDER));
        ww(N_MUSIC_RET_SLOT, rw(N_MOD_REQ_SLOT));
        ww(N_MOD_REQUEST, 0);
        ebx = rw(N_MUSIC_NEXT);
        ww(N_MUSIC_NEXT, 0);
        MUSIC_TRACK(ebx);
        cd_level(0xC0);
        MOD_SILENT();
        SND_PAUSE();
    }
    if (rw(N_MOD_REQUEST) == 0) {                               /* CODE:9E83 */
        ;
    } else if (rw(N_MOD_REQUEST) & 0x8000) {                    /* CODE:9EDB */
        wb(N_TRACK_ON, 0);
        SND_PAUSE();
        cd_level(0);
        wd(N_TRACK_LEFT, 0xFFFFFFFFu);
        SND_PAUSE();
        drv_order(rw(N_MOD_REQ_ORDER), rw(N_MOD_REQ_SLOT));
        MOD_FULL();
    } else {
        wb(N_JINGLE_ON, 0xFF);
        if (rb(N_MUSIC_FLAGS) & 4) {
            SND_PAUSE();
            cd_level(0);
            SND_PAUSE();
        }
        MOD_FULL();
        memset(&r, 0, sizeof r);
        r.eax = 0x0A;
        r.ebx = rw(N_MOD_REQ_ORDER);
        r.ecx = rw(N_MOD_REQ_SLOT);
        tbl_driver(&r);
    }
    ww(N_MOD_REQUEST, 0);                                       /* CODE:9F22 */
}

/* CODE:298EF, the frame step's first routine: MUSIC_UPDATE, then the
 * driver's command 6 (CODE:9C0C; it gives EAX back as it was, 6) */
void FRAME_MUSIC(void)
{
    NsRegs r;

    MUSIC_UPDATE();
    memset(&r, 0, sizeof r);
    r.eax = 6;
    tbl_driver(&r);
}

/* CODE:9AB9: as SOUND_START's driver start (sound.c), with the table's
 * cells; the port loads NOSOUND.SDR whatever the header names */
static int TABLE_SOUND(void)
{
    uint16_t sel, ds = rw(N_TABLE_DS);
    NsRegs r;

    pmax_cfg_header(N_TBL_DRIVER_CFG);
    pmax_name(0x98C8, rw(N_TABLE_DS));       /* "Sound Driver" */
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

/* CODE:A6B4: 32 pictures from the stage's palette toward TBL_PALETTE
 * (the first one 31/32 of the way back), the driver's command 6 after
 * each (its BX, FFh less 8 a picture, not read by NOSOUND) */
static void TABLE_FADE_OUT(void)
{
    uint8_t cl;
    NsRegs r;

    for (cl = 0x20; cl != 0; cl--) {
        FADE_MIX(N_TBL_PALETTE, rd(N_MODULE_HEADER + 0x50), cl);
        frame_wait();                               /* CODE:3D372 */
        FADE_PAL_SET();
        memset(&r, 0, sizeof r);
        r.eax = 6;
        r.ebx = (uint16_t)(0xFF - 8 * (0x21 - cl));
        tbl_driver(&r);
    }
}

/* CODE:9FB1: the CD stopped (between the two command 2s), the driver's
 * player stopped (command 3), its modules freed (command 5, slot 1 not
 * on table 4), its end (command 0Bh); the code alias freed (INT 93h
 * AH=0Dh) */
static void TABLE_SOUND_END(void)
{
    NsRegs r;

    SND_PAUSE();
    CD_STOP();
    SND_PAUSE();
    memset(&r, 0, sizeof r);
    r.eax = 3;
    tbl_driver(&r);
    memset(&r, 0, sizeof r);
    r.eax = 5;
    r.ebx = 0;
    tbl_driver(&r);
    if (rb(N_TABLE_NUM) != 4) {
        memset(&r, 0, sizeof r);
        r.eax = 5;
        r.ebx = 1;
        tbl_driver(&r);
    }
    memset(&r, 0, sizeof r);
    r.eax = 0x0B;
    tbl_driver(&r);
    pmax_free_sel(rw(N_TBL_DRIVER_ENTRY + 4));
}

/* CODE:B3A1 (HISCORES_PUT): the table's high scores back to HISCORES (50 bytes at
 * TABLE_INDEX), when they changed, and the file written (INT 94h AH=6,
 * not in the port yet) */
static void HISCORES_PUT(void)
{
    uint32_t d = N_HISCORES + (uint32_t)rb(N_TABLE_INDEX) * 0x32, i;

    for (i = 0; i < 0x32; i++)
        if (rb(N_TABLE_HISCORES + i) != rb(d + i))
            break;
    if (i == 0x32)
        return;
    for (i = 0; i < 0x32; i++)
        wb(d + i, rb(N_TABLE_HISCORES + i));
    pi_stop("HISCORES_PUT: the file written (CODE:B3D1, INT 94h AH=6)");
}

/* CODE:A448, TABLE after the game: the keyboard's vector back, the
 * fade out, the sound's end, the high scores, then every block of the
 * table freed (INT 92h AH=5; the ALLOCS list's by address, AH=2), last
 * the selectors made at the start (INT 93h AH=0Dh: the driver's
 * selector too, whose block pMAX leaves allocated, as the run's heap at
 * CODE:7182 shows) and the code selector back (CODE:A499) */
static void TABLE_END(void)
{
    uint32_t p;
    int i;

    frame_set_keyboard(NULL);                       /* CODE:A01E */
    TABLE_FADE_OUT();
    TABLE_SOUND_END();
    HISCORES_PUT();
    pmax_free(rw(N_LIGHTS_SEL));                    /* CODE:28D17 */
    pmax_free(rw(N_DROPS_SEL));
    if (rb(N_TABLE_NUM) != 2)
        pmax_free(rw(N_MASKS_SEL));
    for (i = 0; i < 5; i++)                         /* DM_FREE */
        pmax_free(rw(N_DM_FONTS + 2 * i));
    pmax_free(rw(N_DM_TEXT));
    pmax_free(rw(N_DM_TEXT_TEMP));
    pmax_free(rw(N_DM_ANIMS_SEL));
    pmax_free(rw(N_DM_ANIM));
    pmax_free(rw(N_DM_ANIM_TEMP));
    if (rb(N_TABLE_NUM) != 3)
        pmax_free(rw(N_VM_DATA_SEL));
    pmax_free(rw(N_SPOOKY_SEL));                    /* CODE:A4E1 */
    pmax_free(rw(N_MODULE_SEL));
    pmax_free(rw(N_HIDELIGHTS_SEL));
    pmax_free(rw(N_MODULE_REL_SEL));
    /* CODE:A55E and A52E, last first; an empty list would run the
     * LOOP from ECX 0, and a failed free (CF) ends the list: neither
     * followed */
    if (rd(N_BLOCKS_END) == N_BLOCKS || rd(N_ALLOCS_END) == N_ALLOCS)
        pi_stop("TABLE_END: an empty BLOCKS or ALLOCS list (CODE:A55E, A52E)");
    for (p = rd(N_BLOCKS_END); p >= N_BLOCKS + 2; p -= 2) {
        if (!pmax_base(rw(p - 2)))
            pi_stop("TABLE_END: a BLOCKS entry not a block (CODE:A584)");
        pmax_free(rw(p - 2));
    }
    for (p = rd(N_ALLOCS_END); p >= N_ALLOCS + 4; p -= 4)
        pmax_free_linear(rd(p - 4));
    wd(N_TABLE_BACK, 0xA4A9);                       /* CODE:A499 */
    pmax_set_code_sel(rw(N_TABLE_OLD_CS));
    pmax_free_sel(rw(N_TBL_DRIVER_SEL));
    pmax_free_sel(rw(N_TABLE_DS));
    pmax_free_sel(rw(N_TABLE_JUMP + 4));
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
    uint16_t sel;

    pmax_name(0xB1D7, rw(N_TABLE_DS));       /* "VideoMode data" */
    sel = pmax_load_ds(N_VM_DATA_NAME, &n);

    ww(N_VM_DATA_SEL, sel);
    wd(N_VM_DATA_SIZE, n);
    return !sel;
}

/* CODE:B048 */
static int TABLE_LOAD2(void)
{
    uint16_t sel;

    pmax_name(0xB04A, rw(N_TABLE_DS));       /* "Hidelights mask" */
    sel = pmax_alloc(0x33450);

    ww(N_HIDELIGHTS_SEL, sel);
    if (!sel)
        return 1;
    if (rb(N_TABLE_NUM) != 3 && VM_DATA_LOAD())
        return 1;
    if (DM_LOAD() || TABLE_MODULE())
        return 1;
    TBL_VGA_INIT();
    if (rb(N_OPT_RESOLUTION) == 3) {
        /* the attribute controller's mode control 61h, panning 4 */
        vga_inb(0x3DA);
        vga_outb(0x3C0, 0x30);
        vga_outb(0x3C0, 0x61);
        vga_inb(0x3DA);
        vga_outb(0x3C0, 0x33);
        vga_outb(0x3C0, 4);
    }
    TOP_COLOURS_SET();
    BALLS_INIT();
    return LIGHTS_LOAD() || FLIPDAT_LOAD() || MULTIBALL_CAP();
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
        pi_stop("TABLE: TABLE_LOAD2 failed (CODE:A40A)");
    /* IRQ 1 unmasked around the game */
    pic_out21((uint8_t)(pic_in21() & 0xFD));
    TABLE_GAME();
    pic_out21((uint8_t)(pic_in21() | 2));
    TABLE_END();
    return 0;
}
