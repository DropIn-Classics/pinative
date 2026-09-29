/* image.c - see image.h */
#include <stdio.h>
#include <stdlib.h>
#include "archive.h"
#include "gen/names.h"
#include "image.h"
#include "sys.h"

PmImage pi_image;

int pi_load_image(const char *game, char *err, size_t n)
{
    /* descriptor 1 (TAIL) has no relocations; its selector is not known */
    static const uint16_t sels[1] = { PI_SEL_CODE };
    char path[SYS_PATH];
    Archive a;
    const ArcEntry *e;
    uint8_t *f;
    int r;

    sys_join(path, sizeof path, game, "ILLUSION.EXE");
    if (arc_open(&a, path, err, n) != 0)
        return -1;
    e = arc_find(&a, "ILLUSION.386");
    if (!e) {
        snprintf(err, n, "%s has no ILLUSION.386.", path);
        arc_close(&a);
        return -1;
    }
    f = arc_unpack(&a, e, err, n);
    if (!f) {
        arc_close(&a);
        return -1;
    }
    r = pm_load_image(f, e->size, ILLUSION_SIZE, ILLUSION_SHA256, PI_IMAGE_BASE, sels, 1,
                      &pi_image, err, n);
    free(f);
    arc_close(&a);
    if (r == 0)
        pm_ds = pi_image.desc[ILLUSION_CODE].base;
    return r;
}
