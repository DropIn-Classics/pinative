/* names.h - the hints' names as offsets in CODE (gen/names.h, symmap.py):
 * N_OPTIONS and so on, for rb/rw/rd at pm_ds (pmem.h). */
#ifndef PI_NAMES_H
#define PI_NAMES_H

#include "gen/names.h"

enum {
#define X(seg, name, addr) N_##name = addr,
    PI_NAMES(X)
#undef X
};

#endif
