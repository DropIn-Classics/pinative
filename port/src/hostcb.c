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

/* CODE:6244, callback 0: a block of `size` bytes (INT 92h AH=0Ah) */
int HCB_ALLOC(uint32_t size, uint16_t *bx)
{
    *bx = pmax_alloc(size);
    return *bx == 0;
}

/* CODE:61ED, callback 1: the same from DOS memory (policy 2 around it) */
int HCB_ALLOC_LOW(uint32_t size, uint16_t *bx)
{
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

/* the game's own data (HOST_DS), where callbacks 6 to 9 keep the file */
static uint32_t host_ds(void)
{
    return pi_image.desc[ILLUSION_CODE].base;
}

/* CODE:6285, callback 6: the file named at DS:EBX loaded by INT 94h AH=1
 * with policy 1 (from the top) around it; its selector to HCB_FILE_SEL,
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
    sel = pmax_load(name, NULL);
    pmax_policy(0);
    /* AX, whatever INT 94h gave; the port's 0 when there is no file */
    lww(host_ds() + N_HCB_FILE_SEL, sel);
    lwd(host_ds() + N_HCB_CURSOR, 0);
    return sel == 0;
}

/* CODE:62EE, callback 7: ECX bytes from the file at HCB_CURSOR to DS:EDI
 * (the caller's DS), the cursor moved on; CF clear */
void HCB_READ(uint16_t ds, uint32_t edi, uint32_t ecx)
{
    uint32_t from = pmax_base(lrw(host_ds() + N_HCB_FILE_SEL)), cur = lrd(host_ds() + N_HCB_CURSOR);
    uint32_t to = pmax_base(ds) + edi, i;

    for (i = 0; i < ecx; i++)
        lwb(to + i, lrb(from + cur + i));
    lwd(host_ds() + N_HCB_CURSOR, cur + ecx);
}

/* CODE:6318, callback 8: the file's block freed; CF clear */
void HCB_UNLOAD(void)
{
    pmax_free(lrw(host_ds() + N_HCB_FILE_SEL));
}

/* CODE:632E, callback 9: HCB_CURSOR = EDX; CF clear */
void HCB_SEEK(uint32_t edx)
{
    lwd(host_ds() + N_HCB_CURSOR, edx);
}
