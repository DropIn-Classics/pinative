/* hostcb.c - the host callbacks (HOST_CALLBACKS, CODE:618D): the game's
 * routines the sound driver calls, each a wrapper round a pMAX service
 * (docs/audio-driver.md, "Host callback table").  Those the translated
 * driver needs so far.
 */
#include "game.h"
#include "pmax.h"

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
