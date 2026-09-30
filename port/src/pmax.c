/* pmax.c - see pmax.h */
#include <stdlib.h>
#include <string.h>
#include "game.h"
#include "gen/names.h"
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

uint16_t pmax_rm_seg(void)
{
    return 0x0B3E;
}

uint16_t pmax_video_sel(void)
{
    return 0x48;
}

static uint16_t code_sel = 0x14;

uint16_t pmax_code_sel(void)
{
    return code_sel;
}

void pmax_set_code_sel(uint16_t sel)
{
    code_sel = sel;
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

/* ---- memory blocks and selectors
 *
 * pMAX's heap lies behind the image; in the runs the first file loaded
 * (SETSOUND.DAT, later the sound driver) had selector 4 and lay at the
 * image's end + 10h, after a 10h-byte header (docs/HANDOFF.md, "SETUP_ARGS
 * in a run", "The CD check and the driver's start in a run").  The port
 * puts its blocks there too, first fit from the bottom, each after 10h
 * bytes and on 16 bytes.  With policy 2 (INT 92h AH=8 BL=2) a block comes
 * from DOS memory: in the run the first at linear 13120h (docs/HANDOFF.md,
 * "The driver's command 0 in a run"); the port puts the next ones after
 * it the same way, not checked.  With policy 1 a block is cut from the
 * top: the heap's chain ends at FEFFF0h, and in the run MOD.INT (47F88h
 * bytes) lay at FA8060h, so that its size rounded to 16 ended there
 * (docs/HANDOFF.md, "The driver's command 4 in a run"); the port puts the
 * next ones below it the same way, not checked.  The headers themselves
 * are pMAX's and not written.
 *
 * Selectors: the lowest free of 04h, 0Ch, 14h ... (steps of 8, the local
 * table's), with 14h (CS), 1Ch (DS) and 24h taken from the start; the
 * video memory's 48h is another table's (bit 2 clear) and takes none of
 * them: the runs had 4, 0Ch, 2Ch ... 44h, 4Ch ... 12Ch (docs/HANDOFF.md,
 * "The driver's command 4 in a run"). */

#define MAX_BLOCKS 512
#define MAX_SELS 1024

#define LOW_START 0x13120u
#define LOW_END 0xA0000u
#define HEAP_TOP 0xFEFFF0u

static struct { uint32_t base, size; uint16_t sel; int used; } blocks[MAX_BLOCKS];
static uint8_t policy;
static struct { uint32_t base; int used; } sels[MAX_SELS];

static void sels_init(void)
{
    static int done;
    if (!done) {
        sels[0x14 / 8].used = sels[0x1C / 8].used = sels[0x24 / 8].used = 1;
        sels[0x14 / 8].base = sels[0x1C / 8].base = pi_image.desc[ILLUSION_CODE].base;
        done = 1;
    }
}

/* the lowest free selector, given the linear base `base`; 0 if none */
static uint16_t sel_new(uint32_t base)
{
    int k;

    sels_init();
    for (k = 0; k < MAX_SELS; k++)
        if (!sels[k].used) {
            sels[k].used = 1;
            sels[k].base = base;
            return (uint16_t)(8 * k + 4);
        }
    return 0;
}

/* a new block of `size` bytes by `policy`; its selector to *sel unless
 * `sel` is NULL (INT 92h AH=6: a linear address only); its base, 0 when
 * there is no room */
static uint32_t block_alloc(uint32_t size, uint16_t *sel)
{
    uint32_t at = (pi_image.base + pi_image.alloc + 0x10 + 15) & ~15u, end = PM_SIZE;
    int k, free_slot = -1;

    if (policy == 2) {
        at = LOW_START;
        end = LOW_END;
    } else if (policy == 1) {
        /* the highest address below every block in use above it that the
         * new one fits after */
        uint32_t r = (size + 15) & ~15u;
        int moved;

        at = HEAP_TOP - r;
        do {
            moved = 0;
            for (k = 0; k < MAX_BLOCKS; k++)
                if (blocks[k].used && at < blocks[k].base + blocks[k].size + 0x10
                    && blocks[k].base < at + r + 0x10) {
                    at = blocks[k].base - 0x10 - r;
                    moved = 1;
                }
        } while (moved);
    } else if (policy != 0) {
        pi_stop("pMAX: an allocation with another policy");
    }

    /* the lowest address after every block in use that the new one fits
     * before the next (first fit over the used blocks, by address) */
    while (policy != 1) {
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
    if (free_slot < 0 || (uint64_t)at + size > end || (sel && !(*sel = sel_new(at))))
        return 0;
    blocks[free_slot].base = at;
    blocks[free_slot].size = size;
    blocks[free_slot].sel = sel ? *sel : 0;
    blocks[free_slot].used = 1;
    return at;
}

uint16_t pmax_load(const char *name, uint32_t *size)
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
    if (at && size)
        *size = e->size;
    free(f);
    return at ? sel : 0;
}

uint16_t pmax_load_ds(uint32_t off, uint32_t *size)
{
    char name[128];
    size_t i;

    for (i = 0; i < sizeof name - 1 && rb(off + (uint32_t)i); i++)
        name[i] = (char)rb(off + (uint32_t)i);
    name[i] = 0;
    return pmax_load(name, size);
}

uint16_t pmax_alloc(uint32_t size)
{
    uint16_t sel = 0;
    return block_alloc(size, &sel) ? sel : 0;
}

uint16_t pmax_alloc_top(uint32_t size)
{
    uint8_t keep = policy;
    uint16_t sel = 0;

    policy = 1;
    block_alloc(size, &sel);
    policy = keep;
    return sel;
}

uint32_t pmax_alloc_linear(uint32_t size)
{
    uint8_t keep = policy;
    uint32_t at;

    policy = 1;
    at = block_alloc(size, NULL);
    policy = keep;
    return at;
}

void pmax_free_linear(uint32_t base)
{
    int k;

    for (k = 0; k < MAX_BLOCKS; k++)
        if (blocks[k].used && !blocks[k].sel && blocks[k].base == base)
            blocks[k].used = 0;
}

void pmax_policy(uint8_t bl)
{
    policy = bl;
}

uint16_t pmax_alias(uint16_t sel)
{
    return sel_new(pmax_base(sel));
}

uint32_t pmax_base(uint16_t sel)
{
    int k = sel / 8;

    sels_init();
    return (sel & 7) == 4 && k < MAX_SELS && sels[k].used ? sels[k].base : 0;
}

void pmax_free(uint16_t sel)
{
    int k;

    for (k = 0; k < MAX_BLOCKS; k++)
        if (blocks[k].used && sel && blocks[k].sel == sel) {
            blocks[k].used = 0;
            sels[sel / 8].used = 0;
        }
}

void pmax_free_sel(uint16_t sel)
{
    sels_init();
    if ((sel & 7) == 4 && sel / 8 < MAX_SELS)
        sels[sel / 8].used = 0;
}
