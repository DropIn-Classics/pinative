/* pmax.c - see pmax.h */
#include <stdlib.h>
#include <string.h>
#include "pmax.h"
#include "pmem.h"
#include "sys.h"

const char *pmax_tail = " C:\\ILLUSION.CFG /";

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
