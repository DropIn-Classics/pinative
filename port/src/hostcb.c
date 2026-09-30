/* hostcb.c - the host callbacks (HOST_CALLBACKS, CODE:618D): the game's
 * routines the sound driver calls, each a wrapper round a pMAX service
 * (docs/audio-driver.md, "Host callback table").  Those the translated
 * driver needs so far; 6 to 9 are the driver's file access.
 */
#include "game.h"
#include "image.h"
#include "names.h"
#include "pmax.h"
#include "pmem.h"

/* the cells callbacks 6 to 9 keep the file in: HOST_CALLBACKS' own, or
 * the table's copy's (TBL_CALLBACKS, CODE:98FE), which differs in them
 * and in the names its callbacks give pMAX's headers */
static uint32_t cell_sel = N_HCB_FILE_SEL, cell_cursor = N_HCB_CURSOR;
static int in_table;

/* CODE:6244 (the table's CODE:99B5), callback 0: a block of `size` bytes
 * (INT 92h AH=0Ah, named "Used by MS32" at DS:6247h, 99B8h: the name's
 * offset in CODE with the caller's DS `ds`) */
int HCB_ALLOC(uint16_t ds, uint32_t size, uint16_t *bx)
{
    pmax_name(in_table ? 0x99B8 : 0x6247, ds);
    *bx = pmax_alloc(size);
    return *bx == 0;
}

/* CODE:61ED (the table's 9960), callback 1: the same from DOS memory
 * (policy 2 around it; the name DS:61FEh, 996Fh) */
int HCB_ALLOC_LOW(uint16_t ds, uint32_t size, uint16_t *bx)
{
    pmax_name(in_table ? 0x996F : 0x61FE, ds);
    pmax_policy(2);
    *bx = pmax_alloc(size);
    pmax_policy(0);
    return *bx == 0;
}

/* CODE:6234, callback 2 (INT 92h AH=5) */
void HCB_FREE(uint16_t bx)
{
    pmax_free(bx);
}

/* CODE:626D, callback 3 (INT 93h AH=0Bh) */
uint32_t HCB_LINEAR(uint16_t bx)
{
    return pmax_base(bx);
}

/* CODE:627B, callback 4 (INT 93h AH=3): the protected-mode vector BL.
 * The port has pMAX's only as dosrun answered: IRQ 0's (vector 8, BL 0
 * here) 0030:0000414B in the run of docs/HANDOFF.md, "The driver's
 * timer"; the others not seen */
void HCB_GETVEC(uint8_t bl, uint16_t *es, uint32_t *edx)
{
    if (bl != 0)
        pi_stop("HCB_GETVEC: a vector not seen in a run");
    *es = 0x30;
    *edx = 0x414B;
}

/* CODE:6280, callback 5 (INT 93h AH=4): the vector BL set to ES:EDX.  The
 * port runs no interrupts: the driver's timer is a frame tick instead
 * (nosound.c), so only IRQ 0's is taken, and not kept */
void HCB_SETVEC(uint8_t bl, uint16_t es, uint32_t edx)
{
    (void)es;
    (void)edx;
    if (bl != 0)
        pi_stop("HCB_SETVEC: a vector not seen in a run");
}

/* the game's own data (HOST_DS), where callbacks 6 to 9 keep the file */
static uint32_t host_ds(void)
{
    return pi_image.desc[ILLUSION_CODE].base;
}

void hcb_use_table(int table)
{
    in_table = table;
    cell_sel = table ? N_TBL_HCB_FILE_SEL : N_HCB_FILE_SEL;
    cell_cursor = table ? N_TBL_HCB_CURSOR : N_HCB_CURSOR;
}

/* CODE:6285 (the table's 99F6), callback 6: the file named at DS:EBX
 * loaded by INT 94h AH=1 (the name DS:629Ch, 9A0Dh) with policy 1 (from
 * the top) around it; its selector to HCB_FILE_SEL,
 * HCB_CURSOR 0 */
int HCB_LOAD(uint16_t ds, uint32_t ebx)
{
    char name[128];
    uint32_t at = pmax_base(ds) + ebx;
    uint16_t sel;
    size_t i;

    for (i = 0; i < sizeof name - 1 && lrb(at + i); i++)
        name[i] = (char)lrb(at + i);
    name[i] = 0;
    pmax_policy(1);
    pmax_name(in_table ? 0x9A0D : 0x629C, ds);  /* "temporary file" */
    sel = pmax_load(name, NULL);
    pmax_policy(0);
    /* AX, whatever INT 94h gave; the port's 0 when there is no file */
    lww(host_ds() + cell_sel, sel);
    lwd(host_ds() + cell_cursor, 0);
    return sel == 0;
}

/* CODE:62EE, callback 7: ECX bytes from the file at HCB_CURSOR to DS:EDI
 * (the caller's DS), the cursor moved on; CF clear */
void HCB_READ(uint16_t ds, uint32_t edi, uint32_t ecx)
{
    uint32_t from = pmax_base(lrw(host_ds() + cell_sel)), cur = lrd(host_ds() + cell_cursor);
    uint32_t to = pmax_base(ds) + edi, i;

    for (i = 0; i < ecx; i++)
        lwb(to + i, lrb(from + cur + i));
    lwd(host_ds() + cell_cursor, cur + ecx);
}

/* CODE:6318, callback 8: the file's block freed; CF clear */
void HCB_UNLOAD(void)
{
    pmax_free(lrw(host_ds() + cell_sel));
}

/* CODE:632E, callback 9: HCB_CURSOR = EDX; CF clear */
void HCB_SEEK(uint32_t edx)
{
    lwd(host_ds() + cell_cursor, edx);
}
