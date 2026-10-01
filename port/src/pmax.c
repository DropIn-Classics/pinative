/* pmax.c - see pmax.h */
#include <stdio.h>
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
static char cfg_path[1024];

void pmax_cfg_open(const char *path)
{
    size_t n;
    uint8_t *f = path ? sys_load(path, &n) : NULL;

    cfg_there = f && n == CFG_SIZE;
    if (cfg_there)
        memcpy(cfg, f, CFG_SIZE);
    else {
        /* the port's own header, not the sound set-up's: the driver named
         * (the port loads NOSOUND.SDR whatever is named), the rest 0 */
        memset(cfg, 0, 0x20);
        memcpy(cfg, "NOSOUND.SDR", 11);
    }
    snprintf(cfg_path, sizeof cfg_path, "%s", path ? path : "");
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

void pmax_cfg_write(uint32_t off)
{
    static const uint8_t key[4] = { 'S', 'N', '9', '5' };
    uint8_t *s = cfg + 0x20;
    FILE *f;
    uint32_t i;

    if (!cfg_path[0])
        return;
    for (i = 0; i < 0x200; i++)
        s[i] = (uint8_t)(rb(off + i) ^ (i < 4 ? key[i] : s[i - 4]));
    f = fopen(cfg_path, "wb");
    if (!f || fwrite(cfg, 1, CFG_SIZE, f) != CFG_SIZE)
        pi_stop("INT 94h AH=6: the configuration file not written");
    fclose(f);
    cfg_there = 1;
}

int pmax_cfg_header(uint32_t off)
{
    uint32_t i;

    for (i = 0; i < 0x20; i++)
        wb(off + i, cfg[i]);
    return 0;
}

/* ---- memory blocks and selectors
 *
 * pMAX's heap lies behind the image; in the runs the first file loaded
 * (SETSOUND.DAT, later the sound driver) had selector 4 and lay at the
 * image's end + 10h, after a 10h-byte header (docs/HANDOFF.md, "SETUP_ARGS
 * in a run", "The CD check and the driver's start in a run").  The port
 * keeps pMAX's chain there as the runs' memory shows it (docs/HANDOFF.md,
 * "pMAX's heap headers"): from that first header up to HEAP_TOP each
 * block, used or free, after a header of 01h, used FFh or free 00h, the
 * selector word, the size rounded to 16 (a dword), the name's offset (a
 * dword) and selector (a word) and two bytes pMAX leaves as they were;
 * at HEAP_TOP 00h FFh ends it.  Policy 0 takes the first free block from
 * the bottom that the size fits, its rest a free block after it; policy
 * 1 (and INT 92h AH=6, AH=7) cuts the new block from the top end of the
 * highest free block it fits.  A freed block's header gets selector 0;
 * a free block before it takes it in, its own header otherwise left
 * (still FFh), else it is marked free; a free block after it is taken in
 * either way.  Not modelled: pMAX's own blocks (FA00h bytes from the top
 * while it loads a file, name 18h:33A7h), whose data stays in the free
 * space at the top and under later headers there.  With policy 2 (INT 92h AH=8 BL=2) a block comes
 * from DOS memory: in the run the first at linear 13120h (docs/HANDOFF.md,
 * "The driver's command 0 in a run"); the port puts the next ones after
 * it, with no headers (none looked at).
 *
 * Selectors: the lowest free of 04h, 0Ch, 14h ... (steps of 8, the local
 * table's), with 14h (CS), 1Ch (DS) and 24h taken from the start; the
 * video memory's 48h is another table's (bit 2 clear) and takes none of
 * them: the runs had 4, 0Ch, 2Ch ... 44h, 4Ch ... 12Ch (docs/HANDOFF.md,
 * "The driver's command 4 in a run"). */

#define MAX_BLOCKS 512
#define MAX_SELS 1024
#define MAX_SEGS 1024

#define LOW_START 0x13120u
#define LOW_END 0xA0000u
#define HEAP_TOP 0xFEFFF0u

static struct { uint32_t base, size; uint16_t sel; int used; } blocks[MAX_BLOCKS];
static uint8_t policy;
static struct { uint32_t base; int used; } sels[MAX_SELS];

/* the chain: each block's header address, its size (rounded) and whether
 * it is used, in address order */
static struct { uint32_t hdr, size; int used; } segs[MAX_SEGS];
static int nsegs;
/* the name the next block's header gets (pmax_name) */
static uint32_t name_off;
static uint16_t name_sel;

void pmax_name(uint32_t off, uint16_t sel)
{
    name_off = off;
    name_sel = sel;
}

static void hdr_write(int k, uint16_t sel, uint32_t noff, uint16_t nsel)
{
    uint32_t h = segs[k].hdr;

    pmem[h] = 1;
    pmem[h + 1] = segs[k].used ? 0xFF : 0;
    lww(h + 2, sel);
    lwd(h + 4, segs[k].size);
    lwd(h + 8, noff);
    lww(h + 12, nsel);
}

static void chain_init(void)
{
    if (nsegs)
        return;
    segs[0].hdr = ((pi_image.base + pi_image.alloc + 0x10 + 15) & ~15u) - 0x10;
    segs[0].size = HEAP_TOP - segs[0].hdr - 0x10;
    segs[0].used = 0;
    nsegs = 1;
    hdr_write(0, 0, 0, 0);
    pmem[HEAP_TOP] = 0;
    pmem[HEAP_TOP + 1] = 0xFF;
}

static void seg_insert(int at)
{
    memmove(&segs[at + 1], &segs[at], (size_t)(nsegs - at) * sizeof segs[0]);
    nsegs++;
}

static void seg_remove(int at)
{
    memmove(&segs[at], &segs[at + 1], (size_t)(nsegs - at - 1) * sizeof segs[0]);
    nsegs--;
}

/* a block of `size` bytes in the chain, from the top or the bottom; its
 * data's address, 0 when there is no room.  The header is written once
 * the selector is known (chain_name) */
static int chain_alloc(uint32_t size, int top)
{
    uint32_t r = (size + 15) & ~15u;
    int k;

    chain_init();
    if (nsegs + 1 >= MAX_SEGS)
        return -1;
    if (!top) {
        for (k = 0; k < nsegs; k++)
            if (!segs[k].used && segs[k].size >= r)
                break;
        if (k == nsegs)
            return -1;
        if (segs[k].size >= r + 0x10) {
            seg_insert(k + 1);
            segs[k + 1].hdr = segs[k].hdr + 0x10 + r;
            segs[k + 1].size = segs[k].size - r - 0x10;
            segs[k + 1].used = 0;
            hdr_write(k + 1, 0, 0, 0);
            segs[k].size = r;
        }
        segs[k].used = 1;
        return k;
    }
    for (k = nsegs - 1; k >= 0; k--)
        if (!segs[k].used && segs[k].size >= r + 0x10)
            break;
    if (k < 0)
        return -1;
    seg_insert(k + 1);
    segs[k].size -= r + 0x10;
    lwd(segs[k].hdr + 4, segs[k].size);
    segs[k + 1].hdr = segs[k].hdr + 0x10 + segs[k].size;
    segs[k + 1].size = r;
    segs[k + 1].used = 1;
    return k + 1;
}

/* the block whose data is at `base` freed in the chain: its header's
 * selector 0; a free block before it takes it in (and a free one after
 * it), its own header left as it was otherwise; else it is marked free
 * and takes in a free block after it */
static void chain_free(uint32_t base)
{
    int k;

    for (k = 0; k < nsegs; k++)
        if (segs[k].used && segs[k].hdr + 0x10 == base)
            break;
    if (k == nsegs)
        return;
    segs[k].used = 0;
    lww(segs[k].hdr + 2, 0);
    if (k > 0 && !segs[k - 1].used) {
        segs[k - 1].size += 0x10 + segs[k].size;
        seg_remove(k);
        k--;
    } else {
        pmem[segs[k].hdr + 1] = 0;
    }
    if (k + 1 < nsegs && !segs[k + 1].used) {
        segs[k].size += 0x10 + segs[k + 1].size;
        seg_remove(k + 1);
    }
    lwd(segs[k].hdr + 4, segs[k].size);
}

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

/* a new block of `size` bytes by `policy` (`top`: from the top whatever
 * the policy); its selector to *sel unless `sel` is NULL (INT 92h AH=6,
 * AH=9: a linear address only); its base, 0 when there is no room.  The
 * header gets the name pmax_name gave, which is then cleared */
static uint32_t block_alloc_by(uint32_t size, uint16_t *sel, int top)
{
    uint32_t at = LOW_START;
    int k, free_slot = -1, seg = -1;

    for (k = 0; k < MAX_BLOCKS && free_slot < 0; k++)
        if (!blocks[k].used)
            free_slot = k;
    if (free_slot < 0)
        return 0;
    if (policy == 2 && !top) {
        /* the lowest address after every block in use that the new one
         * fits before the next */
        int moved;
        do {
            moved = 0;
            for (k = 0; k < MAX_BLOCKS; k++)
                if (blocks[k].used && at < blocks[k].base + blocks[k].size + 0x10
                    && blocks[k].base < at + size + 0x10) {
                    at = (blocks[k].base + blocks[k].size + 0x10 + 15) & ~15u;
                    moved = 1;
                }
        } while (moved);
        if ((uint64_t)at + size > LOW_END)
            return 0;
    } else if (policy > 2) {
        pi_stop("pMAX: an allocation with another policy");
    } else {
        seg = chain_alloc(size, top || policy == 1);
        if (seg < 0)
            return 0;
        at = segs[seg].hdr + 0x10;
    }
    if (sel && !(*sel = sel_new(at))) {
        if (seg >= 0)
            chain_free(at);
        return 0;
    }
    if (seg >= 0)
        hdr_write(seg, sel ? *sel : 0, name_off, name_sel);
    name_off = 0;
    name_sel = 0;
    blocks[free_slot].base = at;
    blocks[free_slot].size = size;
    blocks[free_slot].sel = sel ? *sel : 0;
    blocks[free_slot].used = 1;
    return at;
}

static uint32_t block_alloc(uint32_t size, uint16_t *sel)
{
    return block_alloc_by(size, sel, 0);
}

static void block_free(int k)
{
    blocks[k].used = 0;
    if (blocks[k].base >= LOW_END)
        chain_free(blocks[k].base);
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
    uint16_t sel = 0;

    block_alloc_by(size, &sel, 1);
    return sel;
}

uint32_t pmax_alloc_linear(uint32_t size)
{
    return block_alloc_by(size, NULL, 1);
}

uint32_t pmax_alloc_linear_here(uint32_t size)
{
    return block_alloc(size, NULL);
}

void pmax_free_linear(uint32_t base)
{
    int k;

    for (k = 0; k < MAX_BLOCKS; k++)
        if (blocks[k].used && !blocks[k].sel && blocks[k].base == base)
            block_free(k);
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
            block_free(k);
            sels[sel / 8].used = 0;
        }
}

void pmax_free_sel(uint16_t sel)
{
    int k;

    sels_init();
    if ((sel & 7) == 4 && sel / 8 < MAX_SELS)
        sels[sel / 8].used = 0;
    /* a block's own selector: the block stays (the table's driver, seen
     * in the run's heap at CODE:7182), no longer reached by it */
    for (k = 0; k < MAX_BLOCKS; k++)
        if (blocks[k].used && sel && blocks[k].sel == sel)
            blocks[k].sel = 0;
}
