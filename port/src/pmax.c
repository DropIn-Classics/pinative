/* pmax.c - see pmax.h */
#include <stdlib.h>
#include <string.h>
#include "image.h"
#include "pmax.h"
#include "pmem.h"
#include "sys.h"

const char *pmax_tail = " C:\\ILLUSION.CFG /";
char pmax_tail_buf[160];

uint16_t pmax_country(void)
{
    return 0;
}

/* ---- the configuration file */

#define CFG_SIZE 0x220          /* a 20h-byte header, 200h bytes of options */

static uint8_t cfg[CFG_SIZE];
static int cfg_there;

void pmax_cfg_open(const char *path)
{
    size_t n;
    uint8_t *f = path ? sys_load(path, &n) : NULL;

    cfg_there = f && n == CFG_SIZE;
    if (cfg_there)
        memcpy(cfg, f, CFG_SIZE);
    free(f);
}

void pmax_cfg_read(uint32_t off)
{
    /* stored chained: a byte XOR the stored byte four before it, the first
     * four XOR "SN95" (docs/HANDOFF.md, "The configuration file's options") */
    static const uint8_t key[4] = { 'S', 'N', '9', '5' };
    uint32_t i;

    for (i = 0; i < 0x200; i++) {
        const uint8_t *s = cfg + 0x20;
        wb(off + i, cfg_there ? (uint8_t)(s[i] ^ (i < 4 ? key[i] : s[i - 4])) : 0);
    }
}

int pmax_cfg_header(uint32_t off)
{
    uint32_t i;

    for (i = 0; i < 0x20; i++)
        wb(off + i, cfg_there ? cfg[i] : 0xFF);
    return 0;
}

/* ---- memory blocks
 *
 * pMAX's heap lies behind the image; in the run the first file loaded
 * (SETSOUND.DAT) had selector 4 and lay at the image's end + 10h, after a
 * 10h-byte header (docs/HANDOFF.md, "SETUP_ARGS in a run").  The port puts
 * its blocks there too, first fit, each after 10h bytes and on 16 bytes;
 * the selectors 4, 0Ch, 14h ... by slot.  Only the first block is seen;
 * the rest is a guess until a run shows more.  The headers themselves are
 * pMAX's and not written. */

#define MAX_BLOCKS 64

static struct { uint32_t base, size; int used; } blocks[MAX_BLOCKS];

static uint16_t slot_sel(int k) { return (uint16_t)(4 + 8 * k); }

static uint32_t block_alloc(uint32_t size, uint16_t *sel)
{
    uint32_t at = (pi_image.base + pi_image.alloc + 0x10 + 15) & ~15u;
    int k, free_slot = -1;

    /* the lowest address after every block in use that the new one fits
     * before the next (first fit over the used blocks, by address) */
    for (;;) {
        int moved = 0;
        for (k = 0; k < MAX_BLOCKS; k++)
            if (blocks[k].used && at < blocks[k].base + blocks[k].size + 0x10
                && blocks[k].base < at + size + 0x10) {
                at = (blocks[k].base + blocks[k].size + 0x10 + 15) & ~15u;
                moved = 1;
            }
        if (!moved)
            break;
    }
    for (k = 0; k < MAX_BLOCKS && free_slot < 0; k++)
        if (!blocks[k].used)
            free_slot = k;
    if (free_slot < 0 || (uint64_t)at + size > PM_SIZE)
        return 0;
    blocks[free_slot].base = at;
    blocks[free_slot].size = size;
    blocks[free_slot].used = 1;
    *sel = slot_sel(free_slot);
    return at;
}

uint16_t pmax_load(const char *name)
{
    char err[256];
    const ArcEntry *e = arc_find(&pi_archive, name);
    uint8_t *f;
    uint16_t sel = 0;
    uint32_t at;

    if (!e || !(f = arc_unpack(&pi_archive, e, err, sizeof err)))
        return 0;
    at = block_alloc(e->size, &sel);
    if (at)
        memcpy(pmem + at, f, e->size);
    free(f);
    return at ? sel : 0;
}

uint16_t pmax_alloc(uint32_t size)
{
    uint16_t sel = 0;
    return block_alloc(size, &sel) ? sel : 0;
}

uint32_t pmax_base(uint16_t sel)
{
    int k = (sel - 4) / 8;
    return k >= 0 && k < MAX_BLOCKS && slot_sel(k) == sel && blocks[k].used ? blocks[k].base : 0;
}

void pmax_free(uint16_t sel)
{
    int k = (sel - 4) / 8;
    if (k >= 0 && k < MAX_BLOCKS && slot_sel(k) == sel)
        blocks[k].used = 0;
}
